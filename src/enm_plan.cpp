#include "enm_param.h"

#include "file.h"

// 0x41b680
EnmPlan::EnmPlan() {
  parts_ = NULL;
  items_ = NULL;
  weapons_ = NULL;
}

// 0x41b6a0
void EnmPlan::Release() {
  if (parts_ != NULL) {
    delete[] parts_;
    parts_ = NULL;
  }
  if (items_ != NULL) {
    delete[] items_;
    items_ = NULL;
  }
  if (weapons_ != NULL) {
    delete[] weapons_;
    weapons_ = NULL;
  }
}

// 0x41b6f0
const EnmPlanPart* EnmPlan::Find(unsigned int code, int kind) {
  if (parts_ == NULL) {
    return NULL;
  }
  code &= CEnmParam::kCodeMask;
  if (code != 0) {
    if (kind != 0) {
      if (kind != kWeaponKind) {
        for (EnmPlanPart* part = &parts_[part_count_ - 1]; part >= parts_;
             --part) {
          if (part->code == code && part->kind == kind) {
            return part;
          }
        }
        return NULL;
      }
    } else {
      for (EnmPlanPart* part = &parts_[part_count_ - 1]; part >= parts_;
           --part) {
        if (part->code == code) {
          return part;
        }
      }
    }
    for (EnmPlanItem* weapon = &weapons_[weapon_count_ - 1]; weapon >= weapons_;
         --weapon) {
      if (weapon->code == code) {
        return reinterpret_cast<EnmPlanPart*>(weapon);
      }
    }
    return NULL;
  }
  return &root_;
}

// 0x41b790
int EnmPlan::Load(const char* path) {
  Release();
  PackFile file;
  if (!file.Open(path, File::kRead)) {
    return 0;
  }
  file.Read(&unknown_04_, 0x58);
  parts_ = new EnmPlanPart[part_count_];
  items_ = new EnmPlanItem[item_count_];
  weapons_ = new EnmPlanItem[weapon_count_];
  if (parts_ == NULL || items_ == NULL || weapons_ == NULL) {
    Release();
    return 0;
  }
  int i;
  for (i = 0; i < item_count_; ++i) {
    file.Read(&items_[i], sizeof(EnmPlanItem));
  }
  for (i = 0; i < weapon_count_; ++i) {
    file.Read(&weapons_[i], sizeof(EnmPlanItem));
  }
  kind_counts_[2] = 0;
  kind_counts_[1] = 0;
  kind_counts_[0] = 0;
  kind_counts_[5] = 0;
  kind_counts_[4] = 0;
  kind_counts_[3] = 0;
  for (i = 0; i < part_count_; ++i) {
    file.Read(&parts_[i], sizeof(EnmPlanPart));
    parts_[i].item = &items_[reinterpret_cast<int>(parts_[i].item)];
    switch (parts_[i].kind) {
      case 0x2010001:
      case 0x2010002:
        ++kind_counts_[0];
        break;
      case 0x2020001:
      case 0x2020002:
        ++kind_counts_[1];
        break;
      case 0x4010001:
        ++kind_counts_[3];
        break;
      case 0x4020001:
        ++kind_counts_[4];
        break;
      case 0x4040001:
        ++kind_counts_[5];
        break;
    }
  }
  kind_counts_[2] = kind_counts_[0] + kind_counts_[1];
  return 1;
}
