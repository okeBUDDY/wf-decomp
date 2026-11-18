// HLImage: a 24 or 32 bit image read from a BMP, TGA or PNG file into
// memory (vtable 0x451fdc). HLSurface::Load() copies it into a surface.
#ifndef WF_HL_IMAGE_H_
#define WF_HL_IMAGE_H_

class File;

class HLImage {
 public:
  enum Format { kAuto = 0, kBmp = 1, kTga = 2, kPng = 3 };

  HLImage();  // 0x40d580
  // 0x40b8e0, scalar deleting dtor 0x40b8f0
  virtual ~HLImage() { Release(); }

  // Frees the pixels.
  void Release();  // 0x40d5a0
  // Reads `path` (from data.bin) as `format`; kAuto picks it by the last
  // letter of the name. Returns the Format read, 0 on failure.
  int Load(const char* path, int format);  // 0x40d9a0

  short width() const { return width_; }
  short height() const { return height_; }
  short bits() const { return bits_; }
  short pitch() const { return pitch_; }
  unsigned char* pixels() const { return pixels_; }

 private:
  // Allocates the pixels for width_ x height_ at bits_ (24 or 32); 0 on
  // failure.
  int Alloc();  // 0x40d5d0
  // The rows of BMP and TGA files are stored bottom-up.
  int LoadBmp(File* file);  // 0x40d620
  int LoadTga(File* file);  // 0x40d6e0
  int LoadPng(File* file);  // 0x40d830
  // Turns RGB(A) into BGR(A).
  void SwapRedBlue();  // 0x40d7b0

  short width_;            // 0x04
  short height_;           // 0x06
  short bits_;             // 0x08, per pixel
  short pitch_;            // 0x0a, bytes per row
  unsigned char* pixels_;  // 0x0c
};

#endif  // WF_HL_IMAGE_H_
