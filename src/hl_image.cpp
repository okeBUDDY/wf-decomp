#include "hl_image.h"

#include <ctype.h>
#include <string.h>
#include <windows.h>

#include <png.h>

#include "file.h"

// 0x40d580
HLImage::HLImage() {
  width_ = height_ = bits_ = pitch_ = 0;
  pixels_ = NULL;
}

// 0x40d5a0
void HLImage::Release() {
  if (pixels_ != NULL) {
    delete[] pixels_;
    pixels_ = NULL;
  }
  width_ = height_ = bits_ = pitch_ = 0;
}

// 0x40d5d0
int HLImage::Alloc() {
  if (bits_ != 24 && bits_ != 32) {
    return 0;
  }
  pitch_ = (bits_ >> 3) * width_;
  pixels_ = new unsigned char[height_ * pitch_];
  return pixels_ != NULL;
}

// 0x40d620
int HLImage::LoadBmp(File* file) {
  BITMAPFILEHEADER file_header;
  BITMAPINFOHEADER info_header;
  unsigned char* row;
  if (file->Read(&file_header, sizeof(file_header)) != sizeof(file_header)) {
    return 0;
  }
  if (file->Read(&info_header, sizeof(info_header)) != sizeof(info_header)) {
    return 0;
  }
  width_ = static_cast<short>(info_header.biWidth);
  height_ = static_cast<short>(info_header.biHeight);
  bits_ = info_header.biBitCount;
  if (!Alloc()) {
    return 0;
  }
  row = pixels_ + (height_ - 1) * pitch_;
  for (int y = height_; y > 0;) {
    if (pitch_ != file->Read(row, pitch_)) {
      return 0;
    }
    --y;
    row -= pitch_;
  }
  return 1;
}

// 0x40d6e0
int HLImage::LoadTga(File* file) {
  // The 18-byte TGA header: the size at 12 and 14, the bits at 16.
  unsigned char header[18];
  if (file->Read(header, sizeof(header)) != sizeof(header)) {
    return 0;
  }
  width_ = *reinterpret_cast<short*>(&header[12]);
  height_ = *reinterpret_cast<short*>(&header[14]);
  bits_ = header[16];
  if (!Alloc()) {
    return 0;
  }
  unsigned char* row = pixels_ + (height_ - 1) * pitch_;
  for (int y = height_; y > 0;) {
    if (pitch_ != file->Read(row, pitch_)) {
      return 0;
    }
    --y;
    row -= pitch_;
  }
  return 1;
}

// 0x40d7b0
void HLImage::SwapRedBlue() {
  unsigned char* p = pixels_;
  if (bits_ == 32) {
    for (int i = width_ * height_; i > 0; --i) {
      unsigned char t = p[2];
      p[2] = p[0];
      p[0] = t;
      p += 4;
    }
  } else {
    for (int i = width_ * height_; i > 0; --i) {
      unsigned char t = p[2];
      p[2] = p[0];
      p[0] = t;
      p += 3;
    }
  }
}

// 0x40d810: the read callback of LoadPng(); the io pointer is the File.
static void ReadPng(png_structp png, png_bytep data, png_size_t length) {
  static_cast<File*>(png_get_io_ptr(png))->Read(data, length);
}

// 0x40d830
int HLImage::LoadPng(File* file) {
  png_structp png = NULL;
  png_infop info = NULL;
  png_bytep row;
  int result = 0;
  png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png == NULL) {
    return 0;
  }
  info = png_create_info_struct(png);
  if (info == NULL) {
    png_destroy_read_struct(&png, NULL, NULL);
    return 0;
  }
  png_set_read_fn(png, file, ReadPng);
  png_read_info(png, info);
  width_ = static_cast<short>(png_get_image_width(png, info));
  height_ = static_cast<short>(png_get_image_height(png, info));
  bits_ = png_get_bit_depth(png, info);
  switch (png_get_color_type(png, info)) {
    case PNG_COLOR_TYPE_RGB:
      bits_ *= 3;
      break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
      bits_ *= 4;
      break;
    default:
      bits_ = 0;
      break;
  }
  if (Alloc()) {
    row = pixels_;
    for (int y = height_; y > 0; --y) {
      png_read_rows(png, &row, NULL, 1);
      row += pitch_;
    }
    SwapRedBlue();
    result = 1;
  }
  png_destroy_read_struct(&png, &info, NULL);
  return result;
}

// 0x40d9a0
int HLImage::Load(const char* path, int format) {
  if (path == NULL || path[0] == '\0') {
    return 0;
  }
  if (pixels_ != NULL) {
    Release();
  }
  PackFile file;
  if (file.Open(path, File::kRead)) {
    if (format != kAuto) {
      switch (format) {
        case kPng:
          return LoadPng(&file) ? kPng : 0;
        case kTga:
          return LoadTga(&file) ? kTga : 0;
        case kBmp:
          return LoadBmp(&file) ? kBmp : 0;
      }
    }
    // By the last letter of the name: .bmp, .png, .tga.
    switch (tolower(path[strlen(path) - 1])) {
      case 'p':
        if (LoadBmp(&file)) {
          return kBmp;
        }
        break;
      case 'g':
        if (LoadPng(&file)) {
          return kPng;
        }
        break;
      case 'a':
        if (LoadTga(&file)) {
          return kTga;
        }
        break;
    }
  }
  return 0;
}
