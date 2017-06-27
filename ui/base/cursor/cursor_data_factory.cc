// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/cursor/cursor_data_factory.h"

#include "ui/base/cursor/cursor.h"

namespace ui {

namespace {

// A magic value that we store at the start of an instance.
const uint32_t kCookie = 0xF60D214C;

const uint32_t kBadCookie = 0xBADBADCC;

CursorDataReference* ToCursorDataReference(PlatformCursor cursor) {
  CursorDataReference* ref = static_cast<CursorDataReference*>(cursor);
#if DCHECK_IS_ON()
  ref->AssertIsACusrorDataReference();
#endif
  return ref;
}

PlatformCursor ToPlatformCursor(CursorDataReference* cursor) {
  return static_cast<PlatformCursor>(cursor);
}

}  // namespace

CursorDataReference::CursorDataReference(const ui::CursorData& data)
    : magic_cookie_(kCookie), data_(data) {}

void CursorDataReference::AssertIsACusrorDataReference() {
  CHECK_EQ(magic_cookie_, kCookie);
}

CursorDataReference::~CursorDataReference() {
  magic_cookie_ = kBadCookie;
}

CursorDataFactory* CursorDataFactory::impl_ = nullptr;

CursorDataFactory::CursorDataFactory() {
  DCHECK(!impl_) << "There should only be a single CursorFactoryOzone.";
  impl_ = this;
}

CursorDataFactory::~CursorDataFactory() {
  DCHECK_EQ(impl_, this);
  impl_ = NULL;
}

// static
CursorDataFactory* CursorDataFactory::GetInstance() {
  DCHECK(impl_) << "No CursorDataFactory implementation set.";
  return impl_;
}

// static
bool CursorDataFactory::Exists() {
  return impl_;
}

// static
const ui::CursorData& CursorDataFactory::GetCursorData(
    PlatformCursor platform_cursor) {
  return ToCursorDataReference(platform_cursor)->data();
}

PlatformCursor CursorDataFactory::GetDefaultCursor(CursorType type) {
  // Unlike BitmapCursorFactoryOzone, we aren't making heavyweight bitmaps, but
  // we still have to cache these forever because objects that come out of the
  // GetDefaultCursor() method aren't treated as refcounted by the ozone
  // interfaces.
  return GetDefaultCursorInternal(type).get();
}

PlatformCursor CursorDataFactory::CreateImageCursor(const SkBitmap& bitmap,
                                                    const gfx::Point& hotspot,
                                                    float bitmap_dpi) {
  CursorDataReference* cursor = new CursorDataReference(
      ui::CursorData(hotspot, {bitmap}, bitmap_dpi, base::TimeDelta()));
  cursor->AddRef();  // Balanced by UnrefImageCursor.
  return ToPlatformCursor(cursor);
}

PlatformCursor CursorDataFactory::CreateAnimatedCursor(
    const std::vector<SkBitmap>& bitmaps,
    const gfx::Point& hotspot,
    int frame_delay_ms,
    float bitmap_dpi) {
  CursorDataReference* cursor = new CursorDataReference(
      ui::CursorData(hotspot, bitmaps, bitmap_dpi,
                     base::TimeDelta::FromMilliseconds(frame_delay_ms)));
  cursor->AddRef();  // Balanced by UnrefImageCursor.
  return ToPlatformCursor(cursor);
}

void CursorDataFactory::RefImageCursor(PlatformCursor cursor) {
  ToCursorDataReference(cursor)->AddRef();
}

void CursorDataFactory::UnrefImageCursor(PlatformCursor cursor) {
  ToCursorDataReference(cursor)->Release();
}

scoped_refptr<CursorDataReference> CursorDataFactory::GetDefaultCursorInternal(
    CursorType type) {
  if (type == CursorType::kNone)
    return nullptr;  // nullptr is used for hidden cursor.

  if (!default_cursors_.count(type)) {
    // We hold a ref forever because clients do not do refcounting for default
    // cursors.
    scoped_refptr<CursorDataReference> cursor =
        make_scoped_refptr(new CursorDataReference(ui::CursorData(type)));
    default_cursors_[type] = std::move(cursor);
  }

  // Returned owned default cursor for this type.
  return default_cursors_[type];
}

}  // namespace ui
