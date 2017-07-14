// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_SOURCE_H_
#define COMPONENTS_EXO_DATA_SOURCE_H_

#include <cstdint>
#include <string>

#include "base/macros.h"
#include "base/threading/thread_checker.h"

namespace exo {

class DataSourceDelegate;

// Object representing transferred data offered by a client.
// Requests and events are compatible wl_data_source. See wl_data_source for
// details.
class DataSource {
 public:
  explicit DataSource(DataSourceDelegate* delegate);
  ~DataSource();

  // Adds an offered mime type
  void Offer(const std::string& mime_type);

  // Sets the available drag-and-drop actions
  void SetActions(uint32_t dnd_actions);

 private:
  DataSourceDelegate* const delegate_;

  DISALLOW_COPY_AND_ASSIGN(DataSource);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_SOURCE_H_
