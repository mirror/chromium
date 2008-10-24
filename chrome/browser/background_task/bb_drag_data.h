// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK

#ifndef CHROME_BROWSER_BACKGROUND_TASK_BB_DRAG_DATA_
#define CHROME_BROWSER_BACKGROUND_TASK_BB_DRAG_DATA_

#include "googleurl/src/gurl.h"

class OSExchangeData;
class Pickle;
struct WebDropData;

// BbDragData is used to represent a dragged <BB> HTML element.
// <BB> is a "special action" element that uses behavior supplied by the
// browser to "install" a new web app or background task.
// The data includes a URL of the app/task and the name which is used to
// identify the app/task. The name is unique for a given url's origin space.

class BbDragData {
 public:
  BbDragData() {}
  // Created a BbDragData populated from WebDropData.
  explicit BbDragData(const WebDropData& data);

  // Writes this BbDragData to data.
  void Write(OSExchangeData* data) const;

  // Restores this data from the clipboard, returning true on success.
  bool Read(const OSExchangeData& data);

  // The URL of application or background task page.
  const GURL& url() {
    return url_;
  }

  // Title of the application or background task.
  const std::wstring& title() {
    return title_;
  }

 private:
  GURL url_;
  std::wstring title_;
  DISALLOW_COPY_AND_ASSIGN(BbDragData);
};

#endif  // CHROME_BROWSER_BACKGROUND_TASK_BB_DRAG_DATA_
#endif  // ENABLE_BACKGROUND_TASK

