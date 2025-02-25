// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_QUERY_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_QUERY_H_

#include <stddef.h>

#include <map>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "content/public/browser/download_item.h"

namespace base {
class Value;
}

// Filter and sort a vector of DownloadItem*s.
//
// The following example copies from |all_items| to |results| those
// DownloadItem*s whose start time is 0 and whose id is odd, sorts primarily by
// bytes received ascending and secondarily by url descending, and limits the
// results to 20 items. Any number of filters or sorters is allowed. If all
// sorters compare two DownloadItems equivalently, then they are sorted by their
// id ascending.
//
// DownloadQuery query;
// base::Value start_time(0);
// CHECK(query.AddFilter(FILTER_START_TIME, start_time));
// bool FilterOutOddDownloads(const DownloadItem& item) {
//   return 0 == (item.GetId() % 2);
// }
// CHECK(query.AddFilter(base::Bind(&FilterOutOddDownloads)));
// query.AddSorter(SORT_BYTES_RECEIVED, ASCENDING);
// query.AddSorter(SORT_URL, DESCENDING);
// query.Limit(20);
// DownloadVector all_items, results;
// query.Search(all_items.begin(), all_items.end(), &results);
class DownloadQuery {
 public:
  typedef std::vector<content::DownloadItem*> DownloadVector;

  // FilterCallback is a Callback that takes a DownloadItem and returns true if
  // the item matches the filter and false otherwise.
  // query.AddFilter(base::Bind(&YourFilterFunction));
  typedef base::Callback<bool(const content::DownloadItem&)> FilterCallback;

  // All times are ISO 8601 strings.
  enum FilterType {
    FILTER_BYTES_RECEIVED,       // double
    FILTER_DANGER_ACCEPTED,      // bool
    FILTER_ENDED_AFTER,          // string
    FILTER_ENDED_BEFORE,         // string
    FILTER_END_TIME,             // string
    FILTER_EXISTS,               // bool
    FILTER_FILENAME,             // string
    FILTER_FILENAME_REGEX,       // string
    FILTER_MIME,                 // string
    FILTER_PAUSED,               // bool
    FILTER_QUERY,                // vector<base::string16>
    FILTER_STARTED_AFTER,        // string
    FILTER_STARTED_BEFORE,       // string
    FILTER_START_TIME,           // string
    FILTER_TOTAL_BYTES,          // double
    FILTER_TOTAL_BYTES_GREATER,  // double
    FILTER_TOTAL_BYTES_LESS,     // double
    FILTER_ORIGINAL_URL,         // string
    FILTER_ORIGINAL_URL_REGEX,   // string
    FILTER_URL,                  // string
    FILTER_URL_REGEX,            // string
  };

  enum SortType {
    SORT_BYTES_RECEIVED,
    SORT_DANGER,
    SORT_DANGER_ACCEPTED,
    SORT_END_TIME,
    SORT_EXISTS,
    SORT_FILENAME,
    SORT_MIME,
    SORT_PAUSED,
    SORT_START_TIME,
    SORT_STATE,
    SORT_TOTAL_BYTES,
    SORT_ORIGINAL_URL,
    SORT_URL,
  };

  enum SortDirection {
    ASCENDING,
    DESCENDING,
  };

  static bool MatchesQuery(const std::vector<base::string16>& query_terms,
                           const content::DownloadItem& item);

  DownloadQuery();
  ~DownloadQuery();

  // Adds a new filter of type |type| with value |value| and returns true if
  // |type| is valid and |value| is the correct Value-type and well-formed.
  // Returns false if |type| is invalid or |value| is the incorrect Value-type
  // or malformed.  Search() will filter out all DownloadItem*s that do not
  // match all filters.  Multiple instances of the same FilterType are allowed,
  // so you can pass two regexes to AddFilter(URL_REGEX,...) in order to
  // Search() for items whose url matches both regexes. You can also pass two
  // different DownloadStates to AddFilter(), which will cause Search() to
  // filter out all items.
  bool AddFilter(const FilterCallback& filter);
  bool AddFilter(FilterType type, const base::Value& value);
  void AddFilter(download::DownloadDangerType danger);
  void AddFilter(content::DownloadItem::DownloadState state);

  // Adds a new sorter of type |type| with direction |direction|.  After
  // filtering DownloadItem*s, Search() will sort the results primarily by the
  // sorter from the first call to Sort(), secondarily by the sorter from the
  // second call to Sort(), and so on. For example, if the InputIterator passed
  // to Search() yields four DownloadItems {id:0, error:0, start_time:0}, {id:1,
  // error:0, start_time:1}, {id:2, error:1, start_time:0}, {id:3, error:1,
  // start_time:1}, and Sort is called twice, once with (SORT_ERROR, ASCENDING)
  // then with (SORT_START_TIME, DESCENDING), then Search() will return items
  // ordered 1,0,3,2.
  void AddSorter(SortType type, SortDirection direction);

  // Limit the size of search results to |limit|.
  void Limit(size_t limit) { limit_ = limit; }

  // Filters DownloadItem*s from |iter| to |last| into |results|, sorts
  // |results|, and limits the size of |results|. |results| must be non-NULL.
  template <typename InputIterator>
  void Search(InputIterator iter, const InputIterator last,
              DownloadVector* results) const {
    results->clear();
    for (; iter != last; ++iter) {
      if (Matches(**iter)) results->push_back(*iter);
    }
    FinishSearch(results);
  }

 private:
  struct Sorter;
  class DownloadComparator;
  typedef std::vector<FilterCallback> FilterCallbackVector;
  typedef std::vector<Sorter> SorterVector;

  bool FilterRegex(const std::string& regex_str,
                   const base::Callback<std::string(
                       const content::DownloadItem&)>& accessor);
  bool Matches(const content::DownloadItem& item) const;
  void FinishSearch(DownloadVector* results) const;

  FilterCallbackVector filters_;
  SorterVector sorters_;
  size_t limit_;

  DISALLOW_COPY_AND_ASSIGN(DownloadQuery);
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_QUERY_H_
