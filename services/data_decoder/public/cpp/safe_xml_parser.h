// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_
#define SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_

#include <map>
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

// Below are conveniance methods for accessing the base::Value tree provided by
// SafelyParseXml().

// Returns the first text value of |node| or an empty string if not a text node.
// Note that there might be more than 1 text value for this node, this method
// returns only the first one. For example <a><b>text1</b><b>text2</b></a>
// gets parsed to {"a": { "b": ["text1, "text2"]}} and GetXmlText on node "b"
// returns "text1". Use GetXmlTextValues() to get all text values.
// std::string GetXmlText(base::Value* node);

// Populates |text_values| with all the text value of |node|. Returns true if
// there was at least one text value, false otherwise.
// bool GetXmlTextValues(base::Value* node, std::vector<std::string>
// text_values);

// Conveniance method that retrieves the a specific attribute stored in a node
// of XML content parsed with SafelyParseXml(). Returns the attribute for |node|
// associated with |key|, or an empty string if |node| is not a dictionary or
// there are no such attribute.
std::string GetXmlNodeAttribute(base::Value* node, const std::string& key);

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_
