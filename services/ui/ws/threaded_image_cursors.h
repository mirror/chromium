// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_THREADED_IMAGE_CURSORS_H_
#define SERVICES_UI_WS_THREADED_IMAGE_CURSORS_H_

#include "base/memory/weak_ptr.h"
#include "ui/base/cursor/cursor.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace display {
class Display;
}

namespace ui {
class ImageCursorsHolder;
class PlatformWindow;

namespace ws {

// Wrapper around ui::ImageCursors, capable of executing its methods
// asynchronously via the provided task runner.
class ThreadedImageCursors {
 public:
  // |resource_runner| is the task runner for the thread which can be used to
  // load resources; |image_cursors_holder_weak_ptr| is the object used to
  // perform cursor operations, and should only be dereferenced on
  // |resource_runner|.
  ThreadedImageCursors(
      scoped_refptr<base::SingleThreadTaskRunner>& resource_task_runner,
      base::WeakPtr<ui::ImageCursorsHolder> image_cursors_holder_weak_ptr);
  ~ThreadedImageCursors();

  // Executes ui::ImageCursors::SetDisplay asynchronously.
  // Sets the display the cursors are loaded for. |scale_factor| determines the
  // size of the image to load. Returns true if the cursor image is reloaded.
  void SetDisplay(const display::Display& display, float scale_factor);

  // Asynchronously sets the size of the mouse cursor icon.
  void SetCursorSize(CursorSize cursor_size);

  // Asynchronously sets the cursor type and then sets the corresponding
  // PlatformCursor on the provided |platform_window|.
  // |platform_window| pointer needs to be valid while this object is alive.
  void SetCursor(ui::CursorType cursor_type,
                 ui::PlatformWindow* platform_window);

  // Helper method. Sets |platform_cursor| on the |platform_window|.
  void SetCursorOnPlatformWindow(ui::PlatformCursor platform_cursor,
                                 ui::PlatformWindow* platform_window);

 private:
  // Task runner used for accessing |image_cursors_holder_weak_ptr_|.
  scoped_refptr<base::SingleThreadTaskRunner> resource_task_runner_;

  // Task runner of the UI Service thread (where this object is created and
  // destroyed).
  scoped_refptr<base::SingleThreadTaskRunner> ui_service_task_runner_;

  // Used to perform the cursor operations. Should only be dereferenced on
  // |resource_runner_|.
  base::WeakPtr<ui::ImageCursorsHolder> image_cursors_holder_weak_ptr_;

  base::WeakPtrFactory<ThreadedImageCursors> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ThreadedImageCursors);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_THREADED_IMAGE_CURSORS_H_
