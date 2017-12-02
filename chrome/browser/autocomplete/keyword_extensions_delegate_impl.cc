// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autocomplete/keyword_extensions_delegate_impl.h"

#include <stddef.h>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/api/omnibox/omnibox_api.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/notification_types.h"

namespace omnibox_api = extensions::api::omnibox;

int KeywordExtensionsDelegateImpl::global_input_uid_ = 0;

KeywordExtensionsDelegateImpl::KeywordExtensionsDelegateImpl(
    Profile* profile,
    KeywordProvider* provider)
    : KeywordExtensionsDelegate(provider),
      on_keyword_entered_(false),
      ended_keyword_mode_(false),
      profile_(profile),
      provider_(provider) {
  DCHECK(provider_);

  current_input_id_ = 0;
  // Extension suggestions always come from the original profile, since that's
  // where extensions run. We use the input ID to distinguish whether the
  // suggestions are meant for us.
  registrar_.Add(this,
                 extensions::NOTIFICATION_EXTENSION_OMNIBOX_SUGGESTIONS_READY,
                 content::Source<Profile>(profile_->GetOriginalProfile()));
  registrar_.Add(
      this,
      extensions::NOTIFICATION_EXTENSION_OMNIBOX_DEFAULT_SUGGESTION_CHANGED,
      content::Source<Profile>(profile_->GetOriginalProfile()));
  registrar_.Add(this,
                 extensions::NOTIFICATION_EXTENSION_OMNIBOX_INPUT_ENTERED,
                 content::Source<Profile>(profile_));
}

KeywordExtensionsDelegateImpl::~KeywordExtensionsDelegateImpl() {
}

void KeywordExtensionsDelegateImpl::DeleteSuggestion(
    const TemplateURL* template_url,
    const base::string16& suggestion_text) {
  extensions::ExtensionOmniboxEventRouter::OnDeleteSuggestion(
      profile_, template_url->GetExtensionId(),
      base::UTF16ToUTF8(suggestion_text));
}

void KeywordExtensionsDelegateImpl::OnKeywordEntered(
    const TemplateURL* template_url) {
  extension_suggest_matches_.clear();
  if (extensions::ExtensionOmniboxEventRouter::OnKeywordEntered(
          profile_, template_url->GetExtensionId(), current_input_id_)) {
    set_done(false);
    on_keyword_entered_ = true;
    ended_keyword_mode_ = false;
  }
}

void  KeywordExtensionsDelegateImpl::IncrementInputId() {
  current_input_id_ = ++global_input_uid_;
}

bool KeywordExtensionsDelegateImpl::IsEnabledExtension(
    const std::string& extension_id) {
  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(
          profile_)->enabled_extensions().GetByID(extension_id);
  return extension &&
      (!profile_->IsOffTheRecord() ||
       extensions::util::IsIncognitoEnabled(extension_id, profile_));
}

bool KeywordExtensionsDelegateImpl::Start(
    const AutocompleteInput& input,
    bool minimal_changes,
    const TemplateURL* template_url,
    const base::string16& remaining_input) {
  DCHECK(template_url);

  std::string extension_id = template_url->GetExtensionId();
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  bool is_on_keyword_entered_event_registered =
      event_router->ExtensionHasEventListener(
          extension_id, extensions::api::omnibox::OnKeywordEntered::kEventName);
  if (ShouldResetOnKeywordEntered(input, remaining_input))
    on_keyword_entered_ = false;
  if (input.want_asynchronous_matches()) {
    if (extension_id != current_keyword_extension_id_)
      MaybeEndExtensionKeywordMode();
    if (current_keyword_extension_id_.empty() &&
        (on_keyword_entered_ || !is_on_keyword_entered_event_registered ||
         (!on_keyword_entered_ && !remaining_input.empty()))) {
      OnInputStarted(extension_id);
    }
  }

  extensions::ApplyDefaultSuggestionForExtensionKeyword(
      profile_, template_url, remaining_input, &matches()->front());
  extension_suggest_last_input_ = input;
  last_remaining_input_ = remaining_input;
  if (minimal_changes) {
    // If the input hasn't significantly changed, we can just use the
    // suggestions from last time. We need to readjust the relevance to
    // ensure it is less than the main match's relevance.
    for (size_t i = 0; i < extension_suggest_matches_.size(); ++i) {
      matches()->push_back(extension_suggest_matches_[i]);
      matches()->back().relevance = matches()->front().relevance - (i + 1);
    }
  } else if (is_on_keyword_entered_event_registered && !on_keyword_entered_ &&
             remaining_input.empty() && input.want_asynchronous_matches()) {
    // When there is no remaining input, the OnKeywordEntered extension event
    // listener was called.
    extension_suggest_matches_.clear();
  } else if (input.want_asynchronous_matches()) {
    extension_suggest_matches_.clear();

    // We only have to wait for suggest results if there are actually
    // extensions listening for input changes.
    if (extensions::ExtensionOmniboxEventRouter::OnInputChanged(
            profile_, template_url->GetExtensionId(),
            base::UTF16ToUTF8(remaining_input), current_input_id_))
      set_done(false);
  }
  return input.want_asynchronous_matches();
}

void KeywordExtensionsDelegateImpl::OnInputStarted(
    const std::string& extension_id) {
  DCHECK(current_keyword_extension_id_.empty());
  current_keyword_extension_id_ = extension_id;

  extensions::ExtensionOmniboxEventRouter::OnInputStarted(
      profile_, current_keyword_extension_id_);
}

void KeywordExtensionsDelegateImpl::MaybeEndExtensionKeywordMode() {
  if (!current_keyword_extension_id_.empty()) {
    ended_keyword_mode_ = true;
    extensions::ExtensionOmniboxEventRouter::OnInputCancelled(
        profile_, current_keyword_extension_id_);
    current_keyword_extension_id_.clear();
    // Ignore stray suggestions_ready events that arrive after
    // OnInputCancelled().
    IncrementInputId();
  }
}

void KeywordExtensionsDelegateImpl::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  TemplateURLService* model = provider_->GetTemplateURLService();
  const AutocompleteInput& input = extension_suggest_last_input_;

  switch (type) {
    case extensions::NOTIFICATION_EXTENSION_OMNIBOX_INPUT_ENTERED:
      // Input has been accepted, so we're done with this input session. Ensure
      // we don't send the OnInputCancelled event, or handle any more stray
      // suggestions_ready events.
      on_keyword_entered_ = false;
      current_keyword_extension_id_.clear();
      IncrementInputId();
      return;

    case extensions::NOTIFICATION_EXTENSION_OMNIBOX_DEFAULT_SUGGESTION_CHANGED
        : {
      DCHECK(model);
      // It's possible to change the default suggestion while not in an editing
      // session.
      base::string16 keyword, remaining_input;
      if (matches()->empty() || current_keyword_extension_id_.empty() ||
          !KeywordProvider::ExtractKeywordFromInput(input, model, &keyword,
                                                    &remaining_input))
        return;

      const TemplateURL* template_url(
          model->GetTemplateURLForKeyword(keyword));
      extensions::ApplyDefaultSuggestionForExtensionKeyword(
          profile_, template_url, remaining_input, &matches()->front());
      OnProviderUpdate(true);
      return;
    }

    case extensions::NOTIFICATION_EXTENSION_OMNIBOX_SUGGESTIONS_READY: {
      const omnibox_api::SendSuggestions::Params& suggestions =
          *content::Details<
              omnibox_api::SendSuggestions::Params>(details).ptr();
      if (suggestions.request_id != current_input_id_)
        return;  // This is an old result. Just ignore.

      DCHECK(model);
      // ExtractKeywordFromInput() can fail if e.g. this code is triggered by
      // direct calls from the development console, outside the normal flow of
      // user input.
      base::string16 keyword, remaining_input;
      if (!KeywordProvider::ExtractKeywordFromInput(input, model, &keyword,
                                                    &remaining_input))
        return;

      const TemplateURL* template_url =
          model->GetTemplateURLForKeyword(keyword);

      int relevance = KeywordProvider::CalculateRelevance(
          input.type(), true, true, false, input.prefer_keyword(),
          input.allow_exact_keyword_match());
      extension_suggest_matches_.push_back(provider_->CreateAutocompleteMatch(
          template_url, keyword.length(), input, keyword.length(),
          remaining_input, input.allow_exact_keyword_match(), relevance,
          false));
      AutocompleteMatch* m = &extension_suggest_matches_.back();
      m->allowed_to_be_default_match = true;
      // TODO(mpcomplete): consider clamping the number of suggestions to
      // AutocompleteProvider::kMaxMatches.
      for (size_t i = 0; i < suggestions.suggest_results.size(); ++i) {
        const omnibox_api::SuggestResult& suggestion =
            suggestions.suggest_results[i];
        // We want to order these suggestions in descending order, so start with
        // the relevance of the first result (added synchronously in Start()),
        // and subtract 1 for each subsequent suggestion from the extension.
        // We recompute the first match's relevance; we know that |complete|
        // is true, because we wouldn't get results from the extension unless
        // the full keyword had been typed.
        int first_relevance = KeywordProvider::CalculateRelevance(
            input.type(), true, true, false, input.prefer_keyword(),
            input.allow_exact_keyword_match());
        // Because these matches are async, we should never let them become the
        // default match, lest we introduce race conditions in the omnibox user
        // interaction.
        extension_suggest_matches_.push_back(provider_->CreateAutocompleteMatch(
            template_url, keyword.length(), input, keyword.length(),
            base::UTF8ToUTF16(suggestion.content), false,
            first_relevance - (i + 1),
            suggestion.deletable && *suggestion.deletable));

        AutocompleteMatch* match = &extension_suggest_matches_.back();
        match->contents.assign(base::UTF8ToUTF16(suggestion.description));
        match->contents_class =
            extensions::StyleTypesToACMatchClassifications(suggestion);
      }

      set_done(true);
      matches()->insert(matches()->end(),
                        extension_suggest_matches_.begin(),
                        extension_suggest_matches_.end());

      OnProviderUpdate(!extension_suggest_matches_.empty());
      return;
    }

    default:
      NOTREACHED();
      return;
  }
}

bool KeywordExtensionsDelegateImpl::ShouldResetOnKeywordEntered(
    const AutocompleteInput& input,
    const base::string16& remaining_input) {
  // bool no_remaining_input = remaining_input.empty() &&
  //    last_remaining_input_.empty();
  base::string16 temp_keyword, remaining;
  if (on_keyword_entered_ && remaining_input.empty() &&
      current_keyword_extension_id_.empty() &&
      KeywordProvider::ExtractKeywordFromInput(
          input, provider_->GetTemplateURLService(), &temp_keyword,
          &remaining)) {
    std::string keyword = base::UTF16ToASCII(temp_keyword);
    std::string keyword_and_space = keyword + " ";

    std::string input_text = base::UTF16ToASCII(input.text());
    std::string last_input_text =
        base::UTF16ToASCII(extension_suggest_last_input_.text());

    int delta = input_text.length() - last_input_text.length();
    /* bool deleted_space = delta == -1;
     bool added_space = delta == 1;*/
    // if (last_input_text == keyword)
    //  on_keyword_entered_ = false;

    return (input_text == last_input_text) ||
           ((input_text == keyword_and_space) &&
            (last_input_text == keyword)) ||
           ((last_input_text == keyword_and_space) &&
            (input_text == keyword)) ||
           ((input_text == keyword_and_space) && !ended_keyword_mode_) ||
           /*((input_text == keyword_and_space) && ended_keyword_mode_ &&
             !last_remaining_input_.empty()) ||*/
           // (input_text == keyword_and_space) && !ended_keyword_mode_)
           // used when ended and then i: i <; last: `      d<. if u don't then
           // oninput chages wont be called and u'll get blank results
           // fails when input ' <; last: ` d< it shud return false
           // so that onInputChanges is not called
           ((last_input_text == keyword_and_space) && (std::abs(delta) < 0)) ||
           ((last_input_text == keyword_and_space) && (std::abs(delta) > 0) &&
            ended_keyword_mode_);
  }
  return false;
}

void KeywordExtensionsDelegateImpl::OnProviderUpdate(bool updated_matches) {
  provider_->listener_->OnProviderUpdate(updated_matches);
}
