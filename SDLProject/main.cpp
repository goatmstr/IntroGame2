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
float g_previous_ticks = 0.0f;

ShaderProgram g_shader_program;
glm::mat4     g_view_matrix,
g_model_matrix,
g_projection_matrix,
g_other_model_matrix;

GLuint g_player_texture_id,
g_other_texture_id;

glm::vec3 g_player_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player_movement = glm::vec3(5.0f, 0.0f, 0.0f);

glm::vec3 g_other_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_other_movement = glm::vec3(-5.0f, 0.0f, 0.0f);

float g_player_speed = 2.0f;

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

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_S] && g_other_position[1] > -3.2f)
    {
        g_other_movement.y = -1.0f;
    }
    else if (key_state[SDL_SCANCODE_W] && g_other_position[1] < 3.2f)
    {
        g_other_movement.y = 1.0f;
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


bool check_collision(glm::vec3& position_a, glm::vec3& position_b)
{
    float x_distance = fabs(position_a[0] - position_b[0]) - 1;
    float y_distance = fabs(position_a[1] - position_b[1]) - 1;

    if (x_distance < 0.0f && y_distance < 0.0f)
    {
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

    g_other_model_matrix = glm::mat4(1.0f);
    g_other_position += g_other_movement * g_player_speed * delta_time;
    g_other_model_matrix = glm::translate(g_other_model_matrix, g_other_position);

    if (check_collision(g_player_position, g_other_position))
    {
        g_model_matrix = glm::mat4(1.0f);
    }

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