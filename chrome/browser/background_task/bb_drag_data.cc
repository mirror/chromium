// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK

#include "chrome/browser/background_task/bb_drag_data.h"

#include "base/pickle.h"
#include "base/string_util.h"
#include "chrome/common/os_exchange_data.h"
#include "webkit/glue/webdropdata.h"

static CLIPFORMAT clipboard_format = 0;

static void RegisterFormat() {
  if (clipboard_format != 0)
    return;
  clipboard_format = ::RegisterClipboardFormat(L"chrome/x-bb-element-entry");
  DCHECK(clipboard_format);
}

BbDragData::BbDragData(const WebDropData& data) {
  // Don't use URL if it is not a <bb>-initiated operation.
  if (!data.is_bb_drag || !data.url.is_valid())
    return;
  url_ = data.url;
  title_ = data.url_title;
  if (title_.empty()) {
    title_ = UTF8ToWide(url_.GetOrigin().spec());
  }
}

void BbDragData::Write(OSExchangeData* data) const {
  DCHECK(data);
  RegisterFormat();

  Pickle data_pickle;
  data_pickle.WriteString(url_.spec());
  data_pickle.WriteWString(title_);

  data->SetPickledData(clipboard_format, data_pickle);
}

bool BbDragData::Read(const OSExchangeData& data) {
  RegisterFormat();

  if (!data.HasFormat(clipboard_format))
    return false;

  Pickle data_pickle;
  if (!data.GetPickledData(clipboard_format, &data_pickle))
    return false;

  void* data_iterator = NULL;
  std::string url_spec;
  std::wstring title;
  if (!data_pickle.ReadString(&data_iterator, &url_spec) ||
      !data_pickle.ReadWString(&data_iterator, &title))
    return false;

  url_ = GURL(url_spec);
  title_ = title;
  return true;
}

#endif  // ENABLE_BACKGROUND_TASK

