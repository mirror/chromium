// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// For WinDDK ATL compatibility, these ATL headers must come first.
#include "build/build_config.h"

#if defined(OS_WIN)
#include <atlbase.h>  // NOLINT
#include <atlwin.h>  // NOLINT
#endif

#include <limits.h>

#include <algorithm>  // NOLINT
#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/views/omnibox/omnibox_result_view.h"

#include "base/feature_list.h"
#include "base/i18n/bidi_line_iterator.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_util.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/location_bar/background_with_1_px_border.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/classified_text_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_popup_contents_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_result_color.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/omnibox/browser/omnibox_popup_model.h"
#include "components/omnibox/browser/vector_icons.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/range/range.h"
#include "ui/gfx/render_text.h"
#include "ui/gfx/text_utils.h"
#include "ui/native_theme/native_theme.h"

using ui::NativeTheme;

namespace {

// The vertical margin that should be used above and below each suggestion.
static const int kVerticalMargin = 1;

// The vertical padding to provide each RenderText in addition to the height of
// the font. Where possible, RenderText uses this additional space to vertically
// center the cap height of the font instead of centering the entire font.
static const int kVerticalPadding = 4;

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// OmniboxResultView, public:

OmniboxResultView::OmniboxResultView(OmniboxPopupContentsView* model,
                                     int model_index,
                                     const gfx::FontList& font_list)
    : model_(model),
      model_index_(model_index),
      is_hovered_(false),
      font_list_(font_list),
      font_height_(std::max(
          font_list.GetHeight(),
          font_list.DeriveWithWeight(gfx::Font::Weight::BOLD).GetHeight())),
      keyword_icon_(new views::ImageView()),
      animation_(new gfx::SlideAnimation(this)) {
  CHECK_GE(model_index, 0);
  keyword_icon_->set_owned_by_client();
  keyword_icon_->EnableCanvasFlippingForRTLUI(true);
  keyword_icon_->SetImage(gfx::CreateVectorIcon(omnibox::kKeywordSearchIcon, 16,
                                                GetVectorIconColor()));
  keyword_icon_->SizeToPreferredSize();
}

OmniboxResultView::~OmniboxResultView() {
}

std::unique_ptr<ClassifiedTextView> OmniboxResultView::CreateClassifiedTextView(
    const base::string16& text,
    const ACMatchClassifications& classifications,
    OmniboxResultColor::HighlightState state) {
  std::unique_ptr<ClassifiedTextView> view;
  view = std::make_unique<ClassifiedTextView>(font_list_);
  CHECK(view);
  view->SetViewState(state);
  view->SetText(text, classifications, /*force_dim=*/false);
  // view->set_owned_by_client();
  view->SizeToPreferredSize();
  AddChildView(view.get());
  return view;
}

std::unique_ptr<ClassifiedTextView> OmniboxResultView::CreateClassifiedTextView(
    const SuggestionAnswer::ImageLine& line,
    OmniboxResultColor::HighlightState state) {
  std::unique_ptr<ClassifiedTextView> view;
  view = std::make_unique<ClassifiedTextView>(font_list_);
  CHECK(view);
  view->SetViewState(state);
  view->SetText(line);
  // view->set_owned_by_client();
  view->SizeToPreferredSize();
  AddChildView(view.get());
  return view;
}

void OmniboxResultView::SetMatch(const AutocompleteMatch& match) {
  match_ = match;
  match_.PossiblySwapContentsAndDescriptionForDisplay();
  animation_->Reset();
  answer_image_ = gfx::ImageSkia();
  is_hovered_ = false;
  Invalidate();

  if (GetWidget())
    Layout();
}

void OmniboxResultView::ShowKeyword(bool show_keyword) {
  if (show_keyword)
    animation_->Show();
  else
    animation_->Hide();
}

void OmniboxResultView::Invalidate() {
  const OmniboxResultColor::HighlightState state = GetState();
  if (state == OmniboxResultColor::NORMAL) {
    SetBackground(nullptr);
  } else {
    const SkColor bg_color = OmniboxResultColor::GetColor(
        state, OmniboxResultColor::BACKGROUND, this);
    SetBackground(
        std::make_unique<BackgroundWith1PxBorder>(bg_color, bg_color));
  }

  // While the text in the RenderTexts may not have changed, the styling
  // (color/bold) may need to change.
  content_view_.reset();
  description_view_.reset();
  keyword_content_view_.reset();
  keyword_description_view_.reset();
  separator_rendertext_.reset();

  if (!separator_rendertext_) {
    const base::string16& separator =
        l10n_util::GetStringUTF16(IDS_AUTOCOMPLETE_MATCH_DESCRIPTION_SEPARATOR);
    separator_rendertext_ = CreateRenderText(separator);
    separator_width_ = separator_rendertext_->GetContentWidth();
    separator_rendertext_->SetColor(OmniboxResultColor::GetColor(
        GetState(), OmniboxResultColor::DIMMED_TEXT, this));
  }

  if (!ShowOnlyKeywordMatch()) {
    if (match_.answer) {
      content_view_ =
          CreateClassifiedTextView(match_.answer->first_line(), GetState());
      description_view_ =
          CreateClassifiedTextView(match_.answer->second_line(), GetState());
    } else {
      content_view_ = CreateClassifiedTextView(
          match_.contents, match_.contents_class, GetState());
      description_view_ = CreateClassifiedTextView(
          match_.description, match_.description_class, GetState());
    }
  }

  AutocompleteMatch* keyword_match = match_.associated_keyword.get();
  if (keyword_match) {
    keyword_content_view_ = CreateClassifiedTextView(
        keyword_match->contents, keyword_match->contents_class, GetState());
    keyword_description_view_ =
        CreateClassifiedTextView(keyword_match->description,
                                 keyword_match->description_class, GetState());
  }

  Layout();
}

void OmniboxResultView::OnSelected() {
  DCHECK_EQ(OmniboxResultColor::SELECTED, GetState());

  // The text is also accessible via text/value change events in the omnibox but
  // this selection event allows the screen reader to get more details about the
  // list and the user's position within it.
  NotifyAccessibilityEvent(ax::mojom::Event::kSelection, true);
}

OmniboxResultColor::HighlightState OmniboxResultView::GetState() const {
  if (model_->IsSelectedIndex(model_index_))
    return OmniboxResultColor::SELECTED;
  return is_hovered_ ? OmniboxResultColor::HOVERED : OmniboxResultColor::NORMAL;
}

void OmniboxResultView::OnMatchIconUpdated() {
  // The new icon will be fetched during repaint.
  Invalidate();  //???????????????????????/
  SchedulePaint();
}

void OmniboxResultView::SetAnswerImage(const gfx::ImageSkia& image) {
  answer_image_ = image;
  // Layout();  //???????????????????????/
  Invalidate();  //???????????????????????/
  SchedulePaint();
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxResultView, views::View overrides:

bool OmniboxResultView::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton())
    model_->SetSelectedLine(model_index_);
  return true;
}

bool OmniboxResultView::OnMouseDragged(const ui::MouseEvent& event) {
  if (HitTestPoint(event.location())) {
    // When the drag enters or remains within the bounds of this view, either
    // set the state to be selected or hovered, depending on the mouse button.
    if (event.IsOnlyLeftMouseButton()) {
      if (GetState() != OmniboxResultColor::SELECTED)
        model_->SetSelectedLine(model_index_);
    } else {
      SetHovered(true);
    }
    return true;
  }

  // When the drag leaves the bounds of this view, cancel the hover state and
  // pass control to the popup view.
  SetHovered(false);
  SetMouseHandler(model_);
  return false;
}

void OmniboxResultView::OnMouseReleased(const ui::MouseEvent& event) {
  if (event.IsOnlyMiddleMouseButton() || event.IsOnlyLeftMouseButton()) {
    model_->OpenMatch(model_index_,
                      event.IsOnlyLeftMouseButton()
                          ? WindowOpenDisposition::CURRENT_TAB
                          : WindowOpenDisposition::NEW_BACKGROUND_TAB);
  }
}

void OmniboxResultView::OnMouseMoved(const ui::MouseEvent& event) {
  SetHovered(true);
}

void OmniboxResultView::OnMouseExited(const ui::MouseEvent& event) {
  SetHovered(false);
}

void OmniboxResultView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  // Get the label without the ", n of m" positional text appended.
  // The positional info is provided via
  // ax::mojom::IntAttribute::kPosInSet/SET_SIZE and providing it via text as
  // well would result in duplicate announcements.
  node_data->SetName(
      AutocompleteMatchType::ToAccessibilityLabel(match_, match_.contents));

  node_data->role = ax::mojom::Role::kListBoxOption;
  node_data->AddIntAttribute(ax::mojom::IntAttribute::kPosInSet,
                             model_index_ + 1);
  node_data->AddIntAttribute(ax::mojom::IntAttribute::kSetSize,
                             model_->child_count());

  node_data->AddState(ax::mojom::State::kSelectable);
  switch (GetState()) {
    case OmniboxResultColor::SELECTED:
      node_data->AddState(ax::mojom::State::kSelected);
      break;
    case OmniboxResultColor::HOVERED:
      node_data->AddState(ax::mojom::State::kHovered);
      break;
    default:
      break;
  }
}

gfx::Size OmniboxResultView::CalculatePreferredSize() const {
  return gfx::Size(0, outer_height_);
}

void OmniboxResultView::OnNativeThemeChanged(const ui::NativeTheme* theme) {
  Invalidate();
  SchedulePaint();
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxResultView, private:

int OmniboxResultView::GetTextHeight() const {
  return font_height_ + kVerticalPadding;
}

std::unique_ptr<gfx::RenderText> OmniboxResultView::CreateRenderText(
    const base::string16& text) const {
  // used for separator CHECK(false);
  auto render_text = gfx::RenderText::CreateHarfBuzzInstance();
  render_text->SetDisplayRect(gfx::Rect(gfx::Size(INT_MAX, 0)));
  render_text->SetCursorEnabled(false);
  render_text->SetElideBehavior(gfx::ELIDE_TAIL);
  render_text->SetFontList(font_list_);
  render_text->SetText(text);
  return render_text;
}

gfx::Image OmniboxResultView::GetIcon() const {
  return model_->GetMatchIcon(match_, GetVectorIconColor());
}

SkColor OmniboxResultView::GetVectorIconColor() const {
  // For selected rows, paint the icon the same color as the text.
  SkColor color =
      OmniboxResultColor::GetColor(GetState(), OmniboxResultColor::TEXT, this);
  if (GetState() != OmniboxResultColor::SELECTED)
    color = color_utils::DeriveDefaultIconColor(color);
  return color;
}

bool OmniboxResultView::ShowOnlyKeywordMatch() const {
  return match_.associated_keyword &&
      (keyword_icon_->x() <= icon_bounds_.right());
}

int OmniboxResultView::GetVerticalMargin() const {
  // Regardless of the text size, we ensure a minimum size for the content line
  // here. This minimum is larger for hybrid mouse/touch devices to ensure an
  // adequately sized touch target.
  using Md = ui::MaterialDesignController;
  const int kIconVerticalPad = base::GetFieldTrialParamByFeatureAsInt(
      omnibox::kUIExperimentVerticalMargin,
      OmniboxFieldTrial::kUIVerticalMarginParam,
      Md::GetMode() == Md::MATERIAL_HYBRID ? 8 : 4);
  const int min_height = LocationBarView::kIconWidth + 2 * kIconVerticalPad;

  return std::max(kVerticalMargin, (min_height - GetTextHeight()) / 2);
}

void OmniboxResultView::SetHovered(bool hovered) {
  if (is_hovered_ != hovered) {
    is_hovered_ = hovered;
    Invalidate();
    SchedulePaint();
  }
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxResultView, views::View overrides, private:

void OmniboxResultView::Layout() {
  const int horizontal_padding =
      GetLayoutConstant(LOCATION_BAR_ELEMENT_PADDING) +
      LocationBarView::kIconInteriorPadding;
  // The horizontal bounds we're given are the outside bounds, so we can match
  // the omnibox border outline shape exactly in OnPaint().  We have to inset
  // here to keep the icons lined up.
  const int start_x = BackgroundWith1PxBorder::kLocationBarBorderThicknessDip +
                      horizontal_padding;
  int end_x = width() - start_x;

  int row_height = GetTextHeight();
  int icon_height = row_height;
  if (base::FeatureList::IsEnabled(omnibox::kUIExperimentVerticalLayout))
    icon_height += GetTextHeight();

  const gfx::Image icon = GetIcon();
  const int icon_y = GetVerticalMargin() + (icon_height - icon.Height()) / 2;
  icon_bounds_.SetRect(start_x, icon_y, icon.Width(), icon.Height());

  // TODO(dschuyler): Perhaps this should be based on the font size
  // instead of hardcoded to 2 dp (e.g. by adding a space in an
  // appropriate font to the beginning of the description, then reducing
  // the additional padding here to zero).
  const int kAnswerIconToTextPadding = 2;

  AutocompleteMatch* keyword_match = match_.associated_keyword.get();
  if (keyword_match) {
    if (!keyword_icon_->parent()) {
      AddChildView(keyword_icon_.get());
    }

    if (match_.associated_keyword.get()) {
      const int max_kw_x = end_x - keyword_icon_->width();
      int kw_x = animation_->CurrentValueBetween(max_kw_x, start_x);
      end_x = kw_x;
      int y = GetVerticalMargin();
      kw_x += BackgroundWith1PxBorder::kLocationBarBorderThicknessDip;
      keyword_icon_->SetPosition(gfx::Point(
          GetMirroredXInView(kw_x), (height() - keyword_icon_->height()) / 2));
      kw_x += keyword_icon_->width() + horizontal_padding;

      int first_width = keyword_content_view_->GetContentsBounds().width();
      int second_width =
          keyword_description_view_
              ? keyword_description_view_->GetContentsBounds().width()
              : 0;
      OmniboxPopupModel::ComputeMatchMaxWidths(
          first_width, separator_width_, second_width, width(),
          /*description_on_separate_line=*/false,
          !AutocompleteMatch::IsSearchType(match_.type), &first_width,
          &second_width);
      keyword_content_view_->SetBounds(GetMirroredXInView(kw_x), y, first_width,
                                       row_height);
      if (second_width != 0) {
        kw_x += keyword_content_view_->width();
        keyword_description_view_->SetBounds(GetMirroredXInView(kw_x), y,
                                             second_width, row_height);
      } else if (base::FeatureList::IsEnabled(
                     omnibox::kUIExperimentVerticalLayout)) {
        keyword_content_view_->SetSize(gfx::Size(first_width, row_height * 2));
      }
    }
  } else {
    if (keyword_icon_->parent()) {
      RemoveChildView(keyword_icon_.get());
    }
  }

  if (content_view_ && description_view_) {
    int x = start_x;
    x += icon.Width() + horizontal_padding;
    int y = GetVerticalMargin();
    // NOTE: While animating the keyword match, both matches may be visible.
    if (!ShowOnlyKeywordMatch()) {
      if (match_.answer) {
        content_view_->SetBounds(GetMirroredXInView(x), y, end_x - x,
                                 row_height);
        int answer_icon_size = 0;
        if (!answer_image_.isNull()) {
          // GetAnswerHeight includes some padding. Using that results in an
          // image that's too large so we use the font height here instead.
          answer_icon_size = description_view_->GetFontHeight();
          answer_icon_bounds_.SetRect(GetMirroredXInView(x),
                                      y + (kVerticalPadding / 2) + row_height,
                                      answer_icon_size, answer_icon_size);
          answer_icon_size += kAnswerIconToTextPadding;
        }
        x += answer_icon_size;
        y += GetTextHeight();
        int description_width = end_x - x;
        description_view_->SetBounds(
            GetMirroredXInView(x), y, description_width,
            description_view_->GetHeightForWidth(description_width) +
                kVerticalPadding);
        outer_height_ = content_view_->height() + description_view_->height() +
                        GetVerticalMargin() * 2;
      } else if (base::FeatureList::IsEnabled(
                     omnibox::kUIExperimentVerticalLayout)) {
        bool is_two_lines = !!description_view_->GetContentsBounds().width();
        int content_height =
            is_two_lines ? GetTextHeight() : GetTextHeight() * 2;
        content_view_->SetBounds(GetMirroredXInView(x), y, end_x - x,
                                 content_height);
        outer_height_ = content_height + GetVerticalMargin() * 2;
        if (is_two_lines) {
          y += GetTextHeight();
          int description_width = end_x - x;
          description_view_->SetBounds(
              GetMirroredXInView(x), y, description_width,
              description_view_->GetHeightForWidth(description_width) +
                  kVerticalPadding);
          outer_height_ += description_view_->height();
        }
      } else {
        int first_width = content_view_->GetContentsBounds().width();
        int second_width = description_view_
                               ? description_view_->GetContentsBounds().width()
                               : 0;
        OmniboxPopupModel::ComputeMatchMaxWidths(
            first_width, separator_width_, second_width, end_x - x,
            /*description_on_separate_line=*/false,
            !AutocompleteMatch::IsSearchType(match_.type), &first_width,
            &second_width);
        ClassifiedTextView* first_view = content_view_.get();
        ClassifiedTextView* second_view = description_view_.get();
        first_view->SetBounds(x, y, first_width, row_height);
        x += first_width;
        if (second_width != 0) {
          separator_rendertext_->SetDisplayRect(gfx::Rect(
              GetMirroredXInView(x), y, separator_width_, row_height));
          x += separator_width_;
        }
        second_view->SetBounds(x, y, second_width, row_height);

        // Determine the outer height for use in CalculatePreferredSize.
        outer_height_ = first_view->height() + GetVerticalMargin() * 2;
      }
    }
  }
}

const char* OmniboxResultView::GetClassName() const {
  return "OmniboxResultView";
}

void OmniboxResultView::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  animation_->SetSlideDuration(width() / 4);
}

void OmniboxResultView::OnPaint(gfx::Canvas* canvas) {
  View::OnPaint(canvas);
  if (!ShowOnlyKeywordMatch()) {
    canvas->DrawImageInt(GetIcon().AsImageSkia(),
                         GetMirroredXForRect(icon_bounds_), icon_bounds_.y());
    if (separator_rendertext_) {
      separator_rendertext_->Draw(canvas);
    }
    if (!answer_image_.isNull()) {
      canvas->DrawImageInt(answer_image_, 0, 0, answer_image_.width(),
                           answer_image_.height(), answer_icon_bounds_.x(),
                           answer_icon_bounds_.y(), answer_icon_bounds_.width(),
                           answer_icon_bounds_.height(), true);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxResultView, gfx::AnimationProgressed overrides, private:

void OmniboxResultView::AnimationProgressed(const gfx::Animation* animation) {
  Layout();
  SchedulePaint();
}
