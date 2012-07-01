// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_USER_IMAGE_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_USER_IMAGE_H_
#pragma once

#include <vector>

#include "googleurl/src/gurl.h"
#include "ui/gfx/image/image_skia.h"

namespace chromeos {

// Wrapper class for bitmaps and raw images when it's necessary. Could
// be used for storing profile images (including animated profile
// images) and user wallpapers.
class UserImage {
 public:
  typedef std::vector<unsigned char> RawImage;

  // Constructs UserImage from bitmap. Should be used where only
  // static image is needed (e.g. wallpapers).
  explicit UserImage(const gfx::ImageSkia& image);

  // Constructs UserImage from raw image and bitmap that should
  // represent single frame from that one. Can be used for wrapping
  // animated images.
  UserImage(const gfx::ImageSkia& image, const RawImage& raw_image);

  virtual ~UserImage();

  // Replaces already stored image to new |image|. Note, that
  // |raw_image| and |animated_image| will be reset after that
  // operation.
  void SetImage(const gfx::ImageSkia& image);
  const gfx::ImageSkia& image() const { return image_; }

  // Returns true iff |image| argument of constructors or |SetImage|
  // can be encoded as a bitmap.
  bool has_raw_image() const { return has_raw_image_; }
  const RawImage& raw_image() const { return raw_image_; }

  // Returns true iff UserImage is constructed from animated image.
  bool has_animated_image() const { return has_animated_image_; }
  const RawImage& animated_image() const { return animated_image_; }

  // URL from which this image was originally downloaded, if any.
  void set_url(const GURL& url) { url_ = url; }
  GURL url() const { return url_; }

 private:
  gfx::ImageSkia image_;
  bool has_raw_image_;
  RawImage raw_image_;
  bool has_animated_image_;
  RawImage animated_image_;
  GURL url_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_USER_IMAGE_H_
