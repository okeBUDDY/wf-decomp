#include "actor_control.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "actor_system.h"
#include "application.h"
#include "cglobal.h"
#include "enm_param.h"
#include "game.h"
#include "game_data.h"
#include "snd_sys.h"
#include "sounds.h"

// Actor::work of control 22: fades in, glows for a while and fades out.
struct GlowWork {
  int timer;     // frames in the state
  int state;     // 0 start, 1 fade in, 2 glow, 3 fade out
  int duration;  // of the glow
  int phase;     // of the glow's pulse
  short spin;    // added to the angle every frame
};

// 0x40fef0
ActorControl::~ActorControl() {}

// 0x40ff00
int cGlobal::RegisterActorControls() {
  if (!g_actor_system.Register(kControlPlayer, new PlayerControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlEffect, new EffectControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlPlayerExplosion,
                               new PlayerExplosionControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlSmallExplosion,
                               new SmallExplosionControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlMediumExplosion,
                               new MediumExplosionControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlLargeExplosion,
                               new LargeExplosionControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlBossExplosion,
                               new BossExplosionControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlSmoke, new SmokeControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlGlow, new GlowControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlFlare, new FlareControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlTurret, new TurretControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlLaser, new LaserControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlBossBody, new BossBodyControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlBossArm, new BossArmControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlJointPart, new JointPartControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlBulletCore, new BulletCoreControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlShellCore, new ShellCoreControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlLaserCore, new LaserCoreControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlHomingCore, new HomingCoreControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlMissileCore, new MissileCoreControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlSparkCore, new SparkCoreControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlStreamCore, new StreamCoreControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlHomingMissile,
                               new HomingMissileControl)) {
    return 0;
  }
  if (!g_actor_system.Register(kControlMissile, new MissileControl)) {
    return 0;
  }
  return 1;
}

// 0x410350
int GlowControl::OnMove(Actor* actor) {
  GlowWork* work = reinterpret_cast<GlowWork*>(actor->work);
  float alpha;
  switch (work->state) {
    case 0:
      work->phase = rand() & 0xff;
      work->spin = (rand() & 1) ? 2 : -2;
      work->timer = 0;
      ++work->state;
    case 1:
      alpha = work->timer * (1.0f / 60.0f);
      if (work->timer >= 60) {
        work->timer = 0;
        ++work->state;
      }
      break;
    case 2:
      alpha = 1.0f;
      if (work->timer >= work->duration) {
        work->timer = 0;
        work->state = 3;
      }
      break;
    case 3:
      alpha = 1.0f - work->timer * (1.0f / 60.0f);
      if (work->timer >= 60) {
        return 1;
      }
      break;
  }
  actor->color =
      (static_cast<int>((g_sin_table[(work->phase & 0xff) * 4] * 0.2f + 0.8f) *
                        alpha * 255.0f)
       << 24) |
      0xff8800;
  actor->angle += work->spin;
  ++work->phase;
  ++work->timer;
  return 1;
}

// 0x410470
int GlowControl::OnCreate(Actor* actor, void* param) {
  actor->hit_points = 1;
  actor->team = 0;
  actor->SetSprite(g_pats.add[0].Get(27));
  actor->flags |= Actor::kAdditive;
  GlowWork* work = reinterpret_cast<GlowWork*>(actor->work);
  work->duration = reinterpret_cast<int>(param) - 120;
  work->state = 0;
  return 1;
}

// 0x4108c0
void Actor::SetLayer(unsigned short new_layer) {
  if (flags & kCreating) {
    layer = new_layer;
    return;
  }
  if (draw_prev != NULL) {
    draw_prev->draw_next = draw_next;
  } else {
    system_->layers_[layer] = draw_next;
  }
  if (draw_next != NULL) {
    draw_next->draw_prev = draw_prev;
  }
  layer = new_layer;
  system_->Link(this);
}

// 0x4104d0
void PlayerExplosionControl::OnDestroy(Actor* actor) { g_game.SpawnPlayer(); }

// 0x4104e0
int PlayerExplosionControl::OnMove(Actor* actor) { return actor->age <= 60; }

// 0x410500
int SmallExplosionControl::OnMove(Actor* actor) { return actor->age <= 18; }

// 0x410520
void BossExplosionControl::OnDestroy(Actor* actor) { g_boss = NULL; }

// 0x4127c0
void TurretControl::OnDestroy(Actor* actor) { --g_enm_param.turret_count_; }

// 0x412820
int TurretControl::Initialize() {
  memset(&desc_, 0, sizeof(desc_));
  desc_.color = 0xffffffff;
  HLSObject* sprite = g_pats.sys[0].Get(64);
  desc_.sprite = sprite;
  desc_.flags = Actor::kFaceStart;
  desc_.team = 9;
  desc_.hit_points = 1;
  desc_.speed = 6.0f;
  desc_.layer = 13;
  return 1;
}

// 0x412fa0
void PlayerControl::OnDestroy(Actor* actor) { g_game.OnPlayerDestroyed(); }

// 0x417c00 (this object, so the enm_param.cpp callers do not inline it)
void CEnmParam::ClearTitles() { title_count_ = 0; }

Actor* BossBodyControl::boss_;
int BossBodyControl::weapons_[7];
int CorePartControl::max_volley_time_;

int g_core_controls[7] = {kControlBulletCore, kControlShellCore,
                          kControlLaserCore,  kControlHomingCore,
                          kControlSparkCore,  kControlMissileCore,
                          kControlStreamCore};
int g_debris_controls[4] = {0, kControlSmallExplosion, kControlMediumExplosion,
                            kControlLargeExplosion};

// 0x418c40
int ScoredControl::Prepare(Actor* actor, void* param) {
  PartWork* work = reinterpret_cast<PartWork*>(actor->work);
  work->slot = static_cast<EnmSlot*>(param);
  work->flags = 0;
  actor->hit_points = GetHitPoints(actor);
  actor->hit_points += actor->hit_points * g_enm_param.state_.toughness / 1024;
  return 1;
}

// 0x418c90
void PartControl::Schedule(Actor* actor, int delay, int next) {
  OnDamage(actor, kEventWait);
  PartWork* work = reinterpret_cast<PartWork*>(actor->work);
  work->timer = delay;
  work->step = next;
}

// 0x418cc0
void CorePartControl::OnEvent(Actor* actor, int event, int targets) {
  PartWork* work = reinterpret_cast<PartWork*>(actor->work);
  if (!(targets & 0x2000) && !(work->flags & PartWork::kReady)) {
    return;
  }
  if (!(targets & (0x10000 << GetLevel()))) {
    return;
  }
  if (!(targets & 0x100) && (targets & 0xff) != 0) {
    return;
  }
  if (event == 5) {
    if (targets & 0x800) {
      if (!(work->flags & PartWork::kBoosted)) {
        Setup(actor, 100);
        work->flags |= PartWork::kBoosted;
      }
    } else if (work->flags & PartWork::kBoosted) {
      Setup(actor, work->rank);
      work->flags &= ~PartWork::kBoosted;
    }
    if (work->volley_time > max_volley_time_) {
      max_volley_time_ = work->volley_time;
    }
  }
  OnDamage(actor, event);
}

// 0x418d70
void PartControl::ScaleHitPoints(Actor* actor) {
  if (actor->team != 0xa1) {
    actor->hit_points = (reinterpret_cast<PartWork*>(actor->work)->level + 3) *
                        actor->hit_points / 4;
  }
}

// 0x418da0
int LinkedPartControl::SpawnChildren(Actor* actor, unsigned int base,
                                     unsigned int index, int count) {
  PartWork* work = reinterpret_cast<PartWork*>(actor->work);
  for (; count > 0; --count, ++index) {
    EnmSlot* slot = g_enm_param.FindSlot(base | index);
    if (slot == NULL) {
      return 0;
    }
    Actor* child;
    switch (slot->flags & 3) {
      case 0:
        child = g_actor_system.Create(g_core_controls[slot->weapon], actor,
                                      slot->part->x, slot->part->y, 6, 0, slot);
        break;
      case 1:
        child =
            g_actor_system.Create(g_core_controls[slot->weapon], actor,
                                  slot->part->x1, slot->part->y1, 6, 0, slot);
        break;
      default:
        child = g_actor_system.Create(kControlJointPart, actor, slot->part->x,
                                      slot->part->y, 4, 0, slot);
        break;
    }
    if (child != NULL) {
      work->level += reinterpret_cast<PartWork*>(child->work)->level;
    }
  }
  return 1;
}

// 0x418ec0
int LinkedPartControl::SpawnGlow(Actor* actor) {
  return g_actor_system.Create(
             22, actor, 0.0f, 0.0f, 5, 0,
             reinterpret_cast<void*>(g_enm_param.glow_time_)) != NULL;
}

// 0x418ef0
unsigned short Actor::ClampAngle(unsigned short value, short low, short high) {
  short range = high - low;
  if (range <= 0) {
    return low;
  }
  if (range >= 0x400) {
    return value;
  }
  short offset = (value - low) & 0x3ff;
  if (offset <= range) {
    return value;
  }
  if (offset < (range >> 1) + 0x200) {
    return high;
  }
  return low;
}

// 0x418f50
int PartControl::SpawnPieces(Actor* actor, const PartPiece* pieces, int count) {
  ActorDesc desc;
  memset(&desc, 0, sizeof(desc));
  desc.slot = reinterpret_cast<PartWork*>(actor->work)->slot;
  desc.color = 0xffffffff;
  for (; count > 0; --count, ++pieces) {
    desc.debris = g_debris_controls[pieces->debris];
    desc.flags = 0;
    desc.team = 0xa1;
    desc.hit_points = 1;
    HLSObject* sprite = pieces->file->Get(pieces->pat);
    desc.x = pieces->x;
    desc.sprite = sprite;
    desc.parent = actor;
    desc.y = pieces->y;
    desc.layer = actor->layer + pieces->layer;
    g_actor_system.Spawn(&desc);
  }
  return 1;
}

// 0x419020
void PartControl::PlayTrans(PartWork* work) {
  if (!(work->flags & PartWork::kTrans)) {
    work->flags |= PartWork::kTrans;
    ++g_enm_param.trans_count_;
    g_application.sound->channel(27)->Assign(&g_sounds[kSndTrans1], 0, 1.0f);
  }
}

// 0x419070
int PartControl::InitPart(Actor* actor, void* param) {
  Prepare(actor, param);
  PartWork* work = reinterpret_cast<PartWork*>(actor->work);
  const EnmPlanPart* part = work->slot->part;
  EnmSlot* parent =
      g_enm_param.FindSlot(CEnmParam::ParentCode(work->slot->code));
  if (part->flags & EnmPlanPart::kGuard) {
    actor->team = 0xa1;
    work->level = 0;
  } else {
    actor->team = 0x61;
    work->level = 1;
  }
  if (parent != NULL && (parent->part->flags & EnmPlanPart::kTurret) &&
      !(work->slot->code & 7)) {
    work->turret = actor->parent;
    work->low = parent->low;
    work->high = parent->high;
  } else {
    work->turret = actor;
    work->low = work->slot->low;
    work->high = work->slot->high;
  }
  work->angle = (part->flags & EnmPlanPart::kAimHigh) ? work->high : work->low;
  if ((work->slot->flags & 3) == 1) {
    work->flags |= PartWork::kAims;
  } else if (part->flags & 0x20) {
    work->flags |= PartWork::kTransforms;
  } else if (part->flags & 0x10) {
    work->flags |= PartWork::kAims;
  }
  work->turret->angle = work->angle;
  work->turret->start_angle = work->turret->angle;
  work->max_hit_points = actor->hit_points;
  work->flags |= PartWork::kReady;
  work->state = 0;
  int side = (work->slot->code & CEnmParam::kSideMask) == 0x80000000;
  ++g_enm_param.parts_alive_[side];
  if (work->flags & PartWork::kTransforms) {
    ++g_enm_param.transform_parts_[side];
  }
  if (work->slot->flags & 8) {
    ++g_enm_param.armored_parts_[side];
    work->flags |= PartWork::kArmored;
  }
  return 1;
}

// 0x419220
int CorePartControl::InitCore(Actor* actor, void* param) {
  InitPart(actor, param);
  PartWork* work = reinterpret_cast<PartWork*>(actor->work);
  work->rank = work->slot->power * 100 / 1024;
  work->volley_time = 0;
  Setup(actor, work->rank);
  g_enm_param.core_power_[(work->slot->code & CEnmParam::kSideMask) ==
                          0x80000000][GetLevel()] += work->rank + 50;
  ++g_enm_param.core_count_;
  return 1;
}

// 0x4192b0
int LinkedPartControl::InitLinked(Actor* actor, void* param) {
  InitPart(actor, param);
  const EnmPlanPart* part =
      reinterpret_cast<PartWork*>(actor->work)->slot->part;
  actor->debris = g_debris_controls[part->debris];
  actor->color = g_enm_param.state_.color;
  if (part->flags & EnmPlanPart::kMirrorX) {
    actor->flags |= Actor::kMirrorX;
    actor->matrix_angle += 0x1000;
  }
  if (part->flags & EnmPlanPart::kMirrorY) {
    actor->flags |= Actor::kMirrorY;
    actor->matrix_angle += 0x1000;
  }
  actor->SetSprite(PlanPatFile(part->pat_file)->Get(part->pat));
  if (part->flags & 0x10) {
    OnDamage(actor, kEventAim);
  } else {
    OnDamage(actor, kEventIdle);
  }
  return 1;
}

// 0x419370
void LinkedPartControl::OnEvent(Actor* actor, int event, int targets) {
  int count = targets & 0xff;
  if ((targets & 0x1000) &&
      ((targets & 0x2000) ||
       (reinterpret_cast<PartWork*>(actor->work)->flags & PartWork::kReady)) &&
      ((targets & 0x100) || count == 0)) {
    OnDamage(actor, event);
  }
  if (--count >= 0) {
    targets = (targets & ~0xff) | count;
    for (Actor* child = actor->first_child; child != NULL;
         child = child->prev_sibling) {
      if (child->control != NULL) {
        child->control->OnEvent(child, event, targets);
      }
    }
  }
}

// 0x419400
void LinkedPartControl::OnChildDestroyed(Actor* parent, Actor* child) {
  PartWork* work = reinterpret_cast<PartWork*>(parent->work);
  if (child->team == 0x61) {
    if (parent->team != 0xa1 && work->level > 1) {
      int old_level = work->level;
      work->level -= reinterpret_cast<PartWork*>(child->work)->level;
      parent->hit_points =
          abs(parent->hit_points) * (work->level + 3) / (old_level + 3);
      if (parent->hit_points < 1) {
        parent->hit_points = 1;
      }
      if (work->level == 1) {
        g_actor_system.Create(kControlTurret, parent, 0.0f, 0.0f, 4, 0,
                              work->slot);
      }
    }
    if (parent->id != kControlBossBody) {
      parent->parent->control->OnChildDestroyed(parent->parent, child);
    }
  }
}

// 0x419580
void PartControl::StopTrans(PartWork* work, int next) {
  if (work->flags & PartWork::kTrans) {
    work->flags &= ~PartWork::kTrans;
    if (--g_enm_param.trans_count_ <= 0) {
      g_application.sound->channel(27)->Stop();
    }
    if (next) {
      g_application.sound->channel(28)->Assign(&g_sounds[kSndTrans2], 0, 1.0f);
    }
  }
}

// 0x4195f0
void PartControl::OnDestroy(Actor* actor) {
  PartWork* work = reinterpret_cast<PartWork*>(actor->work);
  work->slot->lerp_at = actor->angle;
  // Each destroyed ancestor doubles the chain; the root counts as one.
  int chain = 1;
  int rapid = 1;
  Actor* ancestor = actor;
  do {
    ancestor = ancestor->parent;
    if (ancestor == NULL) {
      chain >>= 1;
      break;
    }
    if (ancestor->flags != Actor::kDead) {
      break;
    }
    chain <<= 1;
  } while (ancestor != NULL);
  if (chain > 32) {
    chain = 32;
  } else if (chain == 1) {
    // Destroyed fast: in percent of its hit points per frame.
    int speed = work->max_hit_points * 100 / (actor->age - actor->hit_age + 1);
    rapid = chain;
    if (speed > 110) {
      rapid <<= speed / 55 - 1;
      ++work->slot->hits;
      if (rapid > 32) {
        rapid = 32;
      }
    }
  }
  if (work->flags & PartWork::kTrans) {
    work->flags &= ~PartWork::kTrans;
    if (--g_enm_param.trans_count_ <= 0) {
      g_application.sound->channel(27)->Stop();
    }
  }
  g_enm_param.LogDestroy(work->slot, chain, rapid);
  if (chain == 1) {
    work->slot->power += 20;
    if (work->slot->power > 0x400) {
      work->slot->power = 0x400;
    } else if (work->slot->power < 0) {
      work->slot->power = 0;
    }
    g_game.OnPartDestroyed();
  }
  int side = (work->slot->code & CEnmParam::kSideMask) == 0x80000000;
  --g_enm_param.parts_alive_[side];
  if (work->flags & PartWork::kTransforms) {
    --g_enm_param.transform_parts_[side];
  }
  if (work->flags & PartWork::kArmored) {
    --g_enm_param.armored_parts_[side];
  }
  int score = GetScore(actor);
  g_enm_param.score_base_ += score;
  if (rapid > 1) {
    ++g_enm_param.rapid_count_;
    g_enm_param.score_rapid_ += (rapid - 1) * score;
  }
  if (chain > 1) {
    ++g_enm_param.chain_count_;
    g_enm_param.score_chain_ += (chain - 1) * score;
  }
  g_global.entry.score += score * rapid * chain;
}

// 0x4197d0
void CorePartControl::OnDestroy(Actor* actor) {
  PartControl::OnDestroy(actor);
  PartWork* work = reinterpret_cast<PartWork*>(actor->work);
  g_enm_param.core_power_[(work->slot->code & CEnmParam::kSideMask) ==
                          0x80000000][GetLevel()] -= work->rank + 50;
  --g_enm_param.core_count_;
  EnmSlot* slot = work->slot;
  if (slot->power > g_enm_param.state_.weapon_levels[slot->weapon]) {
    g_enm_param.state_.weapon_levels[slot->weapon] = slot->power;
  }
}

// 0x419850
void PartControl::SpawnFlare(Actor* actor) {
  unsigned short angle = BossBodyControl::boss_->AngleTo(actor->screen.m[2][0],
                                                         actor->screen.m[2][1]);
  short offset = (g_enm_param.dash_angle_ - angle) & 0x3ff;
  if (offset > 0x100 && offset < 0x300) {
    if (actor->transform != &actor->screen) {
      angle = actor->parent->ToLocalAngle(angle);
      if (actor->flags & Actor::kMirrorX) {
        angle = -angle;
      }
      if (actor->flags & Actor::kMirrorY) {
        angle = 0x200 - angle;
      }
    }
    g_actor_system.Create(kControlFlare, actor, 0.0f, 0.0f, 1, angle, NULL);
    ++g_enm_param.flare_count_;
  }
}

// 0x4198e0
void PartControl::OnDamage(Actor* actor, int how) {
  PartWork* work = reinterpret_cast<PartWork*>(actor->work);
  StopTrans(work, 1);
  if (work->state == kEventWait) {
    work->state = 0;
    work->flags |= PartWork::kReady;
  }
  const unsigned int& flags = work->flags;
  switch (how) {
    case kEventAim:
      if (!(flags & PartWork::kAims)) {
        return;
      }
      // Fall through.
    case kEventIdle:
      work->flags |= PartWork::kReady;
      work->Enter(how);
      return;
    case kEventFire:
      if (IsOffScreen(actor)) {
        return;
      }
      work->slot->power += 3;
      if (work->slot->power > 0x400) {
        work->slot->power = 0x400;
      } else if (work->slot->power < 0) {
        work->slot->power = 0;
      }
      break;
    case kEventAimFire:
      if (!(flags & PartWork::kAims)) {
        return;
      }
      break;
    case kEventTransform:
      if (!(flags & PartWork::kTransforms)) {
        return;
      }
      if (flags & PartWork::kTransformed) {
        work->flags &= ~PartWork::kTransformed;
        how = 3;
      } else {
        work->flags |= PartWork::kTransformed;
      }
      PlayTrans(work);
      break;
    case kEventGlow:
      if (!(flags & PartWork::kArmored)) {
        return;
      }
      break;
    case kEventFlare:
      if (work->slot->part->flags & 0x200) {
        SpawnFlare(actor);
      }
      return;
  }
  work->flags &= ~PartWork::kReady;
  work->Enter(how);
}

// 0x419a70
int PartControl::MovePart(Actor* actor) {
  PartWork* work = reinterpret_cast<PartWork*>(actor->work);
  Actor* turret = work->turret;
  switch (work->state) {
    case kEventWait:
      if (work->timer > 0 && --work->timer == 0) {
        OnDamage(actor, work->step);
      }
      break;
    case kEventAim:
    case kEventAimFire: {
      short old_angle = turret->angle;
      turret->TurnTo(g_player, g_enm_param.turn_speed_);
      turret->angle = turret->ClampAngle(turret->angle, work->low, work->high);
      if (old_angle == turret->angle) {
        g_enm_param.state_.flags |= EnmState::kAimed;
        if (work->state == kEventAimFire) {
          OnDamage(actor, kEventFire);
        }
      }
      break;
    }
    case kEventReturn:
      switch (work->step) {
        case 0: {
          unsigned short now = turret->angle;
          short offset = (work->angle - now) & 0x3ff;
          if (offset >= 0x200) {
            offset -= 0x400;
          }
          if (abs(offset) < g_enm_param.turn_speed_) {
            turret->angle += offset;
            StopTrans(work, 1);
            ++work->step;
          } else {
            if (offset < 0) {
              now -= g_enm_param.turn_speed_;
            } else {
              now += g_enm_param.turn_speed_;
            }
            turret->angle = now;
          }
          if (work->timer > 0) {
            --work->timer;
          }
          break;
        }
        case 1:
          if (--work->timer <= 0) {
            OnDamage(actor, kEventIdle);
          }
          break;
      }
      break;
    case kEventTransform:
      switch (work->step) {
        case 0:
          ++work->step;
          // Fall through.
        case 1: {
          short goal = (work->slot->part->flags & EnmPlanPart::kAimHigh)
                           ? work->low
                           : work->high;
          unsigned short now = turret->angle;
          short offset = (goal - now) & 0x3ff;
          if (offset >= 0x200) {
            offset -= 0x400;
          }
          if (abs(offset) < g_enm_param.turn_speed_) {
            turret->angle += offset;
            StopTrans(work, 1);
            OnDamage(actor, kEventIdle);
          } else if (offset < 0) {
            turret->angle -= g_enm_param.turn_speed_;
          } else {
            turret->angle += g_enm_param.turn_speed_;
          }
          break;
        }
      }
      break;
  }
  return 1;
}

// 0x419cc0
int LinkedPartControl::OnMove(Actor* actor) {
  PartWork* work = reinterpret_cast<PartWork*>(actor->work);
  if (work->state != kEventGlow) {
    return MovePart(actor);
  }
  switch (work->step) {
    case 0:
      if (g_enm_param.glow_time_ < 120) {
        g_enm_param.glow_time_ = 120;
      }
      SpawnGlow(actor);
      for (Actor* child = actor->first_child; child != NULL;
           child = child->prev_sibling) {
        if (static_cast<HLSPat*>(child->sprite)->empty()) {
          g_actor_system.Create(
              22, child, 0.0f, 0.0f, 5, 0,
              reinterpret_cast<void*>(g_enm_param.glow_time_));
        }
      }
      g_application.sound->channel(24)->Assign(&g_sounds[kSndBarrier], 0, 1.0f);
      ++work->step;
      work->timer = 0;
      // Fall through.
    case 1:
      ++work->timer;
      actor->LerpColor(g_enm_param.state_.color, 0xffffff00, work->timer, 60);
      if (work->timer >= 60) {
        if (actor->team == 0x61) {
          actor->team = 0xe1;
        }
        work->timer = g_enm_param.glow_time_ - 120;
        ++work->step;
      }
      break;
    case 2:
      if (--work->timer <= 0) {
        ++work->step;
      }
      break;
    case 3:
      ++work->timer;
      actor->LerpColor(0xffffff00, g_enm_param.state_.color, work->timer, 60);
      if (work->timer >= 60) {
        if (actor->team == 0xe1) {
          actor->team = 0x61;
        }
        OnDamage(actor, kEventIdle);
      }
      break;
  }
  return 1;
}

// 0x4194a0
int PartControl::SpawnItems(Actor* actor) {
  EnmSlot* slot = reinterpret_cast<PartWork*>(actor->work)->slot;
  const EnmPlanPart* part = slot->part;
  const EnmPlanItem* item = part->item;
  ActorDesc desc;
  memset(&desc, 0, sizeof(desc));
  desc.color = 0xffffffff;
  desc.slot = slot;
  int count = part->item_count;
  for (int i = 0; i < count; ++i, ++item) {
    desc.debris = g_debris_controls[item->debris];
    desc.flags = 0;
    desc.team = 0xa1;
    desc.hit_points = 1;
    HLSObject* sprite = PlanPatFile(item->pat_file)->Get(item->pat);
    desc.sprite = sprite;
    desc.parent = actor;
    desc.x = item->x;
    desc.y = item->y;
    desc.layer = 5;
    g_actor_system.Spawn(&desc);
  }
  return 1;
}

// 0x4113d0
unsigned short Actor::ScreenAngle() {
  if (transform == &screen) {
    return angle;
  }
  unsigned short result = angle;
  for (Actor* ancestor = parent; ancestor != system_->actors_;
       ancestor = ancestor->parent) {
    if (ancestor->flags & kMirrorX) {
      result = -result;
    }
    if (ancestor->flags & kMirrorY) {
      result = 0x200 - result;
    }
    result += ancestor->angle;
  }
  return result;
}

// 0x411690
unsigned short Actor::ToLocalAngle(unsigned short screen_angle) {
  if (transform != &screen) {
    screen_angle = parent->ToLocalAngle(screen_angle);
  }
  screen_angle -= angle;
  if (flags & kMirrorX) {
    screen_angle = -screen_angle;
  }
  if (flags & kMirrorY) {
    screen_angle = 0x200 - screen_angle;
  }
  return screen_angle;
}

// 0x411d90
short Actor::AngleTo(float x, float y) {
  if (!(flags & kMovedOnce)) {
    return angle;
  }
  float dx = x - screen.m[2][0];
  float dy = y - screen.m[2][1];
  if (dx == 0.0f && dy == 0.0f) {
    return angle;
  }
  unsigned short result = 0x100 - static_cast<int>(atan2(dy, dx) * -162.97466f);
  return transform == &screen ? result : parent->ToLocalAngle(result);
}

// 0x4146a0
int ScoredControl::IsOffScreen(Actor* actor) {
  if (actor->screen.m[2][0] < 0.0f || actor->screen.m[2][0] > 640.0f ||
      actor->screen.m[2][1] < 0.0f || actor->screen.m[2][1] > 480.0f) {
    return 1;
  }
  return 0;
}

// 0x414a00
void Actor::TurnTo(Actor* target, short speed) {
  if (speed >= 0x200) {
    angle = target != NULL
                ? AngleTo(target->screen.m[2][0], target->screen.m[2][1])
                : angle;
    return;
  }
  unsigned short goal =
      target != NULL ? AngleTo(target->screen.m[2][0], target->screen.m[2][1])
                     : angle;
  short offset = (goal - angle) & 0x3ff;
  if (offset >= 0x200) {
    offset -= 0x400;
  }
  if (abs(offset) < speed) {
    angle += offset;
  } else if (offset < 0) {
    angle -= speed;
  } else {
    angle += speed;
  }
}

// 0x410850
HLSAnm* HLSAnmSys::Play(AnmData* data) {
  HLSAnm* anim = anims_;
  for (int i = count_; i > 0; ++anim, --i) {
    if (anim->cursor_ == NULL) {
      anim->data_ = data;
      anim->cursor_ = data->script;
      anim->object_ = data->pat_file->Get(anim->cursor_[0]);
      anim->texture_ = anim->object_->texture_;
      anim->wait_ = anim->cursor_[1];
      anim->loop_ = 0;
      return anim;
    }
  }
  return NULL;
}

// 0x40fe30
int LinkedPartControl::GetScore(Actor* actor) {
  return reinterpret_cast<PartWork*>(actor->work)->slot->part->score;
}

// 0x40fe40
int LinkedPartControl::GetHitPoints(Actor* actor) {
  return reinterpret_cast<PartWork*>(actor->work)->slot->part->hit_points;
}
