// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_manager_impl.h"

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/scoped_temp_dir.h"
#include "base/guid.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/download/public/common/download_interrupt_reasons.h"
#include "content/browser/byte_stream.h"
#include "content/browser/download/download_create_info.h"
#include "content/browser/download/download_file_factory.h"
#include "content/browser/download/download_item_factory.h"
#include "content/browser/download/download_item_impl.h"
#include "content/browser/download/download_item_impl_delegate.h"
#include "content/browser/download/download_request_handle.h"
#include "content/browser/download/mock_download_file.h"
#include "content/browser/download/mock_download_item_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager_delegate.h"
#include "content/public/test/mock_download_item.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gmock_mutant.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

#if !defined(OS_ANDROID)
#include "content/public/browser/zoom_level_delegate.h"
#endif  // !defined(OS_ANDROID)

using ::testing::AllOf;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::ReturnRefOfCopy;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::_;

ACTION_TEMPLATE(RunCallback,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(p0)) {
  return ::std::tr1::get<k>(args).Run(p0);
}

namespace content {
class ByteStreamReader;

namespace {

// Matches a DownloadCreateInfo* that points to the same object as |info| and
// has a |default_download_directory| that matches |download_directory|.
MATCHER_P2(DownloadCreateInfoWithDefaultPath, info, download_directory, "") {
  return arg == info && arg->default_download_directory == download_directory;
}

class MockDownloadManagerDelegate : public DownloadManagerDelegate {
 public:
  MockDownloadManagerDelegate();
  virtual ~MockDownloadManagerDelegate();

  MOCK_METHOD0(Shutdown, void());
  MOCK_METHOD1(GetNextId, void(const DownloadIdCallback&));
  MOCK_METHOD2(DetermineDownloadTarget,
               bool(DownloadItem* item,
                    const DownloadTargetCallback&));
  MOCK_METHOD1(ShouldOpenFileBasedOnExtension, bool(const base::FilePath&));
  MOCK_METHOD2(ShouldCompleteDownload,
               bool(DownloadItem*, const base::Closure&));
  MOCK_METHOD2(ShouldOpenDownload,
               bool(DownloadItem*, const DownloadOpenDelayedCallback&));
  MOCK_METHOD4(GetSaveDir, void(BrowserContext*,
                                base::FilePath*, base::FilePath*, bool*));
  MOCK_METHOD5(ChooseSavePath, void(
      WebContents*, const base::FilePath&, const base::FilePath::StringType&,
      bool, const SavePackagePathPickedCallback&));
  MOCK_CONST_METHOD0(ApplicationClientIdForFileScanning, std::string());
};

MockDownloadManagerDelegate::MockDownloadManagerDelegate() {}

MockDownloadManagerDelegate::~MockDownloadManagerDelegate() {}

class MockDownloadItemFactory
    : public DownloadItemFactory,
      public base::SupportsWeakPtr<MockDownloadItemFactory> {
 public:
  MockDownloadItemFactory();
  ~MockDownloadItemFactory() override;

  // Access to map of created items.
  // TODO(rdsmith): Could add type (save page, persisted, etc.)
  // functionality if it's ever needed by consumers.

  // Returns NULL if no item of that id is present.
  MockDownloadItemImpl* GetItem(int id);

  // Remove and return an item made by the factory.
  // Generally used during teardown.
  MockDownloadItemImpl* PopItem();

  // Should be called when the item of this id is removed so that
  // we don't keep dangling pointers.
  void RemoveItem(int id);

  // Sets |has_observer_calls_| to reflect whether we expect to add/remove
  // observers during the CreateActiveItem.
  void SetHasObserverCalls(bool observer_calls);

  // Overridden methods from DownloadItemFactory.
  DownloadItemImpl* CreatePersistedItem(
      DownloadItemImplDelegate* delegate,
      const std::string& guid,
      uint32_t download_id,
      const base::FilePath& current_path,
      const base::FilePath& target_path,
      const std::vector<GURL>& url_chain,
      const GURL& referrer_url,
      const GURL& site_url,
      const GURL& tab_url,
      const GURL& tab_referrer_url,
      const std::string& mime_type,
      const std::string& original_mime_type,
      base::Time start_time,
      base::Time end_time,
      const std::string& etag,
      const std::string& last_modofied,
      int64_t received_bytes,
      int64_t total_bytes,
      const std::string& hash,
      DownloadItem::DownloadState state,
      download::DownloadDangerType danger_type,
      download::DownloadInterruptReason interrupt_reason,
      bool opened,
      base::Time last_access_time,
      bool transient,
      const std::vector<DownloadItem::ReceivedSlice>& received_slices) override;
  DownloadItemImpl* CreateActiveItem(DownloadItemImplDelegate* delegate,
                                     uint32_t download_id,
                                     const DownloadCreateInfo& info) override;
  DownloadItemImpl* CreateSavePageItem(
      DownloadItemImplDelegate* delegate,
      uint32_t download_id,
      const base::FilePath& path,
      const GURL& url,
      const std::string& mime_type,
      std::unique_ptr<DownloadRequestHandleInterface> request_handle) override;

 private:
  std::map<uint32_t, MockDownloadItemImpl*> items_;
  DownloadItemImplDelegate item_delegate_;
  bool has_observer_calls_;

  DISALLOW_COPY_AND_ASSIGN(MockDownloadItemFactory);
};

MockDownloadItemFactory::MockDownloadItemFactory()
    : has_observer_calls_(false) {}

MockDownloadItemFactory::~MockDownloadItemFactory() {}

MockDownloadItemImpl* MockDownloadItemFactory::GetItem(int id) {
  if (items_.find(id) == items_.end())
    return nullptr;
  return items_[id];
}

MockDownloadItemImpl* MockDownloadItemFactory::PopItem() {
  if (items_.empty())
    return nullptr;

  std::map<uint32_t, MockDownloadItemImpl*>::iterator first_item =
      items_.begin();
  MockDownloadItemImpl* result = first_item->second;
  items_.erase(first_item);
  return result;
}

void MockDownloadItemFactory::RemoveItem(int id) {
  DCHECK(items_.find(id) != items_.end());
  items_.erase(id);
}

void MockDownloadItemFactory::SetHasObserverCalls(bool has_observer_calls) {
  has_observer_calls_ = has_observer_calls;
}

DownloadItemImpl* MockDownloadItemFactory::CreatePersistedItem(
    DownloadItemImplDelegate* delegate,
    const std::string& guid,
    uint32_t download_id,
    const base::FilePath& current_path,
    const base::FilePath& target_path,
    const std::vector<GURL>& url_chain,
    const GURL& referrer_url,
    const GURL& site_url,
    const GURL& tab_url,
    const GURL& tab_referrer_url,
    const std::string& mime_type,
    const std::string& original_mime_type,
    base::Time start_time,
    base::Time end_time,
    const std::string& etag,
    const std::string& last_modified,
    int64_t received_bytes,
    int64_t total_bytes,
    const std::string& hash,
    DownloadItem::DownloadState state,
    download::DownloadDangerType danger_type,
    download::DownloadInterruptReason interrupt_reason,
    bool opened,
    base::Time last_access_time,
    bool transient,
    const std::vector<DownloadItem::ReceivedSlice>& received_slices) {
  DCHECK(items_.find(download_id) == items_.end());
  MockDownloadItemImpl* result =
      new StrictMock<MockDownloadItemImpl>(&item_delegate_);
  EXPECT_CALL(*result, GetId())
      .WillRepeatedly(Return(download_id));
  EXPECT_CALL(*result, GetGuid()).WillRepeatedly(ReturnRefOfCopy(guid));
  items_[download_id] = result;
  return result;
}

DownloadItemImpl* MockDownloadItemFactory::CreateActiveItem(
    DownloadItemImplDelegate* delegate,
    uint32_t download_id,
    const DownloadCreateInfo& info) {
  DCHECK(items_.find(download_id) == items_.end());

  MockDownloadItemImpl* result =
      new StrictMock<MockDownloadItemImpl>(&item_delegate_);
  EXPECT_CALL(*result, GetId())
      .WillRepeatedly(Return(download_id));
  EXPECT_CALL(*result, GetGuid())
      .WillRepeatedly(ReturnRefOfCopy(base::GenerateGUID()));
  items_[download_id] = result;

  // Active items are created and then immediately are called to start
  // the download.
  EXPECT_CALL(*result, MockStart(_, _));

  // In the StartDownload case, expect the remove/add observer calls.
  if (has_observer_calls_) {
    EXPECT_CALL(*result, RemoveObserver(_));
    EXPECT_CALL(*result, AddObserver(_));
  }

  return result;
}

DownloadItemImpl* MockDownloadItemFactory::CreateSavePageItem(
    DownloadItemImplDelegate* delegate,
    uint32_t download_id,
    const base::FilePath& path,
    const GURL& url,
    const std::string& mime_type,
    std::unique_ptr<DownloadRequestHandleInterface> request_handle) {
  DCHECK(items_.find(download_id) == items_.end());

  MockDownloadItemImpl* result =
      new StrictMock<MockDownloadItemImpl>(&item_delegate_);
  EXPECT_CALL(*result, GetId())
      .WillRepeatedly(Return(download_id));
  items_[download_id] = result;

  return result;
}

class MockDownloadFileFactory
    : public DownloadFileFactory,
      public base::SupportsWeakPtr<MockDownloadFileFactory> {
 public:
  MockDownloadFileFactory() {}
  virtual ~MockDownloadFileFactory() {}

  // Overridden method from DownloadFileFactory
  MOCK_METHOD2(MockCreateFile,
               MockDownloadFile*(const download::DownloadSaveInfo&,
                                 DownloadManager::InputStream*));

  DownloadFile* CreateFile(
      std::unique_ptr<download::DownloadSaveInfo> save_info,
      const base::FilePath& default_download_directory,
      std::unique_ptr<DownloadManager::InputStream> stream,
      uint32_t download_id,
      base::WeakPtr<DownloadDestinationObserver> observer) override {
    return MockCreateFile(*save_info, stream.get());
  }
};

class MockBrowserContext : public BrowserContext {
 public:
  MockBrowserContext() {
    content::BrowserContext::Initialize(this, base::FilePath());
  }
  ~MockBrowserContext() {}

  MOCK_CONST_METHOD0(GetPath, base::FilePath());
#if !defined(OS_ANDROID)
  MOCK_METHOD1(CreateZoomLevelDelegateMock,
               ZoomLevelDelegate*(const base::FilePath&));
#endif  // !defined(OS_ANDROID)
  MOCK_CONST_METHOD0(IsOffTheRecord, bool());
  MOCK_METHOD0(GetResourceContext, ResourceContext*());
  MOCK_METHOD0(GetDownloadManagerDelegate, DownloadManagerDelegate*());
  MOCK_METHOD0(GetGuestManager, BrowserPluginGuestManager* ());
  MOCK_METHOD0(GetSpecialStoragePolicy, storage::SpecialStoragePolicy*());
  MOCK_METHOD0(GetPushMessagingService, PushMessagingService*());
  MOCK_METHOD0(GetSSLHostStateDelegate, SSLHostStateDelegate*());
  MOCK_METHOD0(GetPermissionManager, PermissionManager*());
  MOCK_METHOD0(GetBackgroundFetchDelegate, BackgroundFetchDelegate*());
  MOCK_METHOD0(GetBackgroundSyncController, BackgroundSyncController*());
  MOCK_METHOD0(GetBrowsingDataRemoverDelegate, BrowsingDataRemoverDelegate*());
  MOCK_METHOD0(CreateMediaRequestContext,
               net::URLRequestContextGetter*());
  MOCK_METHOD2(CreateMediaRequestContextForStoragePartition,
               net::URLRequestContextGetter*(
                   const base::FilePath& partition_path, bool in_memory));

  // Define these two methods to avoid a
  // 'cannot access private member declared in class
  // URLRequestInterceptorScopedVector'
  // build error if they're put in MOCK_METHOD.
  net::URLRequestContextGetter* CreateRequestContext(
      ProtocolHandlerMap* protocol_handlers,
      URLRequestInterceptorScopedVector request_interceptors) override {
    return nullptr;
  }

  net::URLRequestContextGetter* CreateRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory,
      ProtocolHandlerMap* protocol_handlers,
      URLRequestInterceptorScopedVector request_interceptors) override {
    return nullptr;
  }
#if !defined(OS_ANDROID)
  std::unique_ptr<ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& path) override {
    return std::unique_ptr<ZoomLevelDelegate>(
        CreateZoomLevelDelegateMock(path));
  }
#endif  // !defined(OS_ANDROID)
};

class MockDownloadManagerObserver : public DownloadManager::Observer {
 public:
  MockDownloadManagerObserver() {}
  ~MockDownloadManagerObserver() {}
  MOCK_METHOD2(OnDownloadCreated, void(
        DownloadManager*, DownloadItem*));
  MOCK_METHOD1(ManagerGoingDown, void(DownloadManager*));
  MOCK_METHOD2(SelectFileDialogDisplayed, void(DownloadManager*, int32_t));
};

class MockByteStreamReader : public ByteStreamReader {
 public:
  virtual ~MockByteStreamReader() {}
  MOCK_METHOD2(Read, StreamState(scoped_refptr<net::IOBuffer>*, size_t*));
  MOCK_CONST_METHOD0(GetStatus, int());
  MOCK_METHOD1(RegisterCallback, void(const base::Closure&));
};

}  // namespace

class DownloadManagerTest : public testing::Test {
 public:
  static const char* kTestData;
  static const size_t kTestDataLen;

  DownloadManagerTest()
      : callback_called_(false),
        target_disposition_(DownloadItem::TARGET_DISPOSITION_OVERWRITE),
        danger_type_(download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS),
        interrupt_reason_(download::DOWNLOAD_INTERRUPT_REASON_NONE),
        next_download_id_(0) {}

  // We tear down everything in TearDown().
  ~DownloadManagerTest() override {}

  // Create a MockDownloadItemFactory and MockDownloadManagerDelegate,
  // then create a DownloadManager that points
  // at all of those.
  void SetUp() override {
    DCHECK(!download_manager_);

    mock_download_item_factory_ = (new MockDownloadItemFactory())->AsWeakPtr();
    mock_download_file_factory_ = (new MockDownloadFileFactory())->AsWeakPtr();
    mock_download_manager_delegate_.reset(
        new StrictMock<MockDownloadManagerDelegate>);
    EXPECT_CALL(*mock_download_manager_delegate_.get(), Shutdown())
        .WillOnce(Return());
    mock_browser_context_.reset(new StrictMock<MockBrowserContext>);
    EXPECT_CALL(*mock_browser_context_.get(), IsOffTheRecord())
        .WillRepeatedly(Return(false));

    download_manager_.reset(
        new DownloadManagerImpl(mock_browser_context_.get()));
    download_manager_->SetDownloadItemFactoryForTesting(
        std::unique_ptr<DownloadItemFactory>(
            mock_download_item_factory_.get()));
    download_manager_->SetDownloadFileFactoryForTesting(
        std::unique_ptr<DownloadFileFactory>(
            mock_download_file_factory_.get()));
    observer_.reset(new MockDownloadManagerObserver());
    download_manager_->AddObserver(observer_.get());
    download_manager_->SetDelegate(mock_download_manager_delegate_.get());
    download_urls_.push_back(GURL("http://www.url1.com"));
    download_urls_.push_back(GURL("http://www.url2.com"));
    download_urls_.push_back(GURL("http://www.url3.com"));
    download_urls_.push_back(GURL("http://www.url4.com"));
  }

  void TearDown() override {
    while (MockDownloadItemImpl*
           item = mock_download_item_factory_->PopItem()) {
      EXPECT_CALL(*item, GetState())
          .WillOnce(Return(DownloadItem::CANCELLED));
    }
    EXPECT_CALL(GetMockObserver(), ManagerGoingDown(download_manager_.get()))
        .WillOnce(Return());

    download_manager_->Shutdown();
    download_manager_.reset();
    base::RunLoop().RunUntilIdle();
    ASSERT_FALSE(mock_download_item_factory_);
    mock_download_manager_delegate_.reset();
    mock_browser_context_.reset();
    download_urls_.clear();
  }

  // Returns download id.
  MockDownloadItemImpl& AddItemToManager() {
    DownloadCreateInfo info;

    // Args are ignored except for download id, so everything else can be
    // null.
    uint32_t id = next_download_id_;
    ++next_download_id_;
    info.request_handle.reset(new DownloadRequestHandle);
    download_manager_->CreateActiveItem(id, info);
    DCHECK(mock_download_item_factory_->GetItem(id));
    MockDownloadItemImpl& item(*mock_download_item_factory_->GetItem(id));
    // Satisfy expectation.  If the item is created in StartDownload(),
    // we call Start on it immediately, so we need to set that expectation
    // in the factory.
    std::unique_ptr<DownloadRequestHandleInterface> req_handle;
    item.Start(std::unique_ptr<DownloadFile>(), std::move(req_handle), info);
    DCHECK(id < download_urls_.size());
    EXPECT_CALL(item, GetURL()).WillRepeatedly(ReturnRef(download_urls_[id]));

    return item;
  }

  MockDownloadItemImpl& GetMockDownloadItem(int id) {
    MockDownloadItemImpl* itemp = mock_download_item_factory_->GetItem(id);

    DCHECK(itemp);
    return *itemp;
  }

  void RemoveMockDownloadItem(int id) {
    // Owned by DownloadManager; should be deleted there.
    mock_download_item_factory_->RemoveItem(id);
  }

  MockDownloadManagerDelegate& GetMockDownloadManagerDelegate() {
    return *mock_download_manager_delegate_;
  }

  MockDownloadManagerObserver& GetMockObserver() {
    return *observer_;
  }

  void DownloadTargetDeterminedCallback(
      const base::FilePath& target_path,
      DownloadItem::TargetDisposition disposition,
      download::DownloadDangerType danger_type,
      const base::FilePath& intermediate_path,
      download::DownloadInterruptReason interrupt_reason) {
    callback_called_ = true;
    target_path_ = target_path;
    target_disposition_ = disposition;
    danger_type_ = danger_type;
    intermediate_path_ = intermediate_path;
    interrupt_reason_ = interrupt_reason;
  }

  void DetermineDownloadTarget(DownloadItemImpl* item) {
    download_manager_->DetermineDownloadTarget(
        item, base::Bind(
            &DownloadManagerTest::DownloadTargetDeterminedCallback,
            base::Unretained(this)));
  }

  void SetHasObserverCalls(bool has_observer_calls) {
    mock_download_item_factory_->SetHasObserverCalls(has_observer_calls);
  }

 protected:
  // Key test variable; we'll keep it available to sub-classes.
  std::unique_ptr<DownloadManagerImpl> download_manager_;
  base::WeakPtr<MockDownloadFileFactory> mock_download_file_factory_;

  // Target detetermined callback.
  bool callback_called_;
  base::FilePath target_path_;
  DownloadItem::TargetDisposition target_disposition_;
  download::DownloadDangerType danger_type_;
  base::FilePath intermediate_path_;
  download::DownloadInterruptReason interrupt_reason_;

  std::vector<GURL> download_urls_;

 private:
  TestBrowserThreadBundle thread_bundle_;
  base::WeakPtr<MockDownloadItemFactory> mock_download_item_factory_;
  std::unique_ptr<MockDownloadManagerDelegate> mock_download_manager_delegate_;
  std::unique_ptr<MockBrowserContext> mock_browser_context_;
  std::unique_ptr<MockDownloadManagerObserver> observer_;
  uint32_t next_download_id_;

  DISALLOW_COPY_AND_ASSIGN(DownloadManagerTest);
};

// Confirm the appropriate invocations occur when you start a download.
TEST_F(DownloadManagerTest, StartDownload) {
  SetHasObserverCalls(true);

  std::unique_ptr<DownloadCreateInfo> info(new DownloadCreateInfo);
  std::unique_ptr<ByteStreamReader> stream(new MockByteStreamReader);
  uint32_t local_id(5);  // Random value
  base::FilePath download_path(FILE_PATH_LITERAL("download/path"));

  EXPECT_FALSE(download_manager_->GetDownload(local_id));

  EXPECT_CALL(GetMockObserver(), OnDownloadCreated(download_manager_.get(), _))
      .WillOnce(Return());
  EXPECT_CALL(GetMockDownloadManagerDelegate(), GetNextId(_))
      .WillOnce(RunCallback<0>(local_id));

#if !defined(USE_X11)
  // Doing nothing will set the default download directory to null.
  EXPECT_CALL(GetMockDownloadManagerDelegate(), GetSaveDir(_, _, _, _));
#endif
  EXPECT_CALL(GetMockDownloadManagerDelegate(),
              ApplicationClientIdForFileScanning())
      .WillRepeatedly(Return("client-id"));
  MockDownloadFile* mock_file = new MockDownloadFile;
  auto input_stream =
      std::make_unique<DownloadManager::InputStream>(std::move(stream));
  EXPECT_CALL(*mock_download_file_factory_.get(),
              MockCreateFile(Ref(*info->save_info.get()), input_stream.get()))
      .WillOnce(Return(mock_file));

  download_manager_->StartDownload(std::move(info), std::move(input_stream),
                                   DownloadUrlParameters::OnStartedCallback());
  EXPECT_TRUE(download_manager_->GetDownload(local_id));

  SetHasObserverCalls(false);
}

// Confirm that calling DetermineDownloadTarget behaves properly if the delegate
// blocks starting.
TEST_F(DownloadManagerTest, DetermineDownloadTarget_True) {
  // Put a mock we have a handle to on the download manager.
  MockDownloadItemImpl& item(AddItemToManager());
  EXPECT_CALL(item, GetState())
      .WillRepeatedly(Return(DownloadItem::IN_PROGRESS));

  EXPECT_CALL(GetMockDownloadManagerDelegate(),
              DetermineDownloadTarget(&item, _))
      .WillOnce(Return(true));
  DetermineDownloadTarget(&item);
}

// Confirm that calling DetermineDownloadTarget behaves properly if the delegate
// allows starting.  This also tests OnDownloadTargetDetermined.
TEST_F(DownloadManagerTest, DetermineDownloadTarget_False) {
  // Put a mock we have a handle to on the download manager.
  MockDownloadItemImpl& item(AddItemToManager());

  base::FilePath path(FILE_PATH_LITERAL("random_filepath.txt"));
  EXPECT_CALL(GetMockDownloadManagerDelegate(),
              DetermineDownloadTarget(&item, _))
      .WillOnce(Return(false));
  EXPECT_CALL(item, GetForcedFilePath())
      .WillOnce(ReturnRef(path));

  // Confirm that the callback was called with the right values in this case.
  DetermineDownloadTarget(&item);
  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(path, target_path_);
  EXPECT_EQ(DownloadItem::TARGET_DISPOSITION_OVERWRITE, target_disposition_);
  EXPECT_EQ(download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, danger_type_);
  EXPECT_EQ(path, intermediate_path_);
  EXPECT_EQ(download::DOWNLOAD_INTERRUPT_REASON_NONE, interrupt_reason_);
}

TEST_F(DownloadManagerTest, GetDownloadByGuid) {
  for (uint32_t i = 0; i < 4; ++i)
    AddItemToManager();

  MockDownloadItemImpl& item = GetMockDownloadItem(0);
  DownloadItem* result = download_manager_->GetDownloadByGuid(item.GetGuid());
  ASSERT_TRUE(result);
  ASSERT_EQ(static_cast<DownloadItem*>(&item), result);

  ASSERT_FALSE(download_manager_->GetDownloadByGuid(""));

  const char kGuid[] = "8DF158E8-C980-4618-BB03-EBA3242EB48B";
  DownloadItem* persisted_item = download_manager_->CreateDownloadItem(
      kGuid, 10, base::FilePath(), base::FilePath(), std::vector<GURL>(),
      GURL("http://example.com/a"), GURL("http://example.com/a"),
      GURL("http://example.com/a"), GURL("http://example.com/a"),
      "application/octet-stream", "application/octet-stream", base::Time::Now(),
      base::Time::Now(), std::string(), std::string(), 10, 10, std::string(),
      DownloadItem::INTERRUPTED, download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
      download::DOWNLOAD_INTERRUPT_REASON_SERVER_FAILED, false,
      base::Time::Now(), true, std::vector<DownloadItem::ReceivedSlice>());
  ASSERT_TRUE(persisted_item);

  ASSERT_EQ(persisted_item, download_manager_->GetDownloadByGuid(kGuid));
}

namespace {

base::Callback<bool(const GURL&)> GetSingleURLFilter(const GURL& url) {
  return base::Bind(static_cast<bool (*)(const GURL&, const GURL&)>(operator==),
                    GURL(url));
}

}  // namespace

// Confirm that only downloads with the specified URL are removed.
TEST_F(DownloadManagerTest, RemoveDownloadsByURL) {
  base::Time now(base::Time::Now());
  for (uint32_t i = 0; i < 2; ++i) {
    MockDownloadItemImpl& item(AddItemToManager());
    EXPECT_CALL(item, GetStartTime()).WillRepeatedly(Return(now));
    EXPECT_CALL(item, GetState())
        .WillRepeatedly(Return(DownloadItem::COMPLETE));
  }

  EXPECT_CALL(GetMockDownloadItem(0), Remove());
  EXPECT_CALL(GetMockDownloadItem(1), Remove()).Times(0);

  base::Callback<bool(const GURL&)> url_filter =
      GetSingleURLFilter(download_urls_[0]);
  int remove_count = download_manager_->RemoveDownloadsByURLAndTime(
      url_filter, base::Time(), base::Time::Max());
  EXPECT_EQ(remove_count, 1);
}

}  // namespace content
