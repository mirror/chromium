// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_parser.h"

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"

namespace content {

std::unique_ptr<blink::OriginManifest> OriginManifestParser::Parse(
    std::string json) {
  // TODO throw some meaningful errors when parsing does not go well
  std::unique_ptr<base::DictionaryValue> directives_dict =
      base::DictionaryValue::From(
          base::JSONReader::Read(json, base::JSON_ALLOW_TRAILING_COMMAS));

  std::unique_ptr<blink::OriginManifest> om;

  // TODO(dhausknecht) that's playing safe. But we should do something to tell
  // things went wrong here.
  if (directives_dict == nullptr)
    return om;

  om.reset(new blink::OriginManifest());

  for (base::detail::dict_iterator_proxy::iterator it =
           directives_dict->DictItems().begin();
       it != directives_dict->DictItems().end(); ++it) {
    switch (OriginManifestParser::GetDirectiveType(it->first)) {
      // content-security-policy
      case DirectiveType::kContentSecurityPolicy:
        OriginManifestParser::ParseContentSecurityPolicy(om.get(),
                                                         std::move(it->second));
        break;
      case DirectiveType::kUnknown:
        // just ignore. IMHO that's fair game
        break;
    }
  }

  return om;
}

OriginManifestParser::DirectiveType OriginManifestParser::GetDirectiveType(
    const std::string& name) {
  if (name == "content-security-policy") {
    return DirectiveType::kContentSecurityPolicy;
  }

  return DirectiveType::kUnknown;
}

void OriginManifestParser::ParseContentSecurityPolicy(
    blink::OriginManifest* const om,
    base::Value value) {
  // TODO give respective parsing errors
  if (!value.is_list())
    return;

  for (auto& elem : value.GetList()) {
    if (!elem.is_dict())
      continue;

    std::string policy = "";
    blink::OriginManifest::ContentSecurityPolicyType disposition =
        blink::OriginManifest::ContentSecurityPolicyType::kEnforce;
    blink::ActivationType activation_type = blink::ActivationType::kFallback;

    const base::Value* v = elem.FindKey("policy");
    if (v && v->is_string())
      policy = v->GetString();

    v = elem.FindKey("disposition");
    if (v && v->is_string())
      disposition = OriginManifestParser::GetCSPDisposition(v->GetString());

    v = elem.FindKey("allow-override");
    if (v && v->is_bool())
      activation_type =
          OriginManifestParser::GetCSPActivationType(v->GetBool());

    om->AddContentSecurityPolicy(policy, disposition, activation_type);
  }
}

blink::OriginManifest::ContentSecurityPolicyType
OriginManifestParser::GetCSPDisposition(const std::string& name) {
  if (name == "enforce")
    return blink::OriginManifest::ContentSecurityPolicyType::kEnforce;
  if (name == "report-only")
    return blink::OriginManifest::ContentSecurityPolicyType::kReport;

  return blink::OriginManifest::ContentSecurityPolicyType::kEnforce;
}

blink::ActivationType OriginManifestParser::GetCSPActivationType(
    const bool value) {
  if (value)
    return blink::ActivationType::kBaseline;

  return blink::ActivationType::kFallback;
}

}  // namespace content
