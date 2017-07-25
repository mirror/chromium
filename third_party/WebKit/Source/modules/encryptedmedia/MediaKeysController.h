// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaKeysController_h
#define MediaKeysController_h

#include "core/page/Page.h"
#include "modules/ModulesExport.h"
#include "modules/encryptedmedia/MediaKeysClient.h"

namespace blink {

class ExecutionContext;
class MediaKeysClient;
class WebEncryptedMediaClient;

class MODULES_EXPORT MediaKeysController final
    : public GarbageCollectedFinalized<MediaKeysController>,
      public Supplement<Page> {
  USING_GARBAGE_COLLECTED_MIXIN(MediaKeysController);

 public:
  WebEncryptedMediaClient* EncryptedMediaClient(ExecutionContext*);

  static void ProvideMediaKeysTo(Page&, MediaKeysClient*);
  static MediaKeysController* From(Page* page) {
    return static_cast<MediaKeysController*>(
        Supplement<Page>::From(page, SupplementName()));
  }

  DEFINE_INLINE_VIRTUAL_TRACE() { Supplement<Page>::Trace(visitor); }

 private:
  explicit MediaKeysController(MediaKeysClient*);
  static const char* SupplementName();

  // MediaKeysClient is owned by the controller. It contains a pointer to the
  // WebViewBase which will always outlive the page that controls the
  // lifetime of the MediaKeysController.
  std::unique_ptr<MediaKeysClient> client_;
};

}  // namespace blink

#endif  // MediaKeysController_h
