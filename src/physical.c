// physical.c

#define MAX_FEEDBACK (1024*16)
#define MAX_NODES 64

typedef struct NodePair {
  size_t first;
  size_t second;
} NodePair;

typedef struct PhysicalNode {
  f32 f;
  f32 size;
  f32 feedback[MAX_FEEDBACK];
  f32 feedback_amount;
  f32 tick;
  size_t feedback_tick;
  size_t feedback_offset;
} PhysicalNode;

typedef struct Physical {
  f32 amp;
  f32 tick;
  f32 feedback_scale;
  f32 propagation_speed;
  f32 damping;
  PhysicalNode nodes[MAX_NODES];
  NodePair connection_map[MAX_NODES];
} Physical;

static void physical_default(Instrument* ins);

void physical_default(Instrument* ins) {
  Physical* p = (Physical*)ins->userdata;
  p->amp = 0.2f;
  p->tick = 0;
  p->feedback_scale = 1.0f;
  p->propagation_speed = 0.05f;
  p->damping = 0.1f;
  PhysicalNode default_node;
  memset(&default_node, 0, sizeof(default_node));
  default_node.f = 0;
  default_node.size = 1;
  default_node.feedback_amount = 0.01f;
  default_node.tick = 0.0f;
  default_node.feedback_tick = 0;
  memset(default_node.feedback, 0, sizeof(default_node.feedback));
  for (size_t i = 0; i < MAX_NODES; ++i) {
    PhysicalNode* node = &p->nodes[i];
    node->feedback_amount = random_f32() * 0.5f;
    node->feedback_offset = random_number() % MAX_FEEDBACK;
    *node = default_node;
    NodePair* pair = &p->connection_map[i];
    pair->first = random_number() % MAX_NODES;
    pair->second = random_number() % MAX_NODES;
  }
}

void physical_init(Instrument* ins, struct Mix* mix) {
  Physical* p = ins->userdata = memory_alloc(sizeof(Physical));
  ASSERT(p != NULL);
  physical_default(ins);
}

void physical_ui_new(Instrument* ins, Element* container) {
  Physical* p = ins->userdata;
  container->x_padding = 1; container->y_padding = 1;
  UI_EASY_INPUT_F32(container, "amplitude", &p->amp, 0, 1);
  UI_EASY_INPUT_F32(container, "feedback scale", &p->feedback_scale, 0, 10);
  UI_EASY_INPUT_F32(container, "propagation speed", &p->propagation_speed, 0, 10);
  UI_EASY_INPUT_F32(container, "damping", &p->damping, 0, 10);
  for (i32 i = 0; i < MAX_NODES; ++i) {
    PhysicalNode* node = &p->nodes[i];
    UI_EASY_INPUT_F32(container, "", &node->f, -1.0f, 1.0f);
  }
}

void physical_update(Instrument* ins, struct Mix* mix) {

}

void physical_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
  Physical* p = ins->userdata;
  audio_buffer_zero(ins->out_buffer, ins->samples);
  f32 sample_dt = dt;// / ins->samples;
  f32 speed = p->propagation_speed;
  for (size_t i = 0; i < MAX_NODES; ++i) {
    for (size_t sample_index = 0; sample_index < ins->samples; ++sample_index) {
      PhysicalNode* node = &p->nodes[i];
      NodePair* pair = &p->connection_map[i];
      PhysicalNode* first = &p->nodes[pair->first];
      PhysicalNode* second = &p->nodes[pair->second];

      node->f = lerp_f32(node->f, 0, sample_dt * p->damping);
      node->tick += node->f;
      f32 sample = p->amp * (sine[(size_t)(node->tick) % LENGTH(sine)]) + node->feedback[(node->feedback_tick + node->feedback_offset) % MAX_FEEDBACK] * node->feedback_amount * p->feedback_scale;
      ins->out_buffer[sample_index] += sample;
      if (first != node) {
        first->f = lerp_f32(first->f, node->f, sample_dt * speed);
        first->feedback[first->feedback_tick % MAX_FEEDBACK] += first->feedback_amount * sample;
        first->feedback[first->feedback_tick % MAX_FEEDBACK] = lerp_f32(first->feedback[first->feedback_tick % MAX_FEEDBACK], 0, sample_dt * p->damping);
        first->feedback_tick += 1;
      }
      if (second != node) {
        second->f = lerp_f32(second->f, node->f, sample_dt * speed);
        second->feedback[second->feedback_tick % MAX_FEEDBACK] += second->feedback_amount * sample;
        second->feedback[second->feedback_tick % MAX_FEEDBACK] = lerp_f32(second->feedback[second->feedback_tick % MAX_FEEDBACK], 0, sample_dt * p->damping);
        second->feedback_tick += 1;
      }
      node->feedback_tick += 1;
    }
  }
}

void physical_noteon(struct Instrument* ins, u8 note, f32 velocity) {
  Physical* p = ins->userdata;
  PhysicalNode* n = &p->nodes[random_number() % MAX_NODES];
  n->f = freq_table[random_number() % LENGTH(freq_table)];
  // n->tick = 0;
}

void physical_noteoff(struct Instrument* ins, u8 note) {

}

void physical_destroy(struct Instrument* ins) {

}
