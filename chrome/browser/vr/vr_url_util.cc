// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/vr_url_util.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/vr/color_scheme.h"
#include "chrome/browser/vr/elements/render_text_wrapper.h"
#include "chrome/browser/vr/elements/ui_texture.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/render_text.h"

namespace vr {

namespace {

static constexpr SkScalar kStrikeThicknessFactor = (SK_Scalar1 / 9);

using security_state::SecurityLevel;

// See LocationBarView::GetSecureTextColor().
SkColor GetSchemeColor(SecurityLevel level, const ColorScheme& color_scheme) {
  switch (level) {
    case SecurityLevel::NONE:
    case SecurityLevel::HTTP_SHOW_WARNING:
      return color_scheme.url_deemphasized;
    case SecurityLevel::EV_SECURE:
    case SecurityLevel::SECURE:
      return color_scheme.secure;
    case SecurityLevel::SECURITY_WARNING:
      return color_scheme.url_deemphasized;
    case SecurityLevel::SECURE_WITH_POLICY_INSTALLED_CERT:  // ChromeOS only.
      return color_scheme.insecure;
    case SecurityLevel::DANGEROUS:
      return color_scheme.insecure;
    default:
      NOTREACHED();
      return color_scheme.insecure;
  }
}

void setEmphasis(vr::RenderTextWrapper* render_text,
                 bool emphasis,
                 const gfx::Range& range,
                 const ColorScheme& color_scheme) {
  SkColor color =
      emphasis ? color_scheme.url_emphasized : color_scheme.url_deemphasized;
  if (range.IsValid()) {
    render_text->ApplyColor(color, range);
  } else {
    render_text->SetColor(color);
  }
}

}  // namespace

SkColor GetUrlSecurityChipColor(SecurityLevel level,
                                bool offline_page,
                                const ColorScheme& color_scheme) {
  return offline_page ? color_scheme.offline_page_warning
                      : GetSchemeColor(level, color_scheme);
}

void RenderUrl(const GURL& url,
               const url_formatter::FormatUrlTypes format_types,
               const security_state::SecurityLevel security_level,
               const ColorScheme& color_scheme,
               const base::Callback<void(UiUnsupportedMode)>& failure_callback,
               int pixel_font_height,
               bool apply_styling,
               SkColor text_color,
               gfx::RenderText* rendered_text) {
  DCHECK(rendered_text);
  url::Parsed parsed;

  const base::string16 text = url_formatter::FormatUrl(
      url, format_types, net::UnescapeRule::NORMAL, &parsed, nullptr, nullptr);

  gfx::FontList font_list;
  if (!UiTexture::GetFontList(pixel_font_height, text, &font_list))
    failure_callback.Run(UiUnsupportedMode::kUnhandledCodePoint);

  rendered_text->SetFontList(font_list);
  rendered_text->SetColor(text_color);
  rendered_text->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  rendered_text->SetElideBehavior(gfx::ELIDE_TAIL);
  rendered_text->SetDirectionalityMode(gfx::DIRECTIONALITY_FORCE_LTR);
  rendered_text->SetText(text);

  // Until we can properly elide a URL, we need to bail if the origin portion
  // cannot be displayed in its entirety.
  base::string16 mandatory_prefix = text;
  int length = parsed.CountCharactersBefore(url::Parsed::PORT, false);
  if (length > 0)
    mandatory_prefix = text.substr(0, length);
  // Ellipsis-based eliding replaces the last character in the string with an
  // ellipsis, so to reliably check that the origin is intact, check both length
  // and string equality.
  if (rendered_text->GetDisplayText().size() < mandatory_prefix.size() ||
      rendered_text->GetDisplayText().substr(0, mandatory_prefix.size()) !=
          mandatory_prefix) {
    failure_callback.Run(UiUnsupportedMode::kCouldNotElideURL);
  }

  if (apply_styling) {
    vr::RenderTextWrapper vr_render_text(rendered_text);
    ApplyUrlStyling(text, parsed, security_level, &vr_render_text,
                    color_scheme);
  }
}

// This method replicates behavior in OmniboxView::UpdateTextStyle(), and
// attempts to maintain similar code structure.
void ApplyUrlStyling(const base::string16& formatted_url,
                     const url::Parsed& parsed,
                     const security_state::SecurityLevel security_level,
                     vr::RenderTextWrapper* render_text,
                     const ColorScheme& color_scheme) {
  const url::Component& scheme = parsed.scheme;
  const url::Component& host = parsed.host;

  enum DeemphasizeComponents {
    EVERYTHING,
    ALL_BUT_SCHEME,
    ALL_BUT_HOST,
    NOTHING,
  } deemphasize = NOTHING;

  const base::string16 url_scheme =
      formatted_url.substr(scheme.begin, scheme.len);

  // Data URLs are rarely human-readable and can be used for spoofing, so draw
  // attention to the scheme to emphasize "this is just a bunch of data".  For
  // normal URLs, the host is the best proxy for "identity".
  // TODO(cjgrant): Handle extensions, if required, for desktop.
  if (url_scheme == base::UTF8ToUTF16(url::kDataScheme))
    deemphasize = ALL_BUT_SCHEME;
  else if (host.is_nonempty())
    deemphasize = ALL_BUT_HOST;

  gfx::Range scheme_range = scheme.is_nonempty()
                                ? gfx::Range(scheme.begin, scheme.end())
                                : gfx::Range::InvalidRange();
  switch (deemphasize) {
    case EVERYTHING:
      setEmphasis(render_text, false, gfx::Range::InvalidRange(), color_scheme);
      break;
    case NOTHING:
      setEmphasis(render_text, true, gfx::Range::InvalidRange(), color_scheme);
      break;
    case ALL_BUT_SCHEME:
      DCHECK(scheme_range.IsValid());
      setEmphasis(render_text, false, gfx::Range::InvalidRange(), color_scheme);
      setEmphasis(render_text, true, scheme_range, color_scheme);
      break;
    case ALL_BUT_HOST:
      setEmphasis(render_text, false, gfx::Range::InvalidRange(), color_scheme);
      setEmphasis(render_text, true, gfx::Range(host.begin, host.end()),
                  color_scheme);
      break;
  }

  // Only SECURE and DANGEROUS levels (pages served over HTTPS or flagged by
  // SafeBrowsing) get a special scheme color treatment. If the security level
  // is NONE or HTTP_SHOW_WARNING, we do not override the text style previously
  // applied to the scheme text range by setEmphasis().
  if (scheme_range.IsValid() && security_level != security_state::NONE &&
      security_level != security_state::HTTP_SHOW_WARNING) {
    render_text->ApplyColor(GetSchemeColor(security_level, color_scheme),
                            scheme_range);
    if (security_level == SecurityLevel::DANGEROUS) {
      render_text->SetStrikeThicknessFactor(kStrikeThicknessFactor);
      render_text->ApplyStyle(gfx::TextStyle::STRIKE, true, scheme_range);
    }
  }
}

}  // namespace vr
