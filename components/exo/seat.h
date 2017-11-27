// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SEAT_H_
#define COMPONENTS_EXO_SEAT_H_

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/exo/data_source_observer.h"
#include "ui/aura/client/drag_drop_delegate.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/base/clipboard/clipboard_observer.h"

namespace exo {

class ScopedDataSource;
class SeatObserver;
class Surface;

// Seat object represent a group of input devices such as keyboard, pointer and
// touch devices and keeps track of input focus.
class Seat : public aura::client::FocusChangeObserver,
             public ui::ClipboardObserver,
             public DataSourceObserver {
 public:
  Seat();
  ~Seat() override;

  void AddObserver(SeatObserver* observer);
  void RemoveObserver(SeatObserver* observer);

  // Returns currently focused surface. This is vertual so that we can override
  // the behavior for testing.
  virtual Surface* GetFocusedSurface();

  // Sets clipboard data from |source|.
  void SetSelection(DataSource* source);

  // Overridden from aura::client::FocusChangeObserver:
  void OnWindowFocused(aura::Window* gained_focus,
                       aura::Window* lost_focus) override;

  // Overridden from ui::ClipbaordObserver:
  void OnClipboardDataChanged() override;

  // Overridden from DataSourceObserver:
  void OnDataSourceDestroying(DataSource* source) override;

 private:
  // Called when data is read from FD passed from a client.
  // |data| is read data. |source| is source of the data, or nullptr if
  // DataSource has already been destroyed.
  void OnSetSelectionComplete(uint64_t,
                              const std::vector<uint8_t>& data,
                              DataSource* source);

  base::ObserverList<SeatObserver> observers_;

  // Data source being used as a clipboard content.
  std::unique_ptr<ScopedDataSource> selection_source_;

  base::WeakPtrFactory<Seat> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(Seat);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SEAT_H_
