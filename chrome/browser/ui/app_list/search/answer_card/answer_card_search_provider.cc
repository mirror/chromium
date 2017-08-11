// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/answer_card/answer_card_search_provider.h"

#include <utility>

#include "base/command_line.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/app_list/search/answer_card/answer_card_result.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/search_engines/template_url_service.h"
#include "ui/app_list/app_list_features.h"
#include "ui/app_list/app_list_model.h"
#include "ui/app_list/search_box_model.h"

namespace app_list {

namespace {

enum class SearchAnswerRequestResult {
  REQUEST_RESULT_ANOTHER_REQUEST_STARTED = 0,
  REQUEST_RESULT_REQUEST_FAILED = 1,
  REQUEST_RESULT_NO_ANSWER = 2,
  REQUEST_RESULT_RECEIVED_ANSWER = 3,
  REQUEST_RESULT_RECEIVED_ANSWER_TOO_LARGE = 4,
  REQUEST_RESULT_MAX = 5
};

void RecordRequestResult(SearchAnswerRequestResult request_result) {
  UMA_HISTOGRAM_ENUMERATION("SearchAnswer.RequestResult", request_result,
                            SearchAnswerRequestResult::REQUEST_RESULT_MAX);
}

}  // namespace

AnswerCardSearchProvider::NavigationContext::NavigationContext() {}

AnswerCardSearchProvider::NavigationContext::~NavigationContext() {}

void AnswerCardSearchProvider::NavigationContext::Clear() {
  result_url.clear();
  result_title.clear();
  received_answer = false;
  // Not clearing |preferred_size| since the |contents| remains unchanged, and
  // |preferred_size| always corresponds to the contents's size.
}

AnswerCardSearchProvider::AnswerCardSearchProvider(
    Profile* profile,
    app_list::AppListModel* model,
    AppListControllerDelegate* list_controller,
    std::unique_ptr<AnswerCardContents> contents0,
    std::unique_ptr<AnswerCardContents> contents1)
    : profile_(profile),
      model_(model),
      list_controller_(list_controller),
      answer_server_url_(features::AnswerServerUrl()),
      template_url_service_(TemplateURLServiceFactory::GetForProfile(profile)) {
  navigation_contexts_[0].contents = std::move(contents0);
  navigation_contexts_[1].contents = std::move(contents1);
  navigation_contexts_[0].contents->SetDelegate(this);
  navigation_contexts_[1].contents->SetDelegate(this);
}

AnswerCardSearchProvider::~AnswerCardSearchProvider() {
  RecordReceivedAnswerFinalResult();
}

void AnswerCardSearchProvider::Start(bool is_voice_query,
                                     const base::string16& query) {
  RecordReceivedAnswerFinalResult();
  // Reset the state.
  current_request_url_ = GURL();
  server_request_start_time_ = answer_loaded_time_ = base::TimeTicks();

  if (query.empty()) {
    // Normally, when the user modifies the query, we are keeping the result shown while the new card loads. But for the empty query, we delete the result.
    DeleteCurrentResult();
    return;
  }

  if (is_voice_query) {
    // No need to send a server request and show a card because launcher
    // automatically closes upon voice queries.
    return;
  }

  if (!model_->search_engine_is_google())
    return;

  // Start a request to the answer server.

  // Lifetime of |prefixed_query| should be longer than the one of
  // |replacements|.
  const std::string prefixed_query(
      "q=" + net::EscapeQueryParamValue(base::UTF16ToUTF8(query), true) +
      features::AnswerServerQuerySuffix());
  GURL::Replacements replacements;
  replacements.SetQueryStr(prefixed_query);
  current_request_url_ = answer_server_url_.ReplaceComponents(replacements);
  navigation_contexts_[next_navigation_context_].contents->LoadURL(
      current_request_url_);

  server_request_start_time_ = base::TimeTicks::Now();
}

void AnswerCardSearchProvider::UpdatePreferredSize(
    const AnswerCardContents* source,
    const gfx::Size& pref_size) {
  if (source == navigation_contexts_[0].contents.get()) {
    navigation_contexts_[0].preferred_size = pref_size;
  } else if (source == navigation_contexts_[1].contents.get()) {
    navigation_contexts_[1].preferred_size = pref_size;
  } else
    DCHECK(false);

  const int ARRIVING_INDEX =
      source == navigation_contexts_[0].contents.get() ? 0 : 1;
  const bool REC_ANS = navigation_contexts_[ARRIVING_INDEX].received_answer;

  if (ARRIVING_INDEX == next_navigation_context_) {
    // CAT IS SHOWN, but DOG's size arrived. Then don't care.
  } else {
    OnResultAvailable(ARRIVING_INDEX, REC_ANS && IsCardSizeOk(ARRIVING_INDEX) &&
                                          !source->IsLoading());
  }

  if (!answer_loaded_time_.is_null()) {
    UMA_HISTOGRAM_TIMES("SearchAnswer.ResizeAfterLoadTime",
                        base::TimeTicks::Now() - answer_loaded_time_);
  }
}

void AnswerCardSearchProvider::DidFinishNavigation(
    const AnswerCardContents* source,
    const GURL& url,
    bool has_error,
    bool has_answer_card,
    const std::string& result_title,
    const std::string& issued_query) {
  LOG(ERROR)
      << "******************* AnswerCardSearchProvider::DidFinishNavigation";
  DCHECK_EQ(source,
            navigation_contexts_[next_navigation_context_].contents.get());

  if (url != current_request_url_) {
    // TODO(vadimt): Remove this and similar logging once testing is complete if
    // we think this is not useful after release or happens too frequently.
    LOG(ERROR) << "DidFinishNavigation: Another request started";
    RecordRequestResult(
        SearchAnswerRequestResult::REQUEST_RESULT_ANOTHER_REQUEST_STARTED);
    return;
  }

  LOG(ERROR) << "DidFinishNavigation: Latest request completed";
  if (has_error) {
    RecordRequestResult(
        SearchAnswerRequestResult::REQUEST_RESULT_REQUEST_FAILED);
    DeleteCurrentResult();
    return;
  }

  if (!features::IsAnswerCardDarkRunEnabled()) {
    if (!has_answer_card) {
      RecordRequestResult(SearchAnswerRequestResult::REQUEST_RESULT_NO_ANSWER);
      DeleteCurrentResult();
      return;
    }
    DCHECK(!result_title.empty());
    DCHECK(!issued_query.empty());
    navigation_contexts_[next_navigation_context_].result_title = result_title;
    LOG(ERROR) << "***** AnswerCardSearchProvider::DidFinishNavigation about "
                  "to set result url for "
               << next_navigation_context_ << " to " << issued_query;
    navigation_contexts_[next_navigation_context_].result_url =
        GetResultUrl(base::UTF8ToUTF16(issued_query));
  } else {  ///////////////////////////////////////////////////////////////////////////
            /// what are doing for dark run?????
    // In the dark run mode, every other "server response" contains a card.
    dark_run_received_answer_ = !dark_run_received_answer_;
    if (!dark_run_received_answer_)
      return;
    // SearchResult requires a non-empty id. This "url" will never be opened.
    navigation_contexts_[0].result_url = "https://www.google.com/?q=something";
    navigation_contexts_[1].result_url =
        "https://www.google.com/?q=something";  ///////// move to a
                                                /// single-executed
                                                /// place
  }

  navigation_contexts_[next_navigation_context_].received_answer = true;
  UMA_HISTOGRAM_TIMES("SearchAnswer.NavigationTime",
                      base::TimeTicks::Now() - server_request_start_time_);
}

void AnswerCardSearchProvider::DidStopLoading(
    const AnswerCardContents* source) {
  LOG(ERROR) << "******************* AnswerCardSearchProvider::DidStopLoading";
  DCHECK_EQ(source,
            navigation_contexts_[next_navigation_context_].contents.get());

  navigation_contexts_[1 - next_navigation_context_].received_answer =
      false;  // mark the other result as unreceived; that's a
              // quietionable practice for inermediate
              // DidStopLoading calls

  const bool REC_ANS =
      navigation_contexts_[next_navigation_context_].received_answer;
  if (!REC_ANS)
    return;

  if (IsCardSizeOk(next_navigation_context_)) {
    LOG(ERROR) << "***** AnswerCardSearchProvider::DidStopLoading about to set "
                  "avail true for "
               << next_navigation_context_;
    OnResultAvailable(next_navigation_context_, true);
    // Prepare for loading card into the other contents. Loading will start when
    // the user modifies the query string.
    next_navigation_context_ = 1 - next_navigation_context_;
    navigation_contexts_[next_navigation_context_].Clear();
  }

  answer_loaded_time_ = base::TimeTicks::Now();
  UMA_HISTOGRAM_TIMES("SearchAnswer.LoadingTime",
                      answer_loaded_time_ - server_request_start_time_);
  base::RecordAction(base::UserMetricsAction("SearchAnswer_StoppedLoading"));
}

bool AnswerCardSearchProvider::IsCardSizeOk(int contents_index) const {
  if (features::IsAnswerCardDarkRunEnabled())
    return true;

  const gfx::Size& preferred_size =
      navigation_contexts_[contents_index].preferred_size;

  if (preferred_size.width() <= features::AnswerCardMaxWidth() &&
      preferred_size.height() <= features::AnswerCardMaxHeight()) {
    return true;
  }

  LOG(ERROR) << "Card is too large: width=" << preferred_size.width()
             << ", height=" << preferred_size.height();
  return false;
}

void AnswerCardSearchProvider::RecordReceivedAnswerFinalResult() {
  // Recording whether a server response with an answer contains a card of a
  // fitting size, or a too large one. Cannot do this in DidStopLoading() or
  // UpdatePreferredSize() because this may be followed by a resizing with
  // different dimensions, so this method gets called when card's life ends.
  //  if (!received_answer)
  //  /////////////////////////////////////////////////////////// ????? need to
  //  uncomment
  //    return;

  //  RecordRequestResult(
  //      IsCardSizeOk() ?
  //      SearchAnswerRequestResult::REQUEST_RESULT_RECEIVED_ANSWER
  //                     : SearchAnswerRequestResult::
  //                           REQUEST_RESULT_RECEIVED_ANSWER_TOO_LARGE);
}

void AnswerCardSearchProvider::OnResultAvailable(int contents_index,
                                                 bool is_available) {
  SearchProvider::Results results;
  if (is_available) {
    results.reserve(1);

    const NavigationContext& context = navigation_contexts_[contents_index];
    DCHECK(context.received_answer);

    const GURL stripped_result_url = AutocompleteMatch::GURLToStrippedGURL(
        GURL(context.result_url), AutocompleteInput(), template_url_service_,
        base::string16() /* keyword */);

    results.emplace_back(base::MakeUnique<AnswerCardResult>(
        profile_, list_controller_, context.result_url,
        stripped_result_url.spec(), base::UTF8ToUTF16(context.result_title),
        context.contents.get()));
  }
  SwapResults(&results);
}

std::string AnswerCardSearchProvider::GetResultUrl(
    const base::string16& query) const {
  return template_url_service_->GetDefaultSearchProvider()
      ->url_ref()
      .ReplaceSearchTerms(TemplateURLRef::SearchTermsArgs(query),
                          template_url_service_->search_terms_data());
}

void AnswerCardSearchProvider::DeleteCurrentResult() {
  navigation_contexts_[1 - next_navigation_context_].Clear();
  OnResultAvailable(1 - next_navigation_context_, false);
}

}  // namespace app_list
