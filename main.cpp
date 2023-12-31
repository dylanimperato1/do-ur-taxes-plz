#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define ENEMY_COUNT 1
#define LEVEL1_WIDTH 10
#define LEVEL1_HEIGHT 4
#define PAPER_COUNT 5

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "entity.hpp"
#include "Map.hpp"
#include <random>

// ————— GAME STATE ————— //
struct GameState
{
    Entity *paper[10];
    Entity *blue_paper;
    Entity *red_paper;
    Entity *purple_paper;
    
    bool completed[PAPER_COUNT] = {false};

    Entity *stapler;
    Entity *blue_stamp;
    Entity *red_stamp;
    Entity *telephone;

    Map *map;
    Entity *background;
    
    Mix_Music *bgm;
    Mix_Chunk *ring_tone;
    Mix_Chunk *staple_sound;
    Mix_Chunk *stamp_sound;
    Mix_Chunk *shreaded;
};

// ————— CONSTANTS ————— //
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char GAME_WINDOW_NAME[] = "Do ur Taxes!!!!!";

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;

const char FONT_FILEPATH[] = "font1.png",
           STAPLER_FILEPATH[] = "STAPLER.png",

           RED_PAPER_FILEPATH[] = "RED_PAPER.png",
           BLUE_PAPER_FILEPATH[] = "BLUE_PAPER.png",
           PURPLE_PAPER_FILEPATH[] = "PURPLE_PAPER.png",
           TELEPHONE_FILEPATH[] = "TELEPHONE.png",

           BLUE_STAMP_FILEPATH[]    = "BLUE_STAMP.png",
           RED_STAMP_FILEPATH[]    = "RED_STAMP.png",
           MAP_TILESET_FILEPATH[] = "tileset.png",
           BACKGROUND_FILEPATH[] = "BACKGROUND.jpg",

           BGM_FILEPATH[] = "A Quick One Before the Eternal Worm Devours Connecticut.mp3",
           RINGTONE_FILEPATH[] = "Rakata.wav",
           STAMP_FILEPATH[] = "stamped.wav",
           STAPLE_FILEPATH[] = "stapled.wav",
           SHRED_FILEPATH[] = "shredded.wav";
           

int phone_score = 0;
int papers_finished = 0;
const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL  = 0;
const GLint TEXTURE_BORDER   = 0;
GLuint m_text_texture;

std::random_device rd;
std::mt19937 rng(rd());
std::uniform_int_distribution<int> uni(0,2);
std::uniform_int_distribution<int> tele(0,1000);

unsigned int LEVEL_1_DATA[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1
};

// ————— VARIABLES ————— //
GameState g_state;
SDL_Window* m_display_window;
bool m_game_is_running = true;

ShaderProgram m_program;
glm::mat4 m_view_matrix, m_projection_matrix;

float m_previous_ticks = 0.0f,
      m_accumulator    = 0.0f;


const glm::vec3 init = glm::vec3(-5.0f, 0.0f, 0.0f);

bool m_winned(){
    int temp = 0;
    for(int i = 0; i < PAPER_COUNT; i++){
        if(g_state.completed[i]){
            temp++;
        }
    }
    temp -= phone_score;
    if(temp == PAPER_COUNT){
        return true;
    }
    return false;
}
// ————— GENERAL FUNCTIONS ————— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint texture_id;
    glGenTextures(NUMBER_OF_TEXTURES, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return texture_id;
}

const int FONTBANK_SIZE = 16;

void DrawText(ShaderProgram *program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for each character
    // Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their position
        //    relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (screen_size + spacing) * i;
        
        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->SetModelMatrix(model_matrix);
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void initialise()
{
    // ————— GENERAL ————— //
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    m_display_window = SDL_CreateWindow(GAME_WINDOW_NAME,
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(m_display_window);
    SDL_GL_MakeCurrent(m_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    // ————— VIDEO SETUP ————— //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    m_program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    m_view_matrix = glm::mat4(1.0f);
    m_view_matrix = glm::translate(m_view_matrix, init);
    m_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    m_program.SetProjectionMatrix(m_projection_matrix);
    m_program.SetViewMatrix(m_view_matrix);
    
    glUseProgram(m_program.programID);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // ---- AUDIO SET_UP ----- //
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 8, 4096);
    
    g_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    Mix_PlayMusic(g_state.bgm, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME/16 );
    
    g_state.ring_tone = Mix_LoadWAV(RINGTONE_FILEPATH);
    g_state.stamp_sound = Mix_LoadWAV(STAMP_FILEPATH);
    g_state.staple_sound = Mix_LoadWAV(STAPLE_FILEPATH);
    g_state.shreaded = Mix_LoadWAV(SHRED_FILEPATH);
    
    // ————— MAP SET-UP ————— //
    GLuint map_texture_id = load_texture(MAP_TILESET_FILEPATH);
    g_state.map = new Map(LEVEL1_WIDTH, LEVEL1_HEIGHT, LEVEL_1_DATA, map_texture_id, 1.0f, 4, 1);
    
    m_text_texture = load_texture(FONT_FILEPATH);
    
    // ----- BACKGROUND SET-UP ----- //
    g_state.background = new Entity;
    g_state.background->set_entity_type(BACKGROUND);
    g_state.background->m_texture_id = load_texture(BACKGROUND_FILEPATH);
    g_state.background->set_position(glm::vec3(5.0f, 0.0f, 0.0f));
    g_state.background->init_pos(glm::vec3(5.0f, 0.0f, 0.0f));
    g_state.background->m_model_matrix = glm::scale(g_state.background->m_model_matrix, glm::vec3(10.5f,8.0f,0.0f));
    
    // ----- PAPER SET-UP ----- //
    for(int i = 0; i < PAPER_COUNT; i++){
        g_state.paper[i] = new Entity;
        switch(uni(rng)){
            case 0:
                g_state.paper[i]->set_entity_type(BLUE_PAPER);
                g_state.paper[i]->m_texture_id = load_texture(BLUE_PAPER_FILEPATH);
                g_state.paper[i]->m_animation_cols = 3;
                g_state.paper[i]->m_animation_frames = 3;
                break;
            case 1:
                g_state.paper[i]->set_entity_type(RED_PAPER);
                g_state.paper[i]->m_texture_id = load_texture(RED_PAPER_FILEPATH);
                g_state.paper[i]->m_animation_cols = 3;
                g_state.paper[i]->m_animation_frames = 3;
                break;
            case 2:
                g_state.paper[i]->set_entity_type(PURPLE_PAPER);
                g_state.paper[i]->m_texture_id = load_texture(PURPLE_PAPER_FILEPATH);
                g_state.paper[i]->m_animation_cols = 4;
                g_state.paper[i]->m_animation_frames = 4;
                break;
        }
        g_state.paper[i]->set_position(glm::vec3(-6.0f * i, -1.5f, 0.0f));
        g_state.paper[i]->init_pos(glm::vec3(-6.0f * i, -1.5f, 0.0f));
        g_state.paper[i]->set_movement(glm::vec3(0.0f));
        g_state.paper[i]->set_speed(1.0f);
        g_state.paper[i]->set_sfx(g_state.shreaded);
        g_state.paper[i]->m_model_matrix = glm::scale(g_state.paper[i]->m_model_matrix, glm::vec3(7.0f,7.0f,0.0f));

    }
    
    // ----- STAPLER SET-UP ----- //
    g_state.stapler = new Entity;
    g_state.stapler->set_entity_type(STAPLER);
    g_state.stapler->set_position(glm::vec3(1.7f, -0.7f, 0.0f));
    g_state.stapler->set_movement(glm::vec3(0.0f));
    g_state.stapler->set_speed(2.5f);
    g_state.stapler->m_texture_id = load_texture(STAPLER_FILEPATH);
    g_state.stapler->init_pos(glm::vec3(1.8f, -2.2f, 0.0f));
    g_state.stapler->set_sfx(g_state.staple_sound);
    g_state.stapler->m_model_matrix = glm::scale(g_state.stapler->m_model_matrix, glm::vec3(1.5f,1.5f,0.0f));
    
    // ----- BLUE STAMP SET-UP ----- //
    g_state.blue_stamp = new Entity;
    g_state.blue_stamp->set_entity_type(BLUE_STAMP);
    g_state.blue_stamp->set_position(glm::vec3(5.0f, -0.5f, 0.0f));
    g_state.blue_stamp->set_movement(glm::vec3(0.0f));
    g_state.blue_stamp->set_speed(2.5f);
    g_state.blue_stamp->m_texture_id = load_texture(BLUE_STAMP_FILEPATH);
    g_state.blue_stamp->init_pos(glm::vec3(5.0f, -1.8f, 0.0f));
    g_state.blue_stamp->set_sfx(g_state.stamp_sound);
    g_state.blue_stamp->m_model_matrix = glm::scale(g_state.blue_stamp->m_model_matrix, glm::vec3(1.75f,1.75f,0.0f));
    
    // ----- RED STAMP SET-UP ----- //
    g_state.red_stamp = new Entity;
    g_state.red_stamp->set_entity_type(RED_STAMP);
    g_state.red_stamp->set_position(glm::vec3(6.0f, -0.5f, 0.0f));
    g_state.red_stamp->set_movement(glm::vec3(0.0f));
    g_state.red_stamp->set_speed(2.5f);
    g_state.red_stamp->m_texture_id = load_texture(RED_STAMP_FILEPATH);
    g_state.red_stamp->init_pos(glm::vec3(6.0f, -1.8f, 0.0f));
    g_state.red_stamp->set_sfx(g_state.stamp_sound);
    g_state.red_stamp->m_model_matrix = glm::scale(g_state.red_stamp->m_model_matrix, glm::vec3(1.75f,1.75f,0.0f));
    

    // ----- TELEPHONE SET-UP ----- //
    g_state.telephone = new Entity;
    g_state.telephone->set_entity_type(PHONE);
    g_state.telephone->set_position(glm::vec3(6.0f, -1.5f, 0.0f));
    g_state.telephone->m_texture_id = load_texture(TELEPHONE_FILEPATH);
    g_state.telephone->init_pos(glm::vec3(4.75f, 0.75f, 0.0f));
    g_state.telephone->set_sfx(g_state.ring_tone);
    g_state.telephone->m_animation_frames = 2;
    g_state.telephone->m_animation_cols = 2;
    g_state.telephone->m_model_matrix = glm::scale(g_state.telephone->m_model_matrix, glm::vec3(2.75f, 3.0f, 0.0f));
    
    
    Mix_Volume(-1, MIX_MAX_VOLUME / 4);
    // ————— BLENDING ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                m_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        m_game_is_running = false;
                        break;
                        
                    case SDLK_SPACE:
                        for(int i = 0; i < PAPER_COUNT; i++){
                            switch(g_state.paper[i]->get_entity_type()){
                                case BLUE_PAPER:
                                    if(g_state.paper[i]->check_collision(g_state.stapler)){
                                        g_state.paper[i]->m_stapled = true;
                                        g_state.paper[i]->m_animation_index += 1;
                                    }
                                    break;
                                case RED_PAPER:
                                    if(g_state.paper[i]->check_collision(g_state.stapler)){
                                        g_state.paper[i]->m_stapled = true;
                                        g_state.paper[i]->m_animation_index += 1;
                                    }
                                    break;
                                case PURPLE_PAPER:
                                    if(g_state.paper[i]->check_collision(g_state.stapler)){
                                        g_state.paper[i]->m_stapled = true;
                                        g_state.paper[i]->m_animation_index += 1;
                                    }
                                    break;
                            }
                        }
                        g_state.stapler->play_sfx();
                        break;
                    case SDLK_i:
                        for(int i = 0; i < PAPER_COUNT; i++){
                            switch(g_state.paper[i]->get_entity_type()){
                                case BLUE_PAPER:
                                    if(g_state.paper[i]->check_collision(g_state.red_stamp)){
                                        g_state.paper[i]->m_stampedred = true;
                                        g_state.paper[i]->m_animation_index += 1;
                                    }
                                    break;
                                case RED_PAPER:
                                    if(g_state.paper[i]->check_collision(g_state.red_stamp)){
                                        g_state.paper[i]->m_stampedred = true;
                                        g_state.paper[i]->m_animation_index += 1;
                                    }
                                    break;
                                case PURPLE_PAPER:
                                    if(g_state.paper[i]->check_collision(g_state.red_stamp)){
                                        g_state.paper[i]->m_stampedred = true;
                                        g_state.paper[i]->m_animation_index += 1;
                                    }
                                    break;
                            }
                        }
                        g_state.red_stamp->play_sfx();
                        break;
                    case SDLK_u:
                        for(int i = 0; i < PAPER_COUNT; i++){
                            switch(g_state.paper[i]->get_entity_type()){
                                case BLUE_PAPER:
                                    if(g_state.paper[i]->check_collision(g_state.blue_stamp)){
                                        g_state.paper[i]->m_stampedblue = true;
                                        g_state.paper[i]->m_animation_index += 1;
                                    }
                                    break;
                                case RED_PAPER:
                                    if(g_state.paper[i]->check_collision(g_state.blue_stamp)){
                                        g_state.paper[i]->m_stampedblue = true;
                                        g_state.paper[i]->m_animation_index += 1;
                                    }
                                    break;
                                case PURPLE_PAPER:
                                    if(g_state.paper[i]->check_collision(g_state.blue_stamp)){
                                        g_state.paper[i]->m_stampedblue = true;
                                        g_state.paper[i]->m_animation_index += 1;
                                    }
                                    break;
                            }
                        }
                        g_state.blue_stamp->play_sfx();
                        break;
                    case SDLK_p:
                        g_state.telephone->hang_up();
                    default:
                        break;
                }
            default:
                break;
        }
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - m_previous_ticks;
    m_previous_ticks = ticks;
    
    delta_time += m_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        m_accumulator = delta_time;
        return;
    }
    
    for(int i = 0; i < PAPER_COUNT; i++){
        g_state.paper[i]->update(FIXED_TIMESTEP, g_state.paper[i], NULL, 0, g_state.map);
        if(g_state.paper[i]->completed_tasks()){
            g_state.completed[i] = true;
        }
        if(g_state.paper[i]->failed_tasks()){
            phone_score++;
        }
        
    }
    
    if(tele(rng) == 0){
        g_state.telephone->is_ringing();
    }
    
    if(g_state.telephone->m_ringing){
        g_state.telephone->ringer += delta_time;
        if(g_state.telephone->ringer > 3){
            phone_score++;
            g_state.telephone->hang_up();
        }
    }
    
    g_state.telephone->update(FIXED_TIMESTEP, g_state.telephone, NULL, 0, g_state.map);
    while (delta_time >= FIXED_TIMESTEP)
    {
        delta_time -= FIXED_TIMESTEP;
    }
    m_accumulator = delta_time;
}

void render()
{
    m_program.SetViewMatrix(m_view_matrix);
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    g_state.map->render(&m_program);

    g_state.background->render(&m_program);
    for(int i = 0; i < PAPER_COUNT; i++){
        g_state.paper[i]->render(&m_program);
    }
    g_state.stapler->render(&m_program);
    g_state.blue_stamp->render(&m_program);
    g_state.red_stamp->render(&m_program);
    g_state.telephone->render(&m_program);

    if(m_winned()){
        DrawText(&m_program, m_text_texture, std::string("WINNAR!"), 0.25f, 0.01f, glm::vec3(0.4f, 0.5f, 0.0f));
    }
    if(!m_winned() && !g_state.paper[PAPER_COUNT-1]->check_active()){
        DrawText(&m_program, m_text_texture, std::string("LOZERR!"), 0.25f, 0.01f, glm::vec3(0.4f, 0.5f, 0.0f));
    }
    SDL_GL_SwapWindow(m_display_window);
}

void shutdown()
{
    SDL_Quit();
    delete g_state.stapler;
    delete g_state.blue_stamp;
    delete g_state.red_stamp;
    delete g_state.map;
    delete g_state.telephone;
    delete g_state.background;
    for(int i = 0; i < PAPER_COUNT; i++){
        delete g_state.paper[i];
    }
    Mix_FreeChunk(g_state.stamp_sound);
    Mix_FreeChunk(g_state.ring_tone);
    Mix_FreeChunk(g_state.staple_sound);
    Mix_FreeMusic(g_state.bgm);
}

// ————— GAME LOOP ————— //
int main(int argc, char* argv[])
{
    initialise();
    
    while (m_game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
