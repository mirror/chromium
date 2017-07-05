// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LoaderTrial_h
#define LoaderTrial_h

#include <map>
#include <string>
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

// An utility class to access field trial parameters.
class LoaderTrial final {
  STACK_ALLOCATED();
  WTF_MAKE_NONCOPYABLE(LoaderTrial);

 public:
  LoaderTrial(const char* trial);

  uint32_t GetUint32Param(const char* name, uint32_t default_param);

 private:
  std::map<std::string, std::string> trial_params_;
};

}  // namespace blink

#endif
