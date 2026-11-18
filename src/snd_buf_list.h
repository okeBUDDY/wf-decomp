// CSndBufList: the CSndBuf objects of the sound system (vtable 0x451dd8).
#ifndef WF_SND_BUF_LIST_H_
#define WF_SND_BUF_LIST_H_

#include "snd_buf.h"

class CSndBufList {
 public:
  CSndBufList() {
    tail_ = NULL;
    head_ = NULL;
    count_ = 0;
  }
  // scalar deleting dtor 0x408ea0
  virtual ~CSndBufList() {}
  // Appends `buffer` and gives it the next id.
  void Add(CSndBuf* buffer) {
    buffer->next_ = NULL;
    buffer->prev_ = tail_;
    if (head_ != NULL) {
      tail_->next_ = buffer;
    } else {
      head_ = buffer;
    }
    tail_ = buffer;
    buffer->id_ = count_++;
  }
  void Remove(CSndBuf* buffer);  // 0x40dfd0
  // Releases every buffer, last first.
  void ReleaseAll() {
    for (CSndBuf* buffer = tail_; buffer != NULL; buffer = buffer->prev_) {
      buffer->Release();
    }
    tail_ = NULL;
    head_ = NULL;
  }
  // Releases every buffer; returns 1.
  int Clear();  // 0x409330

 private:
  friend class SoundSystem;

  int count_;      // 0x04, ids handed out so far
  CSndBuf* head_;  // 0x08
  CSndBuf* tail_;  // 0x0c
};

#endif  // WF_SND_BUF_LIST_H_
