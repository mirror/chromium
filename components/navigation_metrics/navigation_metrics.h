// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NAVIGATION_METRICS_NAVIGATION_METRICS_H_
#define COMPONENTS_NAVIGATION_METRICS_NAVIGATION_METRICS_H_

class GURL;

namespace navigation_metrics {

// A Scheme is an C++ enum type loggable in UMA for a histogram of UMA enum type
// NavigationScheme.
//
// These values are written to logs. New enum values can be added, but existing
// value must never be renumbered or deleted and reused. Any new scheme must
// be added at the end, before SCHEME_MAX.
enum Scheme : int {
  SCHEME_UNKNOWN = 0,
  SCHEME_HTTP = 1,
  SCHEME_HTTPS = 2,
  SCHEME_FILE = 3,
  SCHEME_FTP = 4,
  SCHEME_DATA = 5,
  SCHEME_JAVASCRIPT = 6,
  SCHEME_ABOUT = 7,
  SCHEME_CHROME = 8,
  SCHEME_BLOB = 9,
  SCHEME_FILESYSTEM = 10,
  SCHEME_CHROME_NATIVE = 11,
  SCHEME_CHROME_SEARCH = 12,
  SCHEME_CHROME_DISTILLER = 13,
  SCHEME_CHROME_DEVTOOLS = 14,
  SCHEME_CHROME_EXTENSION = 15,
  SCHEME_VIEW_SOURCE = 16,
  SCHEME_EXTERNALFILE = 17,
  SCHEME_MAX,
};

Scheme GetScheme(const GURL& url);

void RecordMainFrameNavigation(const GURL& url,
                               bool is_same_document,
                               bool is_off_the_record);

void RecordOmniboxURLNavigation(const GURL& url);

}  // namespace navigation_metrics

#endif  // COMPONENTS_NAVIGATION_METRICS_NAVIGATION_METRICS_H_
