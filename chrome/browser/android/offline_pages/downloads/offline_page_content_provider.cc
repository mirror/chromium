// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/offline_pages/downloads/offline_page_content_provider.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/guid.h"
#include "chrome/browser/android/offline_pages/offline_page_model_factory.h"
#include "chrome/browser/android/offline_pages/request_coordinator_factory.h"
#include "chrome/browser/offline_items_collection/offline_content_aggregator_factory.h"
#include "components/offline_items_collection/core/offline_content_aggregator.h"
#include "components/offline_pages/core/background/request_coordinator.h"
#include "components/offline_pages/core/client_policy_controller.h"
#include "content/public/browser/browser_context.h"
#include "jni/OfflinePageUtils_jni.h"

namespace offline_pages {
namespace android {

namespace {

class DownloadUIAdapterDelegate : public DownloadUIAdapter::Delegate {
 public:
  explicit DownloadUIAdapterDelegate(OfflinePageModel* model);

  // DownloadUIAdapter::Delegate
  bool IsVisibleInUI(const ClientId& client_id) override;
  bool IsTemporarilyHiddenInUI(const ClientId& client_id) override;
  void SetUIAdapter(DownloadUIAdapter* ui_adapter) override;

 private:
  // Not owned, cached service pointer.
  OfflinePageModel* model_;
};

DownloadUIAdapterDelegate::DownloadUIAdapterDelegate(OfflinePageModel* model)
    : model_(model) {}

bool DownloadUIAdapterDelegate::IsVisibleInUI(const ClientId& client_id) {
  const std::string& name_space = client_id.name_space;
  return model_->GetPolicyController()->IsSupportedByDownload(name_space) &&
         base::IsValidGUID(client_id.id);
}

bool DownloadUIAdapterDelegate::IsTemporarilyHiddenInUI(
    const ClientId& client_id) {
  return false;
}

void DownloadUIAdapterDelegate::SetUIAdapter(DownloadUIAdapter* ui_adapter) {}

}  // namespace

std::vector<int64_t> FilterRequestsByGuid(
    std::vector<std::unique_ptr<SavePageRequest>> requests,
    const std::string& guid,
    ClientPolicyController* policy_controller) {
  std::vector<int64_t> request_ids;
  for (const auto& request : requests) {
    if (request->client_id().id == guid &&
        policy_controller->IsSupportedByDownload(
            request->client_id().name_space)) {
      request_ids.push_back(request->request_id());
    }
  }
  return request_ids;
}

void CancelRequestCallback(const MultipleItemStatuses&) {
  // Results ignored here, as UI uses observer to update itself.
}

void CancelRequestsContinuation(
    content::BrowserContext* browser_context,
    const std::string& guid,
    std::vector<std::unique_ptr<SavePageRequest>> requests) {
  RequestCoordinator* coordinator =
      RequestCoordinatorFactory::GetForBrowserContext(browser_context);
  if (coordinator) {
    std::vector<int64_t> request_ids = FilterRequestsByGuid(
        std::move(requests), guid, coordinator->GetPolicyController());
    coordinator->RemoveRequests(request_ids,
                                base::Bind(&CancelRequestCallback));
  } else {
    LOG(WARNING) << "CancelRequestsContinuation has no valid coordinator.";
  }
}

void PauseRequestsContinuation(
    content::BrowserContext* browser_context,
    const std::string& guid,
    std::vector<std::unique_ptr<SavePageRequest>> requests) {
  RequestCoordinator* coordinator =
      RequestCoordinatorFactory::GetForBrowserContext(browser_context);
  if (coordinator)
    coordinator->PauseRequests(FilterRequestsByGuid(
        std::move(requests), guid, coordinator->GetPolicyController()));
  else
    LOG(WARNING) << "PauseRequestsContinuation has no valid coordinator.";
}

void ResumeRequestsContinuation(
    content::BrowserContext* browser_context,
    const std::string& guid,
    std::vector<std::unique_ptr<SavePageRequest>> requests) {
  RequestCoordinator* coordinator =
      RequestCoordinatorFactory::GetForBrowserContext(browser_context);
  if (coordinator)
    coordinator->ResumeRequests(FilterRequestsByGuid(
        std::move(requests), guid, coordinator->GetPolicyController()));
  else
    LOG(WARNING) << "ResumeRequestsContinuation has no valid coordinator.";
}

OfflinePageContentProvider::OfflinePageContentProvider(
    DownloadUIAdapter* download_ui_adapter,
    content::BrowserContext* browser_context)
    : download_ui_adapter_(download_ui_adapter),
      browser_context_(browser_context) {
  offline_items_collection::OfflineContentAggregator* aggregator =
      offline_items_collection::OfflineContentAggregatorFactory::
          GetForBrowserContext(browser_context_);
  if (!aggregator)
    return;

  VLOG(0) << "shakti, RegisterProvider";
  aggregator->RegisterProvider(kOfflinePageNamespace, this);
  DCHECK(download_ui_adapter_);
  download_ui_adapter_->AddObserver(this);
}

OfflinePageContentProvider::~OfflinePageContentProvider() {
  items_available_ = false;
  download_ui_adapter_->RemoveObserver(this);
  offline_items_collection::OfflineContentAggregator* aggregator =
      offline_items_collection::OfflineContentAggregatorFactory::
          GetForBrowserContext(browser_context_);
  if (!aggregator)
    return;

  VLOG(0) << "shakti, UnegisterProvider";
  aggregator->UnregisterProvider(kOfflinePageNamespace);
}

void OfflinePageContentProvider::AddObserver(
    OfflineContentProvider::Observer* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void OfflinePageContentProvider::RemoveObserver(
    OfflineContentProvider::Observer* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

std::vector<OfflineItem> OfflinePageContentProvider::GetAllItems() {
  std::vector<OfflineItem> items;
  for (const OfflineItem* item : download_ui_adapter_->GetAllItems()) {
    items.push_back(*item);
  }

  return items;
}

const OfflineItem* OfflinePageContentProvider::GetItemById(
    const ContentId& id) {
  return download_ui_adapter_->GetItem(id.id);
}

bool OfflinePageContentProvider::AreItemsAvailable() {
  return items_available_;
}

void OfflinePageContentProvider::OpenItem(const ContentId& id) {
  JNIEnv* env = base::android::AttachCurrentThread();
  const OfflineItem* item = download_ui_adapter_->GetItem(id.id);

  Java_OfflinePageUtils_openItem(
      env, base::android::ConvertUTF8ToJavaString(env, item->page_url.spec()),
      download_ui_adapter_->GetOfflineIdByGuid(id.id));
}

void OfflinePageContentProvider::RemoveItem(const ContentId& id) {
  download_ui_adapter_->DeleteItem(id.id);
}

void OfflinePageContentProvider::CancelDownload(const ContentId& id) {
  RequestCoordinator* coordinator =
      RequestCoordinatorFactory::GetForBrowserContext(browser_context_);

  if (coordinator) {
    coordinator->GetAllRequests(
        base::Bind(&CancelRequestsContinuation, browser_context_, id.id));
  } else {
    LOG(WARNING) << "CancelDownload has no valid coordinator.";
  }
}

void OfflinePageContentProvider::PauseDownload(const ContentId& id) {
  RequestCoordinator* coordinator =
      RequestCoordinatorFactory::GetForBrowserContext(browser_context_);

  if (coordinator) {
    coordinator->GetAllRequests(
        base::Bind(&PauseRequestsContinuation, browser_context_, id.id));
  } else {
    LOG(WARNING) << "PauseDownload has no valid coordinator.";
  }
}

void OfflinePageContentProvider::ResumeDownload(const ContentId& id) {
  RequestCoordinator* coordinator =
      RequestCoordinatorFactory::GetForBrowserContext(browser_context_);

  if (coordinator) {
    coordinator->GetAllRequests(
        base::Bind(&ResumeRequestsContinuation, browser_context_, id.id));
  } else {
    LOG(WARNING) << "ResumeDownload has no valid coordinator.";
  }
}

void OfflinePageContentProvider::ItemsLoaded() {
  items_available_ = true;
  for (auto& observer : observers_) {
    observer.OnItemsAvailable(this);
  }
}

void OfflinePageContentProvider::ItemAdded(const OfflineItem& item) {
  std::vector<OfflineItem> items(1, item);
  for (auto& observer : observers_) {
    observer.OnItemsAdded(items);
  }
}

void OfflinePageContentProvider::ItemDeleted(const ContentId& id) {
  for (auto& observer : observers_) {
    observer.OnItemRemoved(id);
  }
}

void OfflinePageContentProvider::ItemUpdated(const OfflineItem& item) {
  for (auto& observer : observers_) {
    observer.OnItemUpdated(item);
  }
}

// static
std::unique_ptr<OfflinePageContentProvider> OfflinePageContentProvider::Create(
    content::BrowserContext* browser_context) {
  OfflinePageModel* offline_page_model =
      OfflinePageModelFactory::GetForBrowserContext(browser_context);
  DCHECK(offline_page_model);

  DownloadUIAdapter* adapter =
      DownloadUIAdapter::FromOfflinePageModel(offline_page_model);

  if (!adapter) {
    RequestCoordinator* request_coordinator =
        RequestCoordinatorFactory::GetForBrowserContext(browser_context);
    DCHECK(request_coordinator);
    adapter = new DownloadUIAdapter(
        offline_page_model, request_coordinator,
        base::MakeUnique<DownloadUIAdapterDelegate>(offline_page_model));
    DownloadUIAdapter::AttachToOfflinePageModel(base::WrapUnique(adapter),
                                                offline_page_model);
  }

  return base::MakeUnique<OfflinePageContentProvider>(adapter, browser_context);
}

}  // namespace android
}  // namespace offline_pages
