// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_MODULES_MANIFEST_MANIFESTMANAGER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_MODULES_MANIFEST_MANIFESTMANAGER_H_

#include <memory>

#include "core/frame/LocalFrame.h"
#include "modules/ModulesExport.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "platform/Supplementable.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"
#include "third_party/WebKit/public/platform/modules/manifest/manifest_manager.mojom-blink.h"

class KURL;

namespace blink {

// The ManifestManager is a helper class that takes care of fetching and parsing
// the Manifest of the associated RenderFrame. It uses
// ManifestManager::Fetcher and ManifestParser in order to do so.
//
// Consumers should use the mojo ManifestManager interface to use this class.
class MODULES_EXPORT ManifestManager
    : public GarbageCollectedFinalized<ManifestManager>,
      public Supplement<Document>,
      public mojom::blink::ManifestManager {
  WTF_MAKE_NONCOPYABLE(ManifestManager);
  USING_GARBAGE_COLLECTED_MIXIN(ManifestManager);

 public:
  explicit ManifestManager(Document&);
  ~ManifestManager() override;

  void Trace(Visitor*) override;

  static const char* SupplementName();
  static void BindMojoRequest(LocalFrame*,
                              mojom::blink::ManifestManagerRequest);
  static ManifestManager* From(Document*);
  static void ProvideTo(Document&);

 private:
  class Fetcher;

  using InternalRequestManifestCallback =
      base::OnceCallback<void(const KURL&,
                              const mojom::blink::Manifest*,
                              const mojom::blink::ManifestDebugInfo*)>;

  void BindToRequest(mojom::blink::ManifestManagerRequest);

  // mojom::blink::ManifestManager implementation.
  void RequestManifest(RequestManifestCallback) override;
  void RequestManifestDebugInfo(RequestManifestDebugInfoCallback) override;

  void RequestManifestImpl(InternalRequestManifestCallback);

  void FetchManifest();
  void OnManifestFetchComplete(const KURL& manifest_url, const String& data);
  void ResolveCallbacks();

  bool CheckForManifestChange();
  Document& GetDocument() { return *GetSupplementable(); }
  HTMLLinkElement* GetManifestLink() { return GetDocument().LinkManifest(); }
  KURL GetManifestUrl();
  bool UseCredentials();

  Member<Fetcher> fetcher_;

  // Current Manifest.
  mojom::blink::ManifestPtr manifest_;

  // The manifest URL in the Document.
  KURL document_manifest_url_;

  // The manifest URL, taking any redirects into account. If the fetch fails,
  // |manifest_url_| is set to |document_manifest_url_|.
  KURL manifest_url_;

  // Current Manifest debug information.
  mojom::blink::ManifestDebugInfoPtr manifest_debug_info_;

  // This must be before |bindings_| so the bindings are closed before any
  // pending callbacks are discarded.
  Vector<InternalRequestManifestCallback> pending_callbacks_;

  mojo::BindingSet<mojom::blink::ManifestManager> bindings_;
};

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_MODULES_MANIFEST_MANIFESTMANAGER_H_
