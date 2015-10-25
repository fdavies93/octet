////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// invaderer example: simple game with sprites and sounds
//
// Level: 1
//
// Demonstrates:
//   Basic framework app
//   Shaders
//   Basic Matrices
//   Simple game mechanics
//   Texture loaded from GIF file
//   Audio
//

namespace octet {

	enum constants {
		max_sprites = 1024,
		num_sound_sources = 16,
	};

	enum sprite_types {
		type_null = -1,
		player = 0,
		enemy = 1,
		bullet = 2,
		rock = 3,
		ground = 4,
		type_number = 5,
	};

	enum sprite_directions {
		LEFT,
		RIGHT,
		UP,
		DOWN,
	};

	class sprite {
		// where is our sprite (overkill for a 2D game!)
		mat4t modelToWorld;

		// half the width of the sprite
		float halfWidth;

		// half the height of the sprite
		float halfHeight;

		// what texture is on our sprite
		int texture;

		// true if this sprite is enabled.
		bool enabled;

		float prevX;
		float prevY;

		sprite_types type;
	public:
		sprite_directions facing;

		sprite() {
			texture = 0;
			type = sprite_types::type_null;
			enabled = false;
			facing = sprite_directions::DOWN;
		}

		void init(int _texture, float x, float y, float w, float h) {
			modelToWorld.loadIdentity();//resets matrix position to origin, i.e. [0,0,0]
			modelToWorld.translate(x, y, 0);//translate by x,y and not at all on the z-plane
			halfWidth = w * 0.5f;//obvious
			halfHeight = h * 0.5f;
			prevX = 0;
			prevY = 0;
			texture = _texture;//basically just the index of the texture; set in app init
			enabled = true;
		}

		void render(texture_shader &shader, mat4t &cameraToWorld) {
			// invisible sprite... used for gameplay.
			if (!texture) return;

			// build a projection matrix: model -> world -> camera -> projection
			// the projection space is the cube -1 <= x/w, y/w, z/w <= 1
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

			// set up opengl to draw textured triangles using sampler 0 (GL_TEXTURE0)
			glActiveTexture(GL_TEXTURE0);//makes GL_TEXTURE0 the active (draw) texture
			glBindTexture(GL_TEXTURE_2D, texture);//from openGL api, binds the texture under index texture to the active texture

												  // use "old skool" rendering
												  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
												  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			shader.render(modelToProjection, 0);
			//^ treat this as essentially a black box for now; shader code can be dealt with after the basic code is set up

			// this is an array of the positions of the corners of the sprite in 3D
			// a straight "float" here means this array is being generated here at runtime.
			float vertices[] = {
				-halfWidth, -halfHeight, 0,
				halfWidth, -halfHeight, 0,
				halfWidth,  halfHeight, 0,
				-halfWidth,  halfHeight, 0,
			};

			// attribute_pos (=0) is position of each corner
			// each corner has 3 floats (x, y, z)
			// there is no gap between the 3 floats and hence the stride is 3*sizeof(float)
			// 'stride' means span of data from pointer of each 'cell' in vertex
			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)vertices);
			// ^ index; size; type; whether normalized; size of array; and array of vertices, cast to a void pointer
			glEnableVertexAttribArray(attribute_pos);
			// ^ uses current vertex attrib (i.e. vertices) and enables their use for rendering texture
			// v this is an array of the positions of the corners of the texture in 2D

			static const float uvs[] = {
				0,  0,
				1,  0,
				1,  1,
				0,  1,
			};//essentially vertices normalised relative to size of sprite on texture

			  // attribute_uv is position in the texture of each corner
			  // each corner (vertex) has 2 floats (x, y)
			  // there is no gap between the 2 floats and hence the stride is 2*sizeof(float)
			glVertexAttribPointer(attribute_uv, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)uvs);
			glEnableVertexAttribArray(attribute_uv);
			//^ as above, simply casting uvs into attribute_uv
			// finally, draw the sprite (4 vertices)
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		//reset position (to 0,0)
		void reset_position()
		{
			modelToWorld.loadIdentity();
		}

		// move the object
		void translate(float x, float y) {
			prevX = modelToWorld[3][0];
			prevY = modelToWorld[3][1];
			modelToWorld.translate(x, y, 0);
		}

		// position the object relative to another.
		void set_relative(sprite &rhs, float x, float y) {
			modelToWorld = rhs.modelToWorld;
			modelToWorld.translate(x, y, 0);
		}

		// return true if this sprite collides with another.
		// note the "const"s which say we do not modify either sprite
		bool collides_with(const sprite &rhs) const {
			float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];
			float dy = rhs.modelToWorld[3][1] - modelToWorld[3][1];

			// both distances have to be under the sum of the halfwidths
			// for a collision
			return
				(fabsf(dx) < halfWidth + rhs.halfWidth) &&
				(fabsf(dy) < halfHeight + rhs.halfHeight)
				;
		}

		bool is_above(const sprite &rhs, float margin) const {
			float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];

			return
				(fabsf(dx) < halfWidth + margin)
				;
		}

		bool &is_enabled() {
			return enabled;
		}

		sprite_types &get_type()
		{
			return type;
		}

		float getX(){
			return modelToWorld[3][0];
		}

		float getY(){
			return modelToWorld[3][1];
		}

		float getPrevX()
		{
			return prevX;
		}

		float getPrevY()
		{
			return prevY;
		}

		float getHalfWidth()
		{
			return halfWidth;
		}

		float getHalfHeight()
		{
			return halfHeight;
		}
	};

	struct sprite_type_data
	{
		//basic initialisation stuff
		int _texture;
		float w;
		float h;
		bool collides;
	};

	//stores and manages game data in convenient ways
	class game_manager
	{		
	private:
		const float PLAYER_SPEED = 0.05f;
		const float BULLET_BUFFER = 0.1f;
		const float BULLET_SPEED = 0.1f;

		octet::app* myApp;
		sprite dummy;
		sprite contained_sprites[max_sprites];
		ALuint sound_sources[num_sound_sources];
		ALuint cur_sound_source;
		sprite_type_data sprite_data[type_number];
		std::list<int> colliding_sprites;//change to list
		std::list<int> background_sprites;
		std::vector<sprite*> removal_list;
		texture_shader *shader;
		mat4t *worldCamera;
		

	public:

		game_manager()
		{
		}

		ALuint get_sound_source() { return sound_sources[cur_sound_source++ % num_sound_sources]; }

		void init(octet::app* usedApp, texture_shader &texture_shader_, mat4t &cameraToWorld)
		{
			
			myApp = usedApp;
			shader = &texture_shader_;
			worldCamera = &cameraToWorld;

			//setting up sprite data
			//player, enemy, rock, bullet, ground
			sprite_data[player].h = 0.25f;
			sprite_data[player].w = 0.25f;// = 32px
			sprite_data[player]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/player.gif");
			sprite_data[player].collides = true;

			sprite_data[enemy].h = 0.25f;
			sprite_data[enemy].w = 0.25f;
			sprite_data[enemy]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/enemy.gif");
			sprite_data[enemy].collides = true;

			sprite_data[rock].h = 0.25f;
			sprite_data[rock].w = 0.25f;
			sprite_data[rock]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/rock.gif");
			sprite_data[rock].collides = true;

			sprite_data[bullet].h = 0.0625f;
			sprite_data[bullet].w = 0.0625f;
			sprite_data[bullet]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/bullet.gif");
			sprite_data[bullet].collides = true;

			sprite_data[ground].h = 0.25f;
			sprite_data[ground].w = 0.25f;
			sprite_data[ground]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/ground.gif");
			sprite_data[ground].collides = false;

			//...and sounds
			cur_sound_source = 0;
			alGenSources(num_sound_sources, sound_sources);
		}

		void playSound(string fileName)
		{
			ALuint mySound = resource_dict::get_sound_handle(AL_FORMAT_MONO16, fileName.c_str());
			ALuint source = get_sound_source();
			alSourcei(source, AL_BUFFER, mySound);
			alSourcePlay(source);
		}
		
		//checks memory references for a fairly safe way of testing for null returns	
		bool is_dummy(sprite possible_dummy)
		{
			return &dummy == &possible_dummy;
		}

		//initialises and enables a sprite as long as there's space, returns reference
		//if there isn't, returns the dummy sprite
		sprite &add_sprite(int _texture, float x, float y, float w, float h, sprite_types type)
		{
			ALuint cur_sprite = 0;
			while(cur_sprite < max_sprites){
				if (!contained_sprites[cur_sprite].is_enabled()) {
					contained_sprites[cur_sprite].init(_texture, x, y, w, h);
					contained_sprites[cur_sprite].get_type() = type;
					if (sprite_data[type].collides) colliding_sprites.push_back(cur_sprite);
					else background_sprites.push_back(cur_sprite);
					break;
				}
			cur_sprite++;
			}
			if (cur_sprite < max_sprites) return contained_sprites[cur_sprite];
			else return dummy;
		}

		//'removes' (i.e. disables and allows overwrites of) the sprite referenced then 
		//returns whether it was found and removed successfully
		bool remove_sprite(sprite &toRemove)
		{
			ALuint cur_sprite = 0;
			while (cur_sprite < max_sprites)
			{
				if (&contained_sprites[cur_sprite] == &toRemove)
				{
					toRemove.is_enabled() = false;
					/*the below lines are a bit hacky, but while a better solution would be to
					create an abstracted sprite group object, I thought this would just distract
					time and effort from more productive coding*/
					if (sprite_data[contained_sprites[cur_sprite].get_type()].collides)
					{
						std::list<int>::iterator cur_collider = colliding_sprites.begin();
						while (cur_collider != colliding_sprites.end())
						{
							if (&contained_sprites[*cur_collider] == &toRemove) {//ie if they're the same thing
								colliding_sprites.erase(cur_collider);
								break;
							}
							else ++cur_collider;
						}
					}
					else
					{
						std::list<int>::iterator cur_background = background_sprites.begin();
						while (cur_background != background_sprites.end())
						{
							if (&contained_sprites[*cur_background] == &toRemove) {
								background_sprites.erase(cur_background);
								break;
							}
							else ++cur_background;
						}
					}
					break;
				}
				++cur_sprite;
			}
			if (cur_sprite < max_sprites) return true;
			else return false;
		}

		//returns sprite by index; if not found, returns dummy
		sprite &get_sprite_by_index(ALuint index)
		{
			if (index >= -1 && index < max_sprites) {
				if (contained_sprites[index].is_enabled())
				{
					return contained_sprites[index];
				}
			}
			else return dummy;
		}

		//makes adding generic objects much easier by abstracting out their data
		sprite &add_sprite_by_type(sprite_types type, float x, float y)
		{
			return add_sprite(sprite_data[type]._texture, x, y, sprite_data[type].w, sprite_data[type].h, type);
		}
		
		void render()
		{
			std::list<int>::iterator sprite_iterator = background_sprites.begin();
			while (sprite_iterator != background_sprites.end())
			{
				contained_sprites[*sprite_iterator].render(*shader, *worldCamera);
				sprite_iterator++;
			}
			sprite_iterator = colliding_sprites.begin();
			while (sprite_iterator != colliding_sprites.end())
			{
				contained_sprites[*sprite_iterator].render(*shader, *worldCamera);
				sprite_iterator++;
			}
		}

		void simulateObject(sprite &object)
		{
			if (object.get_type() == sprite_types::player)
			{
				//note: all is_key_downs take capital letters
				if (myApp->is_key_down('S')){
					object.translate(0, -PLAYER_SPEED);
					object.facing = sprite_directions::DOWN;
				}
				else if (myApp->is_key_down('W')) {
					object.translate(0, PLAYER_SPEED);
					object.facing = sprite_directions::UP;
				}
				else if (myApp->is_key_down('A')) {
					object.translate(-PLAYER_SPEED, 0);
					object.facing = sprite_directions::LEFT;
				}
				else if (myApp->is_key_down('D')) {
					object.translate(PLAYER_SPEED, 0);
					object.facing = sprite_directions::RIGHT;
				}
				if (myApp->is_key_going_down(' '))
				{
					sprite* newBullet;
					switch (object.facing)
					{
					case sprite_directions::UP:
						newBullet = &add_sprite_by_type(sprite_types::bullet, object.getX(), object.getY() + object.getHalfHeight() + BULLET_BUFFER);
						newBullet->facing = sprite_directions::UP;
						break;
					case sprite_directions::DOWN:
						newBullet = &add_sprite_by_type(sprite_types::bullet, object.getX(), object.getY() - object.getHalfHeight() - BULLET_BUFFER);
						newBullet->facing = sprite_directions::DOWN;
						break;
					case sprite_directions::LEFT:
						newBullet = &add_sprite_by_type(sprite_types::bullet, object.getX() - object.getHalfWidth() - BULLET_BUFFER, object.getY());
						newBullet->facing = sprite_directions::LEFT;
						break;
					case sprite_directions::RIGHT:
						newBullet = &add_sprite_by_type(sprite_types::bullet, object.getX() + object.getHalfWidth() + BULLET_BUFFER, object.getY());
						newBullet->facing = sprite_directions::RIGHT;
						break;
					}
				}
				/*if (myApp->is_key_going_down(key_ctrl)) {
					playSound("assets/invaderers/bang.wav");
				}*/
			}
			if (object.get_type() == sprite_types::bullet) {
				if (object.facing == sprite_directions::UP) object.translate(0, BULLET_SPEED);
				if (object.facing == sprite_directions::DOWN) object.translate(0, -BULLET_SPEED);
				if (object.facing == sprite_directions::LEFT) object.translate(-BULLET_SPEED, 0);
				if (object.facing == sprite_directions::RIGHT) object.translate(BULLET_SPEED, 0);
			}
		}

		void simulateObjects()
		{
			std::list<int>::iterator sprite_i;
			for (sprite_i = colliding_sprites.begin(); sprite_i != colliding_sprites.end(); ++sprite_i)
			{
				simulateObject(contained_sprites[*sprite_i]);
			}
		}

		void resolveCollision(sprite &subject, sprite &object)
		{
			if (subject.get_type() == sprite_types::player && object.get_type() == sprite_types::player)
			{
				//printf("Testing...");
			}
			if (subject.get_type() == sprite_types::rock && object.get_type() == sprite_types::bullet)
			{
				removal_list.push_back(&object);
			}
			if (subject.get_type() == sprite_types::rock && (object.get_type() == sprite_types::player || object.get_type() == sprite_types::enemy))
			{
				object.reset_position();
				object.translate(object.getPrevX(), object.getPrevY());
				//i.e. move it back to its previous location
			}
		}

		void checkCollisions()
		{
			std::list<int>::iterator sprite1_i, sprite2_i;
			sprite *sprite1, *sprite2;
			for (sprite1_i = colliding_sprites.begin(); sprite1_i != colliding_sprites.end(); ++sprite1_i)
			{
				sprite1 = &contained_sprites[*sprite1_i];
				for (sprite2_i = colliding_sprites.begin(); sprite2_i != colliding_sprites.end(); ++sprite2_i)
				{
					sprite2 = &contained_sprites[*sprite2_i];
					if (sprite1->collides_with(*sprite2)) resolveCollision(*sprite1, *sprite2);
				}
			}
		}

		void removeDeadObjects()
		{
			for (ALuint i = 0; i < removal_list.size(); ++i)
			{
				remove_sprite(*removal_list[i]);
			}
			for (ALuint i = 0; i < removal_list.size(); ++i)
			{
				removal_list.pop_back();
			}
		}

		void simulate()
		{
			//move objects
			simulateObjects();
			//resolve collisions
			checkCollisions();
			//remove objects on remove list
			removeDeadObjects();
		}

		void update()
		{
			simulate();
			render();
		}

	};

	class assignment_8_nov : public octet::app {
		// Matrix to transform points in our camera space to the world.
		// This lets us move our camera
		mat4t cameraToWorld;

		// shader to draw a textured triangle
		texture_shader texture_shader_;

		game_manager manager;


		// a texture for our text
		GLuint font_texture;

		// information for our text
		bitmap_font font;

		void draw_text(texture_shader &shader, float x, float y, float scale, const char *text) {
			mat4t modelToWorld;
			modelToWorld.loadIdentity();
			modelToWorld.translate(x, y, 0);
			modelToWorld.scale(scale, scale, 1);
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

			/*mat4t tmp;
			glLoadIdentity();
			glTranslatef(x, y, 0);
			glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);
			glScalef(scale, scale, 1);
			glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);*/

			enum { max_quads = 32 };
			bitmap_font::vertex vertices[max_quads * 4];
			uint32_t indices[max_quads * 6];
			aabb bb(vec3(0, 0, 0), vec3(256, 256, 0));

			unsigned num_quads = font.build_mesh(bb, vertices, indices, max_quads, text, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, font_texture);

			shader.render(modelToProjection, 0);

			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].x);
			glEnableVertexAttribArray(attribute_pos);
			glVertexAttribPointer(attribute_uv, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].u);
			glEnableVertexAttribArray(attribute_uv);

			glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, indices);
		}

	public:

		
		// this is called when we construct the class
		assignment_8_nov(int argc, char **argv) : app(argc, argv), font(512, 256, "assets/big.fnt") {
		}

		// this is called once OpenGL is initialized
		void app_init() {
			// set up the shader

			texture_shader_.init();

			// set up the matrices with a camera 5 units from the origin
			cameraToWorld.loadIdentity();
			cameraToWorld.translate(0, 0, 3);

			manager.init(this, texture_shader_, cameraToWorld);

			font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");

			manager.add_sprite_by_type(sprite_types::player, 0, 0);

			//level = 23x23 grid
			//top
			for (float i = -2.75f; i <= 2.75f; i += 0.25f) manager.add_sprite_by_type(sprite_types::rock, i, 2.75f);
			//bottom
			for (float i = -2.75f; i <= 2.75f; i += 0.25f) manager.add_sprite_by_type(sprite_types::rock, i, -2.75f);
			//left
			for (float i = -2.75f; i <= 2.75f; i += 0.25f) manager.add_sprite_by_type(sprite_types::rock, 2.75f, i);
			//right
			for (float i = -2.75f; i <= 2.75f; i += 0.25f) manager.add_sprite_by_type(sprite_types::rock, -2.75f, i);

			for (float x = -2.75f; x <= 2.75f; x += 0.25f)
			{
				for (float y = -2.75f; y <= 2.75f; y += 0.25f)
				{
					manager.add_sprite_by_type(sprite_types::ground, x, y);
				}
			}
			//manager.add_sprite_by_type(sprite_types::invaderer, 0, -1.75f);
		}

		// called every frame to move things
		void simulate() {
			manager.simulate();
		}

		// this is called to draw the world
		void draw_world(int x, int y, int w, int h) {
			simulate();
			// set a viewport - includes whole window area
			glViewport(x, y, w, h);

			// clear the background to black
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// don't allow Z buffer depth testing (closer objects are always drawn in front of far ones)
			glDisable(GL_DEPTH_TEST);

			// allow alpha blend (transparency when alpha channel is 0)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// draw all the sprites
			manager.render();

			// move the listener with the camera
			vec4 &cpos = cameraToWorld.w();
			alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
		}
	};
}
