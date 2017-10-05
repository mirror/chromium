// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/AllowedByNosniff.h"

#include "core/dom/ExecutionContext.h"
#include "core/frame/UseCounter.h"
#include "core/inspector/ConsoleMessage.h"
#include "platform/HTTPNames.h"
#include "platform/loader/fetch/ResourceResponse.h"
#include "platform/network/mime/MIMETypeRegistry.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace blink {

namespace {

// A list of MIME types/subtypes that we're interested in in this file.
// This declaration is of purely local use.
//
// kTypeWhatever describes anythibng we're not specifically interested in here.
enum TypeIds {
  kTypeApplication = 0,
  kTypeApplicationOctetStream,
  kTypeApplicationXml,
  kTypeAudio,
  kTypeImage,
  kTypeText,
  kTypeTextCsv,
  kTypeTextHtml,
  kTypeTextPlain,
  kTypeTextXml,
  kTypeVideo,
  kTypeWhatever
};

// Strings for each TypeIds value.
const char* type_ids_strings[] = {"application/",    "application/octet-stream",
                                  "application/xml", "audio/",
                                  "image/",          "text/",
                                  "text/csv",        "text/html",
                                  "text/plain",      "text/xml",
                                  "video/"};

const WebFeature kNone = static_cast<WebFeature>(0);

// Map TypeIds to WebFeatures we are interested in counting.
// The array indixes are for:
// - cross/same origin
// - non-worder/worker scope
// - TypeIds
const WebFeature features_for_id[2][2][12] = {
    {{
         // Cross-origin, non-worker scope.
         WebFeature::kCrossOriginApplicationScript,
         WebFeature::kCrossOriginApplicationOctetStream,
         WebFeature::kCrossOriginApplicationXml,
         kNone,  // kTypeAudio
         kNone,  // kTypeImage,
         WebFeature::kCrossOriginTextScript,
         kNone,  // kTypeTextCsv,
         WebFeature::kCrossOriginTextHtml, WebFeature::kCrossOriginTextPlain,
         WebFeature::kCrossOriginTextXml,
         kNone,  // kTypeVideo,
         kNone   // kTypeWhatever
     },
     {
         // Cross-origin, worker scope.
         WebFeature::kCrossOriginApplicationScript,
         WebFeature::kCrossOriginWorkerApplicationOctetStream,
         WebFeature::kCrossOriginWorkerApplicationXml,
         kNone,  // kTypeAudio
         kNone,  // kTypeImage,
         WebFeature::kCrossOriginTextScript,
         kNone,  // kTypeTextCsv,
         WebFeature::kCrossOriginWorkerTextHtml,
         WebFeature::kCrossOriginWorkerTextPlain,
         WebFeature::kCrossOriginWorkerTextXml,
         kNone,  // kTypeVideo,
         kNone   // kTypeWhatever
     }},
    {{
         // Same origin, non-worker scope.
         WebFeature::kSameOriginApplicationScript,
         WebFeature::kSameOriginApplicationOctetStream,
         WebFeature::kSameOriginApplicationXml,
         kNone,  // kTypeAudio
         kNone,  // kTypeImage,
         WebFeature::kSameOriginTextScript,
         kNone,  // kTypeTextCsv,
         WebFeature::kSameOriginTextHtml, WebFeature::kSameOriginTextPlain,
         WebFeature::kSameOriginTextXml,
         kNone,  // kTypeVideo,
         kNone   // kTypeWhatever
     },
     {
         // Same origin, worker scope.
         WebFeature::kSameOriginApplicationScript,
         WebFeature::kSameOriginWorkerApplicationOctetStream,
         WebFeature::kSameOriginWorkerApplicationXml,
         kNone,  // kTypeAudio
         kNone,  // kTypeImage,
         WebFeature::kSameOriginTextScript,
         kNone,  // kTypeTextCsv,
         WebFeature::kSameOriginWorkerTextHtml,
         WebFeature::kSameOriginWorkerTextPlain,
         WebFeature::kSameOriginWorkerTextXml,
         kNone,  // kTypeVideo,
         kNone   // kTypeWhatever
     }}};

// This function will do various tests against the mimetype (either type or
// a specific subtype). Let's turn them into IDs and use those.
// Determine the MIME type and subtype (e.g. "text/" and "text/html"), and
// express it in terms of type_id. Use kTypeWhatever as 'other'.
//
void MimeTypeToTypeIds(const String& mime_type,
                       TypeIds& type_id,
                       TypeIds& subtype_id) {
  DCHECK_EQ(static_cast<size_t>(kTypeWhatever), arraysize(type_ids_strings));
  const auto begin = std::begin(type_ids_strings);
  const auto end = std::end(type_ids_strings);
  DCHECK(std::is_sorted(begin, end, [](const char* a, const char* b) {
    return strcmp(a, b) < 0;
  }));
  auto lower = std::lower_bound(
      begin, end, mime_type,
      [](const char* current, const WTF::String& mime_type) {
        return WTF::CodePointCompareIgnoringASCIICase(mime_type, current) > 0;
      });

  // 'lower' now points to the subtype (or an element after it).
  subtype_id = kTypeWhatever;
  if (lower != end && mime_type.StartsWithIgnoringASCIICase(*lower)) {
    subtype_id = static_cast<TypeIds>(lower - begin);
  }

  // Finding the type is a bit trickier: We need to search backwards until
  // we have an entry that ends in a slash. Since std::lower may also point
  // after the element (e.g. "image/png" will point after the "image/" entry)
  // we may have to check both the most recent element with '/', or the one
  // before.
  type_id = kTypeWhatever;
  if (lower != end) {
    while ((*lower)[strlen(*lower) - 1] != '/')
      lower--;
    if (mime_type.StartsWithIgnoringASCIICase(*lower)) {
      type_id = static_cast<TypeIds>(lower - begin);
    }
  }
  if (lower != begin && mime_type.StartsWithIgnoringASCIICase(*(--lower))) {
    type_id = static_cast<TypeIds>(lower - begin);
  }
}

}  // namespace

bool AllowedByNosniff::MimeTypeAllowedByNosniff(
    ExecutionContext* execution_context,
    const ResourceResponse& response) {
  String mime_type = response.HttpContentType();

  // Allowed by nosniff?
  bool mime_type_allowed_by_nosniff =
      ParseContentTypeOptionsHeader(response.HttpHeaderField(
          HTTPNames::X_Content_Type_Options)) != kContentTypeOptionsNosniff ||
      MIMETypeRegistry::IsSupportedJavaScriptMIMEType(mime_type);
  if (!mime_type_allowed_by_nosniff) {
    execution_context->AddConsoleMessage(ConsoleMessage::Create(
        kSecurityMessageSource, kErrorMessageLevel,
        "Refused to execute script from '" + response.Url().ElidedString() +
            "' because its MIME type ('" + mime_type +
            "') is not executable, and "
            "strict MIME type checking is "
            "enabled."));
    return false;
  }

  // Determine the MIME type and subtype (e.g. "text/" and "text/html"), and
  // express it in terms of type_id. Use kTypeWhatever as 'other'.
  TypeIds type_id, subtype_id;
  MimeTypeToTypeIds(mime_type, type_id, subtype_id);

  // Check for certain non-executable MIME types.
  if (type_id == kTypeImage || subtype_id == kTypeTextCsv ||
      type_id == kTypeAudio || type_id == kTypeVideo) {
    execution_context->AddConsoleMessage(ConsoleMessage::Create(
        kSecurityMessageSource, kErrorMessageLevel,
        "Refused to execute script from '" + response.Url().ElidedString() +
            "' because its MIME type ('" + mime_type +
            "') is not executable."));
    if (type_id == kTypeImage) {
      UseCounter::Count(execution_context,
                        WebFeature::kBlockedSniffingImageToScript);
    } else if (type_id == kTypeAudio) {
      UseCounter::Count(execution_context,
                        WebFeature::kBlockedSniffingAudioToScript);
    } else if (type_id == kTypeVideo) {
      UseCounter::Count(execution_context,
                        WebFeature::kBlockedSniffingVideoToScript);
    } else if (subtype_id == kTypeTextCsv) {
      UseCounter::Count(execution_context,
                        WebFeature::kBlockedSniffingCSVToScript);
    }
    return false;
  }

  // Beyond this point, we accept the given MIME type. Since we also accept
  // some legacy types, We want to log some of them using UseCounter to
  // support deprecation decisions.

  if (MIMETypeRegistry::IsSupportedJavaScriptMIMEType(mime_type))
    return true;

  if (type_id == kTypeText &&
      MIMETypeRegistry::IsLegacySupportedJavaScriptLanguage(
          mime_type.Substring(5)))
    return true;

  bool same_origin =
      execution_context->GetSecurityOrigin()->CanRequest(response.Url());
  bool worker_global_scope = execution_context->IsWorkerGlobalScope();
  const auto& webfeatures = features_for_id[same_origin][worker_global_scope];
  if (webfeatures[type_id] != kNone) {
    UseCounter::Count(execution_context, webfeatures[type_id]);
  }
  if (webfeatures[subtype_id] != kNone) {
    UseCounter::Count(execution_context, webfeatures[subtype_id]);
  }
  return true;
}

}  // namespace blink
