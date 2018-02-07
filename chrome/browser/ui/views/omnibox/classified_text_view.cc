// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// For WinDDK ATL compatibility, these ATL headers must come first.
#include "build/build_config.h"

#if defined(OS_WIN)
#include <atlbase.h>  // NOLINT
#include <atlwin.h>   // NOLINT
#endif

#include <limits.h>

#include <algorithm>  // NOLINT
#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/views/omnibox/classified_text_view.h"

#include "base/feature_list.h"
#include "base/i18n/bidi_line_iterator.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_util.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/location_bar/background_with_1_px_border.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_popup_contents_view.h"
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

// A mapping from ClassifiedTextView's ViewState/ColorKind types to
// NativeTheme colors.
struct TranslationTable {
  ui::NativeTheme::ColorId id;
  OmniboxResultView::ResultViewState state;
  ClassifiedTextView::ColorKind kind;
} static const kTranslationTable[] = {
    {NativeTheme::kColorId_ResultsTableNormalBackground,
     OmniboxResultView::ResultViewState::NORMAL,
     ClassifiedTextView::BACKGROUND},
    {NativeTheme::kColorId_ResultsTableHoveredBackground,
     OmniboxResultView::ResultViewState::HOVERED,
     ClassifiedTextView::BACKGROUND},
    {NativeTheme::kColorId_ResultsTableSelectedBackground,
     OmniboxResultView::ResultViewState::SELECTED,
     ClassifiedTextView::BACKGROUND},
    {NativeTheme::kColorId_ResultsTableNormalText,
     OmniboxResultView::ResultViewState::NORMAL, ClassifiedTextView::TEXT},
    {NativeTheme::kColorId_ResultsTableHoveredText,
     OmniboxResultView::ResultViewState::HOVERED, ClassifiedTextView::TEXT},
    {NativeTheme::kColorId_ResultsTableSelectedText,
     OmniboxResultView::ResultViewState::SELECTED, ClassifiedTextView::TEXT},
    {NativeTheme::kColorId_ResultsTableNormalDimmedText,
     OmniboxResultView::ResultViewState::NORMAL,
     ClassifiedTextView::DIMMED_TEXT},
    {NativeTheme::kColorId_ResultsTableHoveredDimmedText,
     OmniboxResultView::ResultViewState::HOVERED,
     ClassifiedTextView::DIMMED_TEXT},
    {NativeTheme::kColorId_ResultsTableSelectedDimmedText,
     OmniboxResultView::ResultViewState::SELECTED,
     ClassifiedTextView::DIMMED_TEXT},
    {NativeTheme::kColorId_ResultsTableNormalUrl,
     OmniboxResultView::ResultViewState::NORMAL, ClassifiedTextView::URL},
    {NativeTheme::kColorId_ResultsTableHoveredUrl,
     OmniboxResultView::ResultViewState::HOVERED, ClassifiedTextView::URL},
    {NativeTheme::kColorId_ResultsTableSelectedUrl,
     OmniboxResultView::ResultViewState::SELECTED, ClassifiedTextView::URL},
};

struct TextStyle {
  ui::ResourceBundle::FontStyle font;
  ui::NativeTheme::ColorId
      colors[OmniboxResultView::ResultViewState::NUM_STATES];
  gfx::BaselineStyle baseline;
};
#if 10
// Returns the styles that should be applied to the specified answer text type.
//
// Note that the font value is only consulted for the first text type that
// appears on an answer line, because RenderText does not yet support multiple
// font sizes. Subsequent text types on the same line will share the text size
// of the first type, while the color and baseline styles specified here will
// always apply. The gfx::INFERIOR baseline style is used as a workaround to
// produce smaller text on the same line. The way this is used in the current
// set of answers is that the small types (TOP_ALIGNED, DESCRIPTION_NEGATIVE,
// DESCRIPTION_POSITIVE and SUGGESTION_SECONDARY_TEXT_SMALL) only ever appear
// following LargeFont text, so for consistency they specify LargeFont for the
// first value even though this is not actually used (since they're not the
// first value).
TextStyle GetTextStyle(int type) {
  switch (type) {
    case SuggestionAnswer::TOP_ALIGNED:
      return {ui::ResourceBundle::LargeFont,
              {NativeTheme::kColorId_ResultsTableNormalDimmedText,
               NativeTheme::kColorId_ResultsTableHoveredDimmedText,
               NativeTheme::kColorId_ResultsTableSelectedDimmedText},
              gfx::SUPERIOR};
    case SuggestionAnswer::DESCRIPTION_NEGATIVE:
      return {ui::ResourceBundle::LargeFont,
              {NativeTheme::kColorId_ResultsTableNegativeText,
               NativeTheme::kColorId_ResultsTableNegativeHoveredText,
               NativeTheme::kColorId_ResultsTableNegativeSelectedText},
              gfx::INFERIOR};
    case SuggestionAnswer::DESCRIPTION_POSITIVE:
      return {ui::ResourceBundle::LargeFont,
              {NativeTheme::kColorId_ResultsTablePositiveText,
               NativeTheme::kColorId_ResultsTablePositiveHoveredText,
               NativeTheme::kColorId_ResultsTablePositiveSelectedText},
              gfx::INFERIOR};
    case SuggestionAnswer::PERSONALIZED_SUGGESTION:
      return {ui::ResourceBundle::BaseFont,
              {NativeTheme::kColorId_ResultsTableNormalText,
               NativeTheme::kColorId_ResultsTableHoveredText,
               NativeTheme::kColorId_ResultsTableSelectedText},
              gfx::NORMAL_BASELINE};
    case SuggestionAnswer::ANSWER_TEXT_MEDIUM:
      return {ui::ResourceBundle::BaseFont,
              {NativeTheme::kColorId_ResultsTableNormalDimmedText,
               NativeTheme::kColorId_ResultsTableHoveredDimmedText,
               NativeTheme::kColorId_ResultsTableSelectedDimmedText},
              gfx::NORMAL_BASELINE};
    case SuggestionAnswer::ANSWER_TEXT_LARGE:
      return {ui::ResourceBundle::LargeFont,
              {NativeTheme::kColorId_ResultsTableNormalDimmedText,
               NativeTheme::kColorId_ResultsTableHoveredDimmedText,
               NativeTheme::kColorId_ResultsTableSelectedDimmedText},
              gfx::NORMAL_BASELINE};
    case SuggestionAnswer::SUGGESTION_SECONDARY_TEXT_SMALL:
      return {ui::ResourceBundle::LargeFont,
              {NativeTheme::kColorId_ResultsTableNormalDimmedText,
               NativeTheme::kColorId_ResultsTableHoveredDimmedText,
               NativeTheme::kColorId_ResultsTableSelectedDimmedText},
              gfx::INFERIOR};
    case SuggestionAnswer::SUGGESTION_SECONDARY_TEXT_MEDIUM:
      return {ui::ResourceBundle::BaseFont,
              {NativeTheme::kColorId_ResultsTableNormalDimmedText,
               NativeTheme::kColorId_ResultsTableHoveredDimmedText,
               NativeTheme::kColorId_ResultsTableSelectedDimmedText},
              gfx::NORMAL_BASELINE};
    case SuggestionAnswer::SUGGESTION:  // Fall through.
    default:
      return {ui::ResourceBundle::BaseFont,
              {NativeTheme::kColorId_ResultsTableNormalText,
               NativeTheme::kColorId_ResultsTableHoveredText,
               NativeTheme::kColorId_ResultsTableSelectedText},
              gfx::NORMAL_BASELINE};
  }
}
#endif
}  // namespace

ClassifiedTextView::ClassifiedTextView(const gfx::FontList& font_list)
    : font_list_(font_list),
      view_state_(OmniboxResultView::ResultViewState::NORMAL) {
  // CHECK(false);
}

ClassifiedTextView::~ClassifiedTextView() {}

void ClassifiedTextView::SetText(const base::string16& text,
                                 const ACMatchClassifications& classifications,
                                 bool force_dim) {
  render_text_ = CreateClassifiedRenderText(text, classifications, force_dim);
}

void ClassifiedTextView::SetViewState(
    OmniboxResultView::ResultViewState state) {
  view_state_ = state;
  if (render_text_) {
    render_text_->SetColor(GetColor(state, DIMMED_TEXT));
  }
}

gfx::Size ClassifiedTextView::CalculatePreferredSize() const {
  return render_text_->GetStringSize();
}

void ClassifiedTextView::Layout() {
  // CHECK(false);
}

std::unique_ptr<gfx::RenderText> ClassifiedTextView::CreateRenderText(
    const base::string16& text) const {
  auto render_text = gfx::RenderText::CreateHarfBuzzInstance();
  render_text->SetDisplayRect(gfx::Rect(gfx::Size(INT_MAX, 0)));
  render_text->SetCursorEnabled(false);
  render_text->SetElideBehavior(gfx::ELIDE_TAIL);
  render_text->SetFontList(font_list_);
  render_text->SetText(text);
  return render_text;
}

std::unique_ptr<gfx::RenderText> ClassifiedTextView::CreateClassifiedRenderText(
    const base::string16& text,
    const ACMatchClassifications& classifications,
    bool force_dim) const {
  std::unique_ptr<gfx::RenderText> render_text(CreateRenderText(text));
  const size_t text_length = render_text->text().length();
  for (size_t i = 0; i < classifications.size(); ++i) {
    const size_t text_start = classifications[i].offset;
    if (text_start >= text_length)
      break;

    const size_t text_end =
        (i < (classifications.size() - 1))
            ? std::min(classifications[i + 1].offset, text_length)
            : text_length;
    const gfx::Range current_range(text_start, text_end);

    // Calculate style-related data.
    if (classifications[i].style & ACMatchClassification::MATCH)
      render_text->ApplyWeight(gfx::Font::Weight::BOLD, current_range);

    ColorKind color_kind = TEXT;
    if (classifications[i].style & ACMatchClassification::URL) {
      color_kind = URL;
      render_text->SetDirectionalityMode(gfx::DIRECTIONALITY_AS_URL);
    } else if (force_dim ||
               (classifications[i].style & ACMatchClassification::DIM)) {
      color_kind = DIMMED_TEXT;
    } else if (classifications[i].style & ACMatchClassification::INVISIBLE) {
      color_kind = INVISIBLE_TEXT;
    }
    // render_text->ApplyColor(GetColor(GetState(), color_kind), current_range);
    render_text->ApplyColor(GetColor(view_state_, color_kind), current_range);
  }
  return render_text;
}

void ClassifiedTextView::SetFontList(const gfx::FontList& font_list) {
  font_list_ = font_list;
  font_height_ =
      std::max(font_list.GetHeight(),
               font_list.DeriveWithWeight(gfx::Font::Weight::BOLD).GetHeight());
}

void ClassifiedTextView::OnPaint(gfx::Canvas* canvas) {
  View::OnPaint(canvas);

  render_text_->SetDisplayRect(GetContentsBounds());
  render_text_->Draw(canvas);
}

SkColor ClassifiedTextView::GetColor(OmniboxResultView::ResultViewState state,
                                     ColorKind kind) const {
  if (kind == INVISIBLE_TEXT)
    return SK_ColorTRANSPARENT;
  for (size_t i = 0; i < arraysize(kTranslationTable); ++i) {
    if (kTranslationTable[i].state == state &&
        kTranslationTable[i].kind == kind) {
      return GetNativeTheme()->GetSystemColor(kTranslationTable[i].id);
    }
  }

  NOTREACHED();
  return gfx::kPlaceholderColor;
}

void ClassifiedTextView::SetText(const SuggestionAnswer::ImageLine& line) {
  render_text_ =
      CreateAnswerText(line, GetAnswerFont(line.text_fields()[0].type()));
}

std::unique_ptr<gfx::RenderText> ClassifiedTextView::CreateAnswerText(
    const SuggestionAnswer::ImageLine& line,
    const gfx::FontList& font_list) const {
  std::unique_ptr<gfx::RenderText> destination =
      CreateRenderText(base::string16());
  destination->SetFontList(font_list);

  for (const SuggestionAnswer::TextField& text_field : line.text_fields())
    AppendAnswerText(destination.get(), text_field.text(), text_field.type());
  if (!line.text_fields().empty()) {
    constexpr int kMaxDisplayLines = 3;
    const SuggestionAnswer::TextField& first_field = line.text_fields().front();
    if (first_field.has_num_lines() && first_field.num_lines() > 1 &&
        destination->MultilineSupported()) {
      destination->SetMultiline(true);
      destination->SetMaxLines(
          std::min(kMaxDisplayLines, first_field.num_lines()));
    }
  }
  const base::char16 space(' ');
  const auto* text_field = line.additional_text();
  if (text_field) {
    AppendAnswerText(destination.get(), space + text_field->text(),
                     text_field->type());
  }
  text_field = line.status_text();
  if (text_field) {
    AppendAnswerText(destination.get(), space + text_field->text(),
                     text_field->type());
  }
  return destination;
}

void ClassifiedTextView::AppendAnswerText(gfx::RenderText* destination,
                                          const base::string16& text,
                                          int text_type) const {
  // TODO(dschuyler): make this better.  Right now this only supports unnested
  // bold tags.  In the future we'll need to flag unexpected tags while adding
  // support for b, i, u, sub, and sup.  We'll also need to support HTML
  // entities (&lt; for '<', etc.).
  const base::string16 begin_tag = base::ASCIIToUTF16("<b>");
  const base::string16 end_tag = base::ASCIIToUTF16("</b>");
  size_t begin = 0;
  while (true) {
    size_t end = text.find(begin_tag, begin);
    if (end == base::string16::npos) {
      AppendAnswerTextHelper(destination, text.substr(begin), text_type, false);
      break;
    }
    AppendAnswerTextHelper(destination, text.substr(begin, end - begin),
                           text_type, false);
    begin = end + begin_tag.length();
    end = text.find(end_tag, begin);
    if (end == base::string16::npos)
      break;
    AppendAnswerTextHelper(destination, text.substr(begin, end - begin),
                           text_type, true);
    begin = end + end_tag.length();
  }
}

void ClassifiedTextView::AppendAnswerTextHelper(gfx::RenderText* destination,
                                                const base::string16& text,
                                                int text_type,
                                                bool is_bold) const {
  if (text.empty())
    return;
  int offset = destination->text().length();
  gfx::Range range(offset, offset + text.length());
  destination->AppendText(text);
  const TextStyle& text_style = GetTextStyle(text_type);
  // TODO(dschuyler): follow up on the problem of different font sizes within
  // one RenderText.  Maybe with destination->SetFontList(...).
  destination->ApplyWeight(
      is_bold ? gfx::Font::Weight::BOLD : gfx::Font::Weight::NORMAL, range);
  destination->ApplyColor(GetNativeTheme()->GetSystemColor(text_style.colors[

                              view_state_ /*GetState()*/]),
                          range);
  destination->ApplyBaselineStyle(text_style.baseline, range);
}

int ClassifiedTextView::GetFontHeight() const {
  return font_height_;
}

const gfx::FontList& ClassifiedTextView::GetAnswerFont(int text_type) const {
// This assumes that the first text type in the second answer line can be used
// to specify the font for all the text fields in the line. For now this works
// but eventually it will be necessary to get RenderText to support multiple
// font sizes or use multiple RenderTexts.
#if 0
  int text_type =
      match_.answer && !match_.answer->second_line().text_fields().empty()
          ? match_.answer->second_line().text_fields()[0].type()
          : SuggestionAnswer::SUGGESTION;
#endif

  // When BaseFont is specified, reuse font_list_, which may have had size
  // adjustments from BaseFont before it was provided to this class. Otherwise,
  // get the standard font list for the specified style.
  ui::ResourceBundle::FontStyle font_style = GetTextStyle(text_type).font;
  const gfx::FontList& font_list =
      (font_style == ui::ResourceBundle::BaseFont)
          ? font_list_
          : ui::ResourceBundle::GetSharedInstance().GetFontList(font_style);
  font_height_ = font_list.GetHeight();
  return font_list;
}

#if 0
int ClassifiedTextView::GetAnswerHeight() const {
  // If the answer specifies a maximum of 1 line we can simply return the answer
  // font height.
  if (match_.answer->second_line().num_text_lines() == 1)
    return GetAnswerFont().GetHeight() + kVerticalPadding;

  // Multi-line answers require layout in order to determine the number of lines
  // the RenderText will use.
  std::unique_ptr<ClassifiedTextView> answer_view;
  answer_view = std::make_unique<ClassifiedTextView>(GetAnswerFont());
  answer_view->SetText(match_.answer->second_line(), GetAnswerFont());
  // TODO(dschuyler): calculate the accurate inner width, accounting for the
  // answer icon and other indents.
  int hack_indent = 32;
  int internal_width = std::max(0, width() - hack_indent);
  return answer_view->GetHeightForWidth(internal_width) +
      kVerticalPadding;
}
#endif
