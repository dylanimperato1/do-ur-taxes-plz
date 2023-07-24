#include "Map.hpp"

enum EntityType { STAPLER, BLUE_STAMP, RED_STAMP, BLUE_PAPER, RED_PAPER, PURPLE_PAPER, TEXT, PHONE, BACKGROUND};

class Entity
{
private:
    bool m_is_active = true;
    EntityType m_entity_type;
    
    glm::vec3 m_position;
    glm::vec3 m_velocity;
    
    float m_width  = 1.4f;
    float m_height = 1.4f;
    
public:
    // Static attributes
    static const int SECONDS_PER_FRAME = 4;
    
    // Existing
    GLuint m_texture_id;
    glm::mat4 m_model_matrix;
    
    Mix_Chunk *m_sfx = NULL;
    // Translating
    float m_speed;
    glm::vec3 m_movement;
    
    // Animating
    int m_animation_frames   = 0;
    int m_animation_index    = 0;
    float m_animation_time   = 0.0f;
    int m_animation_cols     = 0;
    int m_animation_rows     = 1;
    bool m_ringing           = false;
    int channel              = -1;
    
    // Colliding
    bool m_collided_top    = false;
    bool m_collided_bottom = false;
    bool m_collided_left   = false;
    bool m_collided_right  = false;
    
    //Tasks
    bool m_stapled = false;
    bool m_stampedblue = false;
    bool m_stampedred = false;
    
    // Methods
    Entity();
    ~Entity();
    void init_pos(glm::vec3 initpos);
    void draw_sprite_from_texture_atlas(ShaderProgram *program, GLuint texture_id, int index);
    void update(float delta_time, Entity *player, Entity *objects, int object_count, Map *map); // Now, update should check for both objects in the game AND the map
    void render(ShaderProgram *program);
    void activate_ai(Entity *player);
    void ai_slide();
    void ai_fall();
    bool completed_tasks();
    bool failed_tasks();
    int ringer = 0;
    
    void const check_collision_y(Entity *collidable_entities, int collidable_entity_count);
    void const check_collision_x(Entity *collidable_entities, int collidable_entity_count);
    

    // Overloading our methods to check for only the map
    void const check_collision_y(Map *map);
    void const check_collision_x(Map *map);
    
    bool const check_collision(Entity *other) ;
    
    void activate()   { m_is_active = true;  };
    void deactivate() { m_is_active = false; };
    bool check_active(){ return m_is_active; };
    
    EntityType const get_entity_type()    const { return m_entity_type;   };
    glm::vec3  const get_position()       const { return m_position;      };
    glm::vec3  const get_movement()       const { return m_movement;      };
    glm::vec3  const get_velocity()       const { return m_velocity;      };
    float      const get_speed()          const { return m_speed;         };
    int        const get_width()          const { return m_width;         };
    int        const get_height()         const { return m_height;        };
    Mix_Chunk* const get_sfx()            const { return m_sfx;           };
    bool       const ringing()            const { return m_ringing;       };
    
    void const set_entity_type(EntityType new_entity_type)  { m_entity_type   = new_entity_type;      };
    void const set_position(glm::vec3 new_position)         { m_position      = new_position;         };
    void const set_movement(glm::vec3 new_movement)         { m_movement      = new_movement;         };
    void const set_velocity(glm::vec3 new_velocity)         { m_velocity      = new_velocity;         };
    void const set_speed(float new_speed)                   { m_speed         = new_speed;            };
    void const set_sfx(Mix_Chunk *sfx)                      { m_sfx           = sfx;                  };
    void const set_width(float new_width)                   { m_width         = new_width;            };
    void const set_height(float new_height)                 { m_height        = new_height;           };
    void const play_sfx();
    void const is_ringing();
    void const hang_up();
};
