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

// If a friendly name does not exist, falls back to use model name + last 4
// digits of the UUID as the friendly name.
std::string ComputeFriendlyName(const std::string& unique_id,
                                const std::string& model_name) {
  if (model_name.empty() || unique_id.length() < 4)
    return std::string();

  std::string trimmed_unique_id = unique_id.substr(unique_id.length() - 4);
  return base::StringPrintf("%s [%s]", model_name.c_str(),
                            trimmed_unique_id.c_str());
}

void NotifyParsingError(
    SafeDialDeviceDescriptionParser::DeviceDescriptionCallback callback,
    SafeDialDeviceDescriptionParser::ParsingError error) {
  std::move(callback).Run(ParsedDialDeviceDescription(), error);
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

  data_decoder::ParseXml(
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

  bool unique_device = true;
  const base::Value* device_element =
      data_decoder::FindXmlElementPath(*value, {"root", "device"}, &unique_device);
  if (!device_element) {
    NotifyParsingError(std::move(callback), ParsingError::kInvalidXml);
    return;
  }
  DCHECK(unique_device);

  ParsedDialDeviceDescription device_description;
  constexpr const char* kNodeNames[] =
      { "UDN", "friendlyName", "modelName", "deviceType"};
  constexpr ParsingError kParsingErros[] = {
      ParsingError::kFailedToReadUdn, ParsingError::kFailedToReadFriendlyName,
      ParsingError::kFailedToReadModelName,
      ParsingError::kFailedToReadDeviceType};
  std::string* const kFields[] = {
      &device_description.unique_id, &device_description.friendly_name,
      &device_description.model_name, &device_description.device_type};
  static_assert(arraysize(kNodeNames) == arraysize(kParsingErros) ||
                    arraysize(kParsingErros) == arraysize(kFields),
                "Inconsitent sizes for arrays.");

  for (size_t i = 0; i < arraysize(kNodeNames); i++) {
    const base::Value* value = data_decoder::GetXmlElementChildWithName(
        *device_element, kNodeNames[i]);
    if (value) {
      DCHECK_EQ(1, data_decoder::GetXmlElementChildrenCount(*device_element, kNodeNames[i]));
      bool result = data_decoder::GetXmlElementText(*value, kFields[i]);
      if (!result) {
        NotifyParsingError(std::move(callback), kParsingErros[i]);
        return;
      }
    }
  }

  if (device_description.friendly_name.empty()) {
    device_description.friendly_name = ComputeFriendlyName(
        device_description.unique_id, device_description.model_name);
  }

  std::move(callback).Run(device_description, ParsingError::kNone);
}

}  // namespace media_router
