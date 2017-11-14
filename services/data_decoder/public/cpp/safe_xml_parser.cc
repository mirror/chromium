// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/data_decoder/public/cpp/safe_xml_parser.h"

#include "base/callback.h"
#include "base/sequenced_task_runner.h"
//#include "base/strings/string_number_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/unguessable_token.h"
#include "base/values.h"
#include "services/data_decoder/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/interfaces/constants.mojom.h"

namespace data_decoder {

SafeXmlParser::SafeXmlParser(service_manager::Connector* connector,
                             const std::string& unsafe_xml,
                             Callback callback)
    : unsafe_xml_(unsafe_xml),
      callback_(std::move(callback)),
      connector_(connector) {
  DCHECK(callback_);  // Parsing without ca callback is useless.
}

SafeXmlParser::~SafeXmlParser() = default;

void SafeXmlParser::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!xml_parser_ptr_);

  // Use a random instance ID to guarantee the connection is to a new service
  // running in its own process.
  base::UnguessableToken token = base::UnguessableToken::Create();
  service_manager::Identity identity(mojom::kServiceName,
                                     service_manager::mojom::kInheritUserID,
                                     token.ToString());
  connector_->BindInterface(identity, &xml_parser_ptr_);

  // Unretained(this) is safe as the xml_parser_ptr_ is owner by this class (so
  // it will be deleted before the class is deleted).
  xml_parser_ptr_.set_connection_error_handler(base::BindOnce(
      &SafeXmlParser::ReportResults, base::Unretained(this),
      /*parsed_xml=*/nullptr,
      base::make_optional(
          std::string("Connection error with the XML parser process."))));
  xml_parser_ptr_->Parse(
      unsafe_xml_,
      base::BindOnce(&SafeXmlParser::ReportResults, base::Unretained(this)));
}

void SafeXmlParser::ReportResults(std::unique_ptr<base::Value> parsed_xml,
                                  const base::Optional<std::string>& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Shut down the utility process.
  xml_parser_ptr_.reset();

  std::move(callback_).Run(std::move(parsed_xml), error);
}

}  // namespace data_decoder
