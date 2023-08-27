// entity.h

#ifndef _ENTITY_H
#define _ENTITY_H

typedef enum {
  ENTITY_NONE,
  ENTITY_MODULE,

  MAX_ENTITY_TYPE,
} Entity_type;

typedef enum {
  STATE_DEAD,
  STATE_ACTIVE,
  STATE_INACTIVE,
} Entity_state;

#define MAX_ENTITY_NAME_LENGTH 24

typedef struct Rect {
  i32 x;
  i32 y;
  i32 w;
  i32 h;
} Rect;

struct Entity;

typedef struct Entity {
  char name[MAX_ENTITY_NAME_LENGTH];
  size_t id;
  i32 x;
  i32 y;
  i32 w;
  i32 h;
  Entity_type type;
  Entity_state state;
  bool render;
  bool hover;
} Entity;

struct Mix;

void entity_init(Entity* e);
Entity* entity_alloc(struct Mix* m);
void entity_delete(struct Mix* m, Entity* e);
void entities_init(struct Mix* m);
void entities_update(struct Mix* m);
void entities_render(struct Mix* m);

#endif
