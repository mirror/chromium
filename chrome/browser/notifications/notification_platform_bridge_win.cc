// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_platform_bridge_win.h"

#include <activation.h>
#include <windows.ui.notifications.h>
#include <wrl/client.h>
#include <wrl/event.h>
#include <wrl/wrappers/corewrappers.h>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/win/core_winrt_util.h"
#include "base/win/scoped_hstring.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_common.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/notifications/notification_template_builder.h"
#include "chrome/browser/notifications/temp_file_keeper.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "content/public/browser/browser_thread.h"
#include "ui/message_center/notification.h"

namespace mswr = Microsoft::WRL;
namespace mswrw = Microsoft::WRL::Wrappers;

namespace winfoundtn = ABI::Windows::Foundation;
namespace winui = ABI::Windows::UI;
namespace winxml = ABI::Windows::Data::Xml;

using base::win::ScopedHString;
using message_center::RichNotificationData;

// Hold related data for a notification.
struct NotificationData {
  NotificationData(NotificationCommon::Type notification_type,
                   const std::string& notification_id,
                   const std::string& profile_id,
                   bool incognito,
                   const GURL& origin_url)
      : notification_type(notification_type),
        notification_id(notification_id),
        profile_id(profile_id),
        incognito(incognito),
        origin_url(origin_url) {}

  // Same parameters used by NotificationPlatformBridge::Display().
  NotificationCommon::Type notification_type;
  const std::string notification_id;
  const std::string profile_id;
  const bool incognito;

  // A copy of the origin_url from the underlying message_center::Notification.
  // Used to pass back to NotificationDisplayService.
  const GURL origin_url;

  DISALLOW_COPY_AND_ASSIGN(NotificationData);
};

namespace {

constexpr wchar_t kNotificationsPrefix[] = L"_notifications_images_";

typedef winfoundtn::ITypedEventHandler<winui::Notifications::ToastNotification*,
                                       IInspectable*>
    ToastActivatedHandler;
typedef winfoundtn::ITypedEventHandler<
    winui::Notifications::ToastNotification*,
    winui::Notifications::ToastDismissedEventArgs*>
    ToastDismissedHandler;

// Templated wrapper for winfoundtn::GetActivationFactory().
template <unsigned int size, typename T>
HRESULT CreateActivationFactory(wchar_t const (&class_name)[size], T** object) {
  ScopedHString ref_class_name =
      ScopedHString::Create(base::StringPiece16(class_name, size - 1));
  return base::win::RoGetActivationFactory(ref_class_name.get(),
                                           IID_PPV_ARGS(object));
}

// TODO(finnur): Make this a member function of the class and unit test it.
// Obtain an IToastNotification interface from a given XML (provided by the
// NotificationTemplateBuilder).
HRESULT GetToastNotification(
    const NotificationTemplateBuilder& notification_template_builder,
    winui::Notifications::IToastNotification** toast_notification) {
  *toast_notification = nullptr;

  ScopedHString ref_class_name =
      ScopedHString::Create(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument);
  mswr::ComPtr<IInspectable> inspectable;
  HRESULT hr =
      base::win::RoActivateInstance(ref_class_name.get(), &inspectable);
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to activate the XML Document";
    return hr;
  }

  mswr::ComPtr<winxml::Dom::IXmlDocumentIO> document_io;
  hr = inspectable.As<winxml::Dom::IXmlDocumentIO>(&document_io);
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to get XmlDocument as IXmlDocumentIO";
    return hr;
  }

  base::string16 notification_template =
      notification_template_builder.GetNotificationTemplate();
  ScopedHString ref_template = ScopedHString::Create(notification_template);
  hr = document_io->LoadXml(ref_template.get());
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to load the template's XML into the document";
    return hr;
  }

  mswr::ComPtr<winxml::Dom::IXmlDocument> document;
  hr = document_io.As<winxml::Dom::IXmlDocument>(&document);
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to get as XMLDocument";
    return hr;
  }

  mswr::ComPtr<winui::Notifications::IToastNotificationFactory>
      toast_notification_factory;
  hr = CreateActivationFactory(
      RuntimeClass_Windows_UI_Notifications_ToastNotification,
      toast_notification_factory.GetAddressOf());
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to create the IToastNotificationFactory";
    return hr;
  }

  hr = toast_notification_factory->CreateToastNotification(document.Get(),
                                                           toast_notification);
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to create the IToastNotification";
    return hr;
  }

  return S_OK;
}

// Perform |operation| on a notification once the profile has been loaded.
void ProfileLoadedCallback(NotificationCommon::Operation operation,
                           NotificationCommon::Type notification_type,
                           const std::string& origin,
                           const std::string& notification_id,
                           const base::Optional<int>& action_index,
                           const base::Optional<base::string16>& reply,
                           const base::Optional<bool>& by_user,
                           Profile* profile) {
  if (!profile)
    return;

  auto* display_service =
      NotificationDisplayServiceFactory::GetForProfile(profile);
  display_service->ProcessNotificationOperation(operation, notification_type,
                                                origin, notification_id,
                                                action_index, reply, by_user);
}

void ForwardNotificationOperationOnUiThread(
    NotificationCommon::Operation operation,
    NotificationCommon::Type notification_type,
    const std::string& origin,
    const std::string& notification_id,
    const std::string& profile_id,
    bool incognito) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!g_browser_process)
    return;

  g_browser_process->profile_manager()->LoadProfile(
      profile_id, incognito,
      base::Bind(&ProfileLoadedCallback, operation, notification_type, origin,
                 notification_id, base::nullopt /*action_index*/,
                 base::nullopt /*reply*/, base::nullopt /*by_user*/));
}

void ForwardNotificationOperation(NotificationData* data,
                                  NotificationCommon::Operation operation) {
  if (!data)
    return;

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&ForwardNotificationOperationOnUiThread, operation,
                 data->notification_type, data->origin_url.spec(),
                 data->notification_id, data->profile_id, data->incognito));
}

}  // namespace

// static
NotificationPlatformBridge* NotificationPlatformBridge::Create() {
  return new NotificationPlatformBridgeWin();
}

class NotificationPlatformBridgeWinImpl
    : public NotificationPlatformBridge,
      public base::RefCountedThreadSafe<NotificationPlatformBridgeWinImpl> {
 public:
  NotificationPlatformBridgeWinImpl() {
    task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::USER_BLOCKING});
    com_functions_initialized_ =
        base::win::ResolveCoreWinRTDelayload() &&
        ScopedHString::ResolveCoreWinRTStringDelayload();
    temp_file_keeper_.reset(
        new TempFileKeeperImpl(kNotificationsPrefix, task_runner_.get()));
  }

  void Display(
      NotificationCommon::Type notification_type,
      const std::string& notification_id,
      const std::string& profile_id,
      bool is_incognito,
      const message_center::Notification& notification,
      std::unique_ptr<NotificationCommon::Metadata> metadata) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    // Notifications contain gfx::Image's which have reference counts
    // that are not thread safe.  Because of this, we duplicate the
    // notification and its images.  Wrap the notification in a
    // unique_ptr to transfer ownership of the notification (and the
    // non-thread-safe reference counts) to the task runner thread.
    auto notification_copy =
        message_center::Notification::DeepCopy(notification, true, true, true);

    PostTaskToTaskRunnerThread(
        base::BindOnce(&NotificationPlatformBridgeWinImpl::DisplayOnTaskRunner,
                       this, notification_type, notification_id, profile_id,
                       is_incognito, base::Passed(&notification_copy)));
  }

  void Close(const std::string& profile_id,
             const std::string& notification_id) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    // TODO(peter): Implement the ability to close notifications.
  }

  void GetDisplayed(
      const std::string& profile_id,
      bool incognito,
      const GetDisplayedNotificationsCallback& callback) const override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    auto displayed_notifications = std::make_unique<std::set<std::string>>();
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::Bind(callback, base::Passed(&displayed_notifications),
                   false /* supports_synchronization */));
  }

  void SetReadyCallback(NotificationBridgeReadyCallback callback) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    std::move(callback).Run(com_functions_initialized_);
  }

 private:
  friend class base::RefCountedThreadSafe<NotificationPlatformBridgeWinImpl>;

  ~NotificationPlatformBridgeWinImpl() override {}

  void PostTaskToTaskRunnerThread(base::OnceClosure closure) const {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    DCHECK(task_runner_);
    bool success = task_runner_->PostTask(FROM_HERE, std::move(closure));
    DCHECK(success);
  }

  void DisplayOnTaskRunner(
      NotificationCommon::Type notification_type,
      const std::string& notification_id,
      const std::string& profile_id,
      bool incognito,
      std::unique_ptr<message_center::Notification> notification) {
    // TODO(finnur): Move this to a RoInitialized thread, as per
    // crbug.com/761039.
    DCHECK(task_runner_->RunsTasksInCurrentSequence());

    NotificationData* data = FindNotificationData(
        notification_id, profile_id, notification->origin_url(), incognito);
    if (!data) {
      data =
          new NotificationData(notification_type, notification_id, profile_id,
                               incognito, notification->origin_url());
      notifications_.emplace(data, base::WrapUnique(data));
    }

    mswr::ComPtr<winui::Notifications::IToastNotificationManagerStatics>
        toast_manager;
    HRESULT hr = CreateActivationFactory(
        RuntimeClass_Windows_UI_Notifications_ToastNotificationManager,
        toast_manager.GetAddressOf());
    if (FAILED(hr)) {
      LOG(ERROR) << "Unable to create the ToastNotificationManager";
      return;
    }

    base::string16 browser_model_id =
        ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall());
    ScopedHString application_id = ScopedHString::Create(browser_model_id);
    mswr::ComPtr<winui::Notifications::IToastNotifier> notifier;
    hr = toast_manager->CreateToastNotifierWithId(application_id.get(),
                                                  &notifier);
    if (FAILED(hr)) {
      LOG(ERROR) << "Unable to create the ToastNotifier";
      return;
    }

    std::unique_ptr<NotificationTemplateBuilder> notification_template =
        NotificationTemplateBuilder::Build(notification_id, *temp_file_keeper_,
                                           *notification);
    mswr::ComPtr<winui::Notifications::IToastNotification> toast;
    hr = GetToastNotification(*notification_template, &toast);
    if (FAILED(hr)) {
      LOG(ERROR) << "Unable to get a toast notification";
      return;
    }

    auto activated_handler = mswr::Callback<ToastActivatedHandler>(
        this, &NotificationPlatformBridgeWinImpl::OnActivated);
    EventRegistrationToken activated_token;
    hr = toast->add_Activated(activated_handler.Get(), &activated_token);
    if (FAILED(hr)) {
      LOG(ERROR) << "Unable to add toast activated event handler";
      return;
    }

    auto dismissed_handler = mswr::Callback<ToastDismissedHandler>(
        this, &NotificationPlatformBridgeWinImpl::OnDismissed);
    EventRegistrationToken dismissed_token;
    hr = toast->add_Dismissed(dismissed_handler.Get(), &dismissed_token);
    if (FAILED(hr)) {
      LOG(ERROR) << "Unable to add toast dismissed event handler";
      return;
    }

    hr = notifier->Show(toast.Get());
    if (FAILED(hr))
      LOG(ERROR) << "Unable to display the notification";
  }

  HRESULT OnActivated(winui::Notifications::IToastNotification* notification,
                      IInspectable* /* inspectable */) {
    // TODO(chengx): We need to write profile id and incognito information into
    // the toast, so that we can retrieve them from |notification| here.
    std::string notification_id = "";
    std::string profile_id = "";
    bool incognito = false;
    GURL origin_url;

    NotificationData* data = FindNotificationData(notification_id, profile_id,
                                                  origin_url, incognito);
    if (data)
      ForwardNotificationOperation(data, NotificationCommon::CLICK);

    return S_OK;
  }

  HRESULT OnDismissed(
      winui::Notifications::IToastNotification* notification,
      winui::Notifications::IToastDismissedEventArgs* /* args */) {
    // TODO(chengx): We need to write profile_id and incognito information into
    // the toast, so that we can retrieve them from |notification| here.
    std::string notification_id = "";
    std::string profile_id = "";
    bool incognito = false;
    GURL origin_url;

    NotificationData* data = FindNotificationData(notification_id, profile_id,
                                                  origin_url, incognito);
    if (data) {
      ForwardNotificationOperation(data, NotificationCommon::CLOSE);
      notifications_.erase(data);
    }

    return S_OK;
  }

  // Returns a notification with properties |notification_id|, |profile_id|,
  // |origin_url| and |incognito| if found in notifications_. Returns nullptr if
  // not found.
  NotificationData* FindNotificationData(const std::string& notification_id,
                                         const std::string& profile_id,
                                         const GURL& origin_url,
                                         bool incognito) {
    for (const auto& item : notifications_) {
      NotificationData* data = item.first;
      if (data->notification_id == notification_id &&
          data->profile_id == profile_id && data->origin_url == origin_url &&
          data->incognito == incognito) {
        return data;
      }
    }

    return nullptr;
  }

  // Stores the set of Notifications in a session.
  // A std::set<std::unique_ptr<T>> doesn't work well because e.g.,
  // std::set::erase(T) would require a std::unique_ptr<T> argument, so the data
  // would get double-destructed.
  template <typename T>
  using UnorderedUniqueSet = std::unordered_map<T*, std::unique_ptr<T>>;
  UnorderedUniqueSet<NotificationData> notifications_;

  // Whether the required functions from combase.dll have been loaded.
  bool com_functions_initialized_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // An object that keeps temp files alive long enough for Windows to pick up.
  std::unique_ptr<TempFileKeeper> temp_file_keeper_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPlatformBridgeWinImpl);
};

NotificationPlatformBridgeWin::NotificationPlatformBridgeWin()
    : impl_(new NotificationPlatformBridgeWinImpl()) {}

NotificationPlatformBridgeWin::~NotificationPlatformBridgeWin() = default;

void NotificationPlatformBridgeWin::Display(
    NotificationCommon::Type notification_type,
    const std::string& notification_id,
    const std::string& profile_id,
    bool incognito,
    const message_center::Notification& notification,
    std::unique_ptr<NotificationCommon::Metadata> metadata) {
  impl_->Display(notification_type, notification_id, profile_id, incognito,
                 notification, std::move(metadata));
}

void NotificationPlatformBridgeWin::Close(const std::string& profile_id,
                                          const std::string& notification_id) {
  impl_->Close(profile_id, notification_id);
}

void NotificationPlatformBridgeWin::GetDisplayed(
    const std::string& profile_id,
    bool incognito,
    const GetDisplayedNotificationsCallback& callback) const {
  impl_->GetDisplayed(profile_id, incognito, callback);
}

void NotificationPlatformBridgeWin::SetReadyCallback(
    NotificationBridgeReadyCallback callback) {
  impl_->SetReadyCallback(std::move(callback));
}
