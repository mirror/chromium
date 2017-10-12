// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/domain_reliability/service.h"

#include "base/logging.h"
#include "base/test/test_simple_task_runner.h"
#include "components/domain_reliability/monitor.h"
#include "content/public/browser/permission_manager.h"
#include "content/public/test/test_browser_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/modules/permissions/permission_status.mojom.h"

namespace domain_reliability {

namespace {

class TestPermissionManager : public content::PermissionManager {
 public:
  TestPermissionManager() : get_permission_status_count_(0) {}

  size_t get_permission_status_count() const {
    return get_permission_status_count_;
  }
  content::PermissionType last_permission() const { return last_permission_; }
  const GURL& last_requesting_origin() const { return last_requesting_origin_; }
  const GURL& last_embedding_origin() const { return last_embedding_origin_; }

  void set_permission_status(blink::mojom::PermissionStatus permission_status) {
    permission_status_ = permission_status;
  }

  // content::PermissionManager:

  ~TestPermissionManager() override {}

  blink::mojom::PermissionStatus GetPermissionStatus(
      content::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin) override {
    ++get_permission_status_count_;

    last_permission_ = permission;
    last_requesting_origin_ = requesting_origin;
    last_embedding_origin_ = embedding_origin;

    return permission_status_;
  }

  int RequestPermission(
      content::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::Callback<void(blink::mojom::PermissionStatus)>& callback)
      override {
    NOTIMPLEMENTED();
    return kNoPendingOperation;
  }

  int RequestPermissions(
      const std::vector<content::PermissionType>& permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::Callback<
          void(const std::vector<blink::mojom::PermissionStatus>&)>& callback)
      override {
    NOTIMPLEMENTED();
    return kNoPendingOperation;
  }

  void CancelPermissionRequest(int request_id) override { NOTIMPLEMENTED(); }

  void ResetPermission(content::PermissionType permission,
                       const GURL& requesting_origin,
                       const GURL& embedding_origin) override {
    NOTIMPLEMENTED();
  }

  int SubscribePermissionStatusChange(
      content::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin,
      const base::Callback<void(blink::mojom::PermissionStatus)>& callback)
      override {
    NOTIMPLEMENTED();
    return 0;
  }

  void UnsubscribePermissionStatusChange(int subscription_id) override {
    NOTIMPLEMENTED();
  }

 private:
  // Number of calls made to GetPermissionStatus.
  size_t get_permission_status_count_;

  // Parameters to last call to GetPermissionStatus:

  content::PermissionType last_permission_;
  GURL last_requesting_origin_;
  GURL last_embedding_origin_;

  // Value to return from GetPermissionStatus.
  blink::mojom::PermissionStatus permission_status_;

  DISALLOW_COPY_AND_ASSIGN(TestPermissionManager);
};

class DomainReliabilityServiceTest : public testing::Test {
 protected:
  DomainReliabilityServiceTest()
      : upload_reporter_string_("test"),
        permission_manager_(new TestPermissionManager()),
        ui_task_runner_(new base::TestSimpleTaskRunner()),
        network_task_runner_(new base::TestSimpleTaskRunner()) {
    browser_context_.SetPermissionManager(
        base::WrapUnique(permission_manager_));

    service_ = base::WrapUnique(DomainReliabilityService::Create(
        upload_reporter_string_, &browser_context_));

    monitor_ = service_->CreateMonitor(ui_task_runner_, network_task_runner_);
  }

 private:
  std::string upload_reporter_string_;

  content::TestBrowserContext browser_context_;

  // Owned by browser_context_, not the test class.
  TestPermissionManager* permission_manager_;

  std::unique_ptr<DomainReliabilityService> service_;

  scoped_refptr<base::TestSimpleTaskRunner> ui_task_runner_;
  scoped_refptr<base::TestSimpleTaskRunner> network_task_runner_;

  std::unique_ptr<DomainReliabilityMonitor> monitor_;
};

TEST_F(DomainReliabilityServiceTest, Create) {}

}  // namespace

}  // namespace domain_reliability
