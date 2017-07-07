// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CRASH_UPLOAD_LIST_CRASH_UPLOAD_LIST_ANDROID_H_
#define CHROME_BROWSER_CRASH_UPLOAD_LIST_CRASH_UPLOAD_LIST_ANDROID_H_

#include "base/macros.h"
#include "components/upload_list/text_log_upload_list.h"

namespace base {
class FilePath;
}

// A crash UploadList that retrieves the list of uploaded reports from the
// Android crash reporter.
class CrashUploadListAndroid : public TextLogUploadList {
 public:
  CrashUploadListAndroid(const base::FilePath& upload_log_path);

 protected:
  ~CrashUploadListAndroid() override;

  std::vector<UploadInfo> LoadUploadList() override;
  void PerformSingleUpload(const std::string& local_id) override;

 private:
  void LoadUnsuccessfulUploadList(std::vector<UploadInfo>* uploads);

  DISALLOW_COPY_AND_ASSIGN(CrashUploadListAndroid);
};

#endif  // CHROME_BROWSER_CRASH_UPLOAD_LIST_CRASH_UPLOAD_LIST_ANDROID_H_
