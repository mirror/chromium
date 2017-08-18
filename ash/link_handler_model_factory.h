// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LINK_HANDLER_MODEL_FACTORY_H_
#define ASH_LINK_HANDLER_MODEL_FACTORY_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/public/interfaces/link_handler.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"

class GURL;

namespace ash {

class LinkHandlerModel;

// A class for creating a LinkHandlerModel object.
class ASH_EXPORT LinkHandlerModelFactory : public mojom::LinkHandlerController {
 public:
  LinkHandlerModelFactory();
  ~LinkHandlerModelFactory() override;

  void BindRequest(mojom::LinkHandlerControllerRequest request);

  // mojom::LinkHandlerController
  void ConnectModel(mojom::LinkHandlerObserverAssociatedPtrInfo observer,
                    const GURL& url) override;
  void OpenLinkWithHandler(const GURL& url, uint32_t handler_id) override;

  // Returns a model for the |url|. When such a model for the |url| cannot be
  // created, returns nullptr.
  virtual std::unique_ptr<LinkHandlerModel> CreateModel(const GURL& url);

 private:
  mojo::BindingSet<mojom::LinkHandlerController> bindings_;

  DISALLOW_COPY_AND_ASSIGN(LinkHandlerModelFactory);
};

}  // namespace ash

#endif  // ASH_LINK_HANDLER_MODEL_FACTORY_H_
