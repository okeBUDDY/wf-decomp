#include "actor_system.h"

#include <stdlib.h>

#include "actor.h"
#include "actor_control.h"
#include "application.h"
#include "hls_hit_image.h"
#include "snd_sys.h"

// 0x407900
void Actor::LerpColor(DWORD from, DWORD to, int num, int den) {
  const Color* a = reinterpret_cast<const Color*>(&from);
  const Color* b = reinterpret_cast<const Color*>(&to);
  Color* c = reinterpret_cast<Color*>(&color);
  c->b = a->b + (b->b - a->b) * num / den;
  c->g = a->g + (b->g - a->g) * num / den;
  c->r = a->r + (b->r - a->r) * num / den;
  c->a = a->a + (b->a - a->a) * num / den;
}

// 0x407980
int ActorSystem::Register(int id, ActorControl* control) {
  if (controls_ != NULL && control != NULL) {
    if (id <= 0) {
      // "Invalid control system id"
      Error("不正な制御ｼｽﾃﾑIDです");
      return 0;
    }
    if (!control->Initialize()) {
      return 0;
    }
    if (controls_[id] != NULL) {
      controls_[id]->Finalize();
      delete controls_[id];
    }
    controls_[id] = control;
    return 1;
  }
  // "Out of memory"
  Error("ﾒﾓﾘが足りません");
  return 0;
}

HLSNullObject g_null_sprite;  // 0x468ed8
HLSHitImage g_hit_image;      // 0x468ee0

// 0x407a10
void ActorSystem::SetDamageSound(int damage, CSndBuf* sound, Channel* channel) {
  switch (damage) {
    case Actor::kDamageArmor:
      Actor::hit_sound_ = sound;
      Actor::hit_channel_ = channel;
      break;
    case Actor::kDamageGuard:
      Actor::guard_sound_ = sound;
      Actor::guard_channel_ = channel;
      break;
  }
}

// 0x407aa0
Actor* Actor::HitTest() {
  int shift = g_hit_image.shift_;
  int x = static_cast<int>(screen.m[2][0]) >> shift;
  int y = static_cast<int>(screen.m[2][1]) >> shift;
  if (x >= 0 && x < g_hit_image.width_ && y >= 0 && y < g_hit_image.height_) {
    int id = static_cast<short>(g_hit_image.bits_[y * g_hit_image.width_ + x]);
    if (id != 0) {
      Actor* actor = &system_->actors_[id];
      if (actor->flags != kDead && ((actor->team ^ team) & kTeamMask) != 0) {
        return actor;
      }
    }
  }
  return NULL;
}

// 0x407b30
void Actor::Flash() {
  if (hit_points > 0) {
    hit_points = -hit_points;
    for (Actor* child = first_child; child != NULL;
         child = child->prev_sibling) {
      child->Flash();
    }
  }
}

// 0x407b60
void ActorSystem::Link(Actor* actor) {
  Actor** head = &layers_[actor->layer];
  Actor* other = *head;
  if (other != NULL) {
    HLSurface* texture = actor->sprite->texture_;
    while (other->draw_next != NULL && other->sprite->texture_ != texture) {
      other = other->draw_next;
    }
    actor->draw_next = other->draw_next;
    actor->draw_prev = other;
    other->draw_next = actor;
    if (actor->draw_next != NULL) {
      actor->draw_next->draw_prev = actor;
    }
  } else {
    *head = actor;
    actor->draw_next = NULL;
    actor->draw_prev = NULL;
  }
}

// 0x407be0
void Actor::InitRoot() {
  flags = kDead;
  low_hit_points = 0;
  sprite = NULL;
  control = NULL;
  parent = next_sibling = prev_sibling = first_child = last_child = NULL;
  draw_prev = draw_next = NULL;
  Transform* t = &screen;
  transform = t;
  t->m[0][1] = t->m[0][2] = t->m[1][0] = t->m[1][2] = t->m[2][0] = t->m[2][1] =
      0.0f;
  t->m[0][0] = t->m[1][1] = t->m[2][2] = 1.0f;
  SetIdentity(&relative);
}

// 0x407c80
void Actor::Reset() {
  team = 0;
  id = 0;
  hit_age = 0;
  age = 0;
  debris = 0;
  hit_points = 1;
  angle = 0;
  start_angle = 0;
  acceleration = 0.0f;
  speed = 0.0f;
  scale = 1.0f;
  bounds = NULL;
  sprite->Reset();
  last_child = NULL;
  first_child = NULL;
  sprite = &g_null_sprite;
  color = 0xffffffff;
}

// 0x407ce0
Actor::Actor() : flags(0) {
  Transform* t = &relative;
  t->m[0][0] = t->m[1][1] = t->m[2][2] = 1.0f;
  t->m[0][1] = t->m[0][2] = t->m[1][0] = t->m[1][2] = t->m[2][0] = t->m[2][1] =
      0.0f;
  sprite = &g_null_sprite;
  Reset();
}

// 0x407d40
int Actor::Init(ActorControl* control, Actor* parent, float x, float y,
                unsigned short layer, unsigned short angle, void* param) {
  flags &= 0xffff;
  if (parent == NULL) {
    transform = &screen;
    transform->m[1][2] = 0.0f;
    transform->m[0][2] = 0.0f;
    transform->m[2][2] = 1.0f;
  } else {
    transform = &relative;
    flags |= kChild;
  }
  this->control = control;
  this->parent = parent != NULL ? parent : system_->actors_;
  transform->m[2][0] = x;
  transform->m[2][1] = y;
  flags |= kNew;
  this->layer = layer;
  start_angle = angle;
  if (this->control != NULL && !this->control->OnCreate(this, param)) {
    flags = 0;
    return 0;
  }
  if (flags & kFaceStart) {
    this->angle = start_angle;
  }
  matrix_angle = this->angle + 0x1000;
  low_hit_points = hit_points >> 3;
  transform->m[0][1] = g_sin_table[this->angle & 0x3ff] * scale;
  transform->m[1][0] = -transform->m[0][1];
  transform->m[1][1] = g_sin_table[(this->angle + 0x100) & 0x3ff] * scale;
  transform->m[0][0] = transform->m[1][1];
  prev_sibling = NULL;
  if (this->parent->last_child != NULL) {
    next_sibling = this->parent->last_child;
    this->parent->last_child = this;
    next_sibling->prev_sibling = this;
  } else {
    next_sibling = NULL;
    this->parent->last_child = this;
    this->parent->first_child = this;
  }
  system_->Link(this);
  ++system_->count_;
  flags &= ~kCreating;
  return 1;
}

// 0x407ef0
int Actor::Restore(const ActorDesc* desc) {
  SetSprite(desc->sprite);
  flags = desc->flags;
  hit_points = desc->hit_points;
  angle = desc->angle;
  if (!Init(NULL, desc->parent, desc->x, desc->y, desc->layer,
            desc->start_angle, NULL)) {
    return 0;
  }
  color = desc->color;
  team = desc->team;
  speed = desc->speed;
  acceleration = desc->acceleration;
  debris = desc->debris;
  age = desc->age;
  work[0] = reinterpret_cast<int>(desc->slot);
  return 1;
}

// 0x407f90
Actor* Actor::Draw() {
  if (flags & kAdditive) {
    sprite->Draw(&screen, color, 1);
  } else if (hit_points < 0) {
    sprite->Draw(&screen, color, 0);
    sprite->Draw(&screen, 0xff80ffff, 1);
    hit_points = abs(hit_points);
  } else if (hit_points <= low_hit_points &&
             system_->actors_[0].low_hit_points != 0) {
    sprite->Draw(&screen, 0xffff4000, 0);
  } else {
    sprite->Draw(&screen, color, 0);
  }
  if (team & kHittable) {
    sprite->Hit(&g_hit_image, static_cast<int>(screen.m[2][0]),
                static_cast<int>(screen.m[2][1]), hit_id);
  }
  return draw_next;
}

// 0x408050
ActorSystem::ActorSystem() {
  reserved_ = 0;
  app_ = NULL;
  controls_ = NULL;
  actors_ = NULL;
  name_ = "HLSObjSys";
  max_controls_ = 0x80;
  max_actors_ = 0x400;
}

// 0x4080f0
Actor* ActorSystem::Create(int id, Actor* parent, float x, float y,
                           unsigned short layer, unsigned short angle,
                           void* param) {
  Actor* actor;
  int count;
  if (id < 0) {
    id = -id;
    actor = &actors_[1];
    count = max_actors_;
  } else {
    actor = &actors_[reserved_ + 1];
    count = max_actors_ - reserved_;
  }
  for (; count > 0; --count, ++actor) {
    if (actor->flags == 0) {
      break;
    }
  }
  if (count != 0) {
    actor->id = id;
    if (actor->Init(controls_[id], parent, x, y, layer, angle, param)) {
      return actor;
    }
    actor->id = 0;
  }
  return NULL;
}

// 0x408190
Actor* ActorSystem::Spawn(const ActorDesc* desc) {
  Actor* actor = &actors_[reserved_ + 1];
  int count = max_actors_ - reserved_;
  for (; count > 0; --count, ++actor) {
    if (actor->flags == 0) {
      break;
    }
  }
  if (count == 0) {
    return NULL;
  }
  return actor->Restore(desc) ? actor : NULL;
}

// 0x4081f0
void ActorSystem::DrawAll() {
  g_hit_image.Clear();
  HL3D* d3d = app_->d3d;
  d3d->device_->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
  d3d->device_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  d3d->device_->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
  d3d->device_->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
  d3d = app_->d3d;
  d3d->device_->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
  d3d->device_->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
  for (int i = 0; i < kLayers; ++i) {
    for (Actor* actor = layers_[i]; actor != NULL; actor = actor->Draw()) {
    }
  }
}

// 0x4082b0
void Actor::Destroy(int debris, int notify) {
  if (notify && parent->control != NULL) {
    parent->control->OnChildDestroyed(parent, this);
  }
  if (control != NULL) {
    control->OnDestroy(this);
  }
  flags = kDead;
  while (first_child != NULL) {
    first_child->Destroy(debris, 0);
  }
  if (next_sibling != NULL) {
    next_sibling->prev_sibling = prev_sibling;
  } else {
    parent->first_child = prev_sibling;
  }
  if (prev_sibling != NULL) {
    prev_sibling->next_sibling = next_sibling;
  } else {
    parent->last_child = next_sibling;
  }
  if (draw_prev != NULL) {
    draw_prev->draw_next = draw_next;
  } else {
    system_->layers_[layer] = draw_next;
  }
  if (draw_next != NULL) {
    draw_next->draw_prev = draw_prev;
  }
  if (debris && this->debris != 0) {
    if (this->debris > 0) {
      system_->Create(this->debris, NULL, screen.m[2][0], screen.m[2][1], layer,
                      0, NULL);
    } else if (parent->flags == kDead) {
      unsigned short layer = this->layer;
      float y = screen.m[2][1];
      float x = screen.m[2][0];
      Actor* actor = system_->Create(-this->debris, NULL, x, y, layer, 0, NULL);
      if (actor != NULL) {
        actor->flags |= kDebris;
      }
    } else {
      system_->Create(-this->debris, parent, transform->m[2][0],
                      transform->m[2][1], layer, 0, NULL);
    }
  }
  Reset();
  --system_->count_;
  if (hit_id > system_->last_dead_) {
    system_->last_dead_ = hit_id;
  }
}

// 0x408480
void ActorSystem::DestroyAll() {
  flags_ |= 1;
  Actor* next;
  for (Actor* actor = actors_[0].first_child; actor != NULL; actor = next) {
    next = actor->prev_sibling;
    actor->Destroy(0, 0);
  }
}

// 0x4084b0
int Actor::Damage(int amount, Actor* source) {
  Actor* actor = this;
  for (;;) {
    if (damage_hook_ != NULL) {
      damage_hook_(source, actor, amount);
    }
    switch (actor->team & kDamageMask) {
      case kDamageParent:
        actor = actor->parent;
        continue;
      case kDamageSelf:
      case kDamageArmor:
        actor->hit_points = abs(actor->hit_points) - amount;
        if (actor->hit_points <= 0) {
          if (destroy_hook_ != NULL) {
            destroy_hook_(source, actor);
          }
          actor->hit_points = 0;
          actor->Destroy(1, 1);
          return 0;
        }
        if (actor->hit_age == 0) {
          actor->hit_age = actor->age;
        }
        if (hit_sound_ != NULL) {
          hit_channel_->Assign(hit_sound_, 0, 1.0f);
        }
        actor->Flash();
        return 0;
      case kDamageGuard:
        if (guard_sound_ != NULL) {
          guard_channel_->Assign(guard_sound_, 0, 1.0f);
        }
        return 0;
      case kDamageMessage:
        if (actor->control != NULL) {
          return actor->control->OnMessage(actor, amount,
                                           reinterpret_cast<int>(source));
        }
        return 0;
    }
    return 0;
  }
}

// 0x4085c0
int Actor::Move() {
  if (flags & kDebris) {
    return 0;
  }
  if (control != NULL) {
    if (!control->OnMove(this)) {
      return 0;
    }
    ++age;
  } else if (age != 0) {
    if (--age == 0) {
      return 0;
    }
  }
  if ((team & (kHits | kHitByOthers)) && (ActorSystem::flags_ & 1)) {
    Actor* other = HitTest();
    if (other != NULL && (other->team & kDamageMask) &&
        (other->Damage(abs(hit_points), this) ||
         (team & (kHits | kHitByOthers)) == kHits)) {
      return 0;
    }
  }
  if (flags & kFaceStart) {
    angle = start_angle;
  }
  if (matrix_angle != angle) {
    if (scale == 1.0f) {
      transform->m[0][1] = g_sin_table[angle & 0x3ff];
      transform->m[1][0] = -transform->m[0][1];
      transform->m[1][1] = g_sin_table[(angle + 0x100) & 0x3ff];
      transform->m[0][0] = transform->m[1][1];
    } else {
      transform->m[0][1] = g_sin_table[angle & 0x3ff] * scale;
      transform->m[1][0] = -transform->m[0][1];
      transform->m[1][1] = g_sin_table[(angle + 0x100) & 0x3ff] * scale;
      transform->m[0][0] = transform->m[1][1];
    }
    if (flags & kMirrorX) {
      transform->m[0][0] = -transform->m[0][0];
      transform->m[0][1] = -transform->m[0][1];
    }
    if (flags & kMirrorY) {
      transform->m[1][0] = -transform->m[1][0];
      transform->m[1][1] = -transform->m[1][1];
    }
    flags |= kMoved;
    matrix_angle = angle;
  }
  if (acceleration != 0.0f) {
    if ((speed += acceleration) <= 0.0f) {
      acceleration = 0.0f;
      speed = 0.0f;
      if (flags & kStopDies) {
        return 0;
      }
    }
  }
  if (speed > 0.0f) {
    transform->m[2][0] += g_sin_table[start_angle & 0x3ff] * speed;
    transform->m[2][1] -= g_sin_table[(start_angle + 0x100) & 0x3ff] * speed;
    flags |= kMoved;
    if (transform == &screen) {
      if (bounds != NULL) {
        if (transform->m[2][0] < bounds->left) {
          transform->m[2][0] = bounds->left;
        }
        if (transform->m[2][0] > bounds->right) {
          transform->m[2][0] = bounds->right;
        }
        if (transform->m[2][1] < bounds->top) {
          transform->m[2][1] = bounds->top;
        }
        if (transform->m[2][1] > bounds->bottom) {
          transform->m[2][1] = bounds->bottom;
        }
      } else if (transform->m[2][0] < system_->left_ ||
                 transform->m[2][0] > system_->right_ ||
                 transform->m[2][1] < system_->top_ ||
                 transform->m[2][1] > system_->bottom_) {
        debris = 0;
        return 0;
      }
    }
  }
  flags |= parent->flags & kMoved;
  if ((flags & (kMoved | kChild)) == (kMoved | kChild)) {
    MultiplyTransform(&screen, &parent->screen, transform);
  }
  Actor* next;
  for (Actor* child = first_child; child != NULL; child = next) {
    next = child->prev_sibling;
    if (!child->Move()) {
      child->Destroy(1, 1);
    }
  }
  flags = (flags & ~kMoved) | kMovedOnce;
  return 1;
}

// 0x408970
int ActorSystem::Initialize(Application* app) {
  app_ = app;
  controls_ =
      static_cast<ActorControl**>(malloc(max_controls_ * sizeof(*controls_)));
  actors_ = new Actor[max_actors_ + 1];
  if (controls_ == NULL || actors_ == NULL) {
    // "Out of memory"
    Error("ﾒﾓﾘが足りません");
    return 0;
  }
  if (!g_hit_image.Create(app_->width, app_->height)) {
    return 0;
  }
  int i;
  for (i = 0; i < max_actors_; ++i) {
    actors_[i].hit_id = i;
  }
  for (i = 0; i < max_controls_; ++i) {
    controls_[i] = NULL;
  }
  for (i = 0; i < kLayers; ++i) {
    layers_[i] = NULL;
  }
  actors_[0].InitRoot();
  left_ = -100.0f;
  right_ = static_cast<float>(app_->width + 100);
  top_ = -100.0f;
  bottom_ = static_cast<float>(app_->height + 100);
  count_ = 0;
  instance_ = this;
  Actor::system_ = this;
  flags_ = 1;
  DestroyAll();
  // "%d objects, %d control systems available."
  Log("%dｵﾌﾞｼﾞｪｸﾄ %d制御ｼｽﾃﾑを利用可能.", max_actors_, max_controls_);
  return 1;
}

// 0x408b70
void ActorSystem::Finalize() {
  g_hit_image.Release();
  if (actors_ != NULL) {
    DestroyAll();
    delete[] actors_;
    actors_ = NULL;
  }
  if (controls_ != NULL) {
    for (int i = 0; i < max_controls_; ++i) {
      if (controls_[i] != NULL) {
        controls_[i]->Finalize();
        delete controls_[i];
        controls_[i] = NULL;
      }
    }
    if (controls_ != NULL) {
      free(controls_);
    }
    controls_ = NULL;
  }
}

// 0x408c20
void ActorSystem::MoveAll() {
  actors_[0].low_hit_points = (app_->frame_count & 0x42) == 0;
  last_dead_ = 0;
  Actor* next;
  for (Actor* actor = actors_[0].first_child; actor != NULL; actor = next) {
    next = actor->prev_sibling;
    if (!actor->Move()) {
      actor->Destroy(1, 1);
    }
  }
  Actor* slot = &actors_[1];
  for (int i = last_dead_; i >= 0; --i, ++slot) {
    if (slot->flags & Actor::kDead) {
      slot->flags = 0;
    }
  }
}
