// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LINK_HANDLER_CONTROLLER_H_
#define ASH_LINK_HANDLER_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/public/interfaces/link_handler.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"

class GURL;

namespace ash {

class LinkHandlerModel;

// Handles requests for a list of link handlers based on a given URL by
// generating a LinkHandler and associating it with the client that made the
// request (LinkHandlerObserver).
class ASH_EXPORT LinkHandlerController : public mojom::LinkHandlerController {
 public:
  LinkHandlerController();
  ~LinkHandlerController() override;

  void BindRequest(mojom::LinkHandlerControllerRequest request);

  // mojom::LinkHandlerController
  void ConnectModel(mojom::LinkHandlerObserverAssociatedPtrInfo observer,
                    const GURL& url) override;
  void OpenLinkWithHandler(const GURL& url, uint32_t handler_id) override;

 private:
  // The current link handler (at most one may be active at a time).
  std::unique_ptr<LinkHandlerModel> model_;
  // The URL for |model_|.
  GURL url_;

  mojo::BindingSet<mojom::LinkHandlerController> bindings_;

  DISALLOW_COPY_AND_ASSIGN(LinkHandlerController);
};

}  // namespace ash

#endif  // ASH_LINK_HANDLER_CONTROLLER_H_
