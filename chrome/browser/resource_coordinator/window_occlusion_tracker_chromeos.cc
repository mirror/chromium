// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/window_occlusion_tracker.h"

#include "base/memory/ptr_util.h"
#include "ui/aura/env.h"
#include "ui/aura/env_observer.h"
#include "ui/aura/window_observer.h"

namespace resource_coordinator {

class WindowOcclusionTracker::Impl : public aura::EnvObserver {
 public:
  Impl();
  ~Impl() override;

 private:
  // aura::EnvObserver:
  void OnWindowInitialized(aura::Window* window) override;

  // aura::WindowObserver:
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds);

  DISALLOW_COPY_AND_ASSIGN(Impl);
};

WindowOcclusionTracker::Impl::Impl() {
  aura::Env* env = aura::Env::GetInstanceDontCreate();
  if (env)
    env->AddObserver(this);
}

WindowOcclusionTracker::Impl::~Impl() {
  aura::Env* env = aura::Env::GetInstanceDontCreate();
  if (env)
    env->RemoveObserver(this);
}

void WindowOcclusionTracker::Impl::OnWindowInitialized(aura::Window* window) {}

WindowOcclusionTracker::WindowOcclusionTracker()
    : impl_(base::MakeUnique<Impl>()) {}

WindowOcclusionTracker::~WindowOcclusionTracker() = default;

void WindowOcclusionTracker::AddObserver(gfx::NativeWindow window,
                                         WindowOcclusionObserver* observer) {}

void WindowOcclusionTracker::RemoveObserver(gfx::NativeWindow window,
                                            WindowOcclusionObserver* observer) {

}

}  // namespace resource_coordinator
