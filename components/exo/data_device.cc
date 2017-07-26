// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_device.h"

#include "base/logging.h"
#include "components/exo/data_device_delegate.h"
#include "components/exo/data_event_dispatcher.h"
#include "components/exo/data_offer.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/drop_target_event.h"

namespace exo {

DataDevice::DataDevice(DataDeviceDelegate* delegate,
                       DataEventDispatcher* data_event_dispatcher)
    : delegate_(delegate),
      data_event_dispatcher_(data_event_dispatcher),
      data_offer_(nullptr) {
  data_event_dispatcher_->AddDataDevice(this);
}

DataDevice::~DataDevice() {
  delegate_->OnDataDeviceDestroying(this);
  data_event_dispatcher_->RemoveDataDevice(this);
}

void DataDevice::StartDrag(const DataSource* source_resource,
                           Surface* origin_resource,
                           Surface* icon_resource,
                           uint32_t serial) {
  // TODO(hirono): Check if serial is valid. crbug.com/746111
  NOTIMPLEMENTED();
}

void DataDevice::SetSelection(const DataSource* source, uint32_t serial) {
  // TODO(hirono): Check if serial is valid. crbug.com/746111
  NOTIMPLEMENTED();
}

bool DataDevice::CanAcceptDataEventsForSurface(Surface* surface) const {
  return delegate_->CanAcceptDataEventsForSurface(surface);
}

void DataDevice::OnDragEntered(Surface* surface,
                               const ui::DropTargetEvent& event) {
  DCHECK(!data_offer_);

  const ui::OSExchangeData* data = &event.data();

  std::vector<std::string> mime_types;
  // TODO(hirono): Add more mime type support.
  if (data->HasString())
    mime_types.push_back(ui::Clipboard::kMimeTypeText);

  base::flat_set<DndAction> dnd_actions;
  if (event.source_operations() & ui::DragDropTypes::DRAG_MOVE) {
    dnd_actions.insert(DndAction::kMove);
  }
  if (event.source_operations() & ui::DragDropTypes::DRAG_COPY) {
    dnd_actions.insert(DndAction::kCopy);
  }
  if (event.source_operations() & ui::DragDropTypes::DRAG_LINK) {
    dnd_actions.insert(DndAction::kAsk);
  }

  data_offer_ =
      delegate_->OnDataOffer(mime_types, dnd_actions, DndAction::kNone)
          ->AsWeakPtr();
  delegate_->OnEnter(surface, event.location_f(), *data_offer_);
}

int DataDevice::OnDragUpdated(const ui::DropTargetEvent& event) {
  if (!data_offer_)
    return ui::DragDropTypes::DRAG_NONE;

  delegate_->OnMotion(event.time_stamp(), event.location_f());

  // TODO(hirono): dnd_action() here may not be updated. Chroem needs to provide
  // a way to update DND action asynchronously.
  switch (data_offer_->dnd_action()) {
    case DndAction::kMove:
      return ui::DragDropTypes::DRAG_MOVE;
    case DndAction::kCopy:
      return ui::DragDropTypes::DRAG_COPY;
    case DndAction::kAsk:
      return ui::DragDropTypes::DRAG_LINK;
    case DndAction::kNone:
      return ui::DragDropTypes::DRAG_NONE;
  }
}

void DataDevice::OnDragExited() {
  if (!data_offer_)
    return;

  delegate_->OnLeave();
  data_offer_ = nullptr;
}

void DataDevice::OnPerformDrop(const ui::DropTargetEvent& event) {
  if (!data_offer_)
    return;

  delegate_->OnDrop();
  data_offer_ = nullptr;
}

}  // namespace exo
