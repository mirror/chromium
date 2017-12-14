// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/hosted_app_frame_header_ash.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/extensions/hosted_app_browser_controller.h"
#include "chrome/grit/generated_resources.h"
#include "third_party/icu/source/common/unicode/uchar.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/render_text.h"
#include "ui/views/view.h"
#include "ui/views/widget/native_widget_aura.h"

namespace {

bool IsWhitespaceOrPunctuation(base::char16 c) {
  // Note: Characters above U+FFFF will just show up as two U_SURROGATEs, so
  // punctuation/whitespace above the BMP will not be identified.
  UCharCategory category = static_cast<UCharCategory>(u_charType(c));

  switch (category) {
    case U_SPACE_SEPARATOR:
    case U_LINE_SEPARATOR:
    case U_PARAGRAPH_SEPARATOR:
    case U_CONTROL_CHAR:
    case U_DASH_PUNCTUATION:
    case U_START_PUNCTUATION:
    case U_END_PUNCTUATION:
    case U_CONNECTOR_PUNCTUATION:
    case U_OTHER_PUNCTUATION:
    case U_MATH_SYMBOL:
    case U_CURRENCY_SYMBOL:
    case U_MODIFIER_SYMBOL:
    case U_OTHER_SYMBOL:
    case U_INITIAL_PUNCTUATION:
    case U_FINAL_PUNCTUATION:
      return true;
    default:
      return false;
  }

  return false;
}

// Removes any trailing whitespace or punctuation characters from a string.
base::StringPiece16 TrimTrailingWhitespacePunctuation(base::StringPiece16 str) {
  size_t i;
  for (i = str.size(); i > 0 && IsWhitespaceOrPunctuation(str[i - 1]); --i) {
  }

  return str.substr(0, i);
}

}  // namespace

HostedAppFrameHeaderAsh::HostedAppFrameHeaderAsh(
    extensions::HostedAppBrowserController* app_controller,
    views::Widget* frame,
    views::View* header_view,
    ash::FrameCaptionButtonContainerView* caption_button_container)
    : DefaultFrameHeader(frame, header_view, caption_button_container),
      app_controller_(app_controller),
      app_name_(base::UTF8ToUTF16(app_controller->GetAppShortName())),
      app_and_domain_(l10n_util::GetStringFUTF16(
          IDS_HOSTED_APP_NAME_AND_DOMAIN,
          app_name_,
          base::UTF8ToUTF16(app_controller->GetDomainAndRegistry()))) {
  // Hosted apps apply a theme color if specified by the extension.
  base::Optional<SkColor> theme_color = app_controller->GetThemeColor();
  if (theme_color) {
    SkColor opaque_theme_color =
        SkColorSetA(theme_color.value(), SK_AlphaOPAQUE);
    SetFrameColors(opaque_theme_color, opaque_theme_color);
  }

  title_render_text_ = CreateRenderText();
  app_and_domain_render_text_ = CreateRenderText();
  app_and_domain_render_text_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
}

HostedAppFrameHeaderAsh::~HostedAppFrameHeaderAsh() {}

std::unique_ptr<gfx::RenderText> HostedAppFrameHeaderAsh::CreateRenderText() {
  auto render_text = gfx::RenderText::CreateHarfBuzzInstance();
  render_text->SetFontList(views::NativeWidgetAura::GetWindowTitleFontList());
  render_text->SetCursorEnabled(false);
  render_text->SetColor(GetTitleColor());
  render_text->SetElideBehavior(gfx::FADE_TAIL);
  return render_text;
}

// static
void HostedAppFrameHeaderAsh::GetRenderTexts(
    const base::string16& page_title,
    const base::string16& app_name,
    const base::string16& app_and_domain,
    base::string16* title_label_text,
    base::string16* app_and_domain_label_text) {
  // TODO(calamity): Investigate localization implications of using a separator
  // like this.
  constexpr char kSeparator[] = " | ";

  // If the title ends with the app name, remove it from the title (and any
  // trailing punctuation), in order to avoid duplicating the app name. In many
  // cases, the title will be entirely removed.
  base::StringPiece16 title = page_title;
  if (base::EndsWith(title, app_name, base::CompareCase::SENSITIVE)) {
    title = title.substr(0, title.size() - app_name.size());
    title = TrimTrailingWhitespacePunctuation(title);
  }
  title.CopyToString(title_label_text);

  // Add a separator if the title isn't empty.
  *app_and_domain_label_text =
      title.empty() ? app_and_domain
                    : base::ASCIIToUTF16(kSeparator) + app_and_domain;
}

void HostedAppFrameHeaderAsh::UpdateRenderTexts() {
  base::string16 title_label_text;
  base::string16 app_and_domain_label_text;
  GetRenderTexts(app_controller_->GetTitle(), app_name_, app_and_domain_,
                 &title_label_text, &app_and_domain_label_text);
  app_and_domain_render_text_->SetText(app_and_domain_label_text);
  title_render_text_->SetText(title_label_text);
}

void HostedAppFrameHeaderAsh::LayoutRenderTexts(
    const gfx::Rect& available_title_bounds,
    int title_width,
    int app_and_domain_width) {
  // The title is either its own width if it fits, or the space remaining
  // after rendering the app and domain (which may be negative, but gets clamped
  // to 0).
  gfx::Rect title_bounds = available_title_bounds;
  title_bounds.set_width(
      std::min(title_bounds.width() - app_and_domain_width, title_width));
  title_bounds.set_x(view()->GetMirroredXForRect(title_bounds));
  title_render_text_->SetDisplayRect(title_bounds);

  // The app and domain are placed to the right of the title and clipped to the
  // original title bounds. This string is given full width whenever possible.
  gfx::Rect app_and_domain_bounds = available_title_bounds;
  app_and_domain_bounds.set_x(title_bounds.right());
  app_and_domain_bounds.set_width(app_and_domain_width);
  app_and_domain_bounds.Intersect(available_title_bounds);
  app_and_domain_bounds.set_x(
      view()->GetMirroredXForRect(app_and_domain_bounds));
  app_and_domain_render_text_->SetDisplayRect(app_and_domain_bounds);
}

void HostedAppFrameHeaderAsh::PaintTitleBar(gfx::Canvas* canvas) {
  title_render_text_->Draw(canvas);
  app_and_domain_render_text_->Draw(canvas);
}

// TODO(calamity): Make this elide behavior more sophisticated in handling other
// possible translations and languages (the domain should always render).
void HostedAppFrameHeaderAsh::LayoutHeader() {
  DefaultFrameHeader::LayoutHeader();

  // We only need to recalculate the strings when the window title changes, and
  // that causes the NonClientView to Layout(), which calls into us here.
  UpdateRenderTexts();

  LayoutRenderTexts(GetAvailableTitleBounds(),
                    title_render_text_->GetStringSize().width(),
                    app_and_domain_render_text_->GetStringSize().width());
}
