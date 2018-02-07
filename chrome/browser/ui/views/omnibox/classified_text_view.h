// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_OMNIBOX_CLASSIFIED_TEXT_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_OMNIBOX_CLASSIFIED_TEXT_VIEW_H_

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/views/omnibox/omnibox_result_view.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/suggestion_answer.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/view.h"

namespace gfx {
class Canvas;
class RenderText;
}  // namespace gfx

class ClassifiedTextView : public views::View {
 public:
  explicit ClassifiedTextView(const gfx::FontList& font_list);
  ~ClassifiedTextView() override;

  // Creates a RenderText with default rendering for the given |text|. The
  // |classifications| and |force_dim| are used to style the text.
  void SetText(const base::string16& text,
               const ACMatchClassifications& classifications,
               bool force_dim);

  void SetText(const SuggestionAnswer::ImageLine& line);

  void SetViewState(OmniboxResultColor::HighlightState state);

  gfx::Size CalculatePreferredSize() const override;

  void Layout() override;

  void SetFontList(const gfx::FontList& font_list);

  void OnPaint(gfx::Canvas* canvas) override;

  int GetFontHeight() const;

 private:
  const gfx::FontList& GetFontForType(int text_type) const;

  std::unique_ptr<gfx::RenderText> CreateRenderText(
      const base::string16& text) const;

  std::unique_ptr<gfx::RenderText> CreateClassifiedRenderText(
      const base::string16& text,
      const ACMatchClassifications& classifications,
      bool force_dim) const;

  // Creates a RenderText with text and styling from the image line.
  std::unique_ptr<gfx::RenderText> CreateText(
      const SuggestionAnswer::ImageLine& line,
      const gfx::FontList& font_list) const;

  // Adds |text| to |destination|.  |text_type| is an index into the
  // kTextStyles constant defined in the .cc file and is used to style the text,
  // including setting the font size, color, and baseline style.  See the
  // TextStyle struct in the .cc file for more.
  void AppendText(gfx::RenderText* destination,
                  const base::string16& text,
                  int text_type) const;

  // AppendText will break up the |text| into bold and non-bold pieces
  // and pass each to this helper with the correct |is_bold| value.
  void AppendTextHelper(gfx::RenderText* destination,
                        const base::string16& text,
                        int text_type,
                        bool is_bold) const;

  // Font settings for this view.
  gfx::FontList font_list_;
  mutable int font_height_;

  mutable std::unique_ptr<gfx::RenderText> render_text_;
  OmniboxResultColor::HighlightState view_state_;

  DISALLOW_COPY_AND_ASSIGN(ClassifiedTextView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OMNIBOX_CLASSIFIED_TEXT_VIEW_H_
