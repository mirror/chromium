// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_CLIPBOARD_ARC_CLIPBOARD_BRIDGE_H_
#define COMPONENTS_ARC_CLIPBOARD_ARC_CLIPBOARD_BRIDGE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "components/arc/arc_service.h"
#include "components/arc/common/clipboard.mojom.h"
#include "components/arc/instance_holder.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/base/clipboard/clipboard_observer.h"

class SkBitmap;
namespace arc {

class ArcClipboardBridge
    : public ArcService,
      public ui::ClipboardObserver,
      public InstanceHolder<mojom::ClipboardInstance>::Observer,
      public mojom::ClipboardHost {
 public:
  explicit ArcClipboardBridge(ArcBridgeService* bridge_service);
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
  class DecodeRequest;

  void ConvertAndSendBitmap(const GetClipContentCallback& callback,
                            const SkBitmap& bitmap);
  // void ConvertBitmapAndSendFd(const SkBitmap& bitmap);

  void SendImage(const GetClipContentCallback& callback,
                 const std::vector<uint8_t>& png_bytes);
  void SendFile(const GetClipContentCallback& callback,
                const std::string& mime_type,
                const base::FilePath& file_path);
  void SendHTML(const GetClipContentCallback& callback,
                const std::string& url,
                const std::string& markup,
                const std::string& text);
  void SendText(const GetClipContentCallback& callback,
                const std::string& clip_text);
  void SendUnsupported(const GetClipContentCallback& callback,
                       const std::string& error_msg);

  void ProcessHTML(const mojom::ClipDataPtr& clip_data);
  void ProcessPlainText(const mojom::ClipDataPtr& clip_data);
  void ProcessFileDescriptor(const mojom::ClipDataPtr& clip_data);
  void ProcessBlob(const mojom::ClipDataPtr& clip_data);
  void ProcessUnsupported(const mojom::ClipDataPtr& clip_data);

  void ProcessImage(const std::vector<uint8_t>& data);

  mojo::Binding<mojom::ClipboardHost> binding_;
  THREAD_CHECKER(thread_checker_);
  uint64_t clipboard_seq_from_instance_;
  std::vector<int> fds_to_close_;
  std::unique_ptr<DecodeRequest> decode_request_;
  base::WeakPtrFactory<ArcClipboardBridge> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcClipboardBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_CLIPBOARD_ARC_CLIPBOARD_BRIDGE_H_
