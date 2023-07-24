#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "entity.hpp"

Entity::Entity()
{
    m_position     = glm::vec3(0.0f);
    m_velocity     = glm::vec3(0.0f);
    
    m_movement = glm::vec3(0.0f);
    
    m_speed = 0;
    m_model_matrix = glm::mat4(1.0f);
}

Entity::~Entity()
{
}

void Entity::init_pos(glm::vec3 initpos){
    m_model_matrix = glm::translate(m_model_matrix, initpos);
}

void Entity::draw_sprite_from_texture_atlas(ShaderProgram *program, GLuint texture_id, int index)
{
    // Step 1: Calculate the UV location of the indexed frame
    float u_coord = (float) (index % m_animation_cols) / (float) m_animation_cols;
    float v_coord = (float) (index / m_animation_cols) / (float) m_animation_rows;
    
    // Step 2: Calculate its UV size
    float width = 1.0f / (float) m_animation_cols;
    float height = 1.0f / (float) m_animation_rows;
    
    // Step 3: Just as we have done before, match the texture coordinates to the vertices
    float tex_coords[] =
    {
        u_coord, v_coord + height, u_coord + width, v_coord + height, u_coord + width, v_coord,
        u_coord, v_coord + height, u_coord + width, v_coord, u_coord, v_coord
    };
    
    float vertices[] =
    {
        -0.5, -0.5, 0.5, -0.5,  0.5, 0.5,
        -0.5, -0.5, 0.5,  0.5, -0.5, 0.5
    };
    
    // Step 4: And render
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void Entity::activate_ai(Entity *player)
{
    switch (m_entity_type)
    {
        case BLUE_PAPER || RED_PAPER || PURPLE_PAPER:
            ai_slide();
            break;
        default:
            break;
    }
}

void Entity::ai_slide()
{
    m_movement = glm::vec3(1.0f, 0.0f, 0.0f);
}

void Entity::ai_fall()
{
    m_movement = glm::vec3(0.0f, -0.6f, 0.0f);
}


void Entity::update(float delta_time, Entity *player, Entity *objects, int object_count, Map *map)
{
    if (!m_is_active) return;
    
    if(m_entity_type == PHONE){
        if(m_ringing){
            if(m_animation_time < 0.25){
                m_animation_time += delta_time;
            }
            else{
                if(m_animation_index == 0){ m_animation_index = 1;}
                if(m_animation_index == 1){ m_animation_index = 0;}
                m_animation_time = 0;
            }
        }
        return;
    }
    if(!m_collided_right){
        ai_slide();
        m_velocity.x = m_movement.x * m_speed;
        m_velocity.y = m_movement.y * m_speed;
        m_position.y += m_velocity.y * delta_time;
        m_position.x += m_velocity.x * delta_time;
        check_collision_y(objects, object_count);
        check_collision_y(map);
        check_collision_x(objects, object_count);
        check_collision_x(map);

    }
    else{
        m_is_active = false;

        if(completed_tasks()){
            Mix_PlayChannel(-1, m_sfx, 0);
        }
    }
    
    
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
    m_model_matrix = glm::scale(m_model_matrix, glm::vec3(1.5f,4.0f,0.0f));
}

void const Entity::play_sfx(){
     channel = Mix_PlayChannel(-1, m_sfx, 0);
}


void const Entity::check_collision_y(Entity *collidable_entities, int collidable_entity_count)
{
    for (int i = 0; i < collidable_entity_count; i++)
    {
        Entity *collidable_entity = &collidable_entities[i];
        
        if (check_collision(collidable_entity))
        {
            float y_distance = fabs(m_position.y - collidable_entity->get_position().y);
            float y_overlap = fabs(y_distance - (m_height / 2.0f) - (collidable_entity->m_height / 2.0f));
            if (m_position.y > 0) {
                m_position.y   -= y_overlap;
                m_velocity.y    = 0;
                m_collided_top  = true;
            } else if (m_velocity.y < 0) {
                m_position.y      += y_overlap;
                m_velocity.y       = 0;
                m_collided_bottom  = true;
            }
        }
    }
}

void const Entity::check_collision_x(Entity *collidable_entities, int collidable_entity_count){
    for (int i = 0; i < collidable_entity_count; i++)
    {
        Entity *collidable_entity = &collidable_entities[i];
        
        if (check_collision(collidable_entity))
        {
            float x_distance = fabs(m_position.x - collidable_entity->get_position().x);
            float x_overlap = fabs(x_distance - (m_width / 2.0f) - (collidable_entity->get_width() / 2.0f));
            if (m_velocity.x > 0) {
                m_position.x     -= x_overlap;
                m_velocity.x      = 0;
                m_collided_right  = true;
            } else if (m_velocity.x < 0) {
                m_position.x    += x_overlap;
                m_velocity.x     = 0;
                m_collided_left  = true;
            }
        }
    }
}

void const Entity::check_collision_y(Map *map)
{
    // Probes for tiles above
    glm::vec3 top = glm::vec3(m_position.x, m_position.y + (m_height / 2), m_position.z);
    glm::vec3 top_left = glm::vec3(m_position.x - (m_width / 2), m_position.y + (m_height / 2), m_position.z);
    glm::vec3 top_right = glm::vec3(m_position.x + (m_width / 2), m_position.y + (m_height / 2), m_position.z);
    
    // Probes for tiles below
    glm::vec3 bottom = glm::vec3(m_position.x, m_position.y - (m_height / 2), m_position.z);
    glm::vec3 bottom_left = glm::vec3(m_position.x - (m_width / 2), m_position.y - (m_height / 2), m_position.z);
    glm::vec3 bottom_right = glm::vec3(m_position.x + (m_width / 2), m_position.y - (m_height / 2), m_position.z);
    
    float penetration_x = 0;
    float penetration_y = 0;
    
    // If the map is solid, check the top three points
    if (map->is_solid(top, &penetration_x, &penetration_y) && m_velocity.y > 0)
    {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;
    }
    else if (map->is_solid(top_left, &penetration_x, &penetration_y) && m_velocity.y > 0)
    {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;
    }
    else if (map->is_solid(top_right, &penetration_x, &penetration_y) && m_velocity.y > 0)
    {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;
    }
    
    // And the bottom three points
    if (map->is_solid(bottom, &penetration_x, &penetration_y) && m_velocity.y < 0)
    {
        m_position.y += penetration_y;
        m_velocity.y = 0;
        m_collided_bottom = true;
    }
    else if (map->is_solid(bottom_left, &penetration_x, &penetration_y) && m_velocity.y < 0)
    {
            m_position.y += penetration_y;
            m_velocity.y = 0;
            m_collided_bottom = true;
    }
    else if (map->is_solid(bottom_right, &penetration_x, &penetration_y) && m_velocity.y < 0)
    {
        m_position.y += penetration_y;
        m_velocity.y = 0;
        m_collided_bottom = true;
        
    }
}

bool Entity::completed_tasks(){
    if(m_stapled){
        if(m_stampedblue && m_entity_type == BLUE_PAPER && !m_is_active){
            return true;
        }
        else if(m_stampedred && m_entity_type == RED_PAPER && !m_is_active){
            return true;
        }
        else if(m_stampedred && m_stampedblue && m_entity_type == PURPLE_PAPER && !m_is_active){
            return true;
        }

    }
    
    return false;
}

bool Entity::failed_tasks(){
    if(!m_is_active && !completed_tasks()){
        return true;
    }
    return false;
}
void const Entity::check_collision_x(Map *map)
{
    // Probes for tiles; the x-checking is much simpler
    glm::vec3 left  = glm::vec3(m_position.x - (m_width / 2), m_position.y, m_position.z);
    glm::vec3 right = glm::vec3(m_position.x + (m_width / 2), m_position.y, m_position.z);
    
    float penetration_x = 0;
    float penetration_y = 0;
    
    if (map->is_solid(left, &penetration_x, &penetration_y) && m_velocity.x < 0)
    {
        m_position.x += penetration_x;
        m_velocity.x = 0;
        m_collided_left = true;
    }
    if (map->is_solid(right, &penetration_x, &penetration_y) && m_velocity.x > 0)
    {
        m_position.x -= penetration_x;
        m_velocity.x = 0;
        m_collided_right = true;
    }
}

void Entity::render(ShaderProgram *program)
{
    if (!m_is_active) return;

    program->SetModelMatrix(m_model_matrix);

    if (m_entity_type == BLUE_PAPER || m_entity_type == RED_PAPER || m_entity_type == PURPLE_PAPER|| m_entity_type == PHONE)
    {
        draw_sprite_from_texture_atlas(program, m_texture_id, m_animation_index);
        return;
    }
    float vertices[]   = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float tex_coords[] = {  0.0,  1.0, 1.0,  1.0, 1.0, 0.0,  0.0,  1.0, 1.0, 0.0,  0.0, 0.0 };
    
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

bool const Entity::check_collision(Entity *other) 
{
    // If we are checking with collisions with ourselves, this should be false
    if (other == this) return false;
    
    // If either entity is inactive, there shouldn't be any collision
    if (!m_is_active || !other->m_is_active) return false;
    
    float x_distance = fabs(m_position.x - other->m_position.x) - ((m_width  + other->m_width)  / 2.0f);
    float y_distance = fabs(m_position.y - other->m_position.y) - ((m_height + other->m_height) / 2.0f);
    
    return x_distance < 0.0f && y_distance < 0.0f;
}

void const Entity::is_ringing(){
    if(!m_ringing){
        play_sfx();
        m_ringing = true;
        Mix_Volume(channel, MIX_MAX_VOLUME/16);
    }
}

void const Entity::hang_up(){
    if(m_ringing){
        m_ringing = false;
        Mix_Volume(channel, 0);
        ringer = 0;
    }
}
