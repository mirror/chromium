// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebUnhandledTapInfo_h
#define WebUnhandledTapInfo_h

#include "core/dom/Node.h"
#include "platform/geometry/IntPoint.h"
#include "platform/heap/Persistent.h"
#include "public/platform/WebCommon.h"
#include "public/platform/WebPoint.h"
#include "public/web/WebNode.h"
#include "public/web/unhandled_tap_info.mojom-blink.h"

namespace blink {

class LocalFrame;

/*
 * Encapsulates information needed by ShowUnhandledTapUIIfNeeded to describe
 * the tap.
 */
class CORE_EXPORT WebUnhandledTapInfo final
    : public mojom::blink::UnhandledTapNotifierService {
 public:
  //  TODO(donnd): document!!!!!!!!!!!!
  explicit WebUnhandledTapInfo(LocalFrame& frame);
  static void BindMojoRequest(
      LocalFrame* frame,
      mojom::blink::UnhandledTapNotifierServiceRequest unhandled_tap_info);

  //  TODO(donnd): complete!!!!!!!!!!!!
  virtual void ShowUnhandledTapUIIfNeeded(
      mojom::blink::UnhandledTapInfoPtr unhandled_tap_info);

  // TODO(donnd): remove ?????????????
  void ShowUnhandledTapUIIfNeeded(bool dom_tree_changed,
                                  bool style_changed,
                                  Node* tapped_node,
                                  IntPoint tapped_position_in_viewport) const;

 private:
  WeakPersistent<LocalFrame> frame_;
};

};  // namespace blink

#endif  // WebUnhandledTapInfo_h
