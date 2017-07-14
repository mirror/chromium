// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/exported/ControllerFactory.h"
#include "platform/wtf/Assertions.h"

namespace blink {

ControllerFactory* ControllerFactory::factory_instance_ = nullptr;

ControllerFactory& ControllerFactory::GetInstance() {
  DCHECK(factory_instance_);
  return *factory_instance_;
}

void ControllerFactory::SetInstance(ControllerFactory& factory) {
  DCHECK_EQ(nullptr, factory_instance_);
  factory_instance_ = &factory;
}

}  // namespace blink
