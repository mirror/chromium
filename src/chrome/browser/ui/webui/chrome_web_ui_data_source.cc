// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chrome_web_ui_data_source.h"

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "base/string_util.h"
#include "chrome/common/jstemplate_builder.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

ChromeWebUIDataSource::ChromeWebUIDataSource(const std::string& source_name)
    : DataSource(source_name, MessageLoop::current()),
      default_resource_(-1) {}

ChromeWebUIDataSource::~ChromeWebUIDataSource() {
}

void ChromeWebUIDataSource::AddString(const std::string& name,
                                      const string16& value) {
  localized_strings_.SetString(name, value);
}

void ChromeWebUIDataSource::AddLocalizedString(const std::string& name,
                                               int ids) {
  localized_strings_.SetString(name, l10n_util::GetStringUTF16(ids));
}

std::string ChromeWebUIDataSource::GetMimeType(const std::string& path) const {
  if (EndsWith(path, ".js", false))
    return "application/javascript";

  return "text/html";
}

void ChromeWebUIDataSource::StartDataRequest(const std::string& path,
                                             bool is_incognito,
                                             int request_id) {
  if (path == json_path_) {
    SendLocalizedStringsAsJSON(request_id);
  } else {
    int resource_id = default_resource_;
    std::map<std::string, int>::iterator result;
    result = path_to_idr_map_.find(path);
    if (result != path_to_idr_map_.end())
      resource_id = result->second;
    DCHECK_NE(resource_id, -1);
    SendFromResourceBundle(request_id, resource_id);
  }
}

void ChromeWebUIDataSource::SendLocalizedStringsAsJSON(int request_id) {
  std::string template_data;
  scoped_refptr<RefCountedBytes> response(new RefCountedBytes);
  SetFontAndTextDirection(&localized_strings_);
  jstemplate_builder::AppendJsonJS(&localized_strings_, &template_data);
  response->data.resize(template_data.size());
  std::copy(template_data.begin(), template_data.end(),response->data.begin());
  SendResponse(request_id, response);
}

void ChromeWebUIDataSource::SendFromResourceBundle(int request_id, int idr) {
  scoped_refptr<RefCountedStaticMemory> response(
      ResourceBundle::GetSharedInstance().LoadDataResourceBytes(idr));
  SendResponse(request_id, response);
}
