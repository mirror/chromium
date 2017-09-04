// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLUGINS_PLUGIN_OBSERVER_H_
#define CHROME_BROWSER_PLUGINS_PLUGIN_OBSERVER_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/common/features.h"
#include "chrome/common/plugin.mojom.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_binding_set.h"

namespace content {
class WebContents;
}

class PluginObserver : public content::WebContentsObserver,
                       public chrome::mojom::PluginHost,
                       public content::WebContentsUserData<PluginObserver> {
 public:
  ~PluginObserver() override;

  // content::WebContentsObserver implementation.
  void PluginCrashed(const base::FilePath& plugin_path,
                     base::ProcessId plugin_pid) override;
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

 private:
  class ComponentObserver;
  class PluginPlaceholderHost;
  friend class content::WebContentsUserData<PluginObserver>;

  explicit PluginObserver(content::WebContents* web_contents);

  // chrome::mojom::PluginHost methods.
  void BlockedOutdatedPlugin(chrome::mojom::PluginRendererPtr plugin_renderer,
                             int64_t placeholder_id,
                             const std::string& identifier) override;
  void BlockedComponentUpdatedPlugin(
      chrome::mojom::PluginRendererPtr plugin_renderer,
      int64_t placeholder_id,
      const std::string& identifier) override;
  void ShowFlashPermissionBubble() override;
  void CouldNotLoadPlugin(const base::FilePath& plugin_path) override;

  // Message handlers:
  void OnRemovePluginPlaceholderHost(int placeholder_id);
  void RemoveComponentObserver(int placeholder_id);

  // Stores all PluginPlaceholderHosts, keyed by their routing ID.
  std::map<int, std::unique_ptr<PluginPlaceholderHost>> plugin_placeholders_;

  // Stores all ComponentObservers, keyed by their routing ID.
  std::map<int, std::unique_ptr<ComponentObserver>> component_observers_;

  mojo::AssociatedBindingSet<chrome::mojom::PluginHost> plugin_host_bindings_;

  base::WeakPtrFactory<PluginObserver> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PluginObserver);
};

#endif  // CHROME_BROWSER_PLUGINS_PLUGIN_OBSERVER_H_
