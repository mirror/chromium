// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/devtools_page_analyser.h"

#include <stack>

#include "third_party/WebKit/public/web/WebConsoleMessage.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebFormControlElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebNode.h"

namespace autofill {

using ConsoleLevel = content::ConsoleMessageLevel;

class ConsoleLogger {
 public:
  ConsoleLogger(content::RenderFrame* render_frame)
      : render_frame_(render_frame) {}

  ConsoleLogger& Nodes(const blink::WebNode& node) {
    nodes_.assign({node});
    return *this;
  }

  ConsoleLogger& Nodes(std::vector<blink::WebNode>& nodes) {
    nodes_.clear();
    nodes_.insert(nodes_.end(), std::make_move_iterator(nodes.begin()),
                  std::make_move_iterator(nodes.end()));
    return *this;
  }

  void SendWarning(const std::string& message) { Send(kWarning, message); }

  void SendError(const std::string& message) { Send(kError, message); }

  void Flush() {
    for (ConsoleLevel level : {kError, kWarning}) {
      for (Entry& entry : node_buffer_[level])
        render_frame_->AddMessageToConsole(level, entry.message, entry.nodes);
    }
    node_buffer_.clear();
  }

 private:
  struct Entry {
    const std::string message;
    std::vector<blink::WebNode> nodes;
  };

  static const ConsoleLevel kWarning =
      ConsoleLevel::CONSOLE_MESSAGE_LEVEL_WARNING;

  static const ConsoleLevel kError = ConsoleLevel::CONSOLE_MESSAGE_LEVEL_ERROR;

  void Send(ConsoleLevel level, const std::string& message) {
    node_buffer_[level].push_back(Entry{message, nodes_});
  }

  content::RenderFrame* render_frame_;
  std::vector<blink::WebNode> nodes_;
  std::unordered_map<ConsoleLevel, std::vector<Entry>> node_buffer_;
};

void DevToolsPageAnalyser::AnalyseDocumentDOM(
    content::RenderFrame* render_frame) {
  ConsoleLogger console_logger(render_frame);
  blink::WebDocument document(render_frame->GetWebFrame()->GetDocument());
  blink::WebNode node(document.FirstChild());
  const std::vector<blink::WebNode> nodes{node};

  struct TraversalInfo {
    blink::WebNode node;
    bool in_form;
  };

  struct FormInputCollection {
    blink::WebFormElement form;
    std::vector<blink::WebFormControlElement> inputs;
    std::vector<size_t> username_inputs;
    std::vector<size_t> password_inputs;
    std::string signature;

    void AddInput(const blink::WebFormControlElement& input) {
      std::string type(input.GetAttribute("type").Utf8());
      signature += type != "password" ? "U" : "P";
      if (type != "password")
        username_inputs.push_back(inputs.size());
      else
        password_inputs.push_back(inputs.size());
      inputs.push_back(input);
    }
  };

  std::unordered_map<std::string, std::vector<blink::WebNode>> ids;
  std::set<std::string> duplicateIds;
  std::vector<FormInputCollection> forms;
  std::stack<TraversalInfo> stack;
  for (blink::WebNode child = document.FirstChild(); !child.IsNull();
       child = child.NextSibling())
    stack.push(TraversalInfo{child, 0});

  while (stack.size() > 0) {
    TraversalInfo traversal_info(stack.top());
    stack.pop();
    blink::WebNode node(traversal_info.node);
    if (node.IsElementNode()) {
      blink::WebElement element(node.To<blink::WebElement>());
      bool is_form_element = element.TagName() == "FORM";
      bool in_form = traversal_info.in_form;
      for (blink::WebNode child = element.LastChild(); !child.IsNull();
           child = child.PreviousSibling())
        stack.push(TraversalInfo{child, in_form || is_form_element});

      if (is_form_element)
        forms.push_back(FormInputCollection{node.To<blink::WebFormElement>()});

      if (element.HasAttribute("id")) {
        std::string id_attr = element.GetAttribute("id").Utf8();
        if (ids.count(id_attr))
          duplicateIds.insert(id_attr);
        ids[id_attr].push_back(element);
      }

      std::vector<std::string> typeAttributes{"text", "email", "tel",
                                              "password"};
      std::string type(element.GetAttribute("type").Utf8());
      if (element.TagName() == "INPUT" && element.HasAttribute("type") &&
          std::find(std::begin(typeAttributes), std::end(typeAttributes),
                    type) != std::end(typeAttributes)) {
        if (in_form)
          forms.back().AddInput(node.To<blink::WebFormControlElement>());
        else if (type == "password")
          console_logger.Nodes(node).SendWarning(
              "Password fields should be contained in forms:");
      }
    }
  }
  for (const std::string& id_attr : duplicateIds) {
    std::vector<blink::WebNode>& nodes = ids[id_attr];
    console_logger.Nodes(nodes).SendError(
        "The id attribute must be unique; there are " +
        std::to_string(nodes.size()) + " elements with id #" + id_attr + ":");
  }
  for (const FormInputCollection& form_input_collection : forms) {
    const blink::WebFormElement& form = form_input_collection.form;
    const std::vector<blink::WebFormControlElement>& inputs =
        form_input_collection.inputs;
    const std::vector<size_t>& username_inputs =
        form_input_collection.username_inputs;
    const std::vector<size_t>& password_inputs =
        form_input_collection.password_inputs;
    const std::string& signature = form_input_collection.signature;

    // We're only interested in forms that contain password fields.
    size_t password_count = password_inputs.size();
    if (password_count == 0)
      return;

    // If a username field is not first, or if there is no username field,
    // flag it as an issue.
    bool has_username_field = username_inputs.size() != 0;
    size_t username_field_guess;
    if (!has_username_field || username_inputs[0] > password_inputs[0]) {
      console_logger.Nodes(form).SendWarning(
          "Password forms should have (optionally hidden) "
          "username fields for accessibility:");
    } else
      for (username_field_guess = password_inputs[0] - 1;;
           --username_field_guess) {
        if (signature[username_field_guess] == 'U')
          break;
      }

    std::unordered_map<size_t, std::string> autocomplete_suggestions;
    if (has_username_field)
      autocomplete_suggestions[username_field_guess] = "username";

    unsigned kind_changes = 0;
    for (const char& kind : signature) {
      if (kind == (kind_changes & 1 ? 'U' : 'P'))
        ++kind_changes;
    }
    if (kind_changes >= 3 || password_count > 3) {
      console_logger.Nodes(form).SendWarning(
          "Multiple forms should be contained in their own form "
          "elements; break up complex forms into ones that represent a "
          "single action:");
      continue;
    }

    switch (password_inputs.size()) {
      case 3:
        autocomplete_suggestions[password_inputs[0]] = "current-password";
      // Fall-through here to match the last two password fields.
      case 2:
        autocomplete_suggestions[password_inputs[password_count - 2]] =
            "new-password";
        autocomplete_suggestions[password_inputs[password_count - 1]] =
            "new-password";
        break;
      case 1:
        autocomplete_suggestions[password_inputs[password_count - 1]] =
            has_username_field ? "current-password" : "new-password";
        break;
    }
    for (size_t i = 0; i < inputs.size(); ++i) {
      if (autocomplete_suggestions.count(i) &&
          !inputs[i].HasAttribute("autocomplete"))
        console_logger.Nodes(inputs[i]).SendWarning(
            "Input elements should have autocomplete attributes (suggested: " +
            autocomplete_suggestions[i] + "):");
    }
  }

  console_logger.Flush();
}

}  // namespace autofill