// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_OFFLINE_PAGE_MODEL_UTILS_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_OFFLINE_PAGE_MODEL_UTILS_H_

#include <string>

#include "base/callback_forward.h"

class GURL;

namespace base {
class FilePath;
}

namespace offline_pages {

enum class OfflinePagesNamespaceEnumeration;
struct ClientId;

namespace model_utils {

typedef base::OnceCallback<void(base::FilePath)> GenerateUniqueFileNameCallback;

// Return the enum value of the namespace represented by |name_space|.
OfflinePagesNamespaceEnumeration ToNamespaceEnum(const std::string& name_space);

// Metric collection related.
std::string AddHistogramSuffix(const ClientId& client_id,
                               const char* histogram_name);

// Generate a unique name in |target_dir|.
// The file name is generated based on url and title:
// {hostname of |url|}-{title truncated at 40 bytes} [(unique_counter)].mhtml
// The generated file name will be returned via |callback|, if there's no
// available file names, it will return an empty FilePath.
void GenerateUniqueFileName(const base::FilePath& target_dir,
                            const GURL& url,
                            const std::string& title,
                            GenerateUniqueFileNameCallback callback);

}  // namespace model_utils

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_OFFLINE_PAGE_MODEL_UTILS_H_
