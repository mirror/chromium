// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/cross_site_document_classifier.h"

#include <stddef.h>
#include <string>

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/resource_response_info.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/http/http_response_headers.h"

using base::StringPiece;

namespace content {

namespace {

// MIME types
const char kTextHtml[] = "text/html";
const char kTextXml[] = "text/xml";
const char kAppRssXml[] = "application/rss+xml";
const char kAppXml[] = "application/xml";
const char kAppJson[] = "application/json";
const char kTextJson[] = "text/json";
const char kTextXjson[] = "text/x-json";
const char kTextPlain[] = "text/plain";

void AdvancePastWhitespace(StringPiece* data) {
  size_t offset = data->find_first_not_of(" \t\r\n");
  // There is no not-whitespace character in this document.
  if (offset == base::StringPiece::npos) {
    data->clear();
  } else {
    data->remove_prefix(offset);
  }
}

bool MatchesSignature(StringPiece* data,
                      const StringPiece signatures[],
                      size_t arr_size) {
  for (size_t i = 0; i < arr_size; ++i) {
    if (signatures[i].length() <= data->length()) {
      if (base::StartsWith(*data, signatures[i],
                           base::CompareCase::INSENSITIVE_ASCII)) {
        data->remove_prefix(signatures[i].length());
        return true;
      }
    } else {
      // When |data| is a prefix of |signatures[i]| ...
      if (base::StartsWith(signatures[i], *data,
                           base::CompareCase::INSENSITIVE_ASCII)) {
        data->clear();
        return false;
      }
    }
  }
  return false;
}

}  // namespace

CrossSiteDocumentMimeType CrossSiteDocumentClassifier::GetCanonicalMimeType(
    const std::string& mime_type) {
  if (base::LowerCaseEqualsASCII(mime_type, kTextHtml)) {
    return CROSS_SITE_DOCUMENT_MIME_TYPE_HTML;
  }

  if (base::LowerCaseEqualsASCII(mime_type, kTextPlain)) {
    return CROSS_SITE_DOCUMENT_MIME_TYPE_PLAIN;
  }

  if (base::LowerCaseEqualsASCII(mime_type, kAppJson) ||
      base::LowerCaseEqualsASCII(mime_type, kTextJson) ||
      base::LowerCaseEqualsASCII(mime_type, kTextXjson)) {
    return CROSS_SITE_DOCUMENT_MIME_TYPE_JSON;
  }

  if (base::LowerCaseEqualsASCII(mime_type, kTextXml) ||
      base::LowerCaseEqualsASCII(mime_type, kAppRssXml) ||
      base::LowerCaseEqualsASCII(mime_type, kAppXml)) {
    return CROSS_SITE_DOCUMENT_MIME_TYPE_XML;
  }

  return CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS;
}

bool CrossSiteDocumentClassifier::IsBlockableScheme(const GURL& url) {
  // We exclude ftp:// from here. FTP doesn't provide a Content-Type
  // header which our policy depends on, so we cannot protect any
  // document from FTP servers.
  return url.SchemeIs(url::kHttpScheme) || url.SchemeIs(url::kHttpsScheme);
}

bool CrossSiteDocumentClassifier::IsSameSite(const url::Origin& frame_origin,
                                             const GURL& response_url) {
  if (frame_origin.unique() || !response_url.is_valid())
    return false;

  if (frame_origin.scheme() != response_url.scheme())
    return false;

  // SameDomainOrHost() extracts the effective domains (public suffix plus one)
  // from the two URLs and compare them.
  return net::registry_controlled_domains::SameDomainOrHost(
      response_url, frame_origin,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

// We don't use Webkit's existing CORS policy implementation since
// their policy works in terms of origins, not sites. For example,
// when frame is sub.a.com and it is not allowed to access a document
// with sub1.a.com. But under Site Isolation, it's allowed.
bool CrossSiteDocumentClassifier::IsValidCorsHeaderSet(
    const url::Origin& frame_origin,
    const GURL& website_origin,
    const std::string& access_control_origin) {
  // Many websites are sending back "\"*\"" instead of "*". This is
  // non-standard practice, and not supported by Chrome. Refer to
  // CrossOriginAccessControl::passesAccessControlCheck().

  // Note that "null" offers no more protection than "*" because it matches any
  // unique origin, such as data URLs. Any origin can thus access it, so don't
  // bother trying to block this case.

  // TODO(dsjang): * is not allowed for the response from a request
  // with cookies. This allows for more than what the renderer will
  // eventually be able to receive, so we won't see illegal cross-site
  // documents allowed by this. We have to find a way to see if this
  // response is from a cookie-tagged request or not in the future.
  if (access_control_origin == "*" || access_control_origin == "null")
    return true;

  return IsSameSite(frame_origin, GURL(access_control_origin));
}

// This function is a slight modification of |net::SniffForHTML|.
CrossSiteDocumentClassifier::Result CrossSiteDocumentClassifier::SniffForHTML(
    StringPiece data) {
  // The content sniffer used by Chrome and Firefox are using "<!--"
  // as one of the HTML signatures, but it also appears in valid
  // JavaScript, considered as well-formed JS by the browser.  Since
  // we do not want to block any JS, we exclude it from our HTML
  // signatures. This can weaken our document block policy, but we can
  // break less websites.
  // TODO(dsjang): parameterize |net::SniffForHTML| with an option
  // that decides whether to include <!-- or not, so that we can
  // remove this function.
  // TODO(dsjang): Once CrossSiteDocumentClassifier is moved into the browser
  // process, we should do single-thread checking here for the static
  // initializer.
  static const StringPiece kHtmlSignatures[] = {
    StringPiece("<!doctype html"),  // HTML5 spec
    StringPiece("<script"),         // HTML5 spec, Mozilla
    StringPiece("<html"),           // HTML5 spec, Mozilla
    StringPiece("<head"),           // HTML5 spec, Mozilla
    StringPiece("<iframe"),         // Mozilla
    StringPiece("<h1"),             // Mozilla
    StringPiece("<div"),            // Mozilla
    StringPiece("<font"),           // Mozilla
    StringPiece("<table"),          // Mozilla
    StringPiece("<a"),              // Mozilla
    StringPiece("<style"),          // Mozilla
    StringPiece("<title"),          // Mozilla
    StringPiece("<b"),              // Mozilla
#if 0
      // TODO(nick): These are disabled because <b is a prefix of them.
      StringPiece("<body"),           // Mozilla
      StringPiece("<br"),             // Mozilla
#endif
    StringPiece("<p")  // Mozilla
  };

  while (data.length() > 0) {
    AdvancePastWhitespace(&data);

    if (MatchesSignature(&data, kHtmlSignatures, arraysize(kHtmlSignatures)))
      return Result::kYes;  // Found an HTML tag from the signature list.

    if (!data.length()) {
      // MatchesSignature consumed |data| but returned false, indicating that
      // the the addition of more bytes could cause a match to occur.
      break;
    }

    // "<!--" (the HTML comment syntax) is an indeterminate case, since it's
    // valid JS as well. Treat it as whitespace.
    static const StringPiece kBeginCommentSignature[] = {"<!--"};
    if (MatchesSignature(&data, kBeginCommentSignature,
                         arraysize(kBeginCommentSignature)) ||
        !data.length()) {
      static const StringPiece kEndComment = "-->";
      size_t comment_end = data.find(kEndComment);
      if (comment_end == base::StringPiece::npos) {
        break;  // Hit end of data with open comment.
      } else {
        data.remove_prefix(comment_end + kEndComment.length());
        continue;
      }
    } else {
      // Whatever this is, it's not HTML.
      return Result::kNo;
    }
  }

  // All of |data| was consumed, without a clear determination.
  return Result::kPossiblyWithMoreData;
}

CrossSiteDocumentClassifier::Result CrossSiteDocumentClassifier::SniffForXML(
    base::StringPiece data) {
  // TODO(dsjang): Chrome's mime_sniffer is using strncasecmp() for
  // this signature. However, XML is case-sensitive. Don't we have to
  // be more lenient only to block documents starting with the exact
  // string <?xml rather than <?XML ?
  // TODO(dsjang): Once CrossSiteDocumentClassifier is moved into the browser
  // process, we should do single-thread checking here for the static
  // initializer.
  AdvancePastWhitespace(&data);
  static const StringPiece kXmlSignatures[] = {StringPiece("<?xml")};
  if (MatchesSignature(&data, kXmlSignatures, arraysize(kXmlSignatures)))
    return Result::kYes;
  if (data.empty())
    return Result::kPossiblyWithMoreData;
  return Result::kNo;
}

CrossSiteDocumentClassifier::Result CrossSiteDocumentClassifier::SniffForJSON(
    base::StringPiece data) {
  // TODO(dsjang): We have to come up with a better way to sniff JSON. However,
  // even RE cannot help us that much due to the fact that we don't do full
  // parsing.  This DFA starts with state 0, and finds {, "/' and : in that
  // order.  We're avoiding adding a dependency on a regular expression library.
  enum {
    kStartState,
    kLeftBraceState,
    kLeftQuoteState,
    kEscapeState,
    kRightQuoteState,
  } state = kStartState;

  for (size_t i = 0; i < data.length(); ++i) {
    const char c = data[i];
    if (state != kLeftQuoteState && state != kEscapeState) {
      // Whitespace is ignored (outside of string literals)
      if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        continue;
    } else {
      // Inside string literals, control characters should result in rejection.
      if ((c >= 0 && c < 32) || c == 127)
        return Result::kNo;
    }

    switch (state) {
      case kStartState:
        if (c == '{')
          state = kLeftBraceState;
        else
          return Result::kNo;
        break;
      case kLeftBraceState:
        if (c == '"' || c == '\'')
          state = kLeftQuoteState;
        else
          return Result::kNo;
        break;
      case kLeftQuoteState:
        if (c == '"')
          state = kRightQuoteState;
        else if (c == '\\')
          state = kEscapeState;
        break;
      case kEscapeState:
        // Don't bother rejecting hex escapes.
        state = kLeftQuoteState;
        break;
      case kRightQuoteState:
        if (c == ':')
          return Result::kYes;
        else
          return Result::kNo;
        break;
    }
  }
  return Result::kPossiblyWithMoreData;
}

}  // namespace content
