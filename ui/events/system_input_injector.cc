// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/system_input_injector.h"

#include "base/memory/ptr_util.h"

namespace ui {

namespace {
SystemInputInjectorFactory* factory_ = nullptr;
}  // namespace

void SetSystemInputInjectorFactory(SystemInputInjectorFactory* factory) {
  DCHECK(!factory_);
  factory_ = factory;
}

std::unique_ptr<SystemInputInjector> CreateSystemInputInjector() {
  if (!factory_)
    return nullptr;
  return factory_->CreateSystemInputInjector();
}

}  // namespace ui
