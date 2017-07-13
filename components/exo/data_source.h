// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_SOURCE_H_
#define COMPONENTS_EXO_DATA_SOURCE_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"

namespace exo {

// Provides protocol specific implementations of DataSource.
class DataSourceDelegate {
 public:
  virtual ~DataSourceDelegate();
  virtual void Target(const std::string& mime_type);
  virtual void Send(const std::string& mime_type, base::ScopedFD fd);
  virtual void Cancelled();
  virtual void DndDropPerformed();
  virtual void DndFinished();
  virtual void Action(uint32_t dnd_action);
};

// Object representing the transferred data offered by a client.
// Requests and events are compatible wl_data_source. See wl_data_source for
// details.
class DataSource {
 public:
  explicit DataSource(std::unique_ptr<DataSourceDelegate> delegate);

  void Offer(const std::string& mime_type);
  void SetActions(uint32_t dnd_actions);

 private:
  std::unique_ptr<DataSourceDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(DataSource);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_SOURCE_H_
