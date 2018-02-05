// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MOVE_AND_ADD_RESULTS_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MOVE_AND_ADD_RESULTS_H_

#include <stdint.h>

#include <string>

#include "base/memory/ref_counted.h"
#include "components/offline_pages/core/offline_page_types.h"

namespace offline_pages {

class MoveAndAddResults : public base::RefCountedThreadSafe<MoveAndAddResults> {
 public:
  MoveAndAddResults();
  SavePageResult move_result() { return move_result_; }
  void set_move_result(SavePageResult move_result) {
    move_result_ = move_result;
  }
  base::FilePath new_file_path() { return new_file_path_; }
  void set_new_file_path(base::FilePath new_file_path) {
    new_file_path_ = new_file_path;
  }
  int64_t download_id() { return download_id_; }
  void set_download_id(int64_t download_id) { download_id_ = download_id; }

 private:
  friend class base::RefCountedThreadSafe<MoveAndAddResults>;
  ~MoveAndAddResults();

  SavePageResult move_result_;
  base::FilePath new_file_path_;
  int64_t download_id_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MOVE_AND_ADD_RESULTS_H_
