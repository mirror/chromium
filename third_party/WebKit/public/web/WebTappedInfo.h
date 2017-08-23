// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebTappedInfo_h
#define WebTappedInfo_h

#include "core/dom/Node.h"
#include "platform/geometry/IntPoint.h"
#include "public/platform/WebCommon.h"
#include "public/platform/WebPoint.h"
#include "public/web/WebNode.h"

namespace blink {

struct WebPoint;

/*
 * Encapsulates information needed by ShowUnhandledTapUIIfNeeded to describe
 * the tap.
 */
class BLINK_EXPORT WebTappedInfo {
 public:
  WebTappedInfo(IntPoint tapped_position_in_viewport,
                Node* tapped_node,
                bool page_changed,
                int text_height)
      : point_(WebPoint(tapped_position_in_viewport.X(),
                        tapped_position_in_viewport.Y())),
        web_node_(
            WebNode()),  // TODO(donnd): create from the Node* tapped_node!!!
        page_changed_(page_changed),
        text_height_(text_height) {}

  const WebPoint& Position() const { return point_; }
  const WebNode& GetNode() const { return web_node_; }
  bool PageChanged() const { return page_changed_; }
  int TextHeight() const { return text_height_; }

 private:
  const WebPoint point_;
  const WebNode web_node_;
  bool page_changed_;
  int text_height_;
};

}  // namespace blink

#endif
