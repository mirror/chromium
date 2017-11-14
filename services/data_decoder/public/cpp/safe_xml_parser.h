// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_
#define SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/threading/thread_checker.h"
#include "services/data_decoder/public/interfaces/xml_parser.mojom.h"

namespace base {
class Value;
}

namespace service_manager {
class Connector;
}

namespace data_decoder {

// SafeXmlParser parses given XML text safely in a utility process.
// The XML is returned in a JSONified version backed by a base::Value object.
class SafeXmlParser {
 public:
  using Callback =
      base::OnceCallback<void(std::unique_ptr<base::Value> value,
                              const base::Optional<std::string>& error)>;

  // Creates a SafeXmlParser to parse |unsafe_xml|. It calls |callback| when
  // finished. Note that you must call Start() to start the parsing.
  // |connector| is the connector provided by the service manager and is used
  // to retrieve the XML parser service. It's commonly retrieved from a
  // service manager connection context object that the embedder provides.
  SafeXmlParser(service_manager::Connector* connector,
                const std::string& unsafe_xml,
                Callback callback);
  ~SafeXmlParser();

  // Starts the parsing.
  void Start();

 private:
  void ReportResults(std::unique_ptr<base::Value> parsed_json,
                     const base::Optional<std::string>& error);

  std::string unsafe_xml_;
  Callback callback_;
  service_manager::Connector* connector_;
  mojom::XmlParserPtr xml_parser_ptr_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SafeXmlParser);
};

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_
