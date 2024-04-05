// fx_reverb.c
// cpu intensive, experimental reverb effect

#define MAX_REVERB_CHILD_NODE 3
#define MAX_REVERB_NODE_DEPTH 3
// pow(MAX_REVERB_CHILD_NODE, MAX_REVERB_NODE_DEPTH)
#define MAX_REVERB_NODE_TOTAL ( \
  MAX_REVERB_CHILD_NODE * \
  MAX_REVERB_CHILD_NODE * \
  MAX_REVERB_CHILD_NODE \
)

#define MAX_REVERB_FEEDBACK_BUFFER_LENGTH (4410)
#define NODE_BUFFER_SIZE (MAX_REVERB_FEEDBACK_BUFFER_LENGTH * MAX_REVERB_NODE_TOTAL)

typedef struct Reverb_node {
  Vector2 direction;
  Vector2 initial_direction;
  f32 direction_variation;
  size_t length; // feedback buffer length
  f32* buffer;
  f32 feedback_amount;
  struct Reverb_node* nodes[MAX_REVERB_CHILD_NODE];
  u32 node_count;
  size_t tick;
  f32 rms;
} Reverb_node;

typedef struct Reverb {
  f32 amount;
  // TODO:
  // f32 dry;
  // f32 wet;
  f32 node_buffer[NODE_BUFFER_SIZE];
  size_t node_buffer_index;
  Reverb_node nodes[MAX_REVERB_NODE_TOTAL];
  u32 node_count;
  size_t tick;
  Reverb_node root;
  Ticket mutex;
  i32 model_seed;
  f32 offset_base;
} Reverb;

static void fx_reverb_default(Reverb* reverb);
static void fx_reverb_randomize(Element* e);
static void fx_reverb_tree_new(Reverb* reverb);
static f32* alloc_reverb_buffer(Reverb* reverb, size_t size);
static Reverb_node* alloc_reverb_node(Reverb* reverb);
static void update_model_seed(Element* e);
static void fx_reverb_create_reverb_tree(Reverb* reverb, Reverb_node* root, u32 depth);
static void process_reverb_node(Instrument* ins, Reverb* reverb, Reverb_node* node, u32 depth, f32 dot, f32 dt);
static void reverb_model_render(Element* e);
static void reverb_model_render_nodes(const Box* box, Reverb_node* root, i32 center_x, i32 center_y);

void fx_reverb_default(Reverb* reverb) {
  reverb->amount = 0.5f;
  reverb->tick = 0;
  reverb->offset_base = 411;
  fx_reverb_tree_new(reverb);
}

void fx_reverb_randomize(Element* e) {
  Reverb* reverb = (Reverb*)e->userdata;
  ticket_mutex_begin(&reverb->mutex);
  reverb->model_seed = (i32)random_number();
  reverb->offset_base = random_number() % (MAX_REVERB_FEEDBACK_BUFFER_LENGTH / 2);
  fx_reverb_tree_new(reverb);
  ticket_mutex_end(&reverb->mutex);
}

void fx_reverb_tree_new(Reverb* reverb) {
  random_init((Random)reverb->model_seed);
  reverb->node_buffer_index = 0;
  reverb->node_count = 0;
  memset(reverb->nodes, 0, sizeof(reverb->nodes));
  {
    Reverb_node* n = &reverb->root;
    n->direction = (Vector2) {0, 0};
    n->initial_direction = n->direction;
    n->direction_variation = 0;
    n->length = 0;
    n->buffer = NULL;
    n->feedback_amount = 0;
    n->node_count = 0;
    n->tick = 0;
  }
  fx_reverb_create_reverb_tree(reverb, &reverb->root, 0);
  random_init(random_get_current_seed());
}

f32* alloc_reverb_buffer(Reverb* reverb, size_t size) {
  if (reverb->node_buffer_index + size < NODE_BUFFER_SIZE) {
    f32* buffer = &reverb->node_buffer[reverb->node_buffer_index];
    reverb->node_buffer_index += size;
    return buffer;
  }
  return NULL;
}

Reverb_node* alloc_reverb_node(Reverb* reverb) {
  if (reverb->node_count < MAX_REVERB_NODE_TOTAL) {
    return &reverb->nodes[reverb->node_count++];
  }
  return NULL;
}

void update_model_seed(Element* e) {
  Reverb* reverb = (Reverb*)e->userdata;
  ASSERT(reverb != NULL);
  ticket_mutex_begin(&reverb->mutex);
  fx_reverb_tree_new(reverb);
  ticket_mutex_end(&reverb->mutex);
}

void fx_reverb_create_reverb_tree(Reverb* reverb, Reverb_node* root, u32 depth) {
  if (depth < MAX_REVERB_NODE_DEPTH) {
    u32 num_nodes = random_number() % MAX_REVERB_CHILD_NODE;
    if (depth == 0) {
      num_nodes = MAX_REVERB_CHILD_NODE;
    }
    for (u32 i = 0; i < num_nodes; ++i) {
      Reverb_node* node = alloc_reverb_node(reverb);
      if (!node) {
        break;
      }
      memset(node->nodes, 0, sizeof(node->nodes));
      // -1.0, 1.0
      node->direction = (Vector2) {
        2 * (random_f32() - 0.5f),
        2 * (random_f32() - 0.5f),
      };
      node->initial_direction = node->direction;
      node->direction_variation = random_f32() * .1f;
      node->length = random_number() % MAX_REVERB_FEEDBACK_BUFFER_LENGTH;
      node->buffer = alloc_reverb_buffer(reverb, node->length);
      node->feedback_amount = random_f32();
      root->nodes[root->node_count++] = node;
      fx_reverb_create_reverb_tree(reverb, node, depth + 1);
    }
  }
}

void process_reverb_node(Instrument* ins, Reverb* reverb, Reverb_node* node, u32 depth, f32 dot, f32 dt) {
  // contribution factor, how much this node affects the output reverberated signal
  f32 cf = ((MAX_REVERB_NODE_DEPTH + 1) - depth) / (f32)MAX_REVERB_NODE_DEPTH;
  f32 sample_dt = dt / ins->samples;
  if (node->length >= 2) {
    node->rms = audio_calc_rms_clamp(node->buffer, node->length);
    f32 pan_left = (dot + 1) / 2;
    f32 pan_right = 1 - pan_left;
    f32 offset_table[] = {
      (reverb->offset_base),
      (reverb->offset_base*1.1375035),
      (reverb->offset_base*1.2630344),
      (reverb->offset_base*1.3785116),
      (reverb->offset_base*1.4854268),
      (reverb->offset_base*1.5849625),
      (reverb->offset_base*1.6780719),
      (reverb->offset_base*1.7655347),
      (reverb->offset_base*1.8479969),
      (reverb->offset_base*1.9259994),
    };
    size_t offset  = (size_t)(offset_table[(size_t)((pan_left)  * LENGTH(offset_table)) % LENGTH(offset_table)]);
    for (size_t i = 0; i < ins->samples; i += 2) {
      f32 feedback_left  = reverb->amount * node->feedback_amount * (1 + node->direction.x) * ins->out_buffer[i + 0];
      f32 feedback_right = reverb->amount * node->feedback_amount * (1 + node->direction.y) * ins->out_buffer[i + 1];

      ins->out_buffer[i + 0] += pan_left  * reverb->amount * cf * node->buffer[(node->tick + offset) % node->length];
      ins->out_buffer[i + 1] += pan_right * reverb->amount * cf * node->buffer[(node->tick + offset) % node->length];

      node->buffer[(node->tick) % node->length] = .5f * (feedback_left + feedback_right);
      node->tick += 1;
      node->direction.x = node->initial_direction.x + sinf((node->tick * (node->rms * node->direction_variation * sample_dt) * PI32) / 2.0f);
      node->direction.y = node->initial_direction.y + sinf((node->tick * (node->rms * node->direction_variation * sample_dt) * PI32) / 2.0f);
    }
  }

  for (u32 node_index = 0; node_index < node->node_count; ++node_index) {
    Reverb_node* next = node->nodes[node_index];
    process_reverb_node(ins, reverb, next, depth + 1, CLAMP(Vector2DotProduct(node->direction, next->direction), -1, 1), dt);
  }
}

void reverb_model_render(Element* e) {
  Reverb* reverb = (Reverb*)e->userdata;
  i32 center_x =  e->box.x + e->box.w / 2;
  i32 center_y =  e->box.y + e->box.h / 2;
  Reverb_node* root = &reverb->root;
  reverb_model_render_nodes(&e->box, root, center_x, center_y);
}

void reverb_model_render_nodes(const Box* box, Reverb_node* root, i32 center_x, i32 center_y) {
  i32 max_length = sqrtf(box->w * box->h) / MAX_REVERB_NODE_DEPTH;
  for (u32 i = 0; i < root->node_count; ++i) {
    Reverb_node* node = root->nodes[i];
    i32 length = max_length * (node->length / (f32)MAX_REVERB_FEEDBACK_BUFFER_LENGTH);
    i32 x = CLAMP(center_x + length * root->direction.x, box->x, box->x + box->w);
    i32 y = CLAMP(center_y + length * root->direction.y, box->y, box->y + box->h);
    DrawLine(center_x, center_y, x, y, lerp_color(COLOR_RGB(10, 70, 10), COLOR_RGB(130, 230, 100), CLAMP(sqrtf(node->rms), 0, 1)));
    reverb_model_render_nodes(box, node, x, y);
  }
}

void fx_reverb_init(Instrument* ins) {
  Reverb* reverb = memory_alloc(sizeof(Reverb));
  ASSERT(reverb != NULL);
  ASSERT((size_t)(powf(MAX_REVERB_CHILD_NODE, MAX_REVERB_NODE_DEPTH) == MAX_REVERB_NODE_TOTAL));

  reverb->mutex = ticket_mutex_new();
  reverb->model_seed = time(0);
  ins->userdata = reverb;
  fx_reverb_default(reverb);
}

void fx_reverb_ui_new(Instrument* ins, Element* container) {
  (void)ins; (void)container;
  Reverb* reverb = (Reverb*)ins->userdata;
  const i32 button_height = FONT_SIZE * 2;
  {
    Element e = ui_button("generate");
    e.box.h = button_height;
    e.sizing = SIZING_PERCENT(100, 0);
    e.tooltip = "generate new reverb model";
    e.onclick = fx_reverb_randomize;
    e.userdata = reverb;
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_text("reverb amount");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("model seed");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_input_float("amount", &reverb->amount);
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&reverb->amount, 0.0f, 0.99f);
    e.sizing = SIZING_PERCENT(30, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_int("model seed", &reverb->model_seed);
    e.sizing = SIZING_PERCENT(50, 0);
    e.box.h = button_height;
    e.onenter = update_model_seed;
    e.userdata = reverb;
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_text("offset base");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_float("offset base", &reverb->offset_base);
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&reverb->offset_base, 0.0f, MAX_REVERB_FEEDBACK_BUFFER_LENGTH / 2);
    e.sizing = SIZING_PERCENT(30, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_line_break(0);
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_canvas(true);
    ui_set_title(&e, "reverb model");
    e.sizing = SIZING_PIXELS(140, 140);
    e.userdata = reverb;
    e.onrender = reverb_model_render;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_line_break(0);
    ui_attach_element(container, &e);
  }
  container->sizing.y = 40;
}

void fx_reverb_update(Instrument* ins, struct Mix* mix) {
  (void)ins; (void)mix;
}

void fx_reverb_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
  (void)ins; (void)mix; (void)audio; (void)dt;
  Reverb* reverb = (Reverb*)ins->userdata;
  ticket_mutex_begin(&reverb->mutex);
  process_reverb_node(ins, reverb, &reverb->root, 0, 0, dt);
  ticket_mutex_end(&reverb->mutex);
}

void fx_reverb_destroy(struct Instrument* ins) {
  (void)ins;
}
