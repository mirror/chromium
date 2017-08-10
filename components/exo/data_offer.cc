// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_offer.h"

#include <stdio.h>

#include "base/task_scheduler/post_task.h"
#include "components/exo/data_offer_delegate.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/dragdrop/os_exchange_data.h"

namespace exo {
namespace {
void WriteBytesToFdOnIOThread(base::ScopedFD fd, std::vector<uint8_t> bytes) {
  const uint8_t* it = bytes.data();
  const uint8_t* end = bytes.data() + bytes.size();
  while (it != end) {
    const int result = write(fd.get(), it, end - it);
    if (result == -1) {
      LOG(ERROR) << "Failed to write drop data " << strerror(errno);
      return;
    }
    it += result;
  }
}

std::vector<uint8_t> GetString16Bytes(const base::string16& string) {
  const size_t bytes_size = string.size() * sizeof(base::char16);
  std::vector<uint8_t> bytes(bytes_size);
  memcpy(bytes.data(), string.data(), bytes_size);
  return bytes;
}
}  // namespace

DataOffer::DataOffer(DataOfferDelegate* delegate)
    : delegate_(delegate), weak_factory_(this) {}

DataOffer::~DataOffer() {
  delegate_->OnDataOfferDestroying(this);
}

base::WeakPtr<DataOffer> DataOffer::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void DataOffer::Accept(const std::string& mime_type) {
  NOTIMPLEMENTED();
}

void DataOffer::Receive(const std::string& mime_type, base::ScopedFD fd) {
  const auto it = drop_data_.find(mime_type);
  if (it == drop_data_.end()) {
    LOG(ERROR) << "Unexpected mime type is requested";
    return;
  }

  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      base::BindOnce(&WriteBytesToFdOnIOThread, std::move(fd), it->second));
}

void DataOffer::Finish() {
  NOTIMPLEMENTED();
}

void DataOffer::SetActions(const base::flat_set<DndAction>& dnd_actions,
                           DndAction preferred_action) {
  dnd_action_ = preferred_action;
  delegate_->OnAction(preferred_action);
}

void DataOffer::SetSourceActions(
    const base::flat_set<DndAction>& source_actions) {
  source_actions_ = source_actions;
  delegate_->OnSourceActions(source_actions);
}

void DataOffer::SetDropData(const ui::OSExchangeData& data) {
  DCHECK_EQ(0u, drop_data_.size());
  if (data.HasString()) {
    base::string16 string_content;
    data.GetString(&string_content);
    drop_data_.emplace(std::string(ui::Clipboard::kMimeTypeText),
                       GetString16Bytes(string_content));
  }
  for (const auto& pair : drop_data_) {
    delegate_->OnOffer(pair.first);
  }
}

}  // namespace exo
