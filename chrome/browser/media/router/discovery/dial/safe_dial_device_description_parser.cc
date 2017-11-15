// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/safe_dial_device_description_parser.h"

#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "services/data_decoder/public/cpp/safe_xml_parser.h"

namespace media_router {

namespace {

// If friendly name does not exist, fall back to use model name + last 4
// digits of UUID as friendly name.
std::string ComputeFriendlyName(const std::string& unique_id,
                                const std::string& model_name) {
  if (model_name.empty() || unique_id.length() < 4)
    return std::string();

  std::string trimmed_unique_id = unique_id.substr(unique_id.length() - 4);
  return base::StringPrintf("%s [%s]", model_name.c_str(),
                            trimmed_unique_id.c_str());
}

// Returns the string content of |value| if it is a string value, or of the
// first entry in the list if Value is a list containing a string.
// Returns an empty string otherwise.
std::string GetTextFromNode(const base::Value& value) {
  if (value.is_string())
    return value.GetString();

  if (!value.is_list())
    return "";

  const base::Value::ListStorage& list = value.GetList();
  if (list.empty())
    return "";

  const base::Value& item = list.at(0);
  return item.is_string() ? item.GetString() : "";
}

}  // namespace

SafeDialDeviceDescriptionParser::SafeDialDeviceDescriptionParser()
    : weak_factory_(this) {}

SafeDialDeviceDescriptionParser::~SafeDialDeviceDescriptionParser() = default;

void SafeDialDeviceDescriptionParser::Start(
    service_manager::Connector* connector,
    const std::string& xml_text,
    DeviceDescriptionCallback callback) {
  DVLOG(2) << "Start parsing device description...";

  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  data_decoder::SafelyParseXml(
      connector, xml_text,
      base::BindOnce(&SafeDialDeviceDescriptionParser::OnXmlParsingDone,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void SafeDialDeviceDescriptionParser::OnXmlParsingDone(
    DeviceDescriptionCallback callback,
    std::unique_ptr<base::Value> value,
    const base::Optional<std::string>& error) {
  if (!value || !value->is_dict()) {
    std::move(callback).Run(ParsedDialDeviceDescription(),
                            ParsingError::kInvalidXml);
    return;
  }

  base::DictionaryValue* root_node = nullptr;
  for (const auto& item : value->DictItems()) {
    if (item.second.GetAsDictionary(&root_node))
      break;
  }
  if (!root_node) {
    std::move(callback).Run(ParsedDialDeviceDescription(),
                            ParsingError::kInvalidXml);
    return;
  }

  base::DictionaryValue* device_node = nullptr;
  for (const auto& item : root_node->DictItems()) {
    if (item.first == "device") {
      if (item.second.GetAsDictionary(&device_node))
        break;
    }
  }

  if (!device_node) {
    std::move(callback).Run(ParsedDialDeviceDescription(),
                            ParsingError::kInvalidXml);
    return;
  }

  ParsedDialDeviceDescription device_description;
  ParsingError parsing_error =
      SafeDialDeviceDescriptionParser::ParsingError::kNone;
  for (const auto& item : device_node->DictItems()) {
    if (item.first == "UDN") {
      DCHECK(device_description.unique_id.empty());
      device_description.unique_id = GetTextFromNode(item.second);
      if (device_description.unique_id.empty())
        parsing_error =
            SafeDialDeviceDescriptionParser::ParsingError::kFailedToReadUdn;
    } else if (item.first == "friendlyName") {
      DCHECK(device_description.friendly_name.empty());
      device_description.friendly_name = GetTextFromNode(item.second);
      if (device_description.friendly_name.empty())
        parsing_error = SafeDialDeviceDescriptionParser::ParsingError::
            kFailedToReadFriendlyName;
    } else if (item.first == "modelName") {
      DCHECK(device_description.model_name.empty());
      device_description.model_name = GetTextFromNode(item.second);
      if (device_description.model_name.empty())
        parsing_error = SafeDialDeviceDescriptionParser::ParsingError::
            kFailedToReadModelName;
    } else if (item.first == "deviceType") {
      DCHECK(device_description.device_type.empty());
      device_description.device_type = GetTextFromNode(item.second);
      if (device_description.device_type.empty())
        parsing_error = SafeDialDeviceDescriptionParser::ParsingError::
            kFailedToReadDeviceType;
    }
  }

  if (device_description.friendly_name.empty()) {
    device_description.friendly_name = ComputeFriendlyName(
        device_description.unique_id, device_description.model_name);
  }

  std::move(callback).Run(device_description, parsing_error);
}

}  // namespace media_router
