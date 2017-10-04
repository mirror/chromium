// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/searchbox/searchbox_extension.h"

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "base/i18n/rtl.h"
#include "base/json/string_escape.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/search/instant_types.h"
#include "chrome/common/search/ntp_logging_events.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/renderer_resources.h"
#include "chrome/renderer/searchbox/searchbox.h"
#include "components/crx_file/id_util.h"
#include "components/ntp_tiles/tile_source.h"
#include "components/ntp_tiles/tile_visual_type.h"
#include "content/public/renderer/chrome_object_extensions_utils.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/window_open_disposition.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "url/gurl.h"
#include "url/url_constants.h"
#include "v8/include/v8.h"

namespace internal {  // for testing.

// Returns an array with the RGBA color components.
v8::Local<v8::Value> RGBAColorToArray(v8::Isolate* isolate,
                                      const RGBAColor& color) {
  v8::Local<v8::Array> color_array = v8::Array::New(isolate, 4);
  color_array->Set(0, v8::Int32::New(isolate, color.r));
  color_array->Set(1, v8::Int32::New(isolate, color.g));
  color_array->Set(2, v8::Int32::New(isolate, color.b));
  color_array->Set(3, v8::Int32::New(isolate, color.a));
  return color_array;
}

}  // namespace internal

namespace {

const char kCSSBackgroundImageFormat[] = "-webkit-image-set("
    "url(chrome-search://theme/IDR_THEME_NTP_BACKGROUND?%s) 1x, "
    "url(chrome-search://theme/IDR_THEME_NTP_BACKGROUND@2x?%s) 2x)";

const char kCSSBackgroundColorFormat[] = "rgba(%d,%d,%d,%s)";

const char kCSSBackgroundPositionCenter[] = "center";
const char kCSSBackgroundPositionLeft[] = "left";
const char kCSSBackgroundPositionTop[] = "top";
const char kCSSBackgroundPositionRight[] = "right";
const char kCSSBackgroundPositionBottom[] = "bottom";

const char kCSSBackgroundRepeatNo[] = "no-repeat";
const char kCSSBackgroundRepeatX[] = "repeat-x";
const char kCSSBackgroundRepeatY[] = "repeat-y";
const char kCSSBackgroundRepeat[] = "repeat";

const char kThemeAttributionFormat[] = "-webkit-image-set("
    "url(chrome-search://theme/IDR_THEME_NTP_ATTRIBUTION?%s) 1x, "
    "url(chrome-search://theme/IDR_THEME_NTP_ATTRIBUTION@2x?%s) 2x)";

const char kLTRHtmlTextDirection[] = "ltr";
const char kRTLHtmlTextDirection[] = "rtl";

// Converts string16 to V8 String.
v8::Local<v8::String> UTF16ToV8String(v8::Isolate* isolate,
                                       const base::string16& s) {
  return v8::String::NewFromTwoByte(isolate,
                                    reinterpret_cast<const uint16_t*>(s.data()),
                                    v8::String::kNormalString,
                                    s.size());
}

// Converts std::string to V8 String.
v8::Local<v8::String> UTF8ToV8String(v8::Isolate* isolate,
                                      const std::string& s) {
  return v8::String::NewFromUtf8(
      isolate, s.data(), v8::String::kNormalString, s.size());
}

void Dispatch(blink::WebLocalFrame* frame, const blink::WebString& script) {
  if (!frame) return;
  frame->ExecuteScript(blink::WebScriptSource(script));
}

v8::Local<v8::String> GenerateThumbnailURL(
    v8::Isolate* isolate,
    int render_view_id,
    InstantRestrictedID most_visited_item_id) {
  return UTF8ToV8String(
      isolate,
      base::StringPrintf(
          "chrome-search://thumb/%d/%d", render_view_id, most_visited_item_id));
}

v8::Local<v8::String> GenerateThumb2URL(v8::Isolate* isolate,
                                        const GURL& page_url,
                                        const GURL& fallback_thumb_url) {
  return UTF8ToV8String(isolate,
                        base::StringPrintf("chrome-search://thumb2/%s?fb=%s",
                                           page_url.spec().c_str(),
                                           fallback_thumb_url.spec().c_str()));
}

// Populates a Javascript MostVisitedItem object for returning from
// newTabPage.mostVisited. This does not include private data such as "url" or
// "title".
v8::Local<v8::Object> GenerateMostVisitedItem(
    v8::Isolate* isolate,
    int render_view_id,
    InstantRestrictedID restricted_id,
    const InstantMostVisitedItem& mv_item) {
  v8::Local<v8::Object> obj = v8::Object::New(isolate);
  obj->Set(v8::String::NewFromUtf8(isolate, "rid"),
           v8::Int32::New(isolate, restricted_id));

  // TODO(treib): How do we get window.devicePixelRatio here?
  float device_pixel_ratio = 1.0f;
  std::string favicon =
      base::StringPrintf("chrome-search://favicon/size/16@%fx/%d/%d",
                         device_pixel_ratio, render_view_id, restricted_id);
  obj->Set(v8::String::NewFromUtf8(isolate, "faviconUrl"),
           UTF8ToV8String(isolate, favicon));

  return obj;
}

// Populates a Javascript MostVisitedItem object appropriate for returning from
// newTabPage.getMostVisitedItemData.
// NOTE: Includes private data such as "url", "title", and "domain", so this
// should not be returned to the host page (via newTabPage.mostVisited).
v8::Local<v8::Object> GenerateMostVisitedItemData(
    v8::Isolate* isolate,
    int render_view_id,
    InstantRestrictedID restricted_id,
    const InstantMostVisitedItem& mv_item) {
  // We set the "dir" attribute of the title, so that in RTL locales, a LTR
  // title is rendered left-to-right and truncated from the right. For
  // example, the title of http://msdn.microsoft.com/en-us/default.aspx is
  // "MSDN: Microsoft developer network". In RTL locales, in the New Tab
  // page, if the "dir" of this title is not specified, it takes Chrome UI's
  // directionality. So the title will be truncated as "soft developer
  // network". Setting the "dir" attribute as "ltr" renders the truncated
  // title as "MSDN: Microsoft D...". As another example, the title of
  // http://yahoo.com is "Yahoo!". In RTL locales, in the New Tab page, the
  // title will be rendered as "!Yahoo" if its "dir" attribute is not set to
  // "ltr".
  const char* direction;
  if (base::i18n::StringContainsStrongRTLChars(mv_item.title))
    direction = kRTLHtmlTextDirection;
  else
    direction = kLTRHtmlTextDirection;

  base::string16 title = mv_item.title;
  if (title.empty())
    title = base::UTF8ToUTF16(mv_item.url.spec());

  v8::Local<v8::Object> obj = v8::Object::New(isolate);
  obj->Set(v8::String::NewFromUtf8(isolate, "renderViewId"),
           v8::Int32::New(isolate, render_view_id));
  obj->Set(v8::String::NewFromUtf8(isolate, "rid"),
           v8::Int32::New(isolate, restricted_id));

  // If the suggestion already has a suggested thumbnail, we create a thumbnail
  // URL with both the local thumbnail and the proposed one as a fallback.
  // Otherwise, we just pass on the generated one.
  obj->Set(v8::String::NewFromUtf8(isolate, "thumbnailUrl"),
           mv_item.thumbnail.is_valid()
               ? GenerateThumb2URL(isolate, mv_item.url, mv_item.thumbnail)
               : GenerateThumbnailURL(isolate, render_view_id, restricted_id));

  // If the suggestion already has a favicon, we populate the element with it.
  if (!mv_item.favicon.spec().empty()) {
    obj->Set(v8::String::NewFromUtf8(isolate, "faviconUrl"),
             UTF8ToV8String(isolate, mv_item.favicon.spec()));
  }

  obj->Set(v8::String::NewFromUtf8(isolate, "tileTitleSource"),
           v8::Integer::New(isolate, static_cast<int>(mv_item.title_source)));
  obj->Set(v8::String::NewFromUtf8(isolate, "tileSource"),
           v8::Integer::New(isolate, static_cast<int>(mv_item.source)));
  obj->Set(v8::String::NewFromUtf8(isolate, "title"),
           UTF16ToV8String(isolate, title));
  obj->Set(v8::String::NewFromUtf8(isolate, "domain"),
           UTF8ToV8String(isolate, mv_item.url.host()));
  obj->Set(v8::String::NewFromUtf8(isolate, "direction"),
           v8::String::NewFromUtf8(isolate, direction));
  obj->Set(v8::String::NewFromUtf8(isolate, "url"),
           UTF8ToV8String(isolate, mv_item.url.spec()));
  return obj;
}

v8::Local<v8::Object> GenerateThemeBackgroundInfo(
    v8::Isolate* isolate,
    const ThemeBackgroundInfo& theme_info) {
  v8::Local<v8::Object> info = v8::Object::New(isolate);

  info->Set(v8::String::NewFromUtf8(isolate, "usingDefaultTheme"),
            v8::Boolean::New(isolate, theme_info.using_default_theme));

  // The theme background color is in RGBA format "rgba(R,G,B,A)" where R, G and
  // B are between 0 and 255 inclusive, and A is a double between 0 and 1
  // inclusive.
  // This is the CSS "background-color" format.
  // Value is always valid.
  // TODO(jfweitz): Remove this field after GWS is modified to use the new
  // backgroundColorRgba field.
  info->Set(
      v8::String::NewFromUtf8(isolate, "colorRgba"),
      UTF8ToV8String(
          isolate,
          // Convert the alpha using DoubleToString because StringPrintf will
          // use
          // locale specific formatters (e.g., use , instead of . in German).
          base::StringPrintf(
              kCSSBackgroundColorFormat, theme_info.background_color.r,
              theme_info.background_color.g, theme_info.background_color.b,
              base::DoubleToString(theme_info.background_color.a / 255.0)
                  .c_str())));

  // Theme color for background as an array with the RGBA components in order.
  // Value is always valid.
  info->Set(v8::String::NewFromUtf8(isolate, "backgroundColorRgba"),
            internal::RGBAColorToArray(isolate, theme_info.background_color));

  // Theme color for text as an array with the RGBA components in order.
  // Value is always valid.
  info->Set(v8::String::NewFromUtf8(isolate, "textColorRgba"),
            internal::RGBAColorToArray(isolate, theme_info.text_color));

  // Theme color for links as an array with the RGBA components in order.
  // Value is always valid.
  info->Set(v8::String::NewFromUtf8(isolate, "linkColorRgba"),
            internal::RGBAColorToArray(isolate, theme_info.link_color));

  // Theme color for light text as an array with the RGBA components in order.
  // Value is always valid.
  info->Set(v8::String::NewFromUtf8(isolate, "textColorLightRgba"),
            internal::RGBAColorToArray(isolate, theme_info.text_color_light));

  // Theme color for header as an array with the RGBA components in order.
  // Value is always valid.
  info->Set(v8::String::NewFromUtf8(isolate, "headerColorRgba"),
            internal::RGBAColorToArray(isolate, theme_info.header_color));

  // Theme color for section border as an array with the RGBA components in
  // order. Value is always valid.
  info->Set(
      v8::String::NewFromUtf8(isolate, "sectionBorderColorRgba"),
      internal::RGBAColorToArray(isolate, theme_info.section_border_color));

  // The theme alternate logo value indicates a white logo when TRUE and a
  // colorful one when FALSE.
  info->Set(v8::String::NewFromUtf8(isolate, "alternateLogo"),
            v8::Boolean::New(isolate, theme_info.logo_alternate));

  // The theme background image url is of format kCSSBackgroundImageFormat
  // where both instances of "%s" are replaced with the id that identifies the
  // theme.
  // This is the CSS "background-image" format.
  // Value is only valid if there's a custom theme background image.
  if (crx_file::id_util::IdIsValid(theme_info.theme_id)) {
    info->Set(v8::String::NewFromUtf8(isolate, "imageUrl"),
              UTF8ToV8String(isolate,
                             base::StringPrintf(kCSSBackgroundImageFormat,
                                                theme_info.theme_id.c_str(),
                                                theme_info.theme_id.c_str())));

    // The theme background image horizontal alignment is one of "left",
    // "right", "center".
    // This is the horizontal component of the CSS "background-position" format.
    // Value is only valid if |imageUrl| is not empty.
    std::string alignment = kCSSBackgroundPositionCenter;
    if (theme_info.image_horizontal_alignment ==
        THEME_BKGRND_IMAGE_ALIGN_LEFT) {
      alignment = kCSSBackgroundPositionLeft;
    } else if (theme_info.image_horizontal_alignment ==
               THEME_BKGRND_IMAGE_ALIGN_RIGHT) {
      alignment = kCSSBackgroundPositionRight;
    }
    info->Set(v8::String::NewFromUtf8(isolate, "imageHorizontalAlignment"),
              UTF8ToV8String(isolate, alignment));

    // The theme background image vertical alignment is one of "top", "bottom",
    // "center".
    // This is the vertical component of the CSS "background-position" format.
    // Value is only valid if |image_url| is not empty.
    if (theme_info.image_vertical_alignment == THEME_BKGRND_IMAGE_ALIGN_TOP) {
      alignment = kCSSBackgroundPositionTop;
    } else if (theme_info.image_vertical_alignment ==
               THEME_BKGRND_IMAGE_ALIGN_BOTTOM) {
      alignment = kCSSBackgroundPositionBottom;
    } else {
      alignment = kCSSBackgroundPositionCenter;
    }
    info->Set(v8::String::NewFromUtf8(isolate, "imageVerticalAlignment"),
              UTF8ToV8String(isolate, alignment));

    // The tiling of the theme background image is one of "no-repeat",
    // "repeat-x", "repeat-y", "repeat".
    // This is the CSS "background-repeat" format.
    // Value is only valid if |image_url| is not empty.
    std::string tiling = kCSSBackgroundRepeatNo;
    switch (theme_info.image_tiling) {
      case THEME_BKGRND_IMAGE_NO_REPEAT:
        tiling = kCSSBackgroundRepeatNo;
        break;
      case THEME_BKGRND_IMAGE_REPEAT_X:
        tiling = kCSSBackgroundRepeatX;
        break;
      case THEME_BKGRND_IMAGE_REPEAT_Y:
        tiling = kCSSBackgroundRepeatY;
        break;
      case THEME_BKGRND_IMAGE_REPEAT:
        tiling = kCSSBackgroundRepeat;
        break;
    }
    info->Set(v8::String::NewFromUtf8(isolate, "imageTiling"),
              UTF8ToV8String(isolate, tiling));

    // The theme background image height is only valid if |imageUrl| is valid.
    info->Set(v8::String::NewFromUtf8(isolate, "imageHeight"),
              v8::Int32::New(isolate, theme_info.image_height));

    // The attribution URL is only valid if the theme has attribution logo.
    if (theme_info.has_attribution) {
      info->Set(v8::String::NewFromUtf8(isolate, "attributionUrl"),
                UTF8ToV8String(
                    isolate, base::StringPrintf(kThemeAttributionFormat,
                                                theme_info.theme_id.c_str(),
                                                theme_info.theme_id.c_str())));
    }
  }

  return info;
}

}  // namespace

namespace extensions_v8 {

static const char kDispatchChromeIdentityCheckResult[] =
    "if (window.chrome &&"
    "    window.chrome.embeddedSearch &&"
    "    window.chrome.embeddedSearch.newTabPage &&"
    "    window.chrome.embeddedSearch.newTabPage.onsignedincheckdone &&"
    "    typeof window.chrome.embeddedSearch.newTabPage"
    "        .onsignedincheckdone === 'function') {"
    "  window.chrome.embeddedSearch.newTabPage.onsignedincheckdone(%s, %s);"
    "  true;"
    "}";

static const char kDispatchFocusChangedScript[] =
    "if (window.chrome &&"
    "    window.chrome.embeddedSearch &&"
    "    window.chrome.embeddedSearch.searchBox &&"
    "    window.chrome.embeddedSearch.searchBox.onfocuschange &&"
    "    typeof window.chrome.embeddedSearch.searchBox.onfocuschange =="
    "         'function') {"
    "  window.chrome.embeddedSearch.searchBox.onfocuschange();"
    "  true;"
    "}";

static const char kDispatchHistorySyncCheckResult[] =
    "if (window.chrome &&"
    "    window.chrome.embeddedSearch &&"
    "    window.chrome.embeddedSearch.newTabPage &&"
    "    window.chrome.embeddedSearch.newTabPage.onhistorysynccheckdone &&"
    "    typeof window.chrome.embeddedSearch.newTabPage"
    "        .onhistorysynccheckdone === 'function') {"
    "  window.chrome.embeddedSearch.newTabPage.onhistorysynccheckdone(%s);"
    "  true;"
    "}";

static const char kDispatchInputCancelScript[] =
    "if (window.chrome &&"
    "    window.chrome.embeddedSearch &&"
    "    window.chrome.embeddedSearch.newTabPage &&"
    "    window.chrome.embeddedSearch.newTabPage.oninputcancel &&"
    "    typeof window.chrome.embeddedSearch.newTabPage.oninputcancel =="
    "         'function') {"
    "  window.chrome.embeddedSearch.newTabPage.oninputcancel();"
    "  true;"
    "}";

static const char kDispatchInputStartScript[] =
    "if (window.chrome &&"
    "    window.chrome.embeddedSearch &&"
    "    window.chrome.embeddedSearch.newTabPage &&"
    "    window.chrome.embeddedSearch.newTabPage.oninputstart &&"
    "    typeof window.chrome.embeddedSearch.newTabPage.oninputstart =="
    "         'function') {"
    "  window.chrome.embeddedSearch.newTabPage.oninputstart();"
    "  true;"
    "}";

static const char kDispatchKeyCaptureChangeScript[] =
    "if (window.chrome &&"
    "    window.chrome.embeddedSearch &&"
    "    window.chrome.embeddedSearch.searchBox &&"
    "    window.chrome.embeddedSearch.searchBox.onkeycapturechange &&"
    "    typeof window.chrome.embeddedSearch.searchBox.onkeycapturechange =="
    "        'function') {"
    "  window.chrome.embeddedSearch.searchBox.onkeycapturechange();"
    "  true;"
    "}";

static const char kDispatchMostVisitedChangedScript[] =
    "if (window.chrome &&"
    "    window.chrome.embeddedSearch &&"
    "    window.chrome.embeddedSearch.newTabPage &&"
    "    window.chrome.embeddedSearch.newTabPage.onmostvisitedchange &&"
    "    typeof window.chrome.embeddedSearch.newTabPage.onmostvisitedchange =="
    "         'function') {"
    "  window.chrome.embeddedSearch.newTabPage.onmostvisitedchange();"
    "  true;"
    "}";

static const char kDispatchThemeChangeEventScript[] =
    "if (window.chrome &&"
    "    window.chrome.embeddedSearch &&"
    "    window.chrome.embeddedSearch.newTabPage &&"
    "    window.chrome.embeddedSearch.newTabPage.onthemechange &&"
    "    typeof window.chrome.embeddedSearch.newTabPage.onthemechange =="
    "        'function') {"
    "  window.chrome.embeddedSearch.newTabPage.onthemechange();"
    "  true;"
    "}";

// ----------------------------------------------------------------------------

class SearchBoxBindings : public gin::Wrappable<SearchBoxBindings> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  explicit SearchBoxBindings(content::RenderFrame* render_frame);
  ~SearchBoxBindings() override;

 private:
  // gin::Wrappable.
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) final;

  bool GetRightToLeft();
  bool IsFocused();
  bool IsKeyCaptureEnabled();
  // TODO(treib): Take v8::Value or something?
  void Paste(const std::string& text);
  void StartCapturingKeyStrokes();
  void StopCapturingKeyStrokes();

  // TODO(treib): SearchBox*? Also handle null SearchBox and non-main frame.
  content::RenderFrame* render_frame_;
};

gin::WrapperInfo SearchBoxBindings::kWrapperInfo = {gin::kEmbedderNativeGin};

SearchBoxBindings::SearchBoxBindings(content::RenderFrame* render_frame)
    : render_frame_(render_frame) {}

SearchBoxBindings::~SearchBoxBindings() = default;

gin::ObjectTemplateBuilder SearchBoxBindings::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<SearchBoxBindings>::GetObjectTemplateBuilder(isolate)
      .SetProperty("rtl", &SearchBoxBindings::GetRightToLeft)
      .SetProperty("isFocused", &SearchBoxBindings::IsFocused)
      .SetProperty("isKeyCaptureEnabled",
                   &SearchBoxBindings::IsKeyCaptureEnabled)
      .SetMethod("paste", &SearchBoxBindings::Paste)
      .SetMethod("startCapturingKeyStrokes",
                 &SearchBoxBindings::StartCapturingKeyStrokes)
      .SetMethod("stopCapturingKeyStrokes",
                 &SearchBoxBindings::StopCapturingKeyStrokes);
}

bool SearchBoxBindings::GetRightToLeft() {
  return base::i18n::IsRTL();
}

bool SearchBoxBindings::IsFocused() {
  return SearchBox::Get(render_frame_)->is_focused();
}

bool SearchBoxBindings::IsKeyCaptureEnabled() {
  return SearchBox::Get(render_frame_)->is_key_capture_enabled();
}

void SearchBoxBindings::Paste(const std::string& text) {
  SearchBox::Get(render_frame_)->Paste(base::UTF8ToUTF16(text));
}

void SearchBoxBindings::StartCapturingKeyStrokes() {
  SearchBox::Get(render_frame_)->StartCapturingKeyStrokes();
}

void SearchBoxBindings::StopCapturingKeyStrokes() {
  SearchBox::Get(render_frame_)->StopCapturingKeyStrokes();
}

class NewTabPageBindings : public gin::Wrappable<NewTabPageBindings> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  explicit NewTabPageBindings(content::RenderFrame* render_frame);
  ~NewTabPageBindings() override;

 private:
  // gin::Wrappable.
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) final;

  // TODO(treib): May return null, handle at call sites.
  SearchBox* GetSearchBox();

  bool HasOrigin(const GURL& origin) const;

  bool IsInputInProgress();
  void GetMostVisited(gin::Arguments* args);
  void GetThemeBackgroundInfo(gin::Arguments* args);

  void CheckIsUserSignedInToChromeAs(const std::string& identity);
  void CheckIsUserSyncingHistory();
  void DeleteMostVisitedItem(int rid);
  void GetMostVisitedItemData(gin::Arguments* args);
  void LogEvent(int event);
  void LogMostVisitedImpression(int position,
                                int tile_title_source,
                                int tile_source,
                                int tile_type);
  void LogMostVisitedNavigation(int position,
                                int tile_title_source,
                                int tile_source,
                                int tile_type);
  void UndoAllMostVisitedDeletions();
  void UndoMostVisitedDeletion(int rid);

  content::RenderFrame* render_frame_;
};

gin::WrapperInfo NewTabPageBindings::kWrapperInfo = {gin::kEmbedderNativeGin};

NewTabPageBindings::NewTabPageBindings(content::RenderFrame* render_frame)
    : render_frame_(render_frame) {}

NewTabPageBindings::~NewTabPageBindings() = default;

gin::ObjectTemplateBuilder NewTabPageBindings::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<NewTabPageBindings>::GetObjectTemplateBuilder(isolate)
      .SetProperty("isInputInProgress", &NewTabPageBindings::IsInputInProgress)
      .SetProperty("mostVisited", &NewTabPageBindings::GetMostVisited)
      .SetProperty("themeBackgroundInfo",
                   &NewTabPageBindings::GetThemeBackgroundInfo)
      .SetMethod("checkIsUserSignedIntoChromeAs",
                 &NewTabPageBindings::CheckIsUserSignedInToChromeAs)
      .SetMethod("checkIsUserSyncingHistory",
                 &NewTabPageBindings::CheckIsUserSyncingHistory)
      .SetMethod("deleteMostVisitedItem",
                 &NewTabPageBindings::DeleteMostVisitedItem)
      .SetMethod("getMostVisitedItemData",
                 &NewTabPageBindings::GetMostVisitedItemData)
      .SetMethod("logEvent", &NewTabPageBindings::LogEvent)
      .SetMethod("logMostVisitedImpression",
                 &NewTabPageBindings::LogMostVisitedImpression)
      .SetMethod("logMostVisitedNavigation",
                 &NewTabPageBindings::LogMostVisitedNavigation)
      .SetMethod("undoAllMostVisitedDeletions",
                 &NewTabPageBindings::UndoAllMostVisitedDeletions)
      .SetMethod("undoMostVisitedDeletion",
                 &NewTabPageBindings::UndoMostVisitedDeletion);
}

SearchBox* NewTabPageBindings::GetSearchBox() {
  // TODO(treib): Check that it's the main frame?
  return SearchBox::Get(content::RenderFrame::FromWebFrame(
      render_frame_->GetWebFrame()->LocalRoot()));
}

bool NewTabPageBindings::HasOrigin(const GURL& origin) const {
  GURL url(render_frame_->GetWebFrame()->GetDocument().Url());
  return url.GetOrigin() == origin.GetOrigin();
}

bool NewTabPageBindings::IsInputInProgress() {
  return GetSearchBox()->is_input_in_progress();
}

void NewTabPageBindings::GetMostVisited(gin::Arguments* args) {
  const SearchBox* search_box = GetSearchBox();
  // TODO(treib): Get main frame?
  int render_view_id = render_frame_->GetRenderView()->GetRoutingID();

  std::vector<InstantMostVisitedItemIDPair> instant_mv_items;
  search_box->GetMostVisitedItems(&instant_mv_items);
  v8::Isolate* isolate = args->isolate();
  v8::Local<v8::Object> v8_mv_items =
      v8::Array::New(isolate, instant_mv_items.size());
  for (size_t i = 0; i < instant_mv_items.size(); ++i) {
    v8_mv_items->Set(i, GenerateMostVisitedItem(isolate, render_view_id,
                                                instant_mv_items[i].first,
                                                instant_mv_items[i].second));
  }
  args->Return(v8_mv_items);
}

void NewTabPageBindings::GetThemeBackgroundInfo(gin::Arguments* args) {
  const ThemeBackgroundInfo& theme_info =
      GetSearchBox()->GetThemeBackgroundInfo();
  args->Return(GenerateThemeBackgroundInfo(args->isolate(), theme_info));
}

void NewTabPageBindings::CheckIsUserSignedInToChromeAs(
    const std::string& identity) {
  GetSearchBox()->CheckIsUserSignedInToChromeAs(base::UTF8ToUTF16(identity));
}

void NewTabPageBindings::CheckIsUserSyncingHistory() {
  GetSearchBox()->CheckIsUserSyncingHistory();
}

void NewTabPageBindings::DeleteMostVisitedItem(int rid) {
  GetSearchBox()->DeleteMostVisitedItem(rid);
}

void NewTabPageBindings::GetMostVisitedItemData(gin::Arguments* args) {
  if (!HasOrigin(GURL(chrome::kChromeSearchMostVisitedUrl))) {
    return;
  }
  int rid = 0;
  InstantMostVisitedItem mv_item;
  if (args->Length() != 1 || !args->GetNext(&rid) ||
      !GetSearchBox()->GetMostVisitedItemWithID(rid, &mv_item)) {
    return;
  }
  args->Return(GenerateMostVisitedItemData(
      args->isolate(), render_frame_->GetRenderView()->GetRoutingID(), rid,
      mv_item));
}

void NewTabPageBindings::LogEvent(int event) {
  if (!HasOrigin(GURL(chrome::kChromeSearchMostVisitedUrl)) &&
      !HasOrigin(GURL(chrome::kChromeSearchLocalNtpUrl))) {
    return;
  }
  if (event <= NTP_EVENT_TYPE_LAST) {
    GetSearchBox()->LogEvent(static_cast<NTPLoggingEventType>(event));
  }
}

void NewTabPageBindings::LogMostVisitedImpression(int position,
                                                  int tile_title_source,
                                                  int tile_source,
                                                  int tile_type) {
  if (!HasOrigin(GURL(chrome::kChromeSearchMostVisitedUrl))) {
    return;
  }
  if (tile_title_source <= static_cast<int>(ntp_tiles::TileTitleSource::LAST) &&
      tile_source <= static_cast<int>(ntp_tiles::TileSource::LAST) &&
      tile_type <= ntp_tiles::TileVisualType::TILE_TYPE_MAX) {
    GetSearchBox()->LogMostVisitedImpression(
        position, static_cast<ntp_tiles::TileTitleSource>(tile_title_source),
        static_cast<ntp_tiles::TileSource>(tile_source),
        static_cast<ntp_tiles::TileVisualType>(tile_type));
  }
}

void NewTabPageBindings::LogMostVisitedNavigation(int position,
                                                  int tile_title_source,
                                                  int tile_source,
                                                  int tile_type) {
  if (!HasOrigin(GURL(chrome::kChromeSearchMostVisitedUrl))) {
    return;
  }
  if (tile_title_source <= static_cast<int>(ntp_tiles::TileTitleSource::LAST) &&
      tile_source <= static_cast<int>(ntp_tiles::TileSource::LAST) &&
      tile_type <= ntp_tiles::TileVisualType::TILE_TYPE_MAX) {
    GetSearchBox()->LogMostVisitedNavigation(
        position, static_cast<ntp_tiles::TileTitleSource>(tile_title_source),
        static_cast<ntp_tiles::TileSource>(tile_source),
        static_cast<ntp_tiles::TileVisualType>(tile_type));
  }
}

void NewTabPageBindings::UndoAllMostVisitedDeletions() {
  GetSearchBox()->UndoAllMostVisitedDeletions();
}

void NewTabPageBindings::UndoMostVisitedDeletion(int rid) {
  GetSearchBox()->UndoMostVisitedDeletion(rid);
}

// static
void SearchBoxExtension::Install(content::RenderFrame* render_frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame->GetWebFrame()->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  gin::Handle<SearchBoxBindings> searchbox_controller =
      gin::CreateHandle(isolate, new SearchBoxBindings(render_frame));
  if (searchbox_controller.IsEmpty())
    return;

  gin::Handle<NewTabPageBindings> newtabpage_controller =
      gin::CreateHandle(isolate, new NewTabPageBindings(render_frame));
  if (newtabpage_controller.IsEmpty())
    return;

  v8::Handle<v8::Object> chrome =
      content::GetOrCreateChromeObject(isolate, context->Global());
  v8::Local<v8::Object> embedded_search = v8::Object::New(isolate);
  embedded_search->Set(gin::StringToV8(isolate, "searchBox"),
                       searchbox_controller.ToV8());
  embedded_search->Set(gin::StringToV8(isolate, "newTabPage"),
                       newtabpage_controller.ToV8());
  chrome->Set(gin::StringToSymbol(isolate, "embeddedSearch"), embedded_search);
}

// static
void SearchBoxExtension::DispatchChromeIdentityCheckResult(
    blink::WebLocalFrame* frame,
    const base::string16& identity,
    bool identity_match) {
  std::string escaped_identity = base::GetQuotedJSONString(identity);
  blink::WebString script(blink::WebString::FromUTF8(base::StringPrintf(
      kDispatchChromeIdentityCheckResult, escaped_identity.c_str(),
      identity_match ? "true" : "false")));
  Dispatch(frame, script);
}

// static
void SearchBoxExtension::DispatchFocusChange(blink::WebLocalFrame* frame) {
  Dispatch(frame, kDispatchFocusChangedScript);
}

// static
void SearchBoxExtension::DispatchHistorySyncCheckResult(
    blink::WebLocalFrame* frame,
    bool sync_history) {
  blink::WebString script(blink::WebString::FromUTF8(base::StringPrintf(
      kDispatchHistorySyncCheckResult, sync_history ? "true" : "false")));
  Dispatch(frame, script);
}

// static
void SearchBoxExtension::DispatchInputCancel(blink::WebLocalFrame* frame) {
  Dispatch(frame, kDispatchInputCancelScript);
}

// static
void SearchBoxExtension::DispatchInputStart(blink::WebLocalFrame* frame) {
  Dispatch(frame, kDispatchInputStartScript);
}

// static
void SearchBoxExtension::DispatchKeyCaptureChange(blink::WebLocalFrame* frame) {
  Dispatch(frame, kDispatchKeyCaptureChangeScript);
}

// static
void SearchBoxExtension::DispatchMostVisitedChanged(
    blink::WebLocalFrame* frame) {
  Dispatch(frame, kDispatchMostVisitedChangedScript);
}

// static
void SearchBoxExtension::DispatchThemeChange(blink::WebLocalFrame* frame) {
  Dispatch(frame, kDispatchThemeChangeEventScript);
}

}  // namespace extensions_v8
