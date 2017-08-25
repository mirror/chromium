// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/devtools_page_passwords_analyser.h"

#include <stack>

#include "base/lazy_instance.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "components/autofill/content/renderer/password_form_conversion_utils.h"
#include "content/renderer/devtools/devtools_agent.h"
#include "content/renderer/render_frame_impl.h"
#include "third_party/WebKit/public/web/WebConsoleMessage.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebElementCollection.h"
#include "third_party/WebKit/public/web/WebFormControlElement.h"
#include "third_party/WebKit/public/web/WebLabelElement.h"
#include "third_party/WebKit/public/web/WebNode.h"
#include "third_party/re2/src/re2/re2.h"

namespace autofill {

namespace {

const char* type_attributes[] = {"text", "email", "tel", "password"};

using ConsoleLevel = blink::WebConsoleMessage::Level;

// ConsoleLogger provides a convenient interface for logging messages to the
// DevTools console.
class ConsoleLogger {
 public:
  static const ConsoleLevel kError = blink::WebConsoleMessage::kLevelError;
  static const ConsoleLevel kWarning = blink::WebConsoleMessage::kLevelWarning;
  static const ConsoleLevel kVerbose = blink::WebConsoleMessage::kLevelVerbose;

  ConsoleLogger(blink::WebLocalFrame* frame) : frame_(frame) {}

  void Send(const std::string& message,
            ConsoleLevel level,
            const blink::WebNode& node) {
    node_buffer_[level].push_back(
        Entry{message, std::vector<blink::WebNode>{node}});
  }

  void Send(const std::string& message,
            ConsoleLevel level,
            std::vector<blink::WebNode>& nodes) {
    node_buffer_[level].push_back(Entry{message, nodes});
  }

  void Flush() {
    for (ConsoleLevel level : {kError, kWarning, kVerbose}) {
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

  blink::WebLocalFrame* frame_;
  std::map<ConsoleLevel, std::vector<Entry>> node_buffer_;
};

// A simple wrapper that provides some extra data about nodes
// during the DOM traversal (e.g. whether it lies within a <form>
// element, which is necessary for some of the warnings).
struct TraversalInfo {
  blink::WebNode node;
  bool in_form;
};

// Collects the important elements in a form that are
// relevant to the Password Manager, which consists of the text and password
// inputs in a form, as well as their ordering.
struct FormInputCollection {
  blink::WebFormElement form;
  std::vector<blink::WebFormControlElement> inputs;
  std::vector<size_t> text_inputs;
  std::vector<size_t> password_inputs;
  std::string signature;

  // The signature of a form is a string of 'T's and 'P's, representing
  // username and password fields respectively. This is used to quickly match
  // against well-known <input> patterns to guess what kind of form we are
  // dealing with, and provide intelligent autocomplete suggestions.
  void AddInput(const blink::WebFormControlElement& input) {
    std::string type(input.GetAttribute("type").Utf8());
    signature += type != "password" ? 'T' : 'P';
    if (type != "password")
      text_inputs.push_back(inputs.size());
    else
      password_inputs.push_back(inputs.size());
    inputs.push_back(input);
  }
};

#define DECLARE_LAZY_MATCHER(NAME, PATTERN)                                   \
  struct LabelPatternLazyInstanceTraits_##NAME                                \
      : public base::internal::DestructorAtExitLazyInstanceTraits<re2::RE2> { \
    static re2::RE2* New(void* instance) {                                    \
      return CreateMatcher(instance, PATTERN);                                \
    }                                                                         \
  };                                                                          \
  base::LazyInstance<re2::RE2, LabelPatternLazyInstanceTraits_##NAME> NAME =  \
      LAZY_INSTANCE_INITIALIZER;

DECLARE_LAZY_MATCHER(ignored_characters_matcher, R"(\W)");
DECLARE_LAZY_MATCHER(username_matcher, R"(user(name)?|login)");
DECLARE_LAZY_MATCHER(email_matcher, R"(email(address)?)");
DECLARE_LAZY_MATCHER(telephone_matcher, R"((mobile)?(telephone)?(number|no))");

#undef DECLARE_LAZY_MATCHER

// Represents a common <label> content text-pattern that indicates
// something of the purpose of an element (for example: that it is a username
// field).
struct InputHint {
  const re2::RE2* regex;
  std::string tokens;
  size_t match;

  InputHint(const re2::RE2* regex) : regex(regex), match(std::string::npos) {}

  InputHint(const re2::RE2* regex, const std::string tokens)
      : InputHint(regex) {
    this->tokens = " " + tokens;
  }

  void MatchLabel(std::string& label_content, size_t index) {
    if (re2::RE2::FullMatch(label_content, *regex))
      match = index;
  }
};

// Multiple semantic forms may be contained within a single <form> element,
// which causes confusion to the Password Manager, which acts under the
// assumption each <form> element corresponds to a single form.
// |FormIsTooComplex| uses a simple heuristic to guess whether a form may
// contain too many inputs to be considered a single form.
bool FormIsTooComplex(const std::string& signature) {
  unsigned kind_changes = 0;
  unsigned password_count = 0;
  for (const char kind : signature) {
    if (kind == (kind_changes & 1 ? 'T' : 'P'))
      ++kind_changes;
    password_count += kind == 'P';
  }
  return kind_changes >= 3 || password_count > 3;
}

// Error and warning messages regarding the DOM structure: missing <form> tags,
// duplicate ids, etc. Returns a list of the forms found in the DOM for further
// analysis.
std::vector<FormInputCollection> TraverseDOMForAnalysis(
    const blink::WebDocument& document,
    std::set<blink::WebNode>* skip_nodes,
    ConsoleLogger* console_logger) {
  std::map<std::string, std::vector<blink::WebNode>> ids;
  std::set<std::string> duplicate_ids;
  std::vector<FormInputCollection> forms;
  std::stack<TraversalInfo> stack;
  // Traverse the DOM depth-first (to provide a consistent node ordering).
  for (blink::WebNode child = document.FirstChild(); !child.IsNull();
       child = child.NextSibling())
    stack.push(TraversalInfo{child, 0});

  while (!stack.empty()) {
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

      // We don't want to re-analyse the same nodes each time the method is
      // called. This technically means some warnings might be overlooked (for
      // example if an invalid attribute is added), but these cases are assumed
      // to be rare, and are ignored for the sake of simplicity.
      if (skip_nodes->count(node))
        continue;
      skip_nodes->insert(node);

      // Duplicate id checking.
      if (element.HasAttribute("id")) {
        std::string id_attr = element.GetAttribute("id").Utf8();
        if (ids.count(id_attr))
          duplicate_ids.insert(id_attr);
        ids[id_attr].push_back(element);
      }

      // We are only interested in a subset of input elements -- those likely
      // to be username or password fields.
      std::string type(element.GetAttribute("type").Utf8());
      if (element.TagName() == "INPUT" && element.HasAttribute("type") &&
          std::find(std::begin(type_attributes), std::end(type_attributes),
                    type) != std::end(type_attributes)) {
        if (in_form)
          forms.back().AddInput(node.To<blink::WebFormControlElement>());
        else if (type == "password")
          // Orphan password field warning. Although password fields are
          // permitted outside forms, they make the Password Manager's job
          // significantly harder, as it has to guess what constitutes a single
          // semantic form from the DOM structure.
          console_logger->Send("Password fields should be contained in forms:",
                               ConsoleLogger::kVerbose, node);
      }
    }
  }

  for (const std::string& id_attr : duplicate_ids) {
    std::vector<blink::WebNode>& nodes = ids[id_attr];
    // Duplicate id attributes both are against the HTML specification and can
    // cause issues with password saving/filling, as the Password Manager makes
    // the assumption that ids may be used as a unique identifier for nodes.
    if (!id_attr.empty()) {
      console_logger->Send("The id attribute must be unique; there are " +
                               std::to_string(nodes.size()) +
                               " elements with id #" + id_attr + ":",
                           ConsoleLogger::kError, nodes);
    } else {
      console_logger->Send(
          "The id attribute must be unique and non-empty;"
          "there are " +
              std::to_string(nodes.size()) + " elements with id #" + id_attr +
              ":",
          ConsoleLogger::kError, nodes);
    }
  }

  return forms;
}

// The username field is the most difficult field to identify, as there
// are often many other textual fields in a form, and it is not always
// possible to work out which one is the username. Here, we find any
// <label> elements pointing to the input fields, and check their content.
// Labels containing text such as "Username:" or "Email address:" are
// likely to indicate the desired field, and will be prioritised over
// other fields.
void InferUsernameField(
    const blink::WebFormElement& form,
    const std::vector<blink::WebFormControlElement>& inputs,
    size_t username_field_guess,
    std::map<size_t, std::string>* autocomplete_suggestions) {
  blink::WebElementCollection labels(form.GetElementsByHTMLTagName("label"));
  DCHECK(!labels.IsNull());

  std::vector<InputHint> input_hints;
  std::string username_field_guess_tokens;

  input_hints.push_back(InputHint(username_matcher.Pointer(), ""));
  input_hints.push_back(InputHint(email_matcher.Pointer(), "email"));
  input_hints.push_back(InputHint(telephone_matcher.Pointer(), "tel"));

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
        re2::RE2::GlobalReplace(&label_content,
                                ignored_characters_matcher.Get(), "");
        for (InputHint& input_hint : input_hints)
          input_hint.MatchLabel(label_content, found - inputs.begin());
      }
    }
  }

  for (InputHint& input_hint : input_hints) {
    if (input_hint.match != std::string::npos) {
      username_field_guess = input_hint.match;
      username_field_guess_tokens = input_hint.tokens;
      break;
    }
  }

  (*autocomplete_suggestions)[username_field_guess] =
      "username" + username_field_guess_tokens;
}

// Infer what kind of form a form corresponds to (e.g. a
// registration, log-in or password reset form), based on the structure of
// the form.
void GuessAutocompleteAttributesForPasswordFields(
    const std::vector<size_t>& password_inputs,
    bool has_text_field,
    std::map<size_t, std::string>* autocomplete_suggestions) {
  size_t password_count = password_inputs.size();
  switch (password_count) {
    case 3:
      (*autocomplete_suggestions)[password_inputs[0]] = "current-password";
    // Fall-through here to match the last two password fields.
    case 2:
      (*autocomplete_suggestions)[password_inputs[password_count - 2]] =
          "new-password";
      (*autocomplete_suggestions)[password_inputs[password_count - 1]] =
          "new-password";
      break;
    case 1:
      (*autocomplete_suggestions)[password_inputs[password_count - 1]] =
          has_text_field ? "current-password" : "new-password";
      break;
  }
}

// Error and warning messages specific to an individual form (for example,
// autocomplete attributes, or missing username fields, etc.).
void AnalyseForm(const FormInputCollection& form_input_collection,
                 ConsoleLogger* console_logger) {
  const blink::WebFormElement& form = form_input_collection.form;
  const std::vector<blink::WebFormControlElement>& inputs =
      form_input_collection.inputs;
  const std::vector<size_t>& text_inputs = form_input_collection.text_inputs;
  const std::vector<size_t>& password_inputs =
      form_input_collection.password_inputs;
  const std::string& signature = form_input_collection.signature;

  // We're only interested in forms that contain password fields.
  if (password_inputs.empty())
    return;

  // If a username field is not first, or if there is no username field,
  // flag it as an issue.
  bool has_text_field = !text_inputs.empty();
  size_t username_field_guess;
  if (!has_text_field || text_inputs[0] > password_inputs[0]) {
    // There is no formal requirement to have associated username fields for
    // every password field, but providing one ensures that the Password
    // Manager associates the correct account name with the password (for
    // example in password reset forms).
    console_logger->Send(
        "Password forms should have (optionally hidden) "
        "username fields for accessibility:",
        ConsoleLogger::kVerbose, form);
  } else {
    // By default (if the other heuristics fail), the first text field
    // preceding a password field will be considered the username field.
    for (username_field_guess = password_inputs[0] - 1;;
         --username_field_guess) {
      if (signature[username_field_guess] == 'T')
        break;
    }
  }

  if (FormIsTooComplex(signature)) {
    console_logger->Send(
        "Multiple forms should be contained in their own "
        "form elements; break up complex forms into ones that represent a "
        "single action:",
        ConsoleLogger::kVerbose, form);
    return;
  }

  // The autocomplete attribute provides valuable hints to the Password
  // Manager as to the semantic structure of a form. Rather than simply point
  // out that an autocomplete attribute would be useful, we try to suggest the
  // intended value of the autocomplete attribute in order to save time for
  // the developer.
  std::map<size_t, std::string> autocomplete_suggestions;
  if (has_text_field)
    InferUsernameField(form, inputs, username_field_guess,
                       &autocomplete_suggestions);

  GuessAutocompleteAttributesForPasswordFields(password_inputs, has_text_field,
                                               &autocomplete_suggestions);

  // For each input element that is not annotated with an autocomplete
  // attribute, if we have a guess for what function the input serves, log
  // a warning, suggesting that the inferred attribute value should be added.
  for (size_t i = 0; i < inputs.size(); ++i) {
    if (autocomplete_suggestions.count(i) &&
        !inputs[i].HasAttribute("autocomplete"))
      console_logger->Send(
          "Input elements should have autocomplete "
          "attributes (suggested: \"" +
              autocomplete_suggestions[i] + "\"):",
          ConsoleLogger::kVerbose, inputs[i]);
  }
}

}  // namespace

// Out-of-line definitions to keep [chromium-style] happy.
DevToolsPagePasswordsAnalyser::DevToolsPagePasswordsAnalyser() {}

DevToolsPagePasswordsAnalyser::~DevToolsPagePasswordsAnalyser() {}

void DevToolsPagePasswordsAnalyser::Reset() {
  skip_nodes_.clear();
}

void DevToolsPagePasswordsAnalyser::MarkDOMForAnalysis(
    content::RenderFrame* render_frame) {
  page_dirty_ = true;
  content::RenderFrameImpl* impl =
      static_cast<content::RenderFrameImpl*>(render_frame);
  content::DevToolsAgent* devtools_agent = impl->devtools_agent();
  if (!devtools_agent || !devtools_agent->IsAttached())
    return;
  AnalyseDocumentDOMIfNeedBe(render_frame);
}

void DevToolsPagePasswordsAnalyser::AnalyseDocumentDOMIfNeedBe(
    content::RenderFrame* render_frame) {
  if (page_dirty_)
    AnalyseDocumentDOM(render_frame->GetWebFrame());
}

void DevToolsPagePasswordsAnalyser::AnalyseDocumentDOM(
    blink::WebLocalFrame* frame) {
  DCHECK(frame);
  ConsoleLogger console_logger(frame);

  blink::WebDocument document(frame->GetDocument());
  // Extract all the forms from the DOM, and provide relevant warnings.
  std::vector<FormInputCollection> forms(
      TraverseDOMForAnalysis(document, &skip_nodes_, &console_logger));

  // Analyse each form in turn, for example with respect to autocomplete
  // attributes.
  for (const FormInputCollection& form_input_collection : forms)
    AnalyseForm(form_input_collection, &console_logger);

  // Finally, send all the warnings and errors to the console.
  console_logger.Flush();
  page_dirty_ = false;
}

}  // namespace autofill