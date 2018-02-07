// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UnhandledTapInfo_h
#define UnhandledTapInfo_h

#include "platform/geometry/IntPoint.h"
#include "public/platform/WebCommon.h"
#include "public/platform/WebPoint.h"
#include "public/platform/unhandled_tap_notifier.mojom-blink.h"

namespace blink {

class LocalFrame;
class Node;

/*
 * Encapsulates information needed by ShowUnhandledTapUIIfNeeded to describe
 * the tap.
 */
class UnhandledTapInfo {
 public:
  // Creates an Unhandled Tap Info notifier for the given LocalFrame.
  explicit UnhandledTapInfo(LocalFrame&);
  virtual ~UnhandledTapInfo();

  // Shows the Unhandled Tap UI if needed.
  void ShowUnhandledTapUIIfNeeded(bool dom_tree_changed,
                                  bool style_changed,
                                  Node* tapped_node,
                                  IntPoint tapped_position_in_viewport) const;

 private:
  // Handle to the mojo service.
  mojom::blink::UnhandledTapNotifierPtr provider_;
};

};  // namespace blink

#endif  // UnhandledTapInfo_h
