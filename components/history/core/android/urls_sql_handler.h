// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_ANDROID_URLS_SQL_HANDLER_H_
#define COMPONENTS_HISTORY_CORE_ANDROID_URLS_SQL_HANDLER_H_

#include "components/history/core/android/sql_handler.h"

namespace history {

class URLDatabase;

// This class is the SQLHandler implementation for urls table.
class UrlsSQLHandler : public SQLHandler {
 public:
  explicit UrlsSQLHandler(URLDatabase* url_db);
  virtual ~UrlsSQLHandler();

  // Overriden from SQLHandler.
  virtual bool Insert(HistoryAndBookmarkRow* row) override;
  virtual bool Update(const HistoryAndBookmarkRow& row,
                      const TableIDRows& ids_set) override;
  virtual bool Delete(const TableIDRows& ids_set) override;

 private:
  URLDatabase* url_db_;

  DISALLOW_COPY_AND_ASSIGN(UrlsSQLHandler);
};

}  // namespace history.

#endif  // COMPONENTS_HISTORY_CORE_ANDROID_URLS_SQL_HANDLER_H_
