// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/data_decoder/xml_parser.h"

#include <map>
#include <utility>

#include "base/values.h"
#include "third_party/libxml/chromium/libxml_utils.h"

namespace data_decoder {

using AttributeMap = std::map<std::string, std::string>;
using NamespaceMap = std::map<std::string, std::string>;

namespace {

void ReportError(XmlParser::ParseCallback callback, const std::string& error) {
  std::move(callback).Run(/*result=*/nullptr, base::make_optional(error));
}

// Creates and returns new element node with the tag name |name|.
base::Value CreateNewElement(const std::string& name) {
  base::Value element(base::Value::Type::DICTIONARY);
  element.SetKey(mojom::XmlParser::kTagKey, base::Value(name));
  return element;
}

// Sets the text of |node| to be |text|, replacing any existing text value.
void SetElementText(base::Value* element, const std::string& text) {
  DCHECK(element->is_dict());
  element->SetKey(mojom::XmlParser::kTextKey, base::Value(text));
}

// Adds |child| as a child of |element|, creating the children list if
// necessary. Returns a ponter to |child|.
base::Value* AddChildToElement(base::Value* element, base::Value child) {
  DCHECK(element->is_dict());
  base::Value* children = element->FindKey(mojom::XmlParser::kChildrenKey);
  DCHECK(!children || children->is_list());
  if (!children)
    children = element->SetKey(mojom::XmlParser::kChildrenKey,
                               base::Value(base::Value::Type::LIST));
  children->GetList().push_back(std::move(child));
  return &children->GetList().back();
}

void PopulateNamespaces(base::Value* node_value, XmlReader* xml_reader) {
  DCHECK(node_value->is_dict());
  NamespaceMap namespaces;
  if (!xml_reader->GetAllDeclaredNamespaces(&namespaces) || namespaces.empty())
    return;

  base::Value namespace_dict(base::Value::Type::DICTIONARY);
  for (auto ns : namespaces)
    namespace_dict.SetKey(ns.first, base::Value(ns.second));

  node_value->SetKey(mojom::XmlParser::kNamespacesKey,
                     std::move(namespace_dict));
}

void PopulateAttributes(base::Value* node_value, XmlReader* xml_reader) {
  DCHECK(node_value->is_dict());
  AttributeMap attributes;
  if (!xml_reader->GetAllNodeAttributes(&attributes) || attributes.empty())
    return;

  base::Value attribute_dict(base::Value::Type::DICTIONARY);
  for (auto attribute : attributes)
    attribute_dict.SetKey(attribute.first, base::Value(attribute.second));

  node_value->SetKey(mojom::XmlParser::kAttributesKey,
                     std::move(attribute_dict));
}

}  // namespace

XmlParser::XmlParser(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

XmlParser::~XmlParser() = default;

void XmlParser::Parse(const std::string& xml, ParseCallback callback) {
  XmlReader xml_reader;
  if (!xml_reader.Load(xml)) {
    ReportError(std::move(callback), "Invalid XML: failed to load");
    return;
  }

  base::Value root_element;
  std::vector<base::Value*> element_stack;
  while (xml_reader.Read()) {
    if (xml_reader.IsWhiteSpace() || xml_reader.IsComment())
      continue;

    if (xml_reader.IsClosingElement()) {
      if (element_stack.empty()) {
        ReportError(std::move(callback), "Invalid XML: unbalanced elements");
        return;
      }
      element_stack.pop_back();
      continue;
    }

    std::string text;
    base::Value* current_element =
        element_stack.empty() ? nullptr : element_stack.back();
    if (xml_reader.GetTextIfTextElement(&text)) {
      // Text node.
      SetElementText(current_element, text);
    } else {
      // Element node.
      base::Value new_element = CreateNewElement(xml_reader.NodeFullName());
      base::Value* new_element_ptr;
      if (current_element) {
        new_element_ptr =
            AddChildToElement(current_element, std::move(new_element));
      } else {
        // First element we are parsing, it becomes the root element.
        DCHECK(root_element.is_none());
        root_element = std::move(new_element);
        new_element_ptr = &root_element;
      }
      PopulateNamespaces(new_element_ptr, &xml_reader);
      PopulateAttributes(new_element_ptr, &xml_reader);
      // Self-closing (empty) element have no close tag (or children); don't
      // push them on the element stack.
      if (!xml_reader.IsEmptyElement())
        element_stack.push_back(new_element_ptr);
    }
  }

  if (!element_stack.empty()) {
    ReportError(std::move(callback), "Invalid XML: unbalanced elements");
    return;
  }
  base::DictionaryValue* dictionary = nullptr;
  root_element.GetAsDictionary(&dictionary);
  if (!dictionary || dictionary->empty()) {
    ReportError(std::move(callback), "Invalid XML: bad content");
    return;
  }
  std::move(callback).Run(
      base::Value::ToUniquePtrValue(std::move(root_element)),
      base::Optional<std::string>());
}

}  // namespace data_decoder
