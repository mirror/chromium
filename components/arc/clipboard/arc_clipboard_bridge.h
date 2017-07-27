// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_CLIPBOARD_ARC_CLIPBOARD_BRIDGE_H_
#define COMPONENTS_ARC_CLIPBOARD_ARC_CLIPBOARD_BRIDGE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "components/arc/common/clipboard.mojom.h"
#include "components/arc/instance_holder.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/base/clipboard/clipboard_observer.h"

namespace content {
class BrowserContext;
}  // namespace content
namespace ui {
class Clipboard;
class ScopedClipboardWriter;
}  // namespace ui

namespace arc {

class ArcBridgeService;

class ArcClipboardBridge
    : public KeyedService,
      public ui::ClipboardObserver,
      public InstanceHolder<mojom::ClipboardInstance>::Observer,
      public mojom::ClipboardHost {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcClipboardBridge* GetForBrowserContext(
      content::BrowserContext* context);

  ArcClipboardBridge(content::BrowserContext* context,
                     ArcBridgeService* bridge_service);
  ~ArcClipboardBridge() override;

  // InstanceHolder<mojom::ClipboardInstance>::Observer overrides.
  void OnInstanceReady() override;

  // ClipboardObserver overrides
  void OnClipboardDataChanged() override;

  // mojom::ClipboardHost overrides.
  void SetTextContentDeprecated(const std::string& text) override;
  void GetTextContentDeprecated() override;
  void SetClipContent(mojom::ClipDataPtr clip_data) override;
  void GetClipContent(const GetClipContentCallback& callback) override;

 private:
  static mojom::ClipItemPtr CreateItemHTML(ui::Clipboard* clipboard);
  static mojom::ClipItemPtr CreateItemPlainText(ui::Clipboard* clipboard);

  static void ProcessItemHTML(mojom::ClipItem* clip_item,
                              ui::ScopedClipboardWriter* writer);
  static void ProcessItemPlainText(mojom::ClipItem* clip_item,
                                   ui::ScopedClipboardWriter* writer);

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.
  mojo::Binding<mojom::ClipboardHost> binding_;

  // clipboard sequence number. used to know whether or not a event comes from
  // Host or Instance
  uint64_t clipboard_seq_from_instance_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(ArcClipboardBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_CLIPBOARD_ARC_CLIPBOARD_BRIDGE_H_
