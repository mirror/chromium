// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRI_GPU_PLATFORM_SUPPORT_GBM_H_
#define UI_OZONE_PLATFORM_DRI_GPU_PLATFORM_SUPPORT_GBM_H_

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/public/gpu_platform_support.h"

class SkBitmap;

namespace gfx {
class Point;
class Rect;
}

namespace ui {

class DriSurfaceFactory;
class DriWindowDelegate;
class DriWindowDelegateManager;
class ScreenManager;

class GpuPlatformSupportGbm : public GpuPlatformSupport {
 public:
  GpuPlatformSupportGbm(DriSurfaceFactory* dri,
                        DriWindowDelegateManager* window_manager,
                        ScreenManager* screen_manager);
  virtual ~GpuPlatformSupportGbm();

  void AddHandler(scoped_ptr<GpuPlatformSupport> handler);

  // GpuPlatformSupport:
  virtual void OnChannelEstablished(IPC::Sender* sender) OVERRIDE;

  // IPC::Listener:
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

 private:
  void OnCreateWindowDelegate(gfx::AcceleratedWidget widget);
  void OnDestroyWindowDelegate(gfx::AcceleratedWidget widget);
  void OnWindowBoundsChanged(gfx::AcceleratedWidget widget,
                             const gfx::Rect& bounds);
  void OnCursorSet(gfx::AcceleratedWidget widget,
                   const std::vector<SkBitmap>& bitmaps,
                   const gfx::Point& location,
                   int frame_delay_ms);
  void OnCursorMove(gfx::AcceleratedWidget widget, const gfx::Point& location);

  IPC::Sender* sender_;
  DriSurfaceFactory* dri_;
  DriWindowDelegateManager* window_manager_;
  ScreenManager* screen_manager_;
  ScopedVector<GpuPlatformSupport> handlers_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRI_GPU_PLATFORM_SUPPORT_GBM_H_
