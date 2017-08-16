// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/devtools_page_analyser.h"

#include <stack>

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "third_party/WebKit/public/web/WebConsoleMessage.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebElementCollection.h"
#include "third_party/WebKit/public/web/WebFormControlElement.h"
#include "third_party/WebKit/public/web/WebLabelElement.h"
#include "third_party/WebKit/public/web/WebNode.h"
#include "third_party/re2/src/re2/re2.h"

#import <iostream>

namespace autofill {

using ConsoleLevel = blink::WebConsoleMessage::Level;

// ConsoleLogger provides a convenient interface for logging messages to the
// DevTools console.
class ConsoleLogger {
 public:
  ConsoleLogger(blink::WebLocalFrame* frame) : frame_(frame) {}

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
      for (Entry& entry : node_buffer_[level]) {
        frame_->AddMessageToConsole(blink::WebConsoleMessage(
            level, blink::WebString::FromUTF8(entry.message),
            std::move(entry.nodes)));
      }
    }
    node_buffer_.clear();
  }

 private:
  struct Entry {
    const std::string message;
    std::vector<blink::WebNode> nodes;
  };

  static const ConsoleLevel kWarning = blink::WebConsoleMessage::kLevelWarning;

  static const ConsoleLevel kError = blink::WebConsoleMessage::kLevelError;

  void Send(ConsoleLevel level, const std::string& message) {
    node_buffer_[level].push_back(Entry{message, nodes_});
  }

  blink::WebLocalFrame* frame_;
  std::vector<blink::WebNode> nodes_;
  std::unordered_map<ConsoleLevel, std::vector<Entry>> node_buffer_;
};

// Out-of-line definitions to make [chromium-style] happy.
DevToolsPageAnalyser::DevToolsPageAnalyser() {}

DevToolsPageAnalyser::~DevToolsPageAnalyser() {}

// Clear the set of nodes that have already been analysed, so that they will
// be analysed again next time |AnalyseDocumentDOM| is called. This is called
// upon page load, for instance.
void DevToolsPageAnalyser::Reset() {
  skip_nodes.clear();
}

// |AnalyseDocumentDOM| traverses the DOM, logging potential issues in the
// DevTools console. Errors are logged for those issues that conflict with the
// HTML specification. Warnings are logged for issues that cause problems with
// identification of fields on the web-page for the Password Manager.
void DevToolsPageAnalyser::AnalyseDocumentDOM(blink::WebLocalFrame* frame) {
  ConsoleLogger console_logger(frame);
  blink::WebDocument document(frame->GetDocument());
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

    // The signature of a form is a string of 'U's and 'P's, representing
    // username and password fields respectively. This is used to quickly match
    // against well-known <input> patterns to guess what kind of form we are
    // dealing with, and provide intelligent autocomplete suggestions.
    void AddInput(const blink::WebFormControlElement& input) {
      std::string type(input.GetAttribute("type").Utf8());
      signature += type != "password" ? 'U' : 'P';
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
  // Traverse the DOM depth-first (to provide a consistent node ordering).
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

      // We don't want to re-analyse the same nodes each time the method is
      // called. This technically means some warnings might be overlooked (for
      // example if an invalid attribute is added), but these cases are assumed
      // to be rare, and are ignored for the sake of simplicity.
      if (skip_nodes.count(node))
        continue;
      skip_nodes.insert(node);

      if (is_form_element)
        forms.push_back(FormInputCollection{node.To<blink::WebFormElement>()});

      // Duplicate id checking.
      if (element.HasAttribute("id")) {
        std::string id_attr = element.GetAttribute("id").Utf8();
        if (ids.count(id_attr))
          duplicateIds.insert(id_attr);
        ids[id_attr].push_back(element);
      }

      // We are only interested in a subset of input elements -- those likely
      // to be username or password fields.
      std::vector<std::string> typeAttributes{"text", "email", "tel",
                                              "password"};
      std::string type(element.GetAttribute("type").Utf8());
      if (element.TagName() == "INPUT" && element.HasAttribute("type") &&
          std::find(std::begin(typeAttributes), std::end(typeAttributes),
                    type) != std::end(typeAttributes)) {
        if (in_form)
          forms.back().AddInput(node.To<blink::WebFormControlElement>());
        else if (type == "password")
          // Orphan password field warning. Although password fields are
          // permitted outside forms, they make the Password Manager's job
          // significantly harder, as it has to guess what constitutes a single
          // semantic form from the DOM structure.
          console_logger.Nodes(node).SendWarning(
              "Password fields should be contained in forms:");
      }
    }
  }
  for (const std::string& id_attr : duplicateIds) {
    std::vector<blink::WebNode>& nodes = ids[id_attr];
    // Duplicate id attributes both are against the HTML specification and can
    // cause issues with password saving/filling, as the Password Manager makes
    // the assumption that ids may be used as a unique identifier for nodes.
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
      continue;

    // If a username field is not first, or if there is no username field,
    // flag it as an issue.
    bool has_username_field = username_inputs.size() != 0;
    size_t username_field_guess;
    if (!has_username_field || username_inputs[0] > password_inputs[0]) {
      // There is no formal requirement to have associated username fields for
      // every password field, but providing one ensures that the Password
      // Manager associates the correct account name with the password (for
      // example in password reset forms).
      console_logger.Nodes(form).SendWarning(
          "Password forms should have (optionally hidden) "
          "username fields for accessibility:");
    } else {
      // By default (if the other heuristics fail), the first text field
      // preceding a password field will be considered the username field.
      for (username_field_guess = password_inputs[0] - 1;;
           --username_field_guess) {
        if (signature[username_field_guess] == 'U')
          break;
      }
    }

    // The autocomplete attribute provides valuable hints to the Password
    // Manager as to the semantic structure of a form. Rather than simply point
    // out that an autocomplete attribute would be useful, we try to suggest the
    // intended value of the autocomplete attribute in order to save time for
    // the developer.
    std::unordered_map<size_t, std::string> autocomplete_suggestions;
    if (has_username_field) {
      blink::WebElementCollection labels(
          form.GetElementsByHTMLTagName("label"));
      DCHECK(!labels.IsNull());

      // The username field is the most difficult field to identify, as there
      // are often many other textual fields in a form, and it is not always
      // possible to work out which one is the username. Here, we find any
      // <label> elements pointing to the input fields, and check their content.
      // Labels containing text such as "Username:" or "Email address:" are
      // likely to indicate the desired field, and will be prioritised over
      // other fields.
      struct InputHint {
        std::unique_ptr<re2::RE2> regex;
        std::string tokens;
        size_t match;

        InputHint(std::string pattern) : match((size_t)-1) {
          re2::RE2::Options case_insensitivity;
          case_insensitivity.set_case_sensitive(false);
          regex = std::unique_ptr<re2::RE2>(
              new re2::RE2(pattern, case_insensitivity));
        }

        InputHint(std::string pattern, std::string tokens)
            : InputHint(pattern) {
          this->tokens += " " + tokens;
        }

        void MatchLabel(std::string& label_content, size_t index) {
          if (re2::RE2::FullMatch(label_content, *regex))
            match = index;
        }
      };

      re2::RE2 ignored_characters(R"(\W)");
      std::vector<InputHint> input_hints;
      std::string username_field_guess_tokens;

      input_hints.push_back(InputHint(R"(user(name)?|login)", ""));
      input_hints.push_back(InputHint(R"(email(address)?)", "email"));
      input_hints.push_back(
          InputHint(R"((mobile)?(telephone)?(number|no))", "tel"));

      for (blink::WebElement item = labels.FirstItem(); !item.IsNull();
           item = labels.NextItem()) {
        blink::WebLabelElement label(item.To<blink::WebLabelElement>());
        blink::WebElement control(label.CorrespondingControl());
        if (!control.IsNull() && control.IsFormControlElement()) {
          blink::WebFormControlElement form_control(
              control.To<blink::WebFormControlElement>());
          auto found = std::find(inputs.begin(), inputs.end(), form_control);
          if (found != inputs.end()) {
            std::string label_content(
                base::UTF16ToUTF8(form_util::FindChildText(label)));
            // Reduce to plain-text, as labels often contain extra punctuation.
            re2::RE2::GlobalReplace(&label_content, ignored_characters, "");
            for (InputHint& input_hint : input_hints)
              input_hint.MatchLabel(label_content, found - inputs.begin());
          }
        }
      }

      for (InputHint& input_hint : input_hints) {
        if (input_hint.match != (size_t)-1) {
          username_field_guess = input_hint.match;
          username_field_guess_tokens = input_hint.tokens;
          break;
        }
      }

      autocomplete_suggestions[username_field_guess] =
          "username" + username_field_guess_tokens;
    }

    unsigned kind_changes = 0;
    for (const char& kind : signature) {
      if (kind == (kind_changes & 1 ? 'U' : 'P'))
        ++kind_changes;
    }
    // Multiple semantic forms may be contained within a single <form> element,
    // which causes confusion to the Password Manager, which acts under the
    // assumption each <form> element corresponds to a single form. Here, we use
    // a simple heuristic to guess whether a form may contain too many inputs to
    // be considered a single form.
    if (kind_changes >= 3 || password_count > 3) {
      console_logger.Nodes(form).SendWarning(
          "Multiple forms should be contained in their own form "
          "elements; break up complex forms into ones that represent a "
          "single action:");
      continue;
    }

    // Infer what kind of form the current form corresponds to (e.g. a
    // registration, log-in or password reset form), based on the structure of
    // the form.
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

    // For each input element that is not annotated with an autocomplete
    // attribute, if we have a guess for what function the input serves, log
    // a warning, suggesting that the inferred attribute value should be added.
    for (size_t i = 0; i < inputs.size(); ++i) {
      if (autocomplete_suggestions.count(i) &&
          !inputs[i].HasAttribute("autocomplete"))
        console_logger.Nodes(inputs[i]).SendWarning(
            "Input elements should have autocomplete attributes (suggested: "
            "\"" +
            autocomplete_suggestions[i] + "\"):");
    }
  }

  // Finally, send all the warnings and errors to the console.
  console_logger.Flush();
}

}  // namespace autofill