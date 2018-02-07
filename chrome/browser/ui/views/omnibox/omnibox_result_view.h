// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_RESULT_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_RESULT_VIEW_H_

#include <stddef.h>
#include <memory>
#include <utility>

#include "base/macros.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/suggestion_answer.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/view.h"

#include "chrome/browser/ui/views/omnibox/omnibox_result_color.h"

class OmniboxPopupContentsView;

namespace gfx {
class Canvas;
class Image;
class RenderText;
}

class ClassifiedTextView;

class OmniboxResultView : public views::View,
                          private gfx::AnimationDelegate {
 public:
  OmniboxResultView(OmniboxPopupContentsView* model,
                    int model_index,
                    const gfx::FontList& font_list);
  ~OmniboxResultView() override;

  // Updates the match used to paint the contents of this result view. We copy
  // the match so that we can continue to paint the last result even after the
  // model has changed.
  void SetMatch(const AutocompleteMatch& match);

  void ShowKeyword(bool show_keyword);

  void Invalidate();

  // Invoked when this result view has been selected.
  void OnSelected();

  OmniboxResultColor::HighlightState GetState() const;

  // Notification that the match icon has changed and schedules a repaint.
  void OnMatchIconUpdated();

  // Stores the image in a local data member and schedules a repaint.
  void SetAnswerImage(const gfx::ImageSkia& image);

  // views::View:
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  gfx::Size CalculatePreferredSize() const override;
  void OnNativeThemeChanged(const ui::NativeTheme* theme) override;

 private:
  // Returns the height of the text portion of the result view.
  int GetTextHeight() const;

  // Creates a RenderText with given |text| and rendering defaults.
  std::unique_ptr<gfx::RenderText> CreateRenderText(
      const base::string16& text) const;

  gfx::Image GetIcon() const;

  SkColor GetVectorIconColor() const;

  // Whether to render only the keyword match.  Returns true if |match_| has an
  // associated keyword match that has been animated so close to the start that
  // the keyword match will hide even the icon of the regular match.
  bool ShowOnlyKeywordMatch() const;

  // Returns the margin that should appear at the top and bottom of the result.
  int GetVerticalMargin() const;

  // Sets the hovered state of this result.
  void SetHovered(bool hovered);

  // views::View:
  void Layout() override;
  const char* GetClassName() const override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  void OnPaint(gfx::Canvas* canvas) override;

  // gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override;

  std::unique_ptr<ClassifiedTextView> CreateClassifiedTextView(
      const base::string16& text,
      const ACMatchClassifications& classifications,
      OmniboxResultColor::HighlightState state);

  std::unique_ptr<ClassifiedTextView> CreateClassifiedTextView(
      const SuggestionAnswer::ImageLine& line,
      OmniboxResultColor::HighlightState state);

  // This row's model and model index.
  OmniboxPopupContentsView* model_;
  size_t model_index_;

  // Whether this view is in the hovered state.
  bool is_hovered_;

  // Font settings for this view.
  const gfx::FontList font_list_;
  int font_height_;

#if 0
  // A context used for mirroring regions.
  class MirroringContext;
  std::unique_ptr<MirroringContext> mirroring_context_;
#endif

  AutocompleteMatch match_;

  gfx::Rect answer_icon_bounds_;
  gfx::Rect icon_bounds_;

  gfx::Rect keyword_text_bounds_;
  std::unique_ptr<views::ImageView> keyword_icon_;

  std::unique_ptr<gfx::SlideAnimation> animation_;

  // If the answer has an icon, cache the image.
  gfx::ImageSkia answer_image_;

  mutable std::unique_ptr<ClassifiedTextView> content_view_;
  mutable std::unique_ptr<ClassifiedTextView> description_view_;
  mutable std::unique_ptr<ClassifiedTextView> separator_view_;
  mutable std::unique_ptr<ClassifiedTextView> keyword_content_view_;
  mutable std::unique_ptr<ClassifiedTextView> keyword_description_view_;

  // We preserve these RenderTexts so that we won't recreate them on every call
  // to OnPaint().
  mutable std::unique_ptr<gfx::RenderText> separator_rendertext_;

  mutable int separator_width_;

  int outer_height_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxResultView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_RESULT_VIEW_H_
