/**
* Author: Hongbin Pan
* Assignment: Pong Clone
* Date due: 2024-03-02, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

#include <ctime>   
#include "cmath"   

const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

float BG_RED = 0.0f,
BG_BLUE = 0.1f,
BG_GREEN = 0.1f,
BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char  V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl",
PLAYER_SPRITE_FILEPATH[] = "soph.png";

const float MILLISECONDS_IN_SECOND = 1000.0;
const float MINIMUM_COLLISION_DISTANCE = 1.0f;

const int   NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL = 0;
const GLint TEXTURE_BORDER = 0;

SDL_Window* g_display_window;
bool g_game_is_running = true;
bool game_over = false;
float g_previous_ticks = 0.0f;

ShaderProgram g_shader_program;
glm::mat4     g_view_matrix,
g_model_matrix,
g_projection_matrix,
g_other_model_matrix,
g_ball_matrix;

GLuint g_player_texture_id,
g_other_texture_id,
g_ball_texture_id;

glm::vec3 g_player_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player_movement = glm::vec3(5.0f, 0.0f, 0.0f);

glm::vec3 g_other_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_other_movement = glm::vec3(-5.0f, 0.0f, 0.0f);

glm::vec3 g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(1.0f, 0.0f, 0.0f);

float g_player_speed = 6.0f;
float g_ball_speed = 7.0f;

bool singleplayer_mode = false;

GLuint load_texture(const char* filepath)
{
	int width, height, number_of_components;
	unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

	if (image == NULL)
	{
		LOG("Unable to load image. Make sure the path is correct.");
		assert(false);
	}

	GLuint textureID;
	glGenTextures(NUMBER_OF_TEXTURES, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_image_free(image);

	return textureID;
}

void initialise()
{
	SDL_Init(SDL_INIT_VIDEO);
	g_display_window = SDL_CreateWindow("Pong",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_OPENGL);

	SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
	SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	g_shader_program.Load(V_SHADER_PATH, F_SHADER_PATH);

	g_model_matrix = glm::mat4(1.0f);
    g_other_model_matrix = glm::mat4(1.0f);

	g_model_matrix = glm::translate(g_model_matrix, g_player_movement);
	g_player_position += g_player_movement;

	g_other_model_matrix = glm::translate(g_other_model_matrix, g_other_movement);
	g_other_position += g_other_movement;

	g_view_matrix = glm::mat4(1.0f);
	g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

	//g_player_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);
	//g_other_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);

	g_shader_program.SetProjectionMatrix(g_projection_matrix);
	g_shader_program.SetViewMatrix(g_view_matrix);

	glUseProgram(g_shader_program.programID);
	glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_player_movement = glm::vec3(0.0f);
    g_other_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_q:
                g_game_is_running = false;
                break;
            case SDLK_t:
                if (!game_over) {
                   singleplayer_mode = true;
                }
                
            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);
    
    if (!game_over) {
        if (!singleplayer_mode) {
            if (key_state[SDL_SCANCODE_S] && g_other_position[1] > -3.2f)
            {
                g_other_movement.y = -1.0f;
            }
            else if (key_state[SDL_SCANCODE_W] && g_other_position[1] < 3.2f)
            {
                g_other_movement.y = 1.0f;
            }
        }

        if (key_state[SDL_SCANCODE_UP] && g_player_position[1] < 3.2f)
        {
            g_player_movement.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_DOWN] && g_player_position[1] > -3.2f)
        {
            g_player_movement.y = -1.0f;
        }
    }
    

    
}


bool check_collision(glm::vec3& position_ball, glm::vec3& position_player, glm::vec3& position_other)
{
    float x_distance_p = fabs(position_ball[0] - position_player[0]) - ((1 + 0.25) / 2);
    float y_distance_p = fabs(position_ball [1] - position_player[1]) - ((1 + 0.25) / 2);
    float x_distance_o = fabs(position_ball[0] - position_other[0]) - ((1 + 0.25) / 2);
    float y_distance_o = fabs(position_ball[1] - position_other[1]) - ((1 + 0.25) / 2);

    if (x_distance_p < 0.0f && y_distance_p < 0.0f){
        return true;
    }
    if (x_distance_o < 0.0f && y_distance_o < 0.0f){
        return true;
    }
    else {
        return false;
    }
}


void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_model_matrix = glm::mat4(1.0f);
    g_player_position += g_player_movement * g_player_speed * delta_time;
    g_model_matrix = glm::translate(g_model_matrix, g_player_position);

    if (!singleplayer_mode) {
        g_other_model_matrix = glm::mat4(1.0f);
        g_other_position += g_other_movement * g_player_speed * delta_time;
        g_other_model_matrix = glm::translate(g_other_model_matrix, g_other_position);
    }
    else {
        if (sin(ticks) > 0) { // Using sin for back and forth movement
            g_other_movement = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        else {
            g_other_movement = glm::vec3(0.0f, -1.0f, 0.0f);
        }
        g_other_model_matrix = glm::mat4(1.0f);
        g_other_position += g_other_movement * g_player_speed * delta_time;
        if (g_other_position[1] > 3.2f || g_other_position[1] < -3.2f) { 
            // If second player about to go out of bounds, reverse movement direction
            g_other_position += (g_other_movement *= glm::vec3(0.0f, -1.0f, 0.0f)) * g_player_speed * delta_time;
        }
        g_other_model_matrix = glm::translate(g_other_model_matrix, g_other_position);
    }

    g_ball_matrix = glm::mat4(1.0f);
    g_ball_position += g_ball_movement * g_ball_speed * delta_time;

    if (check_collision(g_ball_position, g_player_position, g_other_position)) 
        // Collision detection and bounce
    {
        g_ball_movement *= glm::vec3(-1.0f, 0.0f, 0.0f);
        if (g_ball_position[1] < g_player_position[1] || g_ball_position[1] < g_other_position[1]) {
            g_ball_movement += glm::vec3(0.0f, -1.0f, 0.0f);
        }
        else if (g_ball_position[1] > g_player_position[1] || g_ball_position[1] > g_other_position[1]) {
            g_ball_movement += glm::vec3(0.0f, 1.0f, 0.0f);
        }
        g_ball_position += g_ball_movement * g_ball_speed * delta_time;
    }
    if (g_ball_position[1] > 3.2f) {
        g_ball_movement *= glm::vec3(1.0f, -1.0f, 0.0f);
    }
    else if (g_ball_position[1] < -3.2f) {
        g_ball_movement *= glm::vec3(1.0f, -1.0f, 0.0f);
    }
    
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);
    if (g_ball_position[0] > 6.0f || g_ball_position[0] < -6.0f) {
        game_over = true;
    }
    g_ball_matrix = glm::scale(g_ball_matrix, glm::vec3(0.25f, 0.25f, 0.0f));
}


void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.SetModelMatrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };

    glVertexAttribPointer(g_shader_program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.positionAttribute);

    glVertexAttribPointer(g_shader_program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.texCoordAttribute);

    draw_object(g_model_matrix, g_player_texture_id);
    draw_object(g_other_model_matrix, g_other_texture_id);
    draw_object(g_ball_matrix, g_ball_texture_id);

    glDisableVertexAttribArray(g_shader_program.positionAttribute);
    glDisableVertexAttribArray(g_shader_program.texCoordAttribute);

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }

int main(int argc, char* argv[])
{
	initialise();

	while (g_game_is_running)
	{
		process_input();
		update();
		render();
	}

	shutdown();
	return 0;
}