// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/data_decoder/xml_parser.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "third_party/libxml/chromium/libxml_utils.h"

namespace data_decoder {

namespace {

void ReportError(XmlParser::ParseCallback callback, const std::string& error) {
  std::move(callback).Run(/*result=*/nullptr, base::make_optional(error));
}

// Removes the empty dictionary in |node| if any associated with the key
// |node_name|. If a list is associated with that key, remove the last element
// from it if it's a dictionary and it's empty.
// Returns true if an empty dictionary was foudn and removed, false otherwise.
bool RemoveEmptyDictionary(base::Value* node,
                           const std::string& dictionary_name) {
  DCHECK(node->is_dict());
  base::Value* child = node->FindKey(dictionary_name);
  if (!child)
    return false;

  base::DictionaryValue* dictionary = nullptr;
  if (child->GetAsDictionary(&dictionary) && dictionary->empty()) {
    bool success = node->RemoveKey(dictionary_name);
    DCHECK(success);
    return true;
  }

  // Remove the empty dictionary if it was added to a list of elements.
  if (child->is_list()) {
    base::Value::ListStorage& list = child->GetList();
    if (list.empty() || !list.back().GetAsDictionary(&dictionary) ||
        !dictionary->empty()) {
      return false;
    }
    list.pop_back();
    return true;
  }
  return false;
}

// Associates |content| with |node_name| in the dictionary |parent_node|.
// Note that if there is already a single value associated with that key, it is
// replaced by a list containing the existing value and the added one. If there
// is already a list, the new value is added to that list.
// Returns a pointer to the added value.
base::Value* AddContentToNode(base::Value* parent_node,
                              const std::string& node_name,
                              base::Value content) {
  DCHECK(parent_node->is_dict());
  base::Value* node = parent_node->FindKey(node_name);
  if (!node) {
    // First time we populate that node, store the value as is.
    parent_node->SetKey(node_name, std::move(content));
    return parent_node->FindKey(node_name);
  }

  if (node->is_string() || node->is_dict()) {
    // There is already one value in this node, we need a list to store 2.
    base::Value list(base::Value::Type::LIST);
    // Note that cloning below might be costly with complex trees.
    list.GetList().push_back(node->Clone());
    list.GetList().push_back(std::move(content));
    bool success = parent_node->RemoveKey(node_name);
    DCHECK(success);
    base::Value* content_ptr = &list.GetList().back();
    parent_node->SetKey(node_name, std::move(list));
    return content_ptr;
  }
  if (node->is_list()) {
    // There are already other values in this node, just add the new one.
    node->GetList().push_back(std::move(content));
    return &node->GetList().back();
  }
  NOTREACHED();
  return nullptr;
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

  auto root = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
  std::vector<std::string> element_name_stack;
  std::vector<base::Value*> element_stack;
  element_stack.push_back(root.get());

  bool pop_element_stack = true;
  while (xml_reader.Read()) {
    if (xml_reader.IsWhiteSpace() || xml_reader.IsComment())
      continue;

    if (xml_reader.IsClosingElement()) {
      // No parent element gets pushed on the element_stack for nodes with no
      // children.
      if (pop_element_stack) {
        if (element_stack.size() <= 1U) {
          ReportError(std::move(callback), "Invalid XML: unbalanced elements");
          return;
        }
        element_stack.pop_back();
      }
      DCHECK(!element_name_stack.empty());  // No root element for names.
      element_name_stack.pop_back();
      pop_element_stack = true;
      continue;
    }

    pop_element_stack = true;
    std::string text;
    if (xml_reader.GetTextIfTextElement(&text)) {
      // Pop the empty Dictionary that was pushed when we parse the containing
      // node, it'll be replaced with the string.
      element_stack.pop_back();
      pop_element_stack = false;  // Don't pop element on the next close tag.
      // Remove the empty dictionary added when the node was parsed.
      bool success = RemoveEmptyDictionary(element_stack.back(),
                                           element_name_stack.back());
      DCHECK(success);
      // And replace it with the string.
      AddContentToNode(element_stack.back(), element_name_stack.back(),
                       base::Value(text));
    } else if (xml_reader.IsEmptyElement()) {
      if (element_name_stack.empty()) {
        // Special case of self-closing root node.
        root->SetKey(xml_reader.NodeName(),
                     base::Value(base::Value::Type::DICTIONARY));
      } else {
        AddContentToNode(element_stack.back(), xml_reader.NodeName(),
                         base::Value(base::Value::Type::DICTIONARY));
      }
    } else {
      const std::string& node_name = xml_reader.NodeName();
      base::Value* dictionary =
          AddContentToNode(element_stack.back(), node_name,
                           base::Value(base::Value::Type::DICTIONARY));
      element_stack.push_back(dictionary);
      element_name_stack.push_back(node_name);
    }
  }

  if (element_stack.size() != 1U || !element_name_stack.empty()) {
    ReportError(std::move(callback), "Invalid XML: unbalanced elements");
    return;
  }
  DCHECK_EQ(element_stack.size(), 1U);
  DCHECK(element_name_stack.empty());
  base::DictionaryValue* dictionary = nullptr;
  bool result = root->GetAsDictionary(&dictionary);
  DCHECK(result);
  if (dictionary->empty()) {
    ReportError(std::move(callback), "Invalid XML: no content");
    return;
  }
  std::move(callback).Run(std::move(root), base::Optional<std::string>());
}

}  // namespace data_decoder
