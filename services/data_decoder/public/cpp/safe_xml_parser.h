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

// Parses |unsafe_xml| using the data_decoder service which, whenever feasible,
// will be run in an isolated sandboxed process. Invokes |callback| when
// done, with a JSONified version of |unsafe_xml| backed by a base::Value
// object.
// |connector| is the Connector to use when connecting to the data_decoder
// service. This should be retrieved from whichever service is requesting the
// decode operation.
// If |batch_id| is non-empty, the system may batch this parse request with
// other parse requests using the same |batch_id| in an effort to amortize the
// overhead of a single request. The trade-off is that batch requests may not be
// well-isolated from each other, so this should be used with appropriate
// caution.
void ParseXml(service_manager::Connector* connector,
              const std::string& unsafe_xml,
              XmlParserCallback callback,
              const std::string& batch_id = "");

// Below are convenience methods for handling the elements returned by
// ParseXml().

// Returns true if |element| is an XML element with a tag name |name|, false
// otherwise.
bool IsXmlElementNamed(const base::Value& element, const std::string& name);

// Sets |name| with the tag name of |element| and returns true.
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

// Returns the value of the element path |path| starting at |element|, or null
// if there is element in the  path is missing. Note that if there are more than
// one element matching any part of the path, the first one is used and
// |unique_path| is set to false. It is set to true otherwise and can be null
// if not needed.
const base::Value* FindXmlElementPath(
    const base::Value& element,
    std::initializer_list<base::StringPiece> path,
    bool* unique_path);

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_XML_PARSER_H_
