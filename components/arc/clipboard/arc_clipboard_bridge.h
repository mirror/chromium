// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_CLIPBOARD_ARC_CLIPBOARD_BRIDGE_H_
#define COMPONENTS_ARC_CLIPBOARD_ARC_CLIPBOARD_BRIDGE_H_

#include <string>

#include "base/macros.h"
#include "components/arc/arc_service.h"
#include "components/arc/common/clipboard.mojom.h"
#include "components/arc/instance_holder.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard_observer.h"

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
  void SetTextContent(const std::string& text) override;
  void GetTextContent() override;
  void SetHostClip(mojom::ClipDataPtr clip_data) override;
  void GetHostClip() override;

 private:
  class DecodeRequest;

  bool CalledOnValidThread();
  void ConvertAndSendBitmapToInstance(const SkBitmap& bitmap);
  void ConvertBitmapAndSendFdToInstance(const SkBitmap& bitmap);

  void SendImageToInstance(const std::vector<uint8_t>& png_bytes);
  void SendFileToInstance(const std::string& mime_type,
                          base::Optional<base::FilePath> file_path);
  void SendHTMLToInstance(const std::string& url,
                          const std::string& markup,
                          const std::string& text);
  void SendTextToInstance(const std::string& clip_text);
  void SendEmptyToInstance();

  mojo::Binding<mojom::ClipboardHost> binding_;
  base::ThreadChecker thread_checker_;
  uint64_t host_clip_version_;
  uint64_t instance_clip_version_;
  uint64_t clipboard_seq_from_instance_;
  std::vector<base::File> files_to_delete_;
  std::unique_ptr<DecodeRequest> decode_request_;

  DISALLOW_COPY_AND_ASSIGN(ArcClipboardBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_CLIPBOARD_ARC_CLIPBOARD_BRIDGE_H_
