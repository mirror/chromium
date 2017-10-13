/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TextMetrics_h
#define TextMetrics_h

#include "core/CoreExport.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/fonts/Font.h"
#include "platform/graphics/GraphicsTypes.h"
#include "platform/heap/Handle.h"

namespace blink {

class CORE_EXPORT TextMetrics final : public GarbageCollected<TextMetrics>,
                                      public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static TextMetrics* Create() { return new TextMetrics; }

  static TextMetrics* Create(const Font& font,
                             const TextDirection& direction,
                             const TextBaseline& baseline,
                             const TextAlign& align,
                             const String& text) {
    TextMetrics* metric = new TextMetrics();
    metric->Update(font, direction, baseline, align, text);
    return metric;
  }

  float width() const { return width_; }
  float actualBoundingBoxLeft() const { return actual_bounding_box_left_; }
  float actualBoundingBoxRight() const { return actual_bounding_box_right_; }
  float fontBoundingBoxAscent() const { return font_bounding_box_ascent_; }
  float fontBoundingBoxDescent() const { return font_bounding_box_descent_; }
  float actualBoundingBoxAscent() const { return actual_bounding_box_ascent_; }
  float actualBoundingBoxDescent() const {
    return actual_bounding_box_descent_;
  }
  float emHeightAscent() const { return em_height_ascent_; }
  float emHeightDescent() const { return em_height_descent_; }
  float hangingBaseline() const { return hanging_baseline_; }
  float alphabeticBaseline() const { return alphabetic_baseline_; }
  float ideographicBaseline() const { return ideographic_baseline_; }

  static float GetFontBaseline(const TextBaseline&, const FontMetrics&);

  DEFINE_INLINE_TRACE() {}

 private:
  void Update(const Font&,
              const TextDirection&,
              const TextBaseline&,
              const TextAlign&,
              const String&);
  TextMetrics() {}

  // x-direction
  float width_ = 0.0;
  float actual_bounding_box_left_ = 0.0;
  float actual_bounding_box_right_ = 0.0;

  // y-direction
  float font_bounding_box_ascent_ = 0.0;
  float font_bounding_box_descent_ = 0.0;
  float actual_bounding_box_ascent_ = 0.0;
  float actual_bounding_box_descent_ = 0.0;
  float em_height_ascent_ = 0.0;
  float em_height_descent_ = 0.0;
  float hanging_baseline_ = 0.0;
  float alphabetic_baseline_ = 0.0;
  float ideographic_baseline_ = 0.0;
};

}  // namespace blink

#endif  // TextMetrics_h
