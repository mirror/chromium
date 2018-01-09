// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/manifest/ManifestParser.h"

#include <stddef.h>
#include <utility>

#include "core/css/parser/CSSParser.h"
#include "modules/manifest/ManifestUmaUtil.h"
#include "platform/graphics/Color.h"
#include "platform/json/JSONParser.h"
#include "platform/json/JSONValues.h"
#include "public/platform/WebIconSizesParser.h"
#include "public/platform/WebSize.h"
#include "public/platform/WebString.h"
#include "public/web/WebCSSParser.h"

namespace blink {
namespace {

const size_t kMaxStringLength = 4 * 1024;

WebDisplayMode WebDisplayModeFromString(const String& display) {
  auto lower_display = display.LowerASCII();
  if (lower_display == "browser")
    return kWebDisplayModeBrowser;
  if (lower_display == "minimal-ui")
    return kWebDisplayModeMinimalUi;
  if (lower_display == "standalone")
    return kWebDisplayModeStandalone;
  if (lower_display == "fullscreen")
    return kWebDisplayModeFullscreen;
  return kWebDisplayModeUndefined;
}

WebScreenOrientationLockType WebScreenOrientationLockTypeFromString(
    const String& orientation) {
  auto lower_orientation = orientation.LowerASCII();
  if (lower_orientation == "portrait-primary")
    return kWebScreenOrientationLockPortraitPrimary;
  if (lower_orientation == "portrait-secondary")
    return kWebScreenOrientationLockPortraitSecondary;
  if (lower_orientation == "landscape-primary")
    return kWebScreenOrientationLockLandscapePrimary;
  if (lower_orientation == "landscape-secondary")
    return kWebScreenOrientationLockLandscapeSecondary;
  if (lower_orientation == "any")
    return kWebScreenOrientationLockAny;
  if (lower_orientation == "landscape")
    return kWebScreenOrientationLockLandscape;
  if (lower_orientation == "portrait")
    return kWebScreenOrientationLockPortrait;
  if (lower_orientation == "natural")
    return kWebScreenOrientationLockNatural;
  return kWebScreenOrientationLockDefault;
}

}  // namespace

ManifestParser::ManifestParser(const String& data,
                               const KURL& manifest_url,
                               const KURL& document_url)
    : data_(data), manifest_url_(manifest_url), document_url_(document_url) {}

ManifestParser::~ManifestParser() {}

void ManifestParser::Parse() {
  std::unique_ptr<JSONValue> value = ParseJSON(data_);

  if (!value) {
    AddErrorInfo("Invalid JSON.", true);
    ManifestUmaUtil::ParseFailed();
    return;
  }

  JSONObject* dictionary = JSONObject::Cast(value.get());
  if (!dictionary) {
    AddErrorInfo("root element must be a valid JSON object.", true);
    ManifestUmaUtil::ParseFailed();
    return;
  }

  manifest_ = mojom::blink::Manifest::New();
  manifest_->name = ParseName(*dictionary);
  manifest_->short_name = ParseShortName(*dictionary);
  manifest_->start_url = ParseStartURL(*dictionary);
  manifest_->scope = ParseScope(*dictionary, manifest_->start_url);
  manifest_->display = ParseDisplay(*dictionary);
  manifest_->orientation = ParseOrientation(*dictionary);
  manifest_->icons = ParseIcons(*dictionary);
  manifest_->share_target = ParseShareTarget(*dictionary);
  manifest_->related_applications = ParseRelatedApplications(*dictionary);
  manifest_->prefer_related_applications =
      ParsePreferRelatedApplications(*dictionary);
  manifest_->theme_color = ParseThemeColor(*dictionary);
  manifest_->background_color = ParseBackgroundColor(*dictionary);
  manifest_->splash_screen_url = ParseSplashScreenURL(*dictionary);
  manifest_->gcm_sender_id = ParseGCMSenderID(*dictionary);
  if ((!manifest_->name && !manifest_->short_name &&
       manifest_->start_url.IsNull() &&
       manifest_->display == kWebDisplayModeUndefined &&
       manifest_->orientation == kWebScreenOrientationLockDefault &&
       manifest_->icons.IsEmpty() && !manifest_->share_target &&
       manifest_->related_applications.IsEmpty() &&
       !manifest_->prefer_related_applications &&
       manifest_->theme_color ==
           mojom::blink::Manifest::kInvalidOrMissingColor &&
       manifest_->background_color ==
           mojom::blink::Manifest::kInvalidOrMissingColor &&
       manifest_->splash_screen_url.IsNull() && !manifest_->gcm_sender_id &&
       manifest_->scope.IsNull())) {
    manifest_ = nullptr;
  }

  ManifestUmaUtil::ParseSucceeded(manifest_.get());
}

mojom::blink::ManifestPtr ManifestParser::Manifest() {
  return std::move(manifest_);
}

Vector<mojom::blink::ManifestErrorPtr> ManifestParser::TakeErrors() {
  return std::move(errors_);
}

bool ManifestParser::ParseBoolean(const JSONObject& dictionary,
                                  const String& key,
                                  bool default_value) {
  if (!dictionary.Get(key))
    return default_value;

  bool value;
  if (!dictionary.GetBoolean(key, &value)) {
    AddErrorInfo("property '" + key + "' ignored, type " + "boolean expected.");
    return default_value;
  }

  return value;
}

String ManifestParser::ParseString(const JSONObject& dictionary,
                                   const String& key,
                                   TrimType trim) {
  if (!dictionary.Get(key))
    return String();

  String value;
  if (!dictionary.GetString(key, &value)) {
    AddErrorInfo("property '" + key + "' ignored, type " + "string expected.");
    return String();
  }

  if (value.CharactersSizeInBytes() > kMaxStringLength) {
    AddErrorInfo("property '" + key + "' ignored, value too long");
    return String();
  }

  if (trim == Trim)
    return value.StripWhiteSpace();
  return value;
}

int64_t ManifestParser::ParseColor(const JSONObject& dictionary,
                                   const String& key) {
  String parsed_color = ParseString(dictionary, key, Trim);
  if (!parsed_color)
    return mojom::blink::Manifest::kInvalidOrMissingColor;

  Color color;
  if (!CSSParser::ParseColor(color, parsed_color, true)) {
    AddErrorInfo("property '" + key + "' ignored, '" + parsed_color +
                 "' is not a " + "valid color.");
    return mojom::blink::Manifest::kInvalidOrMissingColor;
  }

  uint32_t rgb = color.Rgb();
  return reinterpret_cast<int32_t&>(rgb);
}

KURL ManifestParser::ParseURL(
    const JSONObject& dictionary,
    const String& key,
    const KURL& base_url,
    ManifestParser::ParseURLOriginRestrictions origin_restriction) {
  String url_str = ParseString(dictionary, key, NoTrim);
  if (!url_str)
    return KURL();

  KURL resolved = KURL(base_url, url_str);
  if (!resolved.IsValid()) {
    AddErrorInfo("property '" + key + "' ignored, URL is invalid.");
    return KURL();
  }

  switch (origin_restriction) {
    case ParseURLOriginRestrictions::kSameOriginOnly:
      if (!SecurityOrigin::AreSameSchemeHostPort(resolved, document_url_)) {
        AddErrorInfo("property '" + key +
                     "' ignored, should be same origin as document.");
        return KURL();
      }
      return resolved;
    case ParseURLOriginRestrictions::kNoRestrictions:
      return resolved;
  }
  NOTREACHED();
  return KURL();
}

String ManifestParser::ParseName(const JSONObject& dictionary) {
  return ParseString(dictionary, "name", Trim);
}

String ManifestParser::ParseShortName(const JSONObject& dictionary) {
  return ParseString(dictionary, "short_name", Trim);
}

KURL ManifestParser::ParseStartURL(const JSONObject& dictionary) {
  return ParseURL(dictionary, "start_url", manifest_url_,
                  ParseURLOriginRestrictions::kSameOriginOnly);
}

KURL ManifestParser::ParseScope(const JSONObject& dictionary,
                                const KURL& start_url) {
  KURL scope = ParseURL(dictionary, "scope", manifest_url_,
                        ParseURLOriginRestrictions::kSameOriginOnly);
  if (scope.IsValid()) {
    // According to the spec, if the start_url cannot be parsed, the document
    // URL should be used as the start URL. If the start_url could not be
    // parsed, check that the document URL is within scope.
    KURL check_in_scope = start_url.IsEmpty() ? document_url_ : start_url;
    if (!SecurityOrigin::AreSameSchemeHostPort(check_in_scope, scope) ||
        !check_in_scope.GetPath().StartsWith(scope.GetPath())) {
      AddErrorInfo(
          "property 'scope' ignored. Start url should be within scope "
          "of scope URL.");
      return KURL();
    }
  }
  return scope;
}

blink::WebDisplayMode ManifestParser::ParseDisplay(
    const JSONObject& dictionary) {
  String display = ParseString(dictionary, "display", Trim);
  if (!display)
    return blink::kWebDisplayModeUndefined;

  blink::WebDisplayMode display_enum = WebDisplayModeFromString(display);
  if (display_enum == blink::kWebDisplayModeUndefined)
    AddErrorInfo("unknown 'display' value ignored.");
  return display_enum;
}

blink::WebScreenOrientationLockType ManifestParser::ParseOrientation(
    const JSONObject& dictionary) {
  String orientation = ParseString(dictionary, "orientation", Trim);

  if (!orientation)
    return blink::kWebScreenOrientationLockDefault;

  blink::WebScreenOrientationLockType orientation_enum =
      WebScreenOrientationLockTypeFromString(orientation);
  if (orientation_enum == blink::kWebScreenOrientationLockDefault)
    AddErrorInfo("unknown 'orientation' value ignored.");
  return orientation_enum;
}

KURL ManifestParser::ParseIconSrc(const JSONObject& icon) {
  return ParseURL(icon, "src", manifest_url_,
                  ParseURLOriginRestrictions::kNoRestrictions);
}

String ManifestParser::ParseIconType(const JSONObject& icon) {
  String icon_type = ParseString(icon, "type", Trim);
  if (!icon_type)
    return String("", 0);
  return icon_type;
}

Vector<WebSize> ManifestParser::ParseIconSizes(const JSONObject& icon) {
  String sizes_str = ParseString(icon, "sizes", NoTrim);
  Vector<WebSize> sizes;

  if (!sizes_str)
    return sizes;

  blink::WebVector<blink::WebSize> web_sizes =
      blink::WebIconSizesParser::ParseIconSizes(blink::WebString(sizes_str));
  sizes.resize(web_sizes.size());
  for (size_t i = 0; i < web_sizes.size(); ++i)
    sizes[i] = web_sizes[i];
  if (sizes.IsEmpty()) {
    AddErrorInfo("found icon with no valid size.");
  }
  return sizes;
}

Vector<mojom::blink::ManifestIcon::Purpose> ManifestParser::ParseIconPurpose(
    const JSONObject& icon) {
  String purpose_str = ParseString(icon, "purpose", NoTrim);
  Vector<mojom::blink::ManifestIcon::Purpose> purposes;

  if (!purpose_str) {
    purposes.push_back(mojom::blink::ManifestIcon::Purpose::ANY);
    return purposes;
  }

  Vector<String> keywords;
  purpose_str.Split(" ", false, keywords);
  for (const String& keyword : keywords) {
    auto stripped_keyword = keyword.StripWhiteSpace().LowerASCII();
    if (stripped_keyword == "any") {
      purposes.push_back(mojom::blink::ManifestIcon::Purpose::ANY);
    } else if (stripped_keyword == "badge") {
      purposes.push_back(mojom::blink::ManifestIcon::Purpose::BADGE);
    } else {
      AddErrorInfo(
          "found icon with invalid purpose. "
          "Using default value 'any'.");
    }
  }

  if (purposes.IsEmpty()) {
    purposes.push_back(mojom::blink::ManifestIcon::Purpose::ANY);
  }

  return purposes;
}

Vector<mojom::blink::ManifestIconPtr> ManifestParser::ParseIcons(
    const JSONObject& dictionary) {
  Vector<mojom::blink::ManifestIconPtr> icons;
  if (!dictionary.Get("icons"))
    return icons;

  auto* icons_list = dictionary.GetArray("icons");
  if (!icons_list) {
    AddErrorInfo("property 'icons' ignored, type array expected.");
    return icons;
  }

  for (size_t i = 0; i < icons_list->size(); ++i) {
    const JSONObject* icon_dictionary = JSONObject::Cast(icons_list->at(i));
    if (!icon_dictionary)
      continue;

    auto src = ParseIconSrc(*icon_dictionary);
    // An icon MUST have a valid src. If it does not, it MUST be ignored.
    if (!src.IsValid())
      continue;

    icons.emplace_back(WTF::in_place, src, ParseIconType(*icon_dictionary),
                       ParseIconSizes(*icon_dictionary),
                       ParseIconPurpose(*icon_dictionary));
  }

  return icons;
}

String ManifestParser::ParseShareTargetURLTemplate(
    const JSONObject& share_target) {
  return ParseString(share_target, "url_template", Trim);
}

mojom::blink::ManifestShareTargetPtr ManifestParser::ParseShareTarget(
    const JSONObject& dictionary) {
  const JSONObject* share_target_dict =
      dictionary.GetJSONObject("share_target");

  if (!share_target_dict)
    return {};

  auto url_template = ParseShareTargetURLTemplate(*share_target_dict);

  if (!url_template)
    return {};

  return mojom::blink::ManifestShareTarget::New(url_template);
}

String ManifestParser::ParseRelatedApplicationPlatform(
    const JSONObject& application) {
  return ParseString(application, "platform", Trim);
}

KURL ManifestParser::ParseRelatedApplicationURL(const JSONObject& application) {
  return ParseURL(application, "url", manifest_url_,
                  ParseURLOriginRestrictions::kNoRestrictions);
}

String ManifestParser::ParseRelatedApplicationId(
    const JSONObject& application) {
  return ParseString(application, "id", Trim);
}

Vector<mojom::blink::ManifestRelatedApplicationPtr>
ManifestParser::ParseRelatedApplications(const JSONObject& dictionary) {
  Vector<mojom::blink::ManifestRelatedApplicationPtr> applications;
  if (!dictionary.Get("related_applications"))
    return applications;

  auto* applications_list = dictionary.GetArray("related_applications");
  if (!applications_list) {
    AddErrorInfo(
        "property 'related_applications' ignored,"
        " type array expected.");
    return applications;
  }

  for (size_t i = 0; i < applications_list->size(); ++i) {
    auto* application_dictionary = JSONObject::Cast(applications_list->at(i));
    if (!application_dictionary)
      continue;

    auto platform = ParseRelatedApplicationPlatform(*application_dictionary);
    // "If platform is undefined, move onto the next item if any are left."
    if (!platform) {
      AddErrorInfo(
          "'platform' is a required field, related application"
          " ignored.");
      continue;
    }

    auto application = mojom::blink::ManifestRelatedApplication::New(
        platform, ParseRelatedApplicationURL(*application_dictionary),
        ParseRelatedApplicationId(*application_dictionary));
    // "If both id and url are undefined, move onto the next item if any are
    // left."
    if (application->url.IsNull() && !application->id) {
      AddErrorInfo(
          "one of 'url' or 'id' is required, related application"
          " ignored.");
      continue;
    }
    applications.push_back(std::move(application));
  }
  return applications;
}

bool ManifestParser::ParsePreferRelatedApplications(
    const JSONObject& dictionary) {
  return ParseBoolean(dictionary, "prefer_related_applications", false);
}

int64_t ManifestParser::ParseThemeColor(const JSONObject& dictionary) {
  return ParseColor(dictionary, "theme_color");
}

int64_t ManifestParser::ParseBackgroundColor(const JSONObject& dictionary) {
  return ParseColor(dictionary, "background_color");
}

KURL ManifestParser::ParseSplashScreenURL(const JSONObject& dictionary) {
  return ParseURL(dictionary, "splash_screen_url", manifest_url_,
                  ParseURLOriginRestrictions::kSameOriginOnly);
}

String ManifestParser::ParseGCMSenderID(const JSONObject& dictionary) {
  return ParseString(dictionary, "gcm_sender_id", Trim);
}

void ManifestParser::AddErrorInfo(const String& error_msg, bool critical) {
  errors_.emplace_back(WTF::in_place, error_msg, critical);
}

}  // namespace blink
