// The player (control 1): moves with the input of the game, fires the shot
// (weapon 0) or the laser (weapon 1) and saves its state in replays.
#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "actor_control.h"
#include "actor_system.h"
#include "application.h"
#include "enm_param.h"
#include "file.h"
#include "fn_game.h"
#include "game_data.h"
#include "hl_3d.h"
#include "hls_anm.h"
#include "line_sprites.h"
#include "snd_sys.h"
#include "sounds.h"

// Actor::work of the player.
struct PlayerWork {
  Actor* flame;       // 0x30, the muzzle flash, shown while firing
  Actor* laser;       // 0x34, draws g_laser
  short weapon;       // 0x38, Control01::Weapons
  short shots;        // 0x3a, frames fired in a row
  short switch_time;  // 0x3c, frames until the weapon calls its name
  short turn_time;    // 0x3e, frames since a direction was pressed
  short invincible;   // 0x40, frames left; negative: for good
};

// What Control01::Save() writes.
struct PlayerRecord {
  float x;
  float y;
  int weapon;
  float tip_x;
  float tip_y;
  short angle;
  short spread;
};

extern AnmData g_flame_anim;            // 0x45d4a4
extern EffectDesc g_muzzle_effect;      // 0x45d4ac
extern int g_weapon_sounds[2];          // 0x45d4e0, indices of the sounds
extern FloatRect g_player_bounds;       // 0x45d4e8
extern short g_direction_angles[0x10];  // 0x45d4f8, by direction keys

// The sound `index` of the ones in sounds.h (from g_snd_bomb1).
static inline CSndBuf* Sound(int index) { return &g_snd_bomb1 + index; }

__forceinline void Control01::SelectWeapon(Actor* actor, int weapon, float x,
                                           float y) {
  PlayerWork* work = reinterpret_cast<PlayerWork*>(actor->work);
  if (weapon < 0) {
    weapon = work->weapon + 1;
  }
  if (weapon >= kWeapons) {
    weapon = kShot;
  }
  work->weapon = weapon;
  work->shots = 0;
  work->flame->angle = 0;
  Actor* laser = work->laser;
  weapon_ = weapon;
  laser->color = 0;
  switch (weapon) {
    case kLaser:
      work->laser->color = 0xaa00ff00;
      g_laser.Reset(x, y);
      break;
  }
}

// 0x412f90
void Control01::ResetWeapon() { weapon_ = kShot; }

// 0x412fe0
int Control01::Initialize() {
  memset(&shot_, 0, sizeof(shot_));
  shot_.color = 0xffffffff;
  HLSObject* sprite = g_sys_pats.Get(2);
  shot_.sprite = sprite;
  shot_.team = 8;
  shot_.debris = 8;
  shot_.flags = Actor::kAdditive | Actor::kFaceStart;
  shot_.hit_points = 1;
  shot_.speed = 16.0f;
  shot_.layer = 10;
  HLSObject* muzzle = g_add_pats.Get(1);
  g_muzzle_effect.sprite = muzzle;
  return 1;
}

// 0x413070
int Control01::Save(Actor* actor, File* file) {
  PlayerRecord record;
  record.x = actor->transform->m[2][0];
  record.y = actor->transform->m[2][1];
  record.weapon = weapon_;
  record.tip_x = g_laser.tip_x_;
  record.tip_y = g_laser.tip_y_;
  record.angle = g_laser.angle_;
  record.spread = g_laser.spread_;
  return file->WriteExact(&record, sizeof(record));
}

// 0x4130f0
// TODO: nonmatching (the original keeps 0 in a register and -1 as immediates)
void Control01::OnEvent(Actor* actor, int event, int value) {
  PlayerWork* work = reinterpret_cast<PlayerWork*>(actor->work);
  switch (event) {
    case kEventInvincible:
      work->invincible = value;
      if (value != 0) {
        actor->team = 0;
        if (value <= 0) {
          work->invincible = -1;
        }
      } else {
        actor->team = 0x60;
      }
      actor->color = 0xffffffff;
      break;
  }
}

// 0x413130
// TODO: nonmatching (register choice: the original reloads the laser angle as a
// dword and widens it at each use)
void Control01::Fire(Actor* actor) {
  PlayerWork* work = reinterpret_cast<PlayerWork*>(actor->work);
  short angle = 0;
  switch (work->weapon) {
    case kLaser: {
      int spread = g_laser.spread();
      if (work->turn_time != 0) {
        spread >>= 1;
      }
      angle = g_laser.angle();
      float dx = g_sin_table[angle & 0x3ff] * 4.0f;
      float dy = g_sin_table[(angle + 0x100) & 0x3ff] * -4.0f;
      shot_.x = dx + actor->transform->m[2][0];
      shot_.y = dy + actor->transform->m[2][1];
      shot_.start_angle = rand() % spread + angle;
      g_actor_system.Spawn(&shot_);
      shot_.x = dx + actor->transform->m[2][0];
      shot_.y = dy + actor->transform->m[2][1];
      shot_.start_angle = angle - rand() % spread;
      g_actor_system.Spawn(&shot_);
      if (work->flame->angle != angle) {
        work->flame->angle = angle;
      }
      break;
    }
    case kShot: {
      float offset = static_cast<float>(rand() & 7);
      if (work->turn_time != 0) {
        shot_.x = -offset + actor->transform->m[2][0];
        shot_.y = actor->transform->m[2][1] - 4.0f;
        shot_.start_angle = (rand() & 0xf) - 7;
        g_actor_system.Spawn(&shot_);
        shot_.x = offset + actor->transform->m[2][0];
        shot_.y = actor->transform->m[2][1] - 4.0f;
        shot_.start_angle = (rand() & 0xf) - 7;
        g_actor_system.Spawn(&shot_);
        shot_.x = actor->transform->m[2][0];
        shot_.y = actor->transform->m[2][1] - 4.0f;
        shot_.start_angle = (rand() & 0xf) - 7;
        g_actor_system.Spawn(&shot_);
      } else {
        shot_.x = -offset + actor->transform->m[2][0];
        shot_.y = actor->transform->m[2][1] - 4.0f;
        shot_.start_angle = (rand() & 0x3f) - 0x1f;
        g_actor_system.Spawn(&shot_);
        shot_.x = offset + actor->transform->m[2][0];
        shot_.y = actor->transform->m[2][1] - 4.0f;
        shot_.start_angle = (rand() & 0x3f) - 0x1f;
        g_actor_system.Spawn(&shot_);
        rand();
      }
      break;
    }
  }
  g_muzzle_effect.angle_min = angle + 0x138;
  g_muzzle_effect.angle_max = angle + 0x2c8;
  g_actor_system.Create(8, NULL, actor->transform->m[2][0],
                        actor->transform->m[2][1], 3, 0, &g_muzzle_effect);
  if (work->shots % 3 == 0) {
    g_application.sound->channel(16)->Assign(&g_snd_fire_me, 0, 1.0f);
  }
}

// 0x413440
// TODO: nonmatching (the original copies record.y to the transform through the
// FPU)
int Control01::Load(Actor* actor, File* file) {
  PlayerRecord record;
  if (!file->ReadExact(&record, sizeof(record))) {
    return 0;
  }
  actor->transform->m[2][0] = record.x;
  actor->transform->m[2][1] = record.y;
  actor->flags |= Actor::kMoved;
  SelectWeapon(actor, record.weapon, record.x, record.y);
  g_laser.tip_x_ = record.tip_x;
  g_laser.tip_y_ = record.tip_y;
  g_laser.x_ = record.x;
  g_laser.y_ = record.y;
  g_laser.angle_ = record.angle;
  g_laser.spread_ = record.spread;
  return 1;
}

// 0x413550
// TODO: nonmatching (SetSprite() goes through eax in the original; register
// choice)
int Control01::OnCreate(Actor* actor, void* param) {
  PlayerWork* work = reinterpret_cast<PlayerWork*>(actor->work);
  actor->hit_points = 1;
  actor->team = 0x60;
  actor->debris = 16;
  actor->SetLayer(2);
  actor->SetSprite(g_sys_pats.Get(1));
  ActorDesc desc;
  memset(&desc, 0, sizeof(desc));
  desc.color = 0xffffffff;
  desc.flags = Actor::kAdditive;
  desc.team = 0;
  desc.hit_points = 1;
  desc.sprite = g_anm_sys.Play(&g_flame_anim);
  desc.parent = actor;
  desc.x = 0.0f;
  desc.y = 0.0f;
  desc.layer = 11;
  work->flame = g_actor_system.Spawn(&desc);
  work->flame->color = 0;
  memset(&desc, 0, sizeof(desc));
  desc.color = 0xffffffff;
  desc.flags = 0;
  desc.team = 0;
  desc.hit_points = 1;
  desc.sprite = &g_laser;
  desc.parent = actor;
  desc.x = 0.0f;
  desc.y = 0.0f;
  desc.layer = 1;
  work->laser = g_actor_system.Spawn(&desc);
  work->turn_time = 0;
  work->shots = 0;
  work->switch_time = 0;
  actor->start_angle = 0;
  actor->speed = 0.0f;
  float y = actor->transform->m[2][1];
  float x = actor->transform->m[2][0];
  SelectWeapon(actor, weapon_, x, y);
  work->invincible = 120;
  actor->color = 0xffffffff;
  actor->team = 0;
  return 1;
}

// 0x4136f0
// TODO: nonmatching (register choice around the weapon switch and the blink
// color)
int Control01::OnMove(Actor* actor) {
  PlayerWork* work = reinterpret_cast<PlayerWork*>(actor->work);
  int pressed = g_fn_game.pressed();
  int keys = g_fn_game.keys();
  float push_x = 0.0f;
  float push_y = 0.0f;
  short direction = g_direction_angles[keys & FnGame::kKeyDirections];
  float speed;
  if (direction < 0) {
    speed = 0.0f;
  } else if (keys & FnGame::kKeySlow) {
    speed = 2.0f;
  } else {
    speed = 7.0f;
  }
  Transform* transform = actor->transform;
  float x = g_sin_table[direction & 0x3ff] * speed + transform->m[2][0];
  float y =
      transform->m[2][1] - g_sin_table[(direction + 0x100) & 0x3ff] * speed;
  if (x < g_player_bounds.left) {
    push_x = g_player_bounds.left - x;
    x = g_player_bounds.left;
  } else if (x > g_player_bounds.right) {
    push_x = g_player_bounds.right - x;
    x = g_player_bounds.right;
  }
  if (y < g_player_bounds.top) {
    push_y = g_player_bounds.top - y;
    y = g_player_bounds.top;
  } else if (y > g_player_bounds.bottom) {
    push_y = g_player_bounds.bottom - y;
    y = g_player_bounds.bottom;
  }
  transform->m[2][0] = x;
  actor->transform->m[2][1] = y;
  actor->flags |= Actor::kMoved;
  if (pressed & FnGame::kKeyDirections) {
    work->turn_time = 8;
  } else if (work->turn_time > 0) {
    --work->turn_time;
  }
  if (pressed & FnGame::kKeySwitch) {
    SelectWeapon(actor, -1, x, y);
    g_application.sound->channel(12)->Assign(&g_snd_switch, 0, 1.0f);
    work->switch_time = 30;
  }
  int fire = keys & FnGame::kKeyFire;
  g_laser.Move(x, y, fire, push_x, push_y);
  if (work->invincible != 0) {
    if (work->invincible > 0) {
      actor->color = (g_application.frame_count & 2) ? 0xffffffff : 0;
      if (--work->invincible == 0) {
        work->invincible = 0;
        actor->team = 0x60;
        actor->color = 0xffffffff;
      }
    }
  } else if (ActorSystem::flags_ & 1) {
    Actor* hit = actor->HitTest();
    if (hit != NULL) {
      CEnmParam::OnActorDestroyed(hit, actor);
      return 0;
    }
  }
  if (fire) {
    Fire(actor);
    ++work->shots;
    work->flame->color = 0xffffffff;
  } else {
    work->flame->color = 0;
    work->shots = 0;
  }
  if (work->switch_time > 0 && --work->switch_time == 0) {
    int sound = g_weapon_sounds[work->weapon];
    g_application.sound->channel(sound)->Assign(Sound(sound), 0, 1.0f);
  }
  return 1;
}
