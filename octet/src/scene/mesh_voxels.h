////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012, 2013
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// Mesh smooth modifier. Work in progress.
//

namespace octet {
  class entry {
  public:
    ivec3 pos;
    int level;
    bool finished;

    entry() {}

    entry(int level_, ivec3 pos_, bool finished_) {
      level = level_;
      pos = pos_;
      finished = finished_;
    }
  };

  typedef pair<entry, entry> entries;

  class mesh_voxels : public mesh {
    ivec3 size;
    float voxel_size;

    enum { subcube_dim = 32 };

    dynarray<ref<mesh_voxel_subcube> > subcubes;

    struct kd_node {
      int axis;
      int kids[2];
    };

    dynarray<kd_node> kd_tree;

    void update_lod() {
      for (unsigned i = 0; i != subcubes.size(); ++i) {
        mesh_voxel_subcube *p = subcubes[i];
        if (p) {
          p->update_lod();
        }
      }

      /*struct entry {
        int level;
        int parent;
        entry(int lev, int par) { level = lev; parent = par; }
      };

      kd_tree.reset();
      dynarray<entry> builder;
      int levela = ilog2(size.x() * subcube_dim);
      builder.push_back(entry(levela, -1));
      
      //is_any(levela*/
          /*int zdiff = abs(pop_count(any_a&0x0f) - pop_count(any_a&0xf0));
          int ydiff = abs(pop_count(any_a&0x33) - pop_count(any_a&0xcc));
          int xdiff = abs(pop_count(any_a&0x55) - pop_count(any_a&0xaa));
          int maxdiff = max(xdiff, max(ydiff, zdiff));
          int mindiff = min(xdiff, min(ydiff, zdiff));
          if (xdiff == maxdiff) {
          }*/

    }

    unsigned is_all(ivec3_in pos, int level) const {
      if ((1<<level) <= subcube_dim) {
        return 1;
      } else {
        mesh_voxel_subcube *subcube = get_subcube(pos>>level);
        return subcube->is_any(pos & ivec3(subcube_dim-1), level);
      }
    }

    static const ivec3 &delta(int i) {
      static const ivec3 d[] = {
        ivec3(0, 0, 0),
        ivec3(1, 0, 0),
        ivec3(0, 1, 0),
        ivec3(1, 1, 0),
        ivec3(0, 0, 1),
        ivec3(1, 0, 1),
        ivec3(0, 1, 1),
        ivec3(1, 1, 1)
      };
      return d[i];
    }

    unsigned any8(ivec3_in pos, int level) const {
      unsigned res = 0;
      for (unsigned i = 0; i != 8; ++i) {
        res = res * 2 + is_any(pos + delta(i), level);
      }
      return res;
    }

    void update_mesh() {
      mesh_iterate_faces<face_counter, subcube_dim> count;
      for (unsigned i = 0; i != subcubes.size(); ++i) {
        mesh_voxel_subcube *p = subcubes[i];
        if (p) {
          p->count_faces(count);
        }
      }

      allocate(sizeof(vertex)*count.num_faces*4, sizeof(uint32_t)*count.num_faces*6);
      set_num_indices(count.num_faces*6);
      set_num_vertices(count.num_faces*4);

      mesh_iterate_faces<face_adder, subcube_dim> add;
      add.vtx = (vertex *)get_vertices()->lock();
      add.idx = (uint32_t *)get_indices()->lock();
      add.dx = vec3(voxel_size, 0.0f, 0.0f);
      add.dy = vec3(0.0f, voxel_size, 0.0f);
      add.dz = vec3(0.0f, 0.0f, voxel_size);
      add.voxel_size = voxel_size;

      vec3 offset = vec3(size) * (-0.5f * subcube_dim * voxel_size);
      vec3 scale(subcube_dim * voxel_size);
      int idx = 0;
      for (int z = 0; z != size.z(); ++z) {
        for (int y = 0; y != size.y(); ++y) {
          for (int x = 0; x != size.x(); ++x) {
            mesh_voxel_subcube *p = subcubes[idx++];
            add.origin = vec3(x, y, z) * scale + offset;
            if (p) {
              p->add_faces(add);
            }
          }
        }
      }

      assert(count.num_faces == add.num_faces);

      get_vertices()->unlock();
      get_indices()->unlock();
      //dump(app_utils::log("voxels\n"));
    }

  public:
    RESOURCE_META(mesh_voxels)

    mesh_voxels(float voxel_size_in=1.0f/32, const ivec3 &size_in = ivec3(2, 2, 2)) {
      set_default_attributes();
      voxel_size = voxel_size_in;
      size = size_in;
      //set_aabb(aabb(vec3(0, 0, 0), size));

      subcubes.resize(size.x() * size.y() * size.z());
      set_aabb(aabb(vec3(0, 0, 0), vec3(size)*(voxel_size*subcube_dim*0.5f)));
      int idx = 0;
      for (int z = 0; z != size.z(); ++z) {
        for (int y = 0; y != size.y(); ++y) {
          for (int x = 0; x != size.x(); ++x) {
            ivec3 pos(x, y, z);
            subcubes[idx++] = new mesh_voxel_subcube();
          }
        }
      }

      //box(aabb(vec3(8, 8, 8), vec3(8, 8, 8)));
      update_lod();
    }

    void update() {
      update_lod();
      update_mesh();
    }

    void visit(visitor &v) {
      mesh::visit(v);
    }

    template <class set> void add_voxels(mat4t_in voxelToWorld, const set &set_in) {
      int idx = 0;
      vec3 offset = vec3(size) * (-0.5f * subcube_dim) + vec3(0.5f);
      vec3 scale = vec3(subcube_dim);
      for (int z = 0; z != size.z(); ++z) {
        for (int y = 0; y != size.y(); ++y) {
          for (int x = 0; x != size.x(); ++x) {
            mat4t localVoxelToWorld = voxelToWorld;
            vec3 pos = vec3(x, y, z) * scale + offset;
            localVoxelToWorld.translate(pos.x(), pos.y(), pos.z());
            //localVoxelToWorld.w() += vec4(0.5f, 0.5f, 0.5f, 0.0f);
            subcubes[idx++]->add_voxels(localVoxelToWorld, set_in);
          }
        }
      }
    }

    /*mesh_voxels &cylinder(vec3_in centre, vec3_in axis, float radius, float half_length) {
      float r2 = radius * radius;
      for (int z = 0; z != dim; ++z) {
        for (int y = 0; y != dim; ++y) {
          for (int x = 0; x != dim; ++x) {
            vec3 pos = vec3(x-dim/2, y-dim/2, z-dim/2) - centre;
            float adotp = dot(pos, axis);
            vec3 nearest = axis * adotp;
            float d2 = squared(nearest - pos);
            if (d2 <= r2 && abs(adotp) <= half_length) {
              opaque[z*dim+y] |= 1<<x;
            }
          }
        }
      }
      return *this;
    }

    mesh_voxels &sphere(vec3_in centre, float radius) {
      float r2 = radius * radius;
      for (int z = 0; z != dim; ++z) {
        for (int y = 0; y != dim; ++y) {
          for (int x = 0; x != dim; ++x) {
            vec3 pos = vec3(x-dim/2, y-dim/2, z-dim/2) - centre;
            float d2 = squared(pos);
            if (d2 <= r2) {
              opaque[z*dim+y] |= 1<<x;
            }
          }
        }
      }
      return *this;
    }*/

    template <class bounds_t> mesh_voxels &draw(mat4t_in voxelToWorld, const bounds_t &bounds) {
      add_voxels(voxelToWorld, bounds);
      return *this;
    }

    void dump(FILE *fp) {
      int idx = 0;
      for (int z = 0; z != size.z(); ++z) {
        for (int y = 0; y != size.y(); ++y) {
          for (int x = 0; x != size.x(); ++x, idx++) {
            fprintf(fp, "\n%d %d %d\n", x, y, z);
            if (subcubes[idx]) {
              subcubes[idx]->dump(fp);
            }
          }
        }
      }
      mesh::dump(fp);
    }

    mesh_voxel_subcube *get_subcube(ivec3_in pos) const {
      assert(all(pos < size));
      //assert(x < (unsigned)size.x() && y < (unsigned)size.y() && z < (unsigned)size.z());
      return subcubes[pos.x()+ size.x() * (pos.y() + size.y()*pos.z())];
    }

    unsigned is_any(ivec3_in pos, int level) const {
      if ((1<<level) <= subcube_dim) {
        return 1;
      } else {
        mesh_voxel_subcube *subcube = get_subcube(pos>>level);
        return subcube->is_any(pos & ivec3(subcube_dim-1), level);
      }
    }


    // collide two orientated voxel meshes.
    bool intersects(const mesh_voxels &b, const mat4t &mxa, const mat4t &mxb) const {
      const mesh_voxels &a = *this;
      vec3 corner_a(vec3(a.size) * (a.voxel_size * (-0.5f * subcube_dim)));
      vec3 corner_b(vec3(b.size) * (b.voxel_size * (-0.5f * subcube_dim)));

      //mat4t atob = inverse3x4(mxa) * mxb;
      int levela = ilog2(a.size.x() * subcube_dim);
      int levelb = ilog2(b.size.x() * subcube_dim);

      dynarray<entries> stack(32);
      stack.push_back(entries(
        entry(levela, ivec3(0, 0, 0), false),
        entry(levelb, ivec3(0, 0, 0), false)
      ));

      if (
        !a.is_any(ivec3(0, 0, 0), levela) ||
        !b.is_any(ivec3(0, 0, 0), levelb)
      ) {
        return false;
      }

      while(!stack.is_empty()) {
        entry &ta = stack.back().first;
        entry &tb = stack.back().second;
        stack.pop_back();
        float scale_a = a.voxel_size * (1 << ta.level);
        float scale_b = b.voxel_size * (1 << tb.level);
        aabb bounds_a(corner_a + vec3(ta.pos) * scale_a + (scale_a * 0.5f), scale_a * 0.5f);
        aabb bounds_b(corner_b + vec3(tb.pos) * scale_b + (scale_b * 0.5f), scale_b * 0.5f);
        if (bounds_a.intersects(bounds_b, mxa, mxb)) {
          if (ta.level == 0 || tb.level == 0) {
            return true;
          }
          ivec3 npa = ta.pos * 2;
          ivec3 npb = tb.pos * 2;
          int lev_a = ta.level - 1;
          int lev_b = tb.level - 1;
          int any_a = a.any8(npa, lev_a);
          int any_b = b.any8(npb, lev_b);
          for (unsigned i = 0; i != 8; ++i) {
            if ((any_a >> i) & 1) {
              for (unsigned j = 0; j != 8; ++j) {
                if ((any_b >> j) & 1) {
                  stack.push_back(entries(
                    entry(lev_a, npa + delta(i), false),
                    entry(lev_b, npb + delta(j), false)
                  ));
                }
              }
            }
          }
        }
      }
      return false;
    }
  };
}
