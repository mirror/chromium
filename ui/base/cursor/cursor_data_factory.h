// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_CURSOR_CURSOR_DATA_FACTORY_H_
#define UI_BASE_CURSOR_CURSOR_DATA_FACTORY_H_

// This file is a work around.
//
// As is, we need a set of objects which mostly look like CursorFactoryOzone so
// that we can have a second CursorFactoryOzone, since we'll have the normal
// one that's part of Ozone proper, and this one which needs to be used
// opportunistically in //content/.
//
// In the long run, this file and direct usage of CursorFactoryOzone in
// //content/ should go away.

#include <map>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/cursor/cursor_data.h"
#include "ui/base/ui_base_export.h"
#include "ui/gfx/geometry/point.h"

namespace ui {

// A refcounted wrapper around a ui::CursorData to obey CursorFactoryOzone's
// refcounting interface while building ui::CursorData objects for transport
// over mojo pipes.
//
// TODO(erg): In the long term, this should go away. When //content/ switches
// from webcursor.h to use ui::CursorData directly, we should be able to get
// rid of this class which is an adaptor for the existing ozone code.
class UI_BASE_EXPORT CursorDataReference
    : public base::RefCounted<CursorDataReference> {
 public:
  explicit CursorDataReference(const ui::CursorData& data);

  const ui::CursorData& data() const { return data_; }

  // Instances of CursorDataReference are passed around as void* because
  // PlatformCursor resolves to void* on all ozone platforms. Even worse, there
  // can be multiple different types that map to this void* type. This asserts
  // that a magic cookie that we put at the start of valid CursorDataReference
  // objects is correct.
  void AssertIsACusrorDataReference();

 private:
  friend class base::RefCounted<CursorDataReference>;
  ~CursorDataReference();

  // This is always a magic constant value. This is set in the constructor and
  // unset in the destructor.
  uint32_t magic_cookie_;

  ui::CursorData data_;

  DISALLOW_COPY_AND_ASSIGN(CursorDataReference);
};

// CursorFactoryOzone "implementation" for processes which use ui::CursorDatas.
//
// Inside some sandboxed processes, we need to save all source data so it can
// be processed in a remote process. This plugs into the current ozone cursor
// creating code, and builds the cross platform mojo data structure.
class UI_BASE_EXPORT CursorDataFactory {
 public:
  // In a process, there should be a single instance, accessed by
  // GetInstance(). Its existence of non-existence should signal whether it is
  // necessary.
  CursorDataFactory();
  ~CursorDataFactory();

  // Returns the singleton instance.
  static CursorDataFactory* GetInstance();

  // Returns whether the singleton exists.
  static bool Exists();

  // Converts a PlatformCursor back to a ui::CursorData.
  static const ui::CursorData& GetCursorData(PlatformCursor platform_cursor);

  // Implementations of the CursorFactoryOzone interface without actually being
  // a subclass of CursorFactoryOzone so we don't trip the uniqueness check.
  PlatformCursor GetDefaultCursor(CursorType type);
  PlatformCursor CreateImageCursor(const SkBitmap& bitmap,
                                   const gfx::Point& hotspot,
                                   float bitmap_dpi);
  PlatformCursor CreateAnimatedCursor(const std::vector<SkBitmap>& bitmaps,
                                      const gfx::Point& hotspot,
                                      int frame_delay_ms,
                                      float bitmap_dpi);
  void RefImageCursor(PlatformCursor cursor);
  void UnrefImageCursor(PlatformCursor cursor);

 private:
  static CursorDataFactory* impl_;  // now owned

  // Get cached BitmapCursorOzone for a default cursor.
  scoped_refptr<CursorDataReference> GetDefaultCursorInternal(CursorType type);

  // Default cursors are cached & owned by the factory.
  using DefaultCursorMap =
      std::map<CursorType, scoped_refptr<CursorDataReference>>;
  DefaultCursorMap default_cursors_;

  DISALLOW_COPY_AND_ASSIGN(CursorDataFactory);
};

}  // namespace ui

#endif  // UI_BASE_CURSOR_CURSOR_DATA_FACTORY_H_
