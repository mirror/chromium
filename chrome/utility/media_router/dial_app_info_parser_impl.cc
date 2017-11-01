// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/media_router/dial_app_info_parser_impl.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/libxml/chromium/libxml_utils.h"

namespace {

chrome::mojom::DialAppState ParseDialAppState(const std::string& app_state) {
  if (app_state == "running") {
    return chrome::mojom::DialAppState::RUNNING;
  } else if (app_state == "stopped") {
    return chrome::mojom::DialAppState::STOPPED;
  } else {
    return chrome::mojom::DialAppState::ERROR;
  }
}

}  // namespace

namespace media_router {

DialAppInfoParserImpl::DialAppInfoParserImpl() = default;
DialAppInfoParserImpl::~DialAppInfoParserImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

// static
void DialAppInfoParserImpl::Create(
    chrome::mojom::DialAppInfoParserRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<DialAppInfoParserImpl>(),
                          std::move(request));
}

void DialAppInfoParserImpl::ParseDialAppInfo(
    const std::string& app_info_xml_data,
    ParseDialAppInfoCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!callback.is_null());

  chrome::mojom::DialAppInfoParsingError parsing_error =
      chrome::mojom::DialAppInfoParsingError::NONE;
  chrome::mojom::DialAppInfoPtr app_info =
      Parse(app_info_xml_data, &parsing_error);
  std::move(callback).Run(std::move(app_info), parsing_error);
}

chrome::mojom::DialAppInfoPtr DialAppInfoParserImpl::Parse(
    const std::string& xml,
    chrome::mojom::DialAppInfoParsingError* parsing_error) {
  *parsing_error = chrome::mojom::DialAppInfoParsingError::NONE;

  XmlReader xml_reader;
  if (!xml_reader.Load(xml)) {
    *parsing_error = chrome::mojom::DialAppInfoParsingError::INVALID_XML;
    return nullptr;
  }

  int service_count = 0;
  chrome::mojom::DialAppInfoPtr out = chrome::mojom::DialAppInfo::New();
  out->state = chrome::mojom::DialAppState::ERROR;

  while (xml_reader.Read()) {
    xml_reader.SkipToElement();
    std::string node_name(xml_reader.NodeName());

    if (node_name == "service") {
      ++service_count;
    } else if (node_name == "state") {
      std::string app_state;
      if (!xml_reader.ReadElementContent(&app_state))
        continue;
      out->state = ParseDialAppState(app_state);
    } else if (node_name == "name") {
      std::string name;
      if (!xml_reader.ReadElementContent(&name))
        continue;
      out->name = name;
    } else if (node_name == "capabilities") {
      std::string capabilities;
      if (!xml_reader.ReadElementContent(&capabilities))
        continue;
      out->capabilities = capabilities;
    }
  }

  // Should only have one start tag <service> and one end tag </service>.
  if (service_count != 2) {
    *parsing_error = chrome::mojom::DialAppInfoParsingError::INVALID_SERVICE;
    return nullptr;
  }

  // Validate mandatory fields (name, state).
  if (out->name.empty() || out->name == "unknown") {
    *parsing_error =
        chrome::mojom::DialAppInfoParsingError::GET_RESPONSE_MISSING_NAME_VALUE;
    return nullptr;
  }

  if (out->state != chrome::mojom::DialAppState::RUNNING &&
      out->state != chrome::mojom::DialAppState::STOPPED) {
    *parsing_error = chrome::mojom::DialAppInfoParsingError::
        GET_RESPONSE_HAS_INVALID_STATE_VALUE;
    return nullptr;
  }

  return out;
}

}  // namespace media_router
