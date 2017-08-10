// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_amp_converter.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "components/variations/variations_associated_data.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/url_constants.h"

namespace previews {

namespace {

const char kAMPRedirectionFieldTrial[] = "AMPRedirection";
const char kAMPRedirectionConfig[] = "config";

}  // namespace

PreviewsAMPConverter::PreviewsAMPConverter() {
  std::string config_text = variations::GetVariationParamValue(
      kAMPRedirectionFieldTrial, kAMPRedirectionConfig);
  if (config_text.empty())
    return;

  std::unique_ptr<base::Value> value = base::JSONReader::Read(config_text);
  const base::ListValue* entries = nullptr;
  if (!value || !value->GetAsList(&entries))
    return;

  re2::RE2::Options options(re2::RE2::DefaultOptions);
  options.set_case_sensitive(true);

  for (const auto& entry : *entries) {
    const base::DictionaryValue* dict = nullptr;
    if (!entry.GetAsDictionary(&dict))
      continue;
    std::string host, pattern, host_amp, prefix, suffix, suffix_html;
    dict->GetString("host", &host);
    dict->GetString("pattern", &pattern);
    dict->GetString("hostamp", &host_amp);
    dict->GetString("prefix", &prefix);
    dict->GetString("suffix", &suffix);
    dict->GetString("suffixhtml", &suffix_html);
    std::unique_ptr<re2::RE2> pattern_re2(
        base::MakeUnique<re2::RE2>(pattern, options));
    if (host.empty() || !pattern_re2->ok())
      continue;
    amp_converter_.insert(std::make_pair(
        host,
        base::MakeUnique<AMPConverterEntry>(std::move(pattern_re2), host_amp,
                                            prefix, suffix, suffix_html)));
  }
}

PreviewsAMPConverter::~PreviewsAMPConverter() {}

bool PreviewsAMPConverter::GetAMPURL(const GURL& url, GURL* new_amp_url) const {
  if (!url.is_valid() ||
      (!url.SchemeIs(url::kHttpsScheme) && !url.SchemeIs(url::kHttpScheme))) {
    return false;
  }

  const auto amp_converter_iter = amp_converter_.find(url.host());
  if (amp_converter_iter == amp_converter_.end() ||
      !re2::RE2::FullMatch(url.path(), *amp_converter_iter->second->pattern))
    return false;
  const auto& entry = *amp_converter_iter->second;
  GURL amp_url(url);
  GURL::Replacements url_replacements;
  if (!entry.hostname_amp.empty()) {
    url_replacements.SetHostStr(entry.hostname_amp);
  }
  if (!entry.prefix.empty() || !entry.suffix.empty() ||
      !entry.suffix_html.empty()) {
    std::string path = entry.prefix + url.path() + entry.suffix;
    if (!entry.suffix_html.empty() &&
        base::EndsWith(path, ".html", base::CompareCase::SENSITIVE)) {
      path.insert(path.length() - 5, entry.suffix_html);
    }
    url_replacements.SetPathStr(path);
  }
  amp_url = amp_url.ReplaceComponents(url_replacements);
  if (!amp_url.is_valid())
    return false;
  new_amp_url->Swap(&amp_url);
  return true;
}

PreviewsAMPConverter::AMPConverterEntry::AMPConverterEntry(
    std::unique_ptr<re2::RE2> pattern,
    const std::string& hostname_amp,
    const std::string& prefix,
    const std::string& suffix,
    const std::string& suffix_html)
    : pattern(std::move(pattern)),
      hostname_amp(hostname_amp),
      prefix(prefix),
      suffix(suffix),
      suffix_html(suffix_html) {}

PreviewsAMPConverter::AMPConverterEntry::~AMPConverterEntry() {}

}  // namespace previews
