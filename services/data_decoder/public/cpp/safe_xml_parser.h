// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_
#define SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_

#include <initializer_list>
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

// Callback invoked when parsing with ParseXml has finished.
// |value| contains the JSONified version of the parsed XML. See
// xml_parser.mojom for an example.
// If the parsing failed, |error| contains an error message and |value| is null.
using XmlParserCallback =
    base::OnceCallback<void(std::unique_ptr<base::Value> value,
                            const base::Optional<std::string>& error)>;

// Parses |unsafe_xml| safely in a utility process and invokes |callback| when
// done. The XML returned in the callback is a JSONified version backed by a
// base::Value object.
// |connector| is the connector provided by the service manager and is used to
// retrieve the XML parser service. It's commonly retrieved from a service
// manager connection context object that the embedder provides.
void ParseXml(service_manager::Connector* connector,
              const std::string& unsafe_xml,
              XmlParserCallback callback);

// Below are convenience methods for handling the elements returned by
// ParseXml().

// Returns true of |element| is an XML element with a tag name |name|, false
// otherwise.
bool IsXmlElementNamed(const base::Value& element, const std::string& name);

// Sets |name| with the tag name  of |element| and returns true.
// Returns false if |element| does not represent a valid XML element.
bool GetXmlElementTagName(const base::Value& element, std::string* name);

// Sets |text| with the text of |element| and returns true.
// Returns false if |element| does not contain any text.
bool GetXmlElementText(const base::Value& element, std::string* text);

// Returns the number of children of |element| named |name|.
int GetXmlElementChildrenCount(const base::Value& element,
                               const std::string& name);

// Returns the first child of |element| with the name |name|, or null if there
// are no children with that name.
const base::Value* GetXmlElementChildWithName(const base::Value& element,
                                              const std::string& name);

const base::Value* FindXmlElementPath(
    const base::Value& element,
    std::initializer_list<base::StringPiece> path,
    bool* unique_path);

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_
