// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_DEVICE_H_
#define COMPONENTS_EXO_DATA_DEVICE_H_

#include <cstdint>

#include "base/macros.h"

namespace ui {
class DropTargetEvent;
}

namespace exo {

class DataDeviceDelegate;
class DataSource;
class DataEventDispatcher;
class Surface;

enum class DndAction { kNone, kCopy, kMove, kAsk };

// Data transfer device providing access to inter-client data transfer
// mechanisms such as copy-and-paste and drag-and-drop.
class DataDevice {
 public:
  DataDevice(DataDeviceDelegate* delegate,
             DataEventDispatcher* data_event_dispatcher);
  ~DataDevice();

  // Starts drag-and-drop operation.
  // |source| is data source for the eventual transfer or null if data passing
  // is handled by a client internally. |origin| is a surface where the drag
  // originates. |icon| is drag-and-drop icon surface, which can be nullptr.
  // |serial| is a unique number of implicit grab.
  void StartDrag(const DataSource* source,
                 Surface* origin,
                 Surface* icon,
                 uint32_t serial);

  // Copies data to the selection.
  // |source| is data source for the selection, or nullptr to unset the
  // selection. |serial| is a unique number of event which tigers SetSelection.
  void SetSelection(const DataSource* source, uint32_t serial);

  // This should return true if |surface| is a valid target for this data
  // device. E.g. the surface is owned by the same client as the pointer.
  bool CanAcceptDataEventsForSurface(Surface* surface) const;

  // Called when dragging cursor enters the |surface|.
  void OnDragEntered(Surface* surface, const ui::DropTargetEvent& event);

  // Called when dragging cursor updates on the surface.
  int OnDragUpdated(const ui::DropTargetEvent& event);

  // Called when dragging cursor leaves the surface.
  void OnDragExited();

  // Called when drop operation is performed on the surface.
  void OnPerformDrop(const ui::DropTargetEvent& event);

 private:
  DataEventDispatcher* const data_event_dispatcher_;
  DataDeviceDelegate* const delegate_;

  DISALLOW_COPY_AND_ASSIGN(DataDevice);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_DEVICE_H_
