// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_
#define SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/optional.h"

namespace base {
class Value;
}

namespace service_manager {
class Connector;
}

namespace data_decoder {

// Callback invoked when parsing with SafelyParseXml has finished.
// |value| contains the JSONified version of the parsed XML. See
// xml_parser.mojom for an example.
// If the parsing failed, |error| contains an error message and |value| is null.
using SafeXmlParserCallback =
    base::OnceCallback<void(std::unique_ptr<base::Value> value,
                            const base::Optional<std::string>& error)>;

// SafeXmlParser parses given XML text safely in a utility process and invokes
// |callback| when done. The XML returned in the callback is a JSONified version
// backed by a base::Value object.
// |connector| is the connector provided by the service manager and is used to
// retrieve the XML parser service. It's commonly retrieved from a service
// manager connection context object that the embedder provides.
void SafelyParseXml(service_manager::Connector* connector,
                    const std::string& unsafe_xml,
                    SafeXmlParserCallback callback);

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_
