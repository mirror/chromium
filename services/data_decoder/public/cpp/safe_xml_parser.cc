// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/data_decoder/public/cpp/safe_xml_parser.h"

#include "base/callback.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "base/unguessable_token.h"
#include "base/values.h"
#include "services/data_decoder/public/interfaces/constants.mojom.h"
#include "services/data_decoder/public/interfaces/xml_parser.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/interfaces/constants.mojom.h"

namespace data_decoder {

namespace {

// Class that does the actual parsing. Deletes itself when parsing is done.
class SafeXmlParser {
 public:
  SafeXmlParser(service_manager::Connector* connector,
                const std::string& unsafe_xml,
                SafeXmlParserCallback callback);
  ~SafeXmlParser();

 private:
  void ReportResults(std::unique_ptr<base::Value> parsed_json,
                     const base::Optional<std::string>& error);

  SafeXmlParserCallback callback_;
  mojom::XmlParserPtr xml_parser_ptr_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SafeXmlParser);
};

SafeXmlParser::SafeXmlParser(service_manager::Connector* connector,
                             const std::string& unsafe_xml,
                             SafeXmlParserCallback callback)
    : callback_(std::move(callback)) {
  DCHECK(callback_);  // Parsing without ca callback is useless.

  // Use a random instance ID to guarantee the connection is to a new service
  // running in its own process.
  base::UnguessableToken token = base::UnguessableToken::Create();
  service_manager::Identity identity(mojom::kServiceName,
                                     service_manager::mojom::kInheritUserID,
                                     token.ToString());
  connector->BindInterface(identity, &xml_parser_ptr_);

  // Unretained(this) is safe as the xml_parser_ptr_ is owned by this class.
  xml_parser_ptr_.set_connection_error_handler(base::BindOnce(
      &SafeXmlParser::ReportResults, base::Unretained(this),
      /*parsed_xml=*/nullptr,
      base::make_optional(
          std::string("Connection error with the XML parser process."))));
  xml_parser_ptr_->Parse(
      unsafe_xml,
      base::BindOnce(&SafeXmlParser::ReportResults, base::Unretained(this)));
}

SafeXmlParser::~SafeXmlParser() = default;

void SafeXmlParser::ReportResults(std::unique_ptr<base::Value> parsed_xml,
                                  const base::Optional<std::string>& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::move(callback_).Run(std::move(parsed_xml), error);

  // This should be the last interaction with this instance, safely delete.
  delete this;
}

}  // namespace

void SafelyParseXml(service_manager::Connector* connector,
                    const std::string& unsafe_xml,
                    SafeXmlParserCallback callback) {
  new SafeXmlParser(connector, unsafe_xml, std::move(callback));
}

// std::string GetXmlNodeText(base::Value* node) {

// }

// bool GetXmlTextValues(base::Value* node, std::vector<std::string>
// text_values) {

// }

std::string GetXmlNodeAttribute(base::Value* node, const std::string& key) {
  if (!node || !node->is_dict())
    return "";

  base::Value* attributes =
      node->FindKeyOfType(key, base::Value::Type::DICTIONARY);
  if (!attributes)
    return "";

  base::Value* value =
      attributes->FindKeyOfType(key, base::Value::Type::STRING);
  return value ? value->GetString() : "";
}

}  // namespace data_decoder
