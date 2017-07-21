// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_TEST_HARNESS_H_
#define CHROME_BROWSER_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_TEST_HARNESS_H_

#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/prefs/testing_pref_service.h"
#include "components/subresource_filter/content/browser/fake_safe_browsing_database_manager.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "components/subresource_filter/core/browser/subresource_filter_features_test_support.h"
#include "content/public/browser/devtools_agent_host_client.h"

class ChromeSubresourceFilterClient;
class GURL;
class SubresourceFilterContentSettingsManager;

namespace content {
class DevToolsAgentHost;
class RenderFrameHost;
}  // namespace content

// End to end unit test harness of (most of) the browser process portions of the
// subresource filtering code.
class SubresourceFilterTestHarness : public ChromeRenderViewHostTestHarness {
 public:
  static constexpr char const kDefaultDisallowedUrl[] =
      "https://example.test/disallowed.html";
  SubresourceFilterTestHarness();
  ~SubresourceFilterTestHarness() override;

  // ChromeRenderViewHostTestHarness:
  void SetUp() override;
  void TearDown() override;

  // Returns the frame host the navigation commit in, or nullptr if it did not
  // succeed.
  content::RenderFrameHost* SimulateNavigateAndCommit(
      const GURL& url,
      content::RenderFrameHost* rfh);

  // Creates a subframe as a child of |parent|, and navigates it to a URL
  // disallowed by the default ruleset (kDefaultDisallowedUrl). Returns the
  // frame host the navigation commit in, or nullptr if it did not succeed.
  content::RenderFrameHost* CreateAndNavigateDisallowedSubframe(
      content::RenderFrameHost* parent);

  void ConfigureAsSubresourceFilterOnlyURL(const GURL& url);

  ChromeSubresourceFilterClient* GetClient();

  void RemoveURLFromBlacklist(const GURL& url);

  SubresourceFilterContentSettingsManager* GetSettingsManager();

  subresource_filter::testing::ScopedSubresourceFilterConfigurator&
  scoped_configuration() {
    return scoped_configuration_;
  }

  class ScopedDevToolsClientHost : public content::DevToolsAgentHostClient {
   public:
    ScopedDevToolsClientHost(
        scoped_refptr<content::DevToolsAgentHost> agent_host);
    ~ScopedDevToolsClientHost() override;

    // content::DevToolsAgentHostClient:
    void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                                 const std::string& message) override;
    void AgentHostClosed(content::DevToolsAgentHost* agent_host,
                         bool replaced) override;

   private:
    scoped_refptr<content::DevToolsAgentHost> agent_host_;
    DISALLOW_COPY_AND_ASSIGN(ScopedDevToolsClientHost);
  };

 private:
  base::ScopedTempDir ruleset_service_dir_;
  TestingPrefServiceSimple pref_service_;
  subresource_filter::testing::ScopedSubresourceFilterFeatureToggle
      scoped_feature_toggle_;
  subresource_filter::testing::ScopedSubresourceFilterConfigurator
      scoped_configuration_;

  scoped_refptr<FakeSafeBrowsingDatabaseManager> fake_safe_browsing_database_;
  scoped_refptr<content::DevToolsAgentHost> agent_host_;

  DISALLOW_COPY_AND_ASSIGN(SubresourceFilterTestHarness);
};

#endif  // CHROME_BROWSER_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_TEST_HARNESS_H_
