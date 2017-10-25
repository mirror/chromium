// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BOOKMARK_APP_ERROR_H_
#define CHROME_BROWSER_UI_BOOKMARK_APP_ERROR_H_

#include "base/logging.h"
#include "base/macros.h"

namespace content {
class WebContents;
}

namespace chrome {

// Cross-platform interface to show the Bookmark App Error UI.
class BookmarkAppError {
 public:

  static BookmarkAppError* Create(content::WebContents* web_contents);

  virtual ~BookmarkAppError();

  void OpenInChrome();

 protected:
  BookmarkAppError(content::WebContents* web_contents);

 private:
  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkAppError);
};

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_BOOKMARK_APP_ERROR_H_
