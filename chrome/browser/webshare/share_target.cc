// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/webshare/share_target.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "net/base/escape.h"

namespace {

// Determines whether a character is allowed in a URL template placeholder.
bool IsIdentifier(char c) {
  return base::IsAsciiAlpha(c) || base::IsAsciiDigit(c) || c == '-' || c == '_';
}

// Writes to |url_template_filled|, a copy of |url_template| with all
// instances of "{title}", "{text}", and "{url}" replaced with
// |title|, |text|, and |url| respectively.
// Replaces instances of "{X}" where "X" is any string besides "title",
// "text", and "url", with an empty string, for forwards compatibility.
// Returns false, if there are badly nested placeholders.
// This includes any case in which two "{" occur before a "}", or a "}"
// occurs with no preceding "{".
bool ReplacePlaceholders(base::StringPiece url_template,
                                           base::StringPiece title,
                                           base::StringPiece text,
                                           const GURL& share_url,
                                           std::string* url_template_filled) {
  constexpr char kTitlePlaceholder[] = "title";
  constexpr char kTextPlaceholder[] = "text";
  constexpr char kUrlPlaceholder[] = "url";

  std::map<base::StringPiece, std::string> placeholder_to_data;
  placeholder_to_data[kTitlePlaceholder] =
      net::EscapeQueryParamValue(title, false);
  placeholder_to_data[kTextPlaceholder] =
      net::EscapeQueryParamValue(text, false);
  placeholder_to_data[kUrlPlaceholder] =
      net::EscapeQueryParamValue(share_url.spec(), false);

  std::vector<base::StringPiece> split_template;
  bool last_saw_open = false;
  size_t start_index_to_copy = 0;
  for (size_t i = 0; i < url_template.size(); ++i) {
    if (last_saw_open) {
      if (url_template[i] == '}') {
        base::StringPiece placeholder = url_template.substr(
            start_index_to_copy + 1, i - 1 - start_index_to_copy);
        auto it = placeholder_to_data.find(placeholder);
        if (it != placeholder_to_data.end()) {
          // Replace the placeholder text with the parameter value.
          split_template.push_back(it->second);
        }

        last_saw_open = false;
        start_index_to_copy = i + 1;
      } else if (!IsIdentifier(url_template[i])) {
        // Error: Non-identifier character seen after open.
        return false;
      }
    } else {
      if (url_template[i] == '}') {
        // Error: Saw close, with no corresponding open.
        return false;
      } else if (url_template[i] == '{') {
        split_template.push_back(
            url_template.substr(start_index_to_copy, i - start_index_to_copy));

        last_saw_open = true;
        start_index_to_copy = i;
      }
    }
  }
  if (last_saw_open) {
    // Error: Saw open that was never closed.
    return false;
  }
  split_template.push_back(url_template.substr(
      start_index_to_copy, url_template.size() - start_index_to_copy));

  *url_template_filled = base::JoinString(split_template, base::StringPiece());
  return true;
}

}  // namespace


ShareTarget::ShareTarget(GURL manifest_url,
                         std::string name,
                         std::string url_template)
    : manifest_url_(std::move(manifest_url)),
      name_(std::move(name)),
      url_template_(std::move(url_template)) {}

ShareTarget::~ShareTarget() {}

std::pair<bool, GURL> ShareTarget::GetTargetURL(const std::string& title,
                                                const std::string& text,
                                                const GURL& share_url) const {
  std::string url_template_filled;
  if (!ReplacePlaceholders(url_template_, title, text, share_url,
                           &url_template_filled)) {
    // TODO(mgiuca): This error should not be possible at share time, because
    // targets with invalid templates should not be chooseable. Fix
    // https://crbug.com/694380 and replace this with a DCHECK.
    return {false, GURL()};
  }

  const std::string& manifest_url_spec = manifest_url_.spec();
  base::StringPiece url_base(
      manifest_url_spec.data(),
      manifest_url_spec.size() - manifest_url_.ExtractFileName().size());
  const GURL target(url_base.as_string() + url_template_filled);

  return {true, std::move(target)};
}
