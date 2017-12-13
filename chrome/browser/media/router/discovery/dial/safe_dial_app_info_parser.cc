// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/safe_dial_app_info_parser.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/unguessable_token.h"
#include "services/data_decoder/public/cpp/safe_xml_parser.h"
#include "url/gurl.h"

namespace media_router {

namespace {

DialAppState ParseDialAppState(const std::string& app_state) {
  if (app_state == "running") {
    return DialAppState::RUNNING;
  } else if (app_state == "stopped") {
    return DialAppState::STOPPED;
  }
  return DialAppState::UNKNOWN;
}

}  // namespace

// Note we generate a random batch ID so that parsing operations started from
// this SafeDialAppInfoParser instance run in the same utility process.
SafeDialAppInfoParser::SafeDialAppInfoParser(
    service_manager::Connector* connector)
    : connector_(connector),
      xml_parser_batch_id_(base::UnguessableToken::Create().ToString()),
      weak_factory_(this) {}

SafeDialAppInfoParser::~SafeDialAppInfoParser() {}

void SafeDialAppInfoParser::Parse(const std::string& xml_text,
                                  ParseCallback callback) {
  DVLOG(2) << "Parsing app info...";
  DCHECK(callback);

  data_decoder::ParseXml(
      connector_, xml_text,
      base::BindOnce(&SafeDialAppInfoParser::OnXmlParsingDone,
                     weak_factory_.GetWeakPtr(), std::move(callback)),
      xml_parser_batch_id_);
}

void SafeDialAppInfoParser::OnXmlParsingDone(
    SafeDialAppInfoParser::ParseCallback callback,
    std::unique_ptr<base::Value> value,
    const base::Optional<std::string>& error) {
  if (!value || !value->is_dict()) {
    std::move(callback).Run(nullptr, ParsingError::kInvalidXML);
    return;
  }

  // NOTE: enforce namespace check for <service> element in future. Namespace
  // value will be "urn:dial-multiscreen-org:schemas:dial".
  bool unique_service = true;
  const base::Value* service_element =
      data_decoder::FindXmlElementPath(*value, {"service"}, &unique_service);
  if (!service_element || !unique_service) {
    std::move(callback).Run(nullptr, ParsingError::kInvalidXML);
    return;
  }

  // Read optional @dialVer.
  std::unique_ptr<ParsedDialAppInfo> app_info =
      base::MakeUnique<ParsedDialAppInfo>();
  app_info->dial_version =
      data_decoder::GetXmlElementAttribute(*service_element, "dialVer");

  // Fetch all the children of <service> element.
  const base::Value* child_elements =
      data_decoder::GetChildren(*service_element);
  if (!child_elements || !child_elements->is_list()) {
    std::move(callback).Run(nullptr, ParsingError::kInvalidXML);
    return;
  }

  ParsingError parsing_error = ParsingError::kNone;
  for (const auto& child_element : child_elements->GetList()) {
    parsing_error = ProcessChildElement(child_element, app_info.get());
    if (parsing_error != ParsingError::kNone) {
      std::move(callback).Run(nullptr, parsing_error);
      return;
    }
  }

  // Validate mandatory fields (name, state).
  parsing_error = ValidateParsedAppInfo(*app_info);
  if (parsing_error != ParsingError::kNone) {
    std::move(callback).Run(nullptr, parsing_error);
    return;
  }

  std::move(callback).Run(std::move(app_info), ParsingError::kNone);
}

SafeDialAppInfoParser::ParsingError SafeDialAppInfoParser::ProcessChildElement(
    const base::Value& child_element,
    ParsedDialAppInfo* out_app_info) {
  std::string tag_name;
  if (!data_decoder::GetXmlElementTagName(child_element, &tag_name))
    return ParsingError::kInvalidXML;

  if (tag_name == "name") {
    if (!data_decoder::GetXmlElementText(child_element, &out_app_info->name))
      return ParsingError::kFailToReadName;
  } else if (tag_name == "options") {
    out_app_info->allow_stop = data_decoder::GetXmlElementAttribute(
                                   child_element, "allowStop") != "false";
  } else if (tag_name == "link") {
    out_app_info->href =
        data_decoder::GetXmlElementAttribute(child_element, "href");
  } else if (tag_name == "state") {
    std::string state;
    if (!data_decoder::GetXmlElementText(child_element, &state))
      return ParsingError::kFailToReadState;
    out_app_info->state = ParseDialAppState(state);
  } else {
    if (!out_app_info->extra_data) {
      out_app_info->extra_data =
          base::MakeUnique<base::Value>(base::Value::Type::DICTIONARY);
    }
    out_app_info->extra_data->SetKey(tag_name, child_element.Clone());
  }

  return ParsingError::kNone;
}

SafeDialAppInfoParser::ParsingError
SafeDialAppInfoParser::ValidateParsedAppInfo(
    const ParsedDialAppInfo& app_info) {
  if (app_info.name.empty() || app_info.name == "unknown")
    return ParsingError::kMissingName;

  if (app_info.state == DialAppState::UNKNOWN)
    return ParsingError::kInvalidState;

  return ParsingError::kNone;
}

}  // namespace media_router
