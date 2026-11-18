// Node and TaskBase (one object: 0x402700-0x4028c6).
#include "task.h"

// 0x402700. NOT MATCHING yet: same instructions, but the original stores the
// vtable pointer before the scale (and keeps 1.0f in ecx from the start).
Node::Node() {
  position_.x = 0.0f;
  position_.y = 0.0f;
  position_.z = 0.0f;
  rotation_.x = 0.0f;
  rotation_.y = 0.0f;
  rotation_.z = 0.0f;
  mode_ = 0;
  next_ = NULL;
  prev_ = NULL;
  last_child_ = NULL;
  first_child_ = NULL;
  parent_ = NULL;
  flag_b0_ = 0;
  SetScale(1.0f, 1.0f, 1.0f);
  visible_ = 1;
}

// 0x402770
void Node::SetMode(int mode) { mode_ = mode; }

// 0x402780
TaskBase::TaskBase() {}

// 0x4027a0
void Node::Unlink() {
  while (first_child_ != NULL) {
    delete first_child_;
  }
  if (prev_ == NULL) {
    if (next_ == NULL) {
      if (parent_ != NULL) {
        parent_->last_child_ = NULL;
        parent_->first_child_ = NULL;
      }
      return;
    }
    next_->prev_ = NULL;
    if (parent_ != NULL) {
      parent_->first_child_ = next_;
    }
    return;
  }
  if (next_ == NULL && parent_ != NULL) {
    parent_->last_child_ = prev_;
  }
  prev_->next_ = next_;
}

// 0x402850
void Node::Detach() { Unlink(); }

// 0x402880
TaskBase::~TaskBase() { Unlink(); }
