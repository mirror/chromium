// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_H_
#define CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/string16.h"
#include "base/time.h"
#include "base/timer.h"
#include "chrome/browser/autocomplete/autocomplete_types.h"
#include "googleurl/src/gurl.h"
#include "googleurl/src/url_parse.h"

// The AutocompleteController is the center of the autocomplete system.  A
// class creates an instance of the controller, which in turn creates a set of
// AutocompleteProviders to serve it.  The owning class can ask the controller
// to Start() a query; the controller in turn passes this call down to the
// providers, each of which keeps track of its own matches and whether it has
// finished processing the query.  When a provider gets more matches or finishes
// processing, it notifies the controller, which merges the combined matches
// together and makes the result available to interested observers.
//
// The owner may also cancel the current query by calling Stop(), which the
// controller will in turn communicate to all the providers.  No callbacks will
// happen after a request has been stopped.
//
// IMPORTANT: There is NO THREAD SAFETY built into this portion of the
// autocomplete system.  All calls to and from the AutocompleteController should
// happen on the same thread.  AutocompleteProviders are responsible for doing
// their own thread management when they need to return matches asynchronously.
//
// The AutocompleteProviders each return different kinds of matches,
// such as history or search matches.  These matches are given
// "relevance" scores.  Higher scores are better matches than lower
// scores.  The relevance scores and classes providing the respective
// matches are as listed below.
//
// IMPORTANT CAVEAT: The tables below are NOT COMPLETE.  Developers
// often forget to keep these tables in sync with the code when they
// change scoring algorithms or add new providers.  For example,
// neither the HistoryQuickProvider (which is a provider that appears
// often) nor the ShortcutsProvider are listed here.  For the best
// idea of how scoring works and what providers are affecting which
// queries, play with chrome://omnibox/ for a while.  While the tables
// below may have some utility, nothing compares with first-hand
// investigation and experience.
//
// UNKNOWN input type:
// --------------------------------------------------------------------|-----
// Keyword (non-substituting or in keyword UI mode, exact match)       | 1500
// Extension App (exact match)                                         | 1425
// HistoryURL (good exact or inline autocomplete matches, some inexact)| 1410++
// HistoryURL (intranet url never visited match, some inexact matches) | 1400++
// Search Primary Provider (past query in history within 2 days)       | 1399**
// Search Primary Provider (what you typed)                            | 1300
// HistoryURL (what you typed, some inexact matches)                   | 1200++
// Extension App (inexact match)                                       | 1175*~
// Keyword (substituting, exact match)                                 | 1100
// Search Primary Provider (past query in history older than 2 days)   | 1050--
// HistoryContents (any match in title of starred page)                | 1000++
// HistoryURL (some inexact matches)                                   |  900++
// Search Primary Provider (navigational suggestion)                   |  800++
// HistoryContents (any match in title of nonstarred page)             |  700++
// Search Primary Provider (suggestion)                                |  600++
// Built-in                                                            |  575++
// HistoryContents (any match in body of starred page)                 |  550++
// HistoryContents (any match in body of nonstarred page)              |  500++
// Keyword (inexact match)                                             |  450
// Search Secondary Provider (what you typed)                          |  250
// Search Secondary Provider (past query in history)                   |  200--
// Search Secondary Provider (navigational suggestion)                 |  150++
// Search Secondary Provider (suggestion)                              |  100++
//
// REQUESTED_URL input type:
// --------------------------------------------------------------------|-----
// Keyword (non-substituting or in keyword UI mode, exact match)       | 1500
// Extension App (exact match)                                         | 1425
// HistoryURL (good exact or inline autocomplete matches, some inexact)| 1410++
// HistoryURL (intranet url never visited match, some inexact matches) | 1400++
// Search Primary Provider (past query in history within 2 days)       | 1399**
// HistoryURL (what you typed, some inexact matches)                   | 1200++
// Extension App (inexact match)                                       | 1175*~
// Search Primary Provider (what you typed)                            | 1150
// Keyword (substituting, exact match)                                 | 1100
// Search Primary Provider (past query in history older than 2 days)   | 1050--
// HistoryContents (any match in title of starred page)                | 1000++
// HistoryURL (some inexact matches)                                   |  900++
// Search Primary Provider (navigational suggestion)                   |  800++
// HistoryContents (any match in title of nonstarred page)             |  700++
// Search Primary Provider (suggestion)                                |  600++
// Built-in                                                            |  575++
// HistoryContents (any match in body of starred page)                 |  550++
// HistoryContents (any match in body of nonstarred page)              |  500++
// Keyword (inexact match)                                             |  450
// Search Secondary Provider (what you typed)                          |  250
// Search Secondary Provider (past query in history)                   |  200--
// Search Secondary Provider (navigational suggestion)                 |  150++
// Search Secondary Provider (suggestion)                              |  100++
//
// URL input type:
// --------------------------------------------------------------------|-----
// Keyword (non-substituting or in keyword UI mode, exact match)       | 1500
// Extension App (exact match)                                         | 1425
// HistoryURL (good exact or inline autocomplete matches, some inexact)| 1410++
// HistoryURL (intranet url never visited match, some inexact matches) | 1400++
// HistoryURL (what you typed, some inexact matches)                   | 1200++
// Extension App (inexact match)                                       | 1175*~
// Keyword (substituting, exact match)                                 | 1100
// HistoryURL (some inexact matches)                                   |  900++
// Search Primary Provider (what you typed)                            |  850
// Search Primary Provider (navigational suggestion)                   |  800++
// Search Primary Provider (past query in history)                     |  750--
// Keyword (inexact match)                                             |  700
// Built-in                                                            |  575++
// Search Primary Provider (suggestion)                                |  300++
// Search Secondary Provider (what you typed)                          |  250
// Search Secondary Provider (past query in history)                   |  200--
// Search Secondary Provider (navigational suggestion)                 |  150++
// Search Secondary Provider (suggestion)                              |  100++
//
// QUERY input type:
// --------------------------------------------------------------------|-----
// Search Primary or Secondary (past query in history within 2 days)   | 1599**
// Keyword (non-substituting or in keyword UI mode, exact match)       | 1500
// Keyword (substituting, exact match)                                 | 1450
// Extension App (exact match)                                         | 1425
// Search Primary Provider (past query in history within 2 days)       | 1399**
// Search Primary Provider (what you typed)                            | 1300
// Extension App (inexact match)                                       | 1175*~
// Search Primary Provider (past query in history older than 2 days)   | 1050--
// HistoryContents (any match in title of starred page)                | 1000++
// HistoryURL (inexact match)                                          |  900++
// Search Primary Provider (navigational suggestion)                   |  800++
// HistoryContents (any match in title of nonstarred page)             |  700++
// Search Primary Provider (suggestion)                                |  600++
// HistoryContents (any match in body of starred page)                 |  550++
// HistoryContents (any match in body of nonstarred page)              |  500++
// Keyword (inexact match)                                             |  450
// Search Secondary Provider (what you typed)                          |  250
// Search Secondary Provider (past query in history)                   |  200--
// Search Secondary Provider (navigational suggestion)                 |  150++
// Search Secondary Provider (suggestion)                              |  100++
//
// FORCED_QUERY input type:
// --------------------------------------------------------------------|-----
// Extension App (exact match on title only, not url)                  | 1425
// Search Primary Provider (past query in history within 2 days)       | 1399**
// Search Primary Provider (what you typed)                            | 1300
// Extension App (inexact match on title only, not url)                | 1175*~
// Search Primary Provider (past query in history older than 2 days)   | 1050--
// HistoryContents (any match in title of starred page)                | 1000++
// Search Primary Provider (navigational suggestion)                   |  800++
// HistoryContents (any match in title of nonstarred page)             |  700++
// Search Primary Provider (suggestion)                                |  600++
// HistoryContents (any match in body of starred page)                 |  550++
// HistoryContents (any match in body of nonstarred page)              |  500++
//
// (A search keyword is a keyword with a replacement string; a bookmark keyword
// is a keyword with no replacement string, that is, a shortcut for a URL.)
//
// There are two possible providers for search suggestions. If the user has
// typed a keyword, then the primary provider is the keyword provider and the
// secondary provider is the default provider. If the user has not typed a
// keyword, then the primary provider corresponds to the default provider.
//
// Search providers may supply relevance values along with their results to be
// used in place of client-side calculated values.
//
// The value column gives the ranking returned from the various providers.
// ++: a series of matches with relevance from n up to (n + max_matches).
// --: relevance score falls off over time (discounted 50 points @ 15 minutes,
//     450 points @ two weeks)
// **: relevance score falls off over two days (discounted 99 points after two
//     days).
// *~: Partial matches get a score on a sliding scale from about 575-1125 based
//     on how many times the URL for the Extension App has been typed and how
//     many of the letters match.

struct AutocompleteMatch;
class AutocompleteProvider;

// AutocompleteInput ----------------------------------------------------------

// The user input for an autocomplete query.  Allows copying.
class AutocompleteInput {
 public:
  // Note that the type below may be misleading.  For example, "http:/" alone
  // cannot be opened as a URL, so it is marked as a QUERY; yet the user
  // probably intends to type more and have it eventually become a URL, so we
  // need to make sure we still run it through inline autocomplete.
  enum Type {
    INVALID,        // Empty input
    UNKNOWN,        // Valid input whose type cannot be determined
    REQUESTED_URL,  // Input autodetected as UNKNOWN, which the user wants to
                    // treat as an URL by specifying a desired_tld
    URL,            // Input autodetected as a URL
    QUERY,          // Input autodetected as a query
    FORCED_QUERY,   // Input forced to be a query by an initial '?'
  };

  // Enumeration of the possible match query types. Callers who only need some
  // of the matches for a particular input can get answers more quickly by
  // specifying that upfront.
  enum MatchesRequested {
    // Only the best match in the whole result set matters.  Providers should at
    // most return synchronously-available matches, and if possible do even less
    // work, so that it's safe to ask for these repeatedly in the course of one
    // higher-level "synchronous" query.
    BEST_MATCH,

    // Only synchronous matches should be returned.
    SYNCHRONOUS_MATCHES,

    // All matches should be fetched.
    ALL_MATCHES,
  };

  AutocompleteInput();
  AutocompleteInput(const string16& text,
                    const string16& desired_tld,
                    bool prevent_inline_autocomplete,
                    bool prefer_keyword,
                    bool allow_exact_keyword_match,
                    MatchesRequested matches_requested);
  ~AutocompleteInput();

  // If type is |FORCED_QUERY| and |text| starts with '?', it is removed.
  static void RemoveForcedQueryStringIfNecessary(Type type, string16* text);

  // Converts |type| to a string representation.  Used in logging.
  static std::string TypeToString(Type type);

  // Parses |text| and returns the type of input this will be interpreted as.
  // The components of the input are stored in the output parameter |parts|, if
  // it is non-NULL. The scheme is stored in |scheme| if it is non-NULL. The
  // canonicalized URL is stored in |canonicalized_url|; however, this URL is
  // not guaranteed to be valid, especially if the parsed type is, e.g., QUERY.
  static Type Parse(const string16& text,
                    const string16& desired_tld,
                    url_parse::Parsed* parts,
                    string16* scheme,
                    GURL* canonicalized_url);

  // Parses |text| and fill |scheme| and |host| by the positions of them.
  // The results are almost as same as the result of Parse(), but if the scheme
  // is view-source, this function returns the positions of scheme and host
  // in the URL qualified by "view-source:" prefix.
  static void ParseForEmphasizeComponents(const string16& text,
                                          const string16& desired_tld,
                                          url_parse::Component* scheme,
                                          url_parse::Component* host);

  // Code that wants to format URLs with a format flag including
  // net::kFormatUrlOmitTrailingSlashOnBareHostname risk changing the meaning if
  // the result is then parsed as AutocompleteInput.  Such code can call this
  // function with the URL and its formatted string, and it will return a
  // formatted string with the same meaning as the original URL (i.e. it will
  // re-append a slash if necessary).
  static string16 FormattedStringWithEquivalentMeaning(
      const GURL& url,
      const string16& formatted_url);

  // Returns the number of non-empty components in |parts| besides the host.
  static int NumNonHostComponents(const url_parse::Parsed& parts);

  // User-provided text to be completed.
  const string16& text() const { return text_; }

  // Use of this setter is risky, since no other internal state is updated
  // besides |text_| and |parts_|.  Only callers who know that they're not
  // changing the type/scheme/etc. should use this.
  void UpdateText(const string16& text, const url_parse::Parsed& parts);

  // User's desired TLD, if one is not already present in the text to
  // autocomplete.  When this is non-empty, it also implies that "www." should
  // be prepended to the domain where possible.  This should not have a leading
  // '.' (use "com" instead of ".com").
  const string16& desired_tld() const { return desired_tld_; }

  // The type of input supplied.
  Type type() const { return type_; }

  // Returns parsed URL components.
  const url_parse::Parsed& parts() const { return parts_; }

  // The scheme parsed from the provided text; only meaningful when type_ is
  // URL.
  const string16& scheme() const { return scheme_; }

  // The input as an URL to navigate to, if possible.
  const GURL& canonicalized_url() const { return canonicalized_url_; }

  // Returns whether inline autocompletion should be prevented.
  bool prevent_inline_autocomplete() const {
    return prevent_inline_autocomplete_;
  }

  // Returns whether, given an input string consisting solely of a substituting
  // keyword, we should score it like a non-substituting keyword.
  bool prefer_keyword() const { return prefer_keyword_; }

  // Returns whether this input is allowed to be treated as an exact
  // keyword match.  If not, the default result is guaranteed not to be a
  // keyword search, even if the input is "<keyword> <search string>".
  bool allow_exact_keyword_match() const { return allow_exact_keyword_match_; }

  // See description of enum for details.
  MatchesRequested matches_requested() const { return matches_requested_; }

  // operator==() by another name.
  bool Equals(const AutocompleteInput& other) const;

  // Resets all internal variables to the null-constructed state.
  void Clear();

 private:
  string16 text_;
  string16 desired_tld_;
  Type type_;
  url_parse::Parsed parts_;
  string16 scheme_;
  GURL canonicalized_url_;
  bool prevent_inline_autocomplete_;
  bool prefer_keyword_;
  bool allow_exact_keyword_match_;
  MatchesRequested matches_requested_;
};

// AutocompleteResult ---------------------------------------------------------

// All matches from all providers for a particular query.  This also tracks
// what the default match should be if the user doesn't manually select another
// match.
class AutocompleteResult {
 public:
  typedef ACMatches::const_iterator const_iterator;
  typedef ACMatches::iterator iterator;

  // The "Selection" struct is the information we need to select the same match
  // in one result set that was selected in another.
  struct Selection {
    Selection()
        : provider_affinity(NULL),
          is_history_what_you_typed_match(false) {
    }

    // Clear the selection entirely.
    void Clear();

    // True when the selection is empty.
    bool empty() const {
      return destination_url.is_empty() && !provider_affinity &&
          !is_history_what_you_typed_match;
    }

    // The desired destination URL.
    GURL destination_url;

    // The desired provider.  If we can't find a match with the specified
    // |destination_url|, we'll use the best match from this provider.
    const AutocompleteProvider* provider_affinity;

    // True when this is the HistoryURLProvider's "what you typed" match.  This
    // can't be tracked using |destination_url| because its URL changes on every
    // keystroke, so if this is set, we'll preserve the selection by simply
    // choosing the new "what you typed" entry and ignoring |destination_url|.
    bool is_history_what_you_typed_match;
  };

  // Max number of matches we'll show from the various providers.
  static const size_t kMaxMatches;

  // The lowest score a match can have and still potentially become the default
  // match for the result set.
  static const int kLowestDefaultScore;

  AutocompleteResult();
  ~AutocompleteResult();

  // operator=() by another name.
  void CopyFrom(const AutocompleteResult& rhs);

  // Copies matches from |old_matches| to provide a consistant result set. See
  // comments in code for specifics.
  void CopyOldMatches(const AutocompleteInput& input,
                      const AutocompleteResult& old_matches);

  // Adds a single match. The match is inserted at the appropriate position
  // based on relevancy and display order. This is ONLY for use after
  // SortAndCull() has been invoked, and preserves default_match_.
  void AddMatch(const AutocompleteMatch& match);

  // Adds a new set of matches to the result set.  Does not re-sort.
  void AppendMatches(const ACMatches& matches);

  // Removes duplicates, puts the list in sorted order and culls to leave only
  // the best kMaxMatches matches.  Sets the default match to the best match
  // and updates the alternate nav URL.
  void SortAndCull(const AutocompleteInput& input);

  // Returns true if at least one match was copied from the last result.
  bool HasCopiedMatches() const;

  // Vector-style accessors/operators.
  size_t size() const;
  bool empty() const;
  const_iterator begin() const;
  iterator begin();
  const_iterator end() const;
  iterator end();

  // Returns the match at the given index.
  const AutocompleteMatch& match_at(size_t index) const;
  AutocompleteMatch* match_at(size_t index);

  // Get the default match for the query (not necessarily the first).  Returns
  // end() if there is no default match.
  const_iterator default_match() const { return default_match_; }

  const GURL& alternate_nav_url() const { return alternate_nav_url_; }

  // Clears the matches for this result set.
  void Reset();

  void Swap(AutocompleteResult* other);

#ifndef NDEBUG
  // Does a data integrity check on this result.
  void Validate() const;
#endif

 private:
  typedef std::map<AutocompleteProvider*, ACMatches> ProviderToMatches;

#if defined(OS_ANDROID)
  // iterator::difference_type is not defined in the STL that we compile with on
  // Android.
  typedef int matches_difference_type;
#else
  typedef ACMatches::iterator::difference_type matches_difference_type;
#endif

  // Populates |provider_to_matches| from |matches_|.
  void BuildProviderToMatches(ProviderToMatches* provider_to_matches) const;

  // Returns true if |matches| contains a match with the same destination as
  // |match|.
  static bool HasMatchByDestination(const AutocompleteMatch& match,
                                    const ACMatches& matches);

  // Copies matches into this result. |old_matches| gives the matches from the
  // last result, and |new_matches| the results from this result.
  void MergeMatchesByProvider(const ACMatches& old_matches,
                              const ACMatches& new_matches);

  ACMatches matches_;

  const_iterator default_match_;

  // The "alternate navigation URL", if any, for this result set.  This is a URL
  // to try offering as a navigational option in case the user navigated to the
  // URL of the default match but intended something else.  For example, if the
  // user's local intranet contains site "foo", and the user types "foo", we
  // default to searching for "foo" when the user may have meant to navigate
  // there.  In cases like this, the default match will point to the "search for
  // 'foo'" result, and this will contain "http://foo/".
  GURL alternate_nav_url_;

  DISALLOW_COPY_AND_ASSIGN(AutocompleteResult);
};

#endif  // CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_H_
