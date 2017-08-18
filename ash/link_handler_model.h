// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LINK_HANDLER_MODEL_H_
#define ASH_LINK_HANDLER_MODEL_H_

#include <string>
#include <vector>

#include "ash/ash_export.h"
#include "ash/public/interfaces/link_handler.mojom.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "ui/gfx/image/image.h"

class GURL;

namespace ash {

// A model LinkHandlerModelFactory returns.
class ASH_EXPORT LinkHandlerModel {
 public:
  virtual ~LinkHandlerModel();
  void AddObserver(mojom::LinkHandlerObserverAssociatedPtrInfo observer);

  // Opens the URL with the handler specified by the |handler_id|.
  virtual void OpenLinkWithHandler(uint32_t handler_id) = 0;

 protected:
  LinkHandlerModel();

  mojo::AssociatedInterfacePtrSet<mojom::LinkHandlerObserver>* observers() {
    return &observers_;
  }

 private:
  mojo::AssociatedInterfacePtrSet<mojom::LinkHandlerObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(LinkHandlerModel);
};

}  // namespace ash

#endif  // ASH_LINK_HANDLER_MODEL_H_
