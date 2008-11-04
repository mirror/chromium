// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A struct for managing data being dropped on a webview.  This represents a
// union of all the types of data that can be dropped in a platform neutral
// way.

#ifndef WEBKIT_GLUE_WEBDROPDATA_H__
#define WEBKIT_GLUE_WEBDROPDATA_H__

#include <string>
#include <vector>
#include "googleurl/src/gurl.h"

#ifdef ENABLE_BACKGROUND_TASK
#include "webkit/glue/webcursor.h"
#endif  // ENABLE_BACKGROUND_TASK

struct IDataObject;

struct WebDropData {

#ifdef ENABLE_BACKGROUND_TASK
  WebDropData() : is_bb_drag(false), data_object(NULL) {}
#endif  // ENABLE_BACKGROUND_TASK

  // User is dropping a link on the webview.
  GURL url;
  std::wstring url_title;  // The title associated with |url|.

  // User is dropping one or more files on the webview.
  std::vector<std::wstring> filenames;

  // User is dragging plain text into the webview.
  std::wstring plain_text;

  // User is dragging MS HTML into the webview (e.g., out of IE).
  std::wstring cf_html;

  // User is dragging text/html into the webview (e.g., out of Firefox).
  std::wstring text_html;

  // User is dragging data from the webview (e.g., an image).
  std::wstring file_description_filename;
  std::string file_contents;

#ifdef ENABLE_BACKGROUND_TASK
  // HTML5 defines a <BB> element that is a special "Browser Button". A browser
  // implementation decides what happens when a user clicks it or drags it.
  // If the user drags it, WebKit initializes 'url', 'url_title' and starts
  // the drag operation. The WebDropData for such operation has
  // is_bb_drag == true.
  bool is_bb_drag;
  // Contains a picture of the dragged bb element.
  WebCursor drag_cursor;
#endif  // ENABLE_BACKGROUND_TASK

  // A reference to the underlying IDataObject.  This is a Windows drag and
  // drop specific object.  This should only be used by the test shell.
  IDataObject* data_object;

  // Helper method for converting Window's specific IDataObject to a WebDropData
  // object.
  static void PopulateWebDropData(IDataObject* data_object,
                                  WebDropData* drop_data);
};

#endif  // WEBKIT_GLUE_WEBDROPDATA_H__

