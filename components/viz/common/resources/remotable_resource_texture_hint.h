// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_REMOTABLE_RESOURCE_TEXTURE_HINT_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_REMOTABLE_RESOURCE_TEXTURE_HINT_H_

namespace viz {

// A hint for how a resource will be used that is created for sharing
// with the viz compositing service.
struct RemotableResourceTextureHint {
  enum {
    kDefault = 0x0,
    kImmutable = 0x1,
    kMipmap = 0x2,
    kFramebuffer = 0x4,
  };

  RemotableResourceTextureHint() : value_(kDefault) {}

  // Acts like an enum (via the char type).
  RemotableResourceTextureHint(char i) : value_(i) {}  // NOLINT
  operator char() const { return value_; }

  RemotableResourceTextureHint& operator|=(
      const RemotableResourceTextureHint& o) {
    value_ |= o.value_;
    return *this;
  }

 private:
  char value_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_REMOTABLE_RESOURCE_TEXTURE_HINT_H_
