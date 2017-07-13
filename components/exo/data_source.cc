// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_source.h"

namespace exo {

DataSourceDelegate::~DataSourceDelegate() = default;

DataSource::DataSource(std::unique_ptr<DataSourceDelegate> delegate)
    : delegate_(std::move(delegate)) {}

DataSource::~DataSource() = default;

void DataSource::Offer(const std::string& mime_type) {
  NOTIMPLEMENTED();
}

void DataSource::SetActions(uint32_t dnd_actions) {
  NOTIMPLEMENTED();
}

}  // namespace exo
