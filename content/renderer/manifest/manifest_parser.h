// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_MODULES_MANIFEST_MANIFESTPARSER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_MODULES_MANIFEST_MANIFESTPARSER_H_

#include <stdint.h>
#include <memory>

#include "base/macros.h"
#include "modules/ModulesExport.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"
#include "third_party/WebKit/public/platform/modules/manifest/manifest.mojom-blink.h"

namespace blink {
class JSONObject;

// ManifestParser handles the logic of parsing the Web Manifest from a string.
// It implements:
// http://w3c.github.io/manifest/#dfn-steps-for-processing-a-manifest
class MODULES_EXPORT ManifestParser {
 public:
  ManifestParser(const String& data,
                 const KURL& manifest_url,
                 const KURL& document_url);
  ~ManifestParser();

  // Parse the Manifest from a string using following:
  // http://w3c.github.io/manifest/#dfn-steps-for-processing-a-manifest
  void Parse();

  mojom::blink::ManifestPtr Manifest();

  Vector<mojom::blink::ManifestErrorPtr> TakeErrors();

 private:
  // Used to indicate whether to strip whitespace when parsing a string.
  enum TrimType { Trim, NoTrim };

  // Indicate whether a parsed URL should be restricted to document origin.
  enum class ParseURLOriginRestrictions {
    kNoRestrictions = 0,
    kSameOriginOnly,
  };

  // Helper function to parse booleans present on a given |dictionary| in a
  // given field identified by its |key|.
  // Returns the parsed boolean if any, or |default_value| if parsing failed.
  bool ParseBoolean(const JSONObject& dictionary,
                    const String& key,
                    bool default_value);

  // Helper function to parse strings present on a given |dictionary| in a given
  // field identified by its |key|.
  // Returns the parsed string if any, a null string if the parsing failed.
  String ParseString(const JSONObject& dictionary, const String& key, TrimType);

  // Helper function to parse colors present on a given |dictionary| in a given
  // field identified by its |key|.
  // Returns the parsed color as an int64_t if any,
  // Manifest::kInvalidOrMissingColor if the parsing failed.
  int64_t ParseColor(const JSONObject& dictionary, const String& key);

  // Helper function to parse URLs present on a given |dictionary| in a given
  // field identified by its |key|. The URL is first parsed as a string then
  // resolved using |base_url|. |enforce_document_origin| specified whether to
  // enforce matching of the document's and parsed URL's origins.
  // Returns a KURL. If the parsing failed or origin matching was enforced but
  // not present, the returned KURL will be invalid.
  KURL ParseURL(const JSONObject& dictionary,
                const String& key,
                const KURL& base_url,
                ParseURLOriginRestrictions);

  // Parses the 'name' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-name-member
  // Returns the parsed string if any, a null string if the parsing failed.
  String ParseName(const JSONObject& dictionary);

  // Parses the 'short_name' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-short-name-member
  // Returns the parsed string if any, a null string if the parsing failed.
  String ParseShortName(const JSONObject& dictionary);

  // Parses the 'scope' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-scope-member
  // Returns the parsed KURL if any, an empty KURL if the parsing failed.
  KURL ParseScope(const JSONObject& dictionary, const KURL& start_url);

  // Parses the 'start_url' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-start_url-member
  // Returns the parsed KURL if any, an empty KURL if the parsing failed.
  KURL ParseStartURL(const JSONObject& dictionary);

  // Parses the 'display' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-display-member
  // Returns the parsed DisplayMode if any, WebDisplayModeUndefined if the
  // parsing failed.
  blink::WebDisplayMode ParseDisplay(const JSONObject& dictionary);

  // Parses the 'orientation' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-orientation-member
  // Returns the parsed WebScreenOrientationLockType if any,
  // WebScreenOrientationLockDefault if the parsing failed.
  blink::WebScreenOrientationLockType ParseOrientation(
      const JSONObject& dictionary);

  // Parses the 'src' field of an icon, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-src-member-of-an-image
  // Returns the parsed KURL if any, an empty KURL if the parsing failed.
  KURL ParseIconSrc(const JSONObject& icon);

  // Parses the 'type' field of an icon, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-type-member-of-an-image
  // Returns the parsed string if any, an empty string if the parsing failed.
  String ParseIconType(const JSONObject& icon);

  // Parses the 'sizes' field of an icon, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-a-sizes-member-of-an-image
  // Returns a vector of gfx::Size with the successfully parsed sizes, if any.
  // An empty vector if the field was not present or empty. "Any" is represented
  // by gfx::Size(0, 0).
  Vector<WebSize> ParseIconSizes(const JSONObject& icon);

  // Parses the 'purpose' field of an icon, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-a-purpose-member-of-an-image
  // Returns a vector of mojom::blink::ManifestIcon::IconPurposePtr with the
  // successfully parsed icon purposes, and a vector with
  // mojom::blink::ManifestIcon::IconPurpose::AnyPtr if the parsing failed.
  Vector<mojom::blink::ManifestIcon::Purpose> ParseIconPurpose(
      const JSONObject& icon);

  // Parses the 'icons' field of a Manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-an-array-of-images
  // Returns a vector of mojom::blink::ManifestIconPtr with the successfully
  // parsed icons, if any. An empty vector if the field was not present or
  // empty.
  Vector<mojom::blink::ManifestIconPtr> ParseIcons(
      const JSONObject& dictionary);

  // Parses the 'url_template' field of a Share Target, as defined in:
  // https://github.com/WICG/web-share-target/blob/master/docs/interface.md
  // Returns the parsed string if any, or a null string if the field was not
  // present, or didn't contain a string.
  String ParseShareTargetURLTemplate(const JSONObject& share_target);

  // Parses the 'share_target' field of a Manifest, as defined in:
  // https://github.com/WICG/web-share-target/blob/master/docs/interface.md
  // Returns the parsed Web Share target. The returned Share Target is null if
  // the field didn't exist, parsing failed, or it was empty.
  mojom::blink::ManifestShareTargetPtr ParseShareTarget(
      const JSONObject& dictionary);

  // Parses the 'platform' field of a related application, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-platform-member-of-an-application
  // Returns the parsed string if any, a null string if the parsing failed.
  String ParseRelatedApplicationPlatform(const JSONObject& application);

  // Parses the 'url' field of a related application, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-url-member-of-an-application
  // Returns the paresed KURL if any, an empty KURL if the parsing failed.
  KURL ParseRelatedApplicationURL(const JSONObject& application);

  // Parses the 'id' field of a related application, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-id-member-of-an-application
  // Returns the parsed string if any, a null string if the parsing failed.
  String ParseRelatedApplicationId(const JSONObject& application);

  // Parses the 'related_applications' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-related_applications-member
  // Returns a vector of mojom::blink::ManifestRelatedApplicationPtr with the
  // successfully parsed applications, if any. An empty vector if the field was
  // not present or empty.
  Vector<mojom::blink::ManifestRelatedApplicationPtr> ParseRelatedApplications(
      const JSONObject& dictionary);

  // Parses the 'prefer_related_applications' field on the manifest, as defined
  // in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-prefer_related_applications-member
  // returns true iff the field could be parsed as the boolean true.
  bool ParsePreferRelatedApplications(const JSONObject& dictionary);

  // Parses the 'theme_color' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-theme_color-member
  // Returns the parsed theme color if any,
  // Manifest::kInvalidOrMissingColor if the parsing failed.
  int64_t ParseThemeColor(const JSONObject& dictionary);

  // Parses the 'background_color' field of the manifest, as defined in:
  // https://w3c.github.io/manifest/#dfn-steps-for-processing-the-background_color-member
  // Returns the parsed background color if any,
  // Manifest::kInvalidOrMissingColor if the parsing failed.
  int64_t ParseBackgroundColor(const JSONObject& dictionary);

  // Parses the 'splash_screen_url' field of the manifest.
  // Returns the parsed GURL if any, an empty GURL if the parsing failed.
  KURL ParseSplashScreenURL(const JSONObject& dictionary);

  // Parses the 'gcm_sender_id' field of the manifest.
  // This is a proprietary extension of the Web Manifest specification.
  // Returns the parsed string if any, a null string if the parsing failed.
  String ParseGCMSenderID(const JSONObject& dictionary);

  void AddErrorInfo(const String& error_msg, bool critical = false);

  const String data_;
  const KURL manifest_url_;
  const KURL document_url_;

  mojom::blink::ManifestPtr manifest_;
  Vector<mojom::blink::ManifestErrorPtr> errors_;

  DISALLOW_COPY_AND_ASSIGN(ManifestParser);
};

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_MODULES_MANIFEST_MANIFESTPARSER_H_
