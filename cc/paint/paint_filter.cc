// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_filter.h"

namespace cc {

PaintFilter::PaintFilter(sk_sp<PaintRecord> record, SkRect bounds)
    : skia::SkImageFilterBase(bounds), record_(std::move(record)) {
  DCHECK(record_);
}

PaintFilter::PaintFilter() : skia::SkImageFilterBase(SkRect()) {}

PaintFilter::~PaintFilter() = default;

sk_sp<SkFlattenable> PaintFilter::CreateProc(SkReadBuffer& buffer) {
  auto filter = sk_sp<PaintFilter>(new PaintFilter());
  if (filter->InitFromBuffer(buffer))
    return filter;
  return nullptr;
}

void PaintFilter::Draw(SkCanvas* canvas) const {
  record_->Playback(canvas);
}

sk_sp<SkData> PaintFilter::Serialize(void* ctx) const {
  // TODO(khushalsagar): Serialize the record.
  return nullptr;
}

bool PaintFilter::Deserialize(const SkData* data, void* ctx) {
  // TODO(khushalsagar): Deserialize the record.
  return false;
}

sk_sp<skia::SkImageFilterBase> PaintFilter::Clone() const {
  return sk_make_sp<PaintFilter>(record_, bounds());
}

#ifndef SK_IGNORE_TO_STRING
void PaintFilter::toString(SkString* str) const {
  str->appendf("PaintFilter");
}
#endif

SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_START(PaintFilter)
SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(PaintFilter)
SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_END

}  // namespace cc
