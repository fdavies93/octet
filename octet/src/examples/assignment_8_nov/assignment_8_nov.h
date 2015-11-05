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
		lock = 5,
		boss = 6,
		enemy_bullet = 7,
		type_number = 8,
	};

	enum sprite_directions {
		NONE = -1,
		LEFT = 0,
		RIGHT = 1,
		UP = 2,
		DOWN = 3,
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

		int health;

		sprite() {
			texture = 0;
			type = sprite_types::type_null;
			enabled = false;
			facing = sprite_directions::DOWN;
			health = 0;
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

		void render(assignment_shader &shader, mat4t &cameraToWorld) {
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

		//up and right are the positive axis; just as you would expect in cartesian space
		void separateFrom(sprite& toSeparateFrom) {//moves self away from toSeparateFrom; does not affect other sprite
			/*float dx = fabsf(toSeparateFrom.getX() - getX());
			float dy = fabsf(toSeparateFrom.getY() - getY());
			float sumHalfWidths = toSeparateFrom.getHalfWidth() + getHalfWidth();
			float sumHalfHeights = toSeparateFrom.getHalfHeight() + getHalfHeight();

			if (fabsf(getPrevX() - getX()) > fabsf(getPrevY() - getY()))
			{
				if (getX() < toSeparateFrom.getX())//i.e. to LEFT of object
				{
					translate(dx - sumHalfWidths - 0.00125f, 0);
				}
				else//to RIGHT
				{
					translate(sumHalfWidths - dx + 0.00125f, 0);
				}
			}
			else if (fabsf(getPrevY() - getY()) > fabsf(getPrevX() - getX())){
				if (getY() < toSeparateFrom.getY())
				{
					translate(0, dy - sumHalfHeights - 0.00125f);
				}
				else
				{
					translate(0, sumHalfHeights - dy + 0.00125f);
				}
			}*/
			
			if (getX() != getPrevX())//moved on the x axis
			{
				translate(getPrevX()-getX(),0);
			}

			if (getY() != getPrevY())//moved on the y axis
			{
				translate(0, getPrevY() - getY());
			}
		}
	};

	struct sprite_type_data
	{
		//basic initialisation stuff
		int _texture;
		float w;
		float h;
		bool collides;
		int health;
	};

	struct exit_data
	{
		std::string leads_to;
		float new_x;
		float new_y;
	};

	//stores and manages game data in convenient ways
	class game_manager
	{		
	private:
		const float PLAYER_SPEED = 0.075f;
		const float BULLET_BUFFER = 0.1f;
		const float BULLET_SPEED = 0.1f;
		const unsigned int BULLET_DAMAGE = 10;
		const float ENEMY_SPEED = 0.05f;
		const float ENEMY_TURN_CHANCE = 0.05f;//a percentage
		const float TILE_WIDTH = 0.25f;//i.e. 32 pixels
		const float MAP_X_OFFSET = -3.0f;
		const float MAP_Y_OFFSET = -3.0f;

		octet::app* myApp;
		sprite dummy;
		sprite contained_sprites[max_sprites];
		ALuint sound_sources[num_sound_sources];
		ALuint cur_sound_source;
		sprite_type_data sprite_data[type_number];
		std::list<int> colliding_sprites;
		std::list<int> background_sprites;
		std::vector<sprite*> removal_list;
		assignment_shader *shader;
		mat4t *worldCamera;
		math::random randomiser;//declared up here so it has a better range of (pseudo-)randomness
		GLuint font_texture;
		bitmap_font* text_font;
		
		
		exit_data exits[4];//same enumeration as sprite directions
		int enemy_count; 
		sprite* player_sprite;
		sprite* boss_sprite;
		sprite_directions exit_flag;//set to direction of exit on request for exit

		bool win_flag;
		bool lose_flag;

	public:

		game_manager()
		{
		}

		ALuint get_sound_source() { return sound_sources[cur_sound_source++ % num_sound_sources]; }

		void init(octet::app* usedApp, assignment_shader &texture_shader_, mat4t &cameraToWorld, bitmap_font *font)
		{
			
			myApp = usedApp;
			shader = &texture_shader_;
			worldCamera = &cameraToWorld;
			text_font = font;

			//setting up sprite data
			//player, enemy, rock, bullet, ground
			sprite_data[player].h = 0.25f;
			sprite_data[player].w = 0.25f;// = 32px
			sprite_data[player]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/player.gif");
			sprite_data[player].collides = true;
			sprite_data[player].health = 100;

			sprite_data[enemy].h = 0.25f;
			sprite_data[enemy].w = 0.25f;
			sprite_data[enemy]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/enemy.gif");
			sprite_data[enemy].collides = true;
			sprite_data[enemy].health = 50;

			sprite_data[rock].h = 0.25f;
			sprite_data[rock].w = 0.25f;
			sprite_data[rock]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/rock.gif");
			sprite_data[rock].collides = true;

			sprite_data[bullet].h = 0.0625f;
			sprite_data[bullet].w = 0.0625f;
			sprite_data[bullet]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/bullet.gif");
			sprite_data[bullet].collides = true;

			sprite_data[enemy_bullet].h = 0.0625f;
			sprite_data[enemy_bullet].w = 0.0625f;
			sprite_data[enemy_bullet]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/enemy_bullet.gif");
			sprite_data[enemy_bullet].collides = true;

			sprite_data[ground].h = 6.0f;
			sprite_data[ground].w = 6.0f;
			sprite_data[ground]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/ground.gif");
			sprite_data[ground].collides = false;

			sprite_data[lock].h = 0.25f;
			sprite_data[lock].w = 0.25f;
			sprite_data[lock]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/lock.gif");
			sprite_data[lock].collides = true;

			sprite_data[boss].h = 0.25f;
			sprite_data[boss].w = 0.25f;
			sprite_data[boss]._texture = resource_dict::get_texture_handle(GL_RGBA, "assets/assignment_8_nov/enemy.gif");
			sprite_data[boss].collides = true;
			sprite_data[boss].health = 500;

			//...and sounds
			cur_sound_source = 0;
			alGenSources(num_sound_sources, sound_sources);

			//and text
			font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");

			//and misc
			time_t now = time(0);
			unsigned int now_i = unsigned int(now);//just here for a bit of indeterminacy
			randomiser.set_seed(now_i);
			enemy_count = 0;
			player_sprite = NULL;
			boss_sprite = NULL;
			exit_flag = sprite_directions::NONE;
			win_flag = false;
			lose_flag = false;

			//draws ground, loads objects
			load_map_from_csv("../assets/assignment_8_nov/stage1.csv");
		}

		void play_sound(string fileName)
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
					contained_sprites[cur_sprite].health = sprite_data[type].health;
					if (sprite_data[type].collides) colliding_sprites.push_back(cur_sprite);
					else background_sprites.push_back(cur_sprite);
					if (type == sprite_types::enemy || type == sprite_types::boss) ++enemy_count;
					else if (type == sprite_types::player) player_sprite = &contained_sprites[cur_sprite];
					if (type == sprite_types::boss) boss_sprite = &contained_sprites[cur_sprite];
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
			if (toRemove.get_type() == sprite_types::enemy) enemy_count--;
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
			sprite& returnSprite = add_sprite(sprite_data[type]._texture, x, y, sprite_data[type].w, sprite_data[type].h, type);
			return returnSprite;
		}
		
		bool load_map_from_csv(std::string file_path)
		{
			enemy_count = 0;
			for (int i = 0; i < 4; ++i) {
				exits[i].leads_to = "NULL";
				exits[i].new_x = 0.0f;
				exits[i].new_y = 0.0f;
			}
			std::ifstream input_file;
			char cur_line[2048];
			std::string cur_data;
			int type_id;
			int cur_entry;
			char data_entry, cur_exit;
			unsigned int cur_x, cur_y;
			bool readHeader = false;
			sprite_types cur_type = sprite_types::type_null;

			input_file.open(file_path.c_str(), std::ios_base::in);

			if (input_file.fail()) {
				printf("error in opening file\n");
				return false;
			}

			cur_y = -1;

			while (!input_file.eof())
			{
				input_file.getline(cur_line, sizeof(cur_line));
				cur_data.clear();
				cur_x = 0;
				for (int col = 0; ; ++col)
				{
					if (cur_line[col] == ',' || (cur_line[col] == 0 && !cur_data.empty())) {
						if (cur_y == -1 && cur_x < 12)
						{
							data_entry = cur_x % 3;
							switch (data_entry)
							{
								case 0:
									cur_exit = cur_x / 3;
									exits[cur_exit].leads_to = cur_data;
									break;
								case 1:
									exits[cur_exit].new_x = std::stof(cur_data);
									break;
								case 2:
									exits[cur_exit].new_y = std::stof(cur_data);
									break;
							}
						}
						else if (cur_y != -1) {
							cur_type = sprite_types::type_null;
							if (cur_data == "R") cur_type = sprite_types::rock;
							else if (cur_data == "L") cur_type = sprite_types::lock;
							else if (cur_data == "E") cur_type = sprite_types::enemy;
							else if (cur_data == "B") cur_type = sprite_types::boss;
							if (cur_type != sprite_types::type_null) add_sprite_by_type(cur_type, (cur_x * TILE_WIDTH) + MAP_X_OFFSET, (cur_y * TILE_WIDTH) + MAP_Y_OFFSET);
						}
						cur_data.clear();
						++cur_x;
					}
					else if (cur_line[col] != 0) cur_data += cur_line[col];
					if (cur_line[col] == 0 && cur_data.empty()) break;
				}
				++cur_y;
			}
			input_file.close();
			return true;
		}

		bool go_through_exit(sprite_directions exit_direction)
		{
			if (exits[exit_direction].leads_to == "NULL") return false;

			for (int i = 0; i < constants::max_sprites; ++i)
			{
				if (contained_sprites[i].is_enabled() && contained_sprites[i].get_type() != sprite_types::player) remove_sprite(contained_sprites[i]);
			}
			player_sprite->reset_position();
			player_sprite->translate(exits[exit_direction].new_x, exits[exit_direction].new_y);
			load_map_from_csv(exits[exit_direction].leads_to);
			return true;
		}

		void draw_text(assignment_shader &shader, float x, float y, float scale, const char *text) {
			mat4t modelToWorld;
			modelToWorld.loadIdentity();
			modelToWorld.translate(x, y, 0);
			modelToWorld.scale(scale, scale, 1);
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, *worldCamera);

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

			unsigned num_quads = text_font->build_mesh(bb, vertices, indices, max_quads, text, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, font_texture);

			shader.render(modelToProjection, 0);

			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].x);
			glEnableVertexAttribArray(attribute_pos);
			glVertexAttribPointer(attribute_uv, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].u);
			glEnableVertexAttribArray(attribute_uv);

			glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, indices);
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
			if (player_sprite != NULL)
			{
				char cur_text[32];
				sprintf(cur_text,"HP: %d",player_sprite->health);
				draw_text(*shader, -1.75f, 2, 1.0f / 256, cur_text);
			}
			if (boss_sprite != NULL)
			{
				char boss_text[32];
				sprintf(boss_text, "Boss HP: %d", boss_sprite->health);
				draw_text(*shader, 2.0f, 2, 1.0f / 256, boss_text);
			}
			if (win_flag) {
				draw_text(*shader, 0.5f, -0.75f, 1.0f / 256, "VICTORY!");
			}
			if (lose_flag) {
				draw_text(*shader, 0.5f, -0.75f, 1.0f / 256, "GAME OVER!");
			}
		}

		void simulate_object(sprite &object)
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

				if (object.getX() > 3.0f || object.getX() < -3.0f || object.getY() > 3.0f || object.getY() < -3.0f)
				{
					if (object.getX() > 3.0f) exit_flag = sprite_directions::RIGHT;
					else if (object.getX() < -3.0f) exit_flag = sprite_directions::LEFT;
					if (object.getY() > 3.0f) exit_flag = sprite_directions::UP;
					else if (object.getY() < -3.0f) exit_flag = sprite_directions::DOWN;
				}
				if (object.health <= 0) {
					lose_flag = true;
					play_sound("assets/assignment_8_nov/failure.wav");
				}
			}
			else if (object.get_type() == sprite_types::enemy) {
				switch (object.facing){
				case sprite_directions::UP:
					object.translate(0, ENEMY_SPEED);
					break;
				case sprite_directions::DOWN:
					object.translate(0, -ENEMY_SPEED);
					break;
				case sprite_directions::LEFT:
					object.translate(-ENEMY_SPEED, 0);
					break;
				case sprite_directions::RIGHT:
					object.translate(ENEMY_SPEED, 0);
					break;
				}
				
				float will_turn = randomiser.get(0.0f, 1.0f);
				if (will_turn < ENEMY_TURN_CHANCE)
				{
					float turn_direction = randomiser.get(0.0f, 1.0f);
					if (turn_direction < 0.25f) object.facing = sprite_directions::UP;
					else if (turn_direction < 0.5f) object.facing = sprite_directions::DOWN;
					else if (turn_direction < 0.75f) object.facing = sprite_directions::LEFT;
					else if (turn_direction < 1.0f) object.facing = sprite_directions::RIGHT;
					add_sprite_by_type(sprite_types::enemy_bullet, object.getX(), object.getY()).facing = object.facing;
				}
				if (object.getX() < -3.0f || object.getX() > 3.0f || object.getY() < -3.0f || object.getY() > 3.0f) removal_list.push_back(&object);
				//this just prevents the minor collision issues from ever being game breaking
			}
			else if (object.get_type() == sprite_types::bullet) {
				if (object.facing == sprite_directions::UP) object.translate(0, BULLET_SPEED);
				if (object.facing == sprite_directions::DOWN) object.translate(0, -BULLET_SPEED);
				if (object.facing == sprite_directions::LEFT) object.translate(-BULLET_SPEED, 0);
				if (object.facing == sprite_directions::RIGHT) object.translate(BULLET_SPEED, 0);
				if (object.getX() < -3.0f || object.getX() > 3.0f || object.getY() > 3.0f || object.getY() < -3.0f) removal_list.push_back(&object);
			}
			else if (object.get_type() == sprite_types::enemy_bullet) {
				if (object.facing == sprite_directions::UP) object.translate(0, BULLET_SPEED);
				if (object.facing == sprite_directions::DOWN) object.translate(0, -BULLET_SPEED);
				if (object.facing == sprite_directions::LEFT) object.translate(-BULLET_SPEED, 0);
				if (object.facing == sprite_directions::RIGHT) object.translate(BULLET_SPEED, 0);
				if (object.getX() < -3.0f || object.getX() > 3.0f || object.getY() > 3.0f || object.getY() < -3.0f) removal_list.push_back(&object);
			}
			else if (object.get_type() == sprite_types::lock) {
				if (enemy_count == 0) removal_list.push_back(&object);
			}
			else if (object.get_type() == sprite_types::boss) {
				
				if (player_sprite->getX() - player_sprite->getHalfWidth() >= object.getX() && fabsf(object.getX() - player_sprite->getX()) > 0.5f)
				{
					object.translate(ENEMY_SPEED, 0);
				}
				else if (player_sprite->getX() + player_sprite->getHalfWidth() <= object.getX() && fabsf(object.getX() - player_sprite->getX()) > 0.5f)
				{
					object.translate(-ENEMY_SPEED, 0);
				}
				if (player_sprite->getY() - player_sprite->getHalfHeight() >= object.getY() && fabsf(object.getY() - player_sprite->getY()) > 0.5f)
				{
					object.translate(0, ENEMY_SPEED);
				}
				else if (player_sprite->getY() + player_sprite->getHalfHeight() <= object.getY() && fabsf(object.getY() - player_sprite->getY()) > 0.5f)
				{
					object.translate(0, -ENEMY_SPEED);
				}

				if (randomiser.get(0.0f, 1.0f) < 0.05f) {
					sprite* newBullet = &add_sprite_by_type(sprite_types::enemy_bullet, object.getX(), object.getY());
					if (player_sprite->getX() < object.getX()) newBullet->facing = sprite_directions::LEFT;
					else newBullet->facing = sprite_directions::RIGHT;
					newBullet = &add_sprite_by_type(sprite_types::enemy_bullet, object.getX(), object.getY());
					if (player_sprite->getY() < object.getY()) newBullet->facing = sprite_directions::DOWN;
					else newBullet->facing = sprite_directions::UP;
				}

				/*float fire_chance = randomiser.get(0.0f, 1.0f);
				if (fire_chance < 0.25f)
				{
					sprite& newBullet = add_sprite_by_type(sprite_types::enemy_bullet, object.getX(), object.getY());
					newBullet.facing = sprite_directions::LEFT;
				}*/
			}
		}

		void simulate_objects()
		{
			std::list<int>::iterator sprite_i;
			for (sprite_i = colliding_sprites.begin(); sprite_i != colliding_sprites.end(); ++sprite_i)
			{
				simulate_object(contained_sprites[*sprite_i]);
			}
		}

		void turn_enemy(sprite &object)
		{
			float newFacing = randomiser.get(0.0f, 1.0f);
			if (newFacing < 0.25f) object.facing = sprite_directions::DOWN;
			else if (newFacing < 0.5f) object.facing = sprite_directions::UP;
			else if (newFacing < 0.75f) object.facing = sprite_directions::LEFT;
			else if (newFacing < 1.00f) object.facing = sprite_directions::RIGHT;
		}

		void resolve_collision(sprite &subject, sprite &object)
		{
			//note: subject is always the thing ACTING UPON the object
			//that is, the properties of the subject NEVER change
			//this just keeps things organised
			//also; objects never remove one another directly -- they always add one another to the removal list
			//this is to prevent problems with ordering objects / multiple collisions
			if (subject.get_type() == sprite_types::rock)
			{
				if(object.get_type() == sprite_types::bullet) removal_list.push_back(&object);
				else if (object.get_type() == sprite_types::enemy_bullet) removal_list.push_back(&object);
				else if (object.get_type() == sprite_types::player || object.get_type() == sprite_types::enemy || object.get_type() == sprite_types::boss)
				{
					object.separateFrom(subject);
					//i.e. move it back to its previous location
					if (object.get_type() == sprite_types::enemy) turn_enemy(object);
				}
			}
			else if (subject.get_type() == sprite_types::bullet)
			{
				if (object.get_type() == sprite_types::enemy || object.get_type() == sprite_types::boss)
				{
					object.health -= 10;
					if (object.health <= 0) {
						removal_list.push_back(&object);
						if (object.get_type() == sprite_types::boss) {
							win_flag = true;
							play_sound("assets/assignment_8_nov/triumph.wav");
						}
					}
				}
			}
			else if (subject.get_type() == sprite_types::enemy)
			{
				if (object.get_type() == sprite_types::enemy)
				{
					//object.reset_position();
					//object.translate(object.getPrevX(),object.getPrevY());
					object.separateFrom(subject);
					float newFacing = randomiser.get(0.0f, 1.0f);
					if (newFacing < 0.25f) object.facing = sprite_directions::DOWN;
					else if (newFacing < 0.5f) object.facing = sprite_directions::UP;
					else if (newFacing < 0.75f) object.facing = sprite_directions::LEFT;
					else if (newFacing < 1.00f) object.facing = sprite_directions::RIGHT;
				}
				else if (object.get_type() == sprite_types::bullet)
				{
					removal_list.push_back(&object);
				}
				else if (object.get_type() == sprite_types::player)
				{
					object.health -= 2;
					object.separateFrom(subject);
				}
			}
			else if (subject.get_type() == sprite_types::player) {
				if (object.get_type() == sprite_types::enemy)
				{
					object.separateFrom(subject);
					float newFacing = randomiser.get(0.0f, 1.0f);
					if (newFacing < 0.25f) object.facing = sprite_directions::DOWN;
					else if (newFacing < 0.5f) object.facing = sprite_directions::UP;
					else if (newFacing < 0.75f) object.facing = sprite_directions::LEFT;
					else if (newFacing < 1.00f) object.facing = sprite_directions::RIGHT;
				}
				else if (object.get_type() == sprite_types::enemy_bullet)
				{
					removal_list.push_back(&object);
				}
				else if (object.get_type() == sprite_types::boss)
				{
					object.separateFrom(subject);
				}
			}
			else if (subject.get_type() == sprite_types::lock)
			{
				if (object.get_type() == sprite_types::bullet) removal_list.push_back(&object);
				else if (object.get_type() == sprite_types::enemy_bullet) removal_list.push_back(&object);
				else if (object.get_type() == sprite_types::player || object.get_type() == sprite_types::enemy)
				{
					object.separateFrom(subject);
					//i.e. move it back to its previous location
					if (object.get_type() == sprite_types::enemy) turn_enemy(object);
				}
			}
			else if (subject.get_type() == sprite_types::boss) {
				if (object.get_type() == sprite_types::bullet) removal_list.push_back(&object);
				if (object.get_type() == sprite_types::player)
				{
					object.health -= 4;
					object.separateFrom(subject);
					if (object.health <= 0) play_sound("assets/assignment_8_nov/failure.wav");
				}
			}
			else if (subject.get_type() == sprite_types::enemy_bullet)
			{
				if (object.get_type() == sprite_types::player) object.health -= 5;
			}
		}

		void check_collisions()
		{
			std::list<int>::iterator sprite1_i, sprite2_i;
			sprite *sprite1, *sprite2;
			for (sprite1_i = colliding_sprites.begin(); sprite1_i != colliding_sprites.end(); ++sprite1_i)
			{
				sprite1 = &contained_sprites[*sprite1_i];
				for (sprite2_i = colliding_sprites.begin(); sprite2_i != colliding_sprites.end(); ++sprite2_i)
				{
					sprite2 = &contained_sprites[*sprite2_i];
					if (sprite1->collides_with(*sprite2) && sprite1 != sprite2) resolve_collision(*sprite1, *sprite2);
				}
			}
		}

		void remove_dead_objects()
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
			simulate_objects();
			//resolve collisions
			check_collisions();
			//remove objects on remove list
			remove_dead_objects();

			if (exit_flag != sprite_directions::NONE)
			{
				go_through_exit(exit_flag);
				exit_flag = sprite_directions::NONE;
			}
		}

		void update()
		{
			if(!win_flag && !lose_flag)	simulate();
			render();
		}

	};

	class assignment_8_nov : public octet::app {
		// Matrix to transform points in our camera space to the world.
		// This lets us move our camera
		mat4t cameraToWorld;

		// shader to draw a textured triangle
		assignment_shader texture_shader_;

		game_manager manager;

		// a texture for our text
		GLuint font_texture;

		// information for our text
		bitmap_font font;

		

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

			manager.init(this, texture_shader_, cameraToWorld, &font);

			manager.add_sprite_by_type(sprite_types::player, 0, 0);
		}

		// called every frame to move things
		void simulate() {
			manager.simulate();
		}

		// this is called to draw the world
		void draw_world(int x, int y, int w, int h) {
			//simulate();
			// set a viewport - includes whole window area
			glViewport(x, y, w, h);

			// clear the background to a nice color
			// rgb thing
			glClearColor(0.125, 0.125, 0.2, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// don't allow Z buffer depth testing (closer objects are always drawn in front of far ones)
			glDisable(GL_DEPTH_TEST);

			// allow alpha blend (transparency when alpha channel is 0)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// draw all the sprites, etc.
			manager.update();

			//draw_text(texture_shader_, -1.75f, 2, 1.0f / 256, "drawn");

			// move the listener with the camera
			vec4 &cpos = cameraToWorld.w();
			alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
		}
	};
}
