// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PATCH_SERVICE_PUBLIC_CPP_PATCH_H_
#define COMPONENTS_PATCH_SERVICE_PUBLIC_CPP_PATCH_H_

#include <string>

#include "base/callback_forward.h"

namespace base {
class FilePath;
}

namespace service_manager {
class Connector;
}

namespace unzip {

// Unzips |zip_file| into |output_dir|.
using UnzipCallback = base::OnceCallback<void(bool result)>;
void Unzip(service_manager::Connector* connector,
           const base::FilePath& zip_file,
           const base::FilePath& output_dir,
           UnzipCallback result_callback);

}  // namespace unzip

#endif  // COMPONENTS_PATCH_SERVICE_PUBLIC_CPP_PATCH_H_
