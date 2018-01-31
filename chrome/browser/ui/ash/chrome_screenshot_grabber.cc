// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/chrome_screenshot_grabber.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "ash/shell.h"
#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/i18n/time_formatting.h"
#include "base/macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/file_manager/open_util.h"
#include "chrome/browser/chromeos/note_taking_helper.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "chromeos/login/login_state.h"
#include "components/drive/chromeos/file_system_interface.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "services/data_decoder/public/cpp/decode_image.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/paint_vector_icon.h"

namespace {

const char kNotificationId[] = "screenshot";
const char kNotifierScreenshot[] = "ash.screenshot";

const char kNotificationOriginUrl[] = "chrome://screenshot";

// User is waiting for the screenshot-taken notification, hence USER_VISIBLE.
constexpr base::TaskTraits kBlockingTaskTraits = {
    base::MayBlock(), base::TaskPriority::USER_VISIBLE,
    base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN};

ChromeScreenshotGrabber* g_chrome_screenshot_grabber_instance = nullptr;

void CopyScreenshotToClipboard(const SkBitmap& decoded_image) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  {
    ui::ScopedClipboardWriter scw(ui::CLIPBOARD_TYPE_COPY_PASTE);
    scw.WriteImage(decoded_image);
  }
  base::RecordAction(base::UserMetricsAction("Screenshot_CopyClipboard"));
}

void DecodeFileAndCopyToClipboard(
    scoped_refptr<base::RefCountedString> png_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  service_manager::mojom::ConnectorRequest connector_request;
  std::unique_ptr<service_manager::Connector> connector =
      service_manager::Connector::Create(&connector_request);
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindConnectorRequest(std::move(connector_request));

  // Decode the image in sandboxed process becuase |png_data| comes from
  // external storage.
  data_decoder::DecodeImage(
      connector.get(),
      std::vector<uint8_t>(png_data->data().begin(), png_data->data().end()),
      data_decoder::mojom::ImageCodec::DEFAULT, false,
      data_decoder::kDefaultMaxSizeInBytes, gfx::Size(),
      base::BindOnce(&CopyScreenshotToClipboard));
}

void ReadFileAndCopyToClipboardLocal(const base::FilePath& screenshot_path) {
  base::AssertBlockingAllowed();
  scoped_refptr<base::RefCountedString> png_data(new base::RefCountedString());
  if (!base::ReadFileToString(screenshot_path, &(png_data->data()))) {
    LOG(ERROR) << "Failed to read the screenshot file: "
               << screenshot_path.value();
    return;
  }

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&DecodeFileAndCopyToClipboard, png_data));
}

void ReadFileAndCopyToClipboardDrive(
    drive::FileError error,
    const base::FilePath& file_path,
    std::unique_ptr<drive::ResourceEntry> entry) {
  if (error != drive::FILE_ERROR_OK) {
    LOG(ERROR) << "Failed to read the screenshot path on drive: "
               << drive::FileErrorToString(error);
    return;
  }
  base::PostTaskWithTraits(
      FROM_HERE, kBlockingTaskTraits,
      base::BindOnce(&ReadFileAndCopyToClipboardLocal, file_path));
}

// Delegate for a notification. This class has two roles: to implement callback
// methods for notification, and to provide an identity of the associated
// notification.
class ScreenshotGrabberNotificationDelegate
    : public message_center::NotificationDelegate {
 public:
  ScreenshotGrabberNotificationDelegate(bool success,
                                        Profile* profile,
                                        const base::FilePath& screenshot_path)
      : success_(success),
        profile_(profile),
        screenshot_path_(screenshot_path) {}

  // message_center::NotificationDelegate:
  void Click() override {
    if (!success_)
      return;
    platform_util::ShowItemInFolder(profile_, screenshot_path_);
  }
  void ButtonClick(int button_index) override {
    DCHECK(success_);

    switch (button_index) {
      case BUTTON_COPY_TO_CLIPBOARD: {
        // To avoid keeping the screenshot image in memory, re-read the
        // screenshot file and copy it to the clipboard.
        if (drive::util::IsUnderDriveMountPoint(screenshot_path_)) {
          drive::FileSystemInterface* file_system =
              drive::util::GetFileSystemByProfile(profile_);
          file_system->GetFile(drive::util::ExtractDrivePath(screenshot_path_),
                               base::Bind(&ReadFileAndCopyToClipboardDrive));
          return;
        }
        base::PostTaskWithTraits(
            FROM_HERE, kBlockingTaskTraits,
            base::BindOnce(&ReadFileAndCopyToClipboardLocal, screenshot_path_));
        break;
      }
      case BUTTON_ANNOTATE: {
        chromeos::NoteTakingHelper* helper = chromeos::NoteTakingHelper::Get();
        if (helper->IsAppAvailable(profile_)) {
          helper->LaunchAppForNewNote(profile_, screenshot_path_);
          base::RecordAction(base::UserMetricsAction("Screenshot_Annotate"));
        }
        break;
      }
      default:
        NOTREACHED() << "Unhandled button index " << button_index;
    }
  }

 private:
  ~ScreenshotGrabberNotificationDelegate() override {}

  enum ButtonIndex {
    BUTTON_COPY_TO_CLIPBOARD = 0,
    BUTTON_ANNOTATE,
  };

  const bool success_;
  Profile* profile_;
  const base::FilePath screenshot_path_;

  DISALLOW_COPY_AND_ASSIGN(ScreenshotGrabberNotificationDelegate);
};

int GetScreenshotNotificationTitle(
    ui::ScreenshotGrabberObserver::Result screenshot_result) {
  switch (screenshot_result) {
    case ui::ScreenshotGrabberObserver::SCREENSHOTS_DISABLED:
      return IDS_SCREENSHOT_NOTIFICATION_TITLE_DISABLED;
    case ui::ScreenshotGrabberObserver::SCREENSHOT_SUCCESS:
      return IDS_SCREENSHOT_NOTIFICATION_TITLE_SUCCESS;
    default:
      return IDS_SCREENSHOT_NOTIFICATION_TITLE_FAIL;
  }
}

int GetScreenshotNotificationText(
    ui::ScreenshotGrabberObserver::Result screenshot_result) {
  switch (screenshot_result) {
    case ui::ScreenshotGrabberObserver::SCREENSHOTS_DISABLED:
      return IDS_SCREENSHOT_NOTIFICATION_TEXT_DISABLED;
    case ui::ScreenshotGrabberObserver::SCREENSHOT_SUCCESS:
      return IDS_SCREENSHOT_NOTIFICATION_TEXT_SUCCESS;
    default:
      return IDS_SCREENSHOT_NOTIFICATION_TEXT_FAIL;
  }
}

void PrepareWritableFileCallback(
    const ChromeScreenshotGrabber::FileCallback& callback,
    drive::FileError error,
    const base::FilePath& local_path) {
  callback.Run(error == drive::FILE_ERROR_OK
                   ? ui::ScreenshotGrabberDelegate::FILE_SUCCESS
                   : ui::ScreenshotGrabberDelegate::FILE_CREATE_FAILED,
               local_path);
}

void EnsureDirectoryExistsCallback(
    const ChromeScreenshotGrabber::FileCallback& callback,
    Profile* profile,
    const base::FilePath& path,
    drive::FileError error) {
  // It is okay to fail with FILE_ERROR_EXISTS since anyway the directory
  // of the target file exists.
  if (error == drive::FILE_ERROR_OK || error == drive::FILE_ERROR_EXISTS) {
    drive::util::PrepareWritableFileAndRun(
        profile, path, base::Bind(&PrepareWritableFileCallback, callback));
  } else {
    LOG(ERROR) << "Failed to ensure the existence of the specified directory "
               << "in Google Drive: " << error;
    base::PostTaskWithTraits(
        FROM_HERE, kBlockingTaskTraits,
        base::BindOnce(callback,
                       ui::ScreenshotGrabberDelegate::FILE_CHECK_DIR_FAILED,
                       base::FilePath()));
  }
}

bool ScreenshotsDisabled() {
  return g_browser_process->local_state()->GetBoolean(
      prefs::kDisableScreenshots);
}

bool ShouldUse24HourClock() {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  if (profile)
    return profile->GetPrefs()->GetBoolean(prefs::kUse24HourClock);
  return base::GetHourClockType() == base::k24HourClock;
}

bool GetScreenshotDirectory(base::FilePath* directory) {
  if (chromeos::LoginState::Get()->IsUserLoggedIn()) {
    DownloadPrefs* download_prefs = DownloadPrefs::FromBrowserContext(
        ProfileManager::GetActiveUserProfile());
    *directory = download_prefs->DownloadPath();
  } else {
    if (!base::GetTempDir(directory)) {
      LOG(ERROR) << "Failed to find temporary directory.";
      return false;
    }
  }
  return true;
}

std::string GetScreenshotBaseFilename(const base::Time& unexploded_now) {
  base::Time::Exploded now;
  unexploded_now.LocalExplode(&now);

  // We don't use base/i18n/time_formatting.h here because it doesn't
  // support our format.  Don't use ICU either to avoid i18n file names
  // for non-English locales.
  // TODO(mukai): integrate this logic somewhere time_formatting.h
  std::string file_name = base::StringPrintf(
      "Screenshot %d-%02d-%02d at ", now.year, now.month, now.day_of_month);

  if (ShouldUse24HourClock()) {
    file_name.append(
        base::StringPrintf("%02d.%02d.%02d", now.hour, now.minute, now.second));
  } else {
    int hour = now.hour;
    if (hour > 12) {
      hour -= 12;
    } else if (hour == 0) {
      hour = 12;
    }
    file_name.append(
        base::StringPrintf("%d.%02d.%02d ", hour, now.minute, now.second));
    file_name.append((now.hour >= 12) ? "PM" : "AM");
  }

  return file_name;
}

// Read a file to a string and return.
std::string ReadFileToString(const base::FilePath& path) {
  std::string data;
  // It may fail, but it doesn't matter for our purpose.
  base::ReadFileToString(path, &data);
  return data;
}

}  // namespace

ChromeScreenshotGrabber::ChromeScreenshotGrabber()
    : screenshot_grabber_(new ui::ScreenshotGrabber(this)),
      weak_factory_(this) {
  screenshot_grabber_->AddObserver(this);
  DCHECK(!g_chrome_screenshot_grabber_instance);
  g_chrome_screenshot_grabber_instance = this;
}

ChromeScreenshotGrabber::~ChromeScreenshotGrabber() {
  DCHECK_EQ(this, g_chrome_screenshot_grabber_instance);
  g_chrome_screenshot_grabber_instance = nullptr;
  screenshot_grabber_->RemoveObserver(this);
}

// static
ChromeScreenshotGrabber* ChromeScreenshotGrabber::Get() {
  return g_chrome_screenshot_grabber_instance;
}

// OK, so the ordering here means that the notification shown is for the
// secondary display's screenshot. Is that what we really want?
//
// Otherwise, this isn't the
//
// Time needs to go into the default separation. As written, we start doing
// time when we start,

void ChromeScreenshotGrabber::HandleTakeScreenshotForAllRootWindows() {
  NewHandleTakeScreenshotForAllRootWindows();
  return;

  if (ScreenshotsDisabled()) {
    screenshot_grabber_->NotifyScreenshotCompleted(
        ui::ScreenshotGrabberObserver::SCREENSHOTS_DISABLED, base::FilePath());
    return;
  }

  base::FilePath screenshot_directory;
  if (!GetScreenshotDirectory(&screenshot_directory)) {
    screenshot_grabber_->NotifyScreenshotCompleted(
        ui::ScreenshotGrabberObserver::SCREENSHOT_GET_DIR_FAILED,
        base::FilePath());
    return;
  }

  std::string screenshot_basename = GetScreenshotBaseFilename(
      base::Time::Now());
  aura::Window::Windows root_windows = ash::Shell::GetAllRootWindows();
  // Reorder root_windows to take the primary root window's snapshot at first.
  aura::Window* primary_root = ash::Shell::GetPrimaryRootWindow();
  if (*(root_windows.begin()) != primary_root) {
    root_windows.erase(
        std::find(root_windows.begin(), root_windows.end(), primary_root));
    root_windows.insert(root_windows.begin(), primary_root);
  }
  std::vector<base::FilePath> filenames;
  for (size_t i = 0; i < root_windows.size(); ++i) {
    aura::Window* root_window = root_windows[i];
    std::string basename = screenshot_basename;
    gfx::Rect rect = root_window->bounds();
    if (root_windows.size() > 1)
      basename += base::StringPrintf(" - Display %d", static_cast<int>(i + 1));
    base::FilePath screenshot_path =
        screenshot_directory.AppendASCII(basename + ".png");
    screenshot_grabber_->TakeScreenshot(root_window, rect, screenshot_path);
  }
  base::RecordAction(base::UserMetricsAction("Screenshot_TakeFull"));
}

// -------------------
// -------------------
// -------------------


void ChromeScreenshotGrabber::NewHandleTakeScreenshotForAllRootWindows() {
  aura::Window::Windows root_windows = ash::Shell::GetAllRootWindows();
  // Reorder root_windows to take the primary root window's snapshot at first.
  aura::Window* primary_root = ash::Shell::GetPrimaryRootWindow();
  if (*(root_windows.begin()) != primary_root) {
    root_windows.erase(
        std::find(root_windows.begin(), root_windows.end(), primary_root));
    root_windows.insert(root_windows.begin(), primary_root);
  }
  base::Time time = base::Time::Now();
  for (size_t i = 0; i < root_windows.size(); ++i) {
    aura::Window* root_window = root_windows[i];
    gfx::Rect rect = root_window->bounds();

    base::Optional<int> display_id;
    if (root_windows.size() > 1)
      display_id = static_cast<int>(i + 1);
    screenshot_grabber_->TakeScreenshotAndCall(
        root_window,
        rect,
        base::Bind(&ChromeScreenshotGrabber::OnTookRootWindowScreenshot,
                   weak_factory_.GetWeakPtr(),
                   time,
                   display_id));
  }
  base::RecordAction(base::UserMetricsAction("Screenshot_TakeFull"));
}

// THIS IS ALL COPY PASTA FROM SCREENSHOT GRABBER CC.
namespace {

using ui::ScreenshotGrabberDelegate;
using ui::ScreenshotGrabberObserver;

using ShowNotificationCallback =
    base::Callback<void(ScreenshotGrabberObserver::Result screenshot_result,
                        const base::FilePath& screenshot_path)>;

void SaveScreenshot(scoped_refptr<base::TaskRunner> ui_task_runner,
                    const ShowNotificationCallback& callback,
                    const base::FilePath& screenshot_path,
                    scoped_refptr<base::RefCountedMemory> png_data,
                    ScreenshotGrabberDelegate::FileResult result,
                    const base::FilePath& local_path) {
  DCHECK(!base::MessageLoopForUI::IsCurrent());
  DCHECK(!screenshot_path.empty());

  // Convert FileResult into ScreenshotGrabberObserver::Result.
  ScreenshotGrabberObserver::Result screenshot_result =
      ScreenshotGrabberObserver::SCREENSHOT_SUCCESS;
  switch (result) {
    case ScreenshotGrabberDelegate::FILE_SUCCESS:
      // Successfully got a local file to write to, write png data.
      DCHECK_GT(static_cast<int>(png_data->size()), 0);
      if (static_cast<size_t>(base::WriteFile(
              local_path, reinterpret_cast<const char*>(png_data->front()),
              static_cast<int>(png_data->size()))) != png_data->size()) {
        LOG(ERROR) << "Failed to save to " << local_path.value();
        screenshot_result =
            ScreenshotGrabberObserver::SCREENSHOT_WRITE_FILE_FAILED;
      }
      break;
    case ScreenshotGrabberDelegate::FILE_CHECK_DIR_FAILED:
      screenshot_result =
          ScreenshotGrabberObserver::SCREENSHOT_CHECK_DIR_FAILED;
      break;
    case ScreenshotGrabberDelegate::FILE_CREATE_DIR_FAILED:
      screenshot_result =
          ScreenshotGrabberObserver::SCREENSHOT_CREATE_DIR_FAILED;
      break;
    case ScreenshotGrabberDelegate::FILE_CREATE_FAILED:
      screenshot_result =
          ScreenshotGrabberObserver::SCREENSHOT_CREATE_FILE_FAILED;
      break;
  }

  // Report the result on the UI thread.
  ui_task_runner->PostTask(
      FROM_HERE, base::Bind(callback, screenshot_result, screenshot_path));
}

void EnsureLocalDirectoryExists(
    const base::FilePath& path,
    ScreenshotGrabberDelegate::FileCallback callback) {
  DCHECK(!base::MessageLoopForUI::IsCurrent());
  DCHECK(!path.empty());

  if (!base::CreateDirectory(path.DirName())) {
    LOG(ERROR) << "Failed to ensure the existence of "
               << path.DirName().value();
    callback.Run(ScreenshotGrabberDelegate::FILE_CREATE_DIR_FAILED, path);
    return;
  }

  callback.Run(ScreenshotGrabberDelegate::FILE_SUCCESS, path);
}

}  // namespace

void ChromeScreenshotGrabber::OnTookRootWindowScreenshot(
    const base::Time& screenshot_time,
    const base::Optional<int>& display_num,
    ScreenshotGrabberObserver::Result result,
    scoped_refptr<base::RefCountedMemory> png_data) {
  if (result != ui::ScreenshotGrabberObserver::SCREENSHOT_SUCCESS) {
    // We didn't complete taking the screenshot.
    OnScreenshotCompleted(result, base::FilePath());
    return;
  }

  if (ScreenshotsDisabled()) {
    OnScreenshotCompleted(
        ui::ScreenshotGrabberObserver::SCREENSHOTS_DISABLED,
        base::FilePath());
    return;
  }

  base::FilePath screenshot_directory;
  if (!GetScreenshotDirectory(&screenshot_directory)) {
    OnScreenshotCompleted(
        ui::ScreenshotGrabberObserver::SCREENSHOT_GET_DIR_FAILED,
        base::FilePath());
    return;
  }

  // Calculate the path.
  std::string screenshot_basename = GetScreenshotBaseFilename(screenshot_time);
  if (display_num.has_value()) {
    screenshot_basename +=
        base::StringPrintf(" - Display %d", *display_num);
  }
  base::FilePath screenshot_path =
      screenshot_directory.AppendASCII(screenshot_basename + ".png");

  ShowNotificationCallback screenshot_complete_callback(base::Bind(
      &ChromeScreenshotGrabber::OnScreenshotCompleted,
      weak_factory_.GetWeakPtr()));

  NewPrepareFileAndRunOnBlockingPool(
      screenshot_path,
      base::Bind(&SaveScreenshot, base::ThreadTaskRunnerHandle::Get(),
                 screenshot_complete_callback, screenshot_path, png_data));
}

void ChromeScreenshotGrabber::NewPrepareFileAndRunOnBlockingPool(
    const base::FilePath& path,
    const FileCallback& callback) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  if (drive::util::IsUnderDriveMountPoint(path)) {
    drive::util::EnsureDirectoryExists(
        profile, path.DirName(),
        base::Bind(&EnsureDirectoryExistsCallback, callback, profile, path));
    return;
  }

  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::Bind(EnsureLocalDirectoryExists, path, callback));
}

// -------------------
// -------------------
// -------------------

void ChromeScreenshotGrabber::HandleTakePartialScreenshot(
    aura::Window* window,
    const gfx::Rect& rect) {
  if (ScreenshotsDisabled()) {
    screenshot_grabber_->NotifyScreenshotCompleted(
        ui::ScreenshotGrabberObserver::SCREENSHOTS_DISABLED, base::FilePath());
    return;
  }

  base::FilePath screenshot_directory;
  if (!GetScreenshotDirectory(&screenshot_directory)) {
    screenshot_grabber_->NotifyScreenshotCompleted(
        ui::ScreenshotGrabberObserver::SCREENSHOT_GET_DIR_FAILED,
        base::FilePath());
    return;
  }

  base::FilePath screenshot_path =
      screenshot_directory.AppendASCII(GetScreenshotBaseFilename(
      base::Time::Now()) + ".png");
  screenshot_grabber_->TakeScreenshot(window, rect, screenshot_path);
  base::RecordAction(base::UserMetricsAction("Screenshot_TakePartial"));
}

void ChromeScreenshotGrabber::HandleTakeWindowScreenshot(aura::Window* window) {
  if (ScreenshotsDisabled()) {
    screenshot_grabber_->NotifyScreenshotCompleted(
        ui::ScreenshotGrabberObserver::SCREENSHOTS_DISABLED, base::FilePath());
    return;
  }

  base::FilePath screenshot_directory;
  if (!GetScreenshotDirectory(&screenshot_directory)) {
    screenshot_grabber_->NotifyScreenshotCompleted(
        ui::ScreenshotGrabberObserver::SCREENSHOT_GET_DIR_FAILED,
        base::FilePath());
    return;
  }

  base::FilePath screenshot_path =
      screenshot_directory.AppendASCII(GetScreenshotBaseFilename(
      base::Time::Now()) + ".png");
  screenshot_grabber_->TakeScreenshot(
      window, gfx::Rect(window->bounds().size()), screenshot_path);
  base::RecordAction(base::UserMetricsAction("Screenshot_TakeWindow"));
}

bool ChromeScreenshotGrabber::CanTakeScreenshot() {
  return screenshot_grabber_->CanTakeScreenshot();
}

void ChromeScreenshotGrabber::PrepareFileAndRunOnBlockingPool(
    const base::FilePath& path,
    const FileCallback& callback) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  if (drive::util::IsUnderDriveMountPoint(path)) {
    drive::util::EnsureDirectoryExists(
        profile, path.DirName(),
        base::Bind(&EnsureDirectoryExistsCallback, callback, profile, path));
    return;
  }

  ui::ScreenshotGrabberDelegate::PrepareFileAndRunOnBlockingPool(path,
                                                                 callback);
}

void ChromeScreenshotGrabber::OnScreenshotCompleted(
    ui::ScreenshotGrabberObserver::Result result,
    const base::FilePath& screenshot_path) {
  LOG(ERROR) << "OnScreenshotCompleted(" << result << ", " << screenshot_path << ")";
  
  // TODO(erg): This is the only thing actually called by
  // ScreenshotGrabber::NotifyScreenshotCompleted.

  // Do not show a notification that a screenshot was taken while no user is
  // logged in, since it is confusing for the user to get a message about it
  // after they log in (crbug.com/235217).
  if (!chromeos::LoginState::Get()->IsUserLoggedIn())
    return;

  if (result != ui::ScreenshotGrabberObserver::SCREENSHOT_SUCCESS) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &ChromeScreenshotGrabber::OnReadScreenshotFileForPreviewCompleted,
            weak_factory_.GetWeakPtr(), result, screenshot_path, gfx::Image()));
    return;
  }

  if (drive::util::IsUnderDriveMountPoint(screenshot_path)) {
    drive::FileSystemInterface* file_system =
        drive::util::GetFileSystemByProfile(GetProfile());
    if (!file_system) {
      LOG(ERROR) << "Failed to get file system of current profile";

      content::BrowserThread::PostTask(
          content::BrowserThread::UI, FROM_HERE,
          base::BindOnce(
              &ChromeScreenshotGrabber::OnReadScreenshotFileForPreviewCompleted,
              weak_factory_.GetWeakPtr(), result, screenshot_path,
              gfx::Image()));
      return;
    }
    file_system->GetFile(
        drive::util::ExtractDrivePath(screenshot_path),
        base::BindRepeating(
            &ChromeScreenshotGrabber::ReadScreenshotFileForPreviewDrive,
            weak_factory_.GetWeakPtr(), screenshot_path));
  } else {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &ChromeScreenshotGrabber::ReadScreenshotFileForPreviewLocal,
            weak_factory_.GetWeakPtr(), screenshot_path, screenshot_path));
  }
}

void ChromeScreenshotGrabber::ReadScreenshotFileForPreviewLocal(
    const base::FilePath& screenshot_path,
    const base::FilePath& screenshot_cache_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, kBlockingTaskTraits,
      base::BindOnce(&ReadFileToString, screenshot_cache_path),
      base::BindOnce(&ChromeScreenshotGrabber::DecodeScreenshotFileForPreview,
                     weak_factory_.GetWeakPtr(), screenshot_path));
}

void ChromeScreenshotGrabber::ReadScreenshotFileForPreviewDrive(
    const base::FilePath& screenshot_path,
    drive::FileError error,
    const base::FilePath& screenshot_cache_path,
    std::unique_ptr<drive::ResourceEntry> entry) {
  if (error != drive::FILE_ERROR_OK) {
    LOG(ERROR) << "Failed to read the screenshot path on drive: "
               << drive::FileErrorToString(error);
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &ChromeScreenshotGrabber::OnReadScreenshotFileForPreviewCompleted,
            weak_factory_.GetWeakPtr(),
            ui::ScreenshotGrabberObserver::SCREENSHOT_SUCCESS, screenshot_path,
            gfx::Image()));
    return;
  }
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &ChromeScreenshotGrabber::ReadScreenshotFileForPreviewLocal,
          weak_factory_.GetWeakPtr(), screenshot_path, screenshot_cache_path));
}

void ChromeScreenshotGrabber::DecodeScreenshotFileForPreview(
    const base::FilePath& screenshot_path,
    std::string image_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (image_data.empty()) {
    LOG(ERROR) << "Failed to read the screenshot file: "
               << screenshot_path.value();
    OnReadScreenshotFileForPreviewCompleted(
        ui::ScreenshotGrabberObserver::SCREENSHOT_SUCCESS, screenshot_path,
        gfx::Image());
    return;
  }

  service_manager::mojom::ConnectorRequest connector_request;
  std::unique_ptr<service_manager::Connector> connector =
      service_manager::Connector::Create(&connector_request);
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindConnectorRequest(std::move(connector_request));

  // Decode the image in sandboxed process becuase decode image_data comes from
  // external storage.
  data_decoder::DecodeImage(
      connector.get(),
      std::vector<uint8_t>(image_data.begin(), image_data.end()),
      data_decoder::mojom::ImageCodec::DEFAULT, false,
      data_decoder::kDefaultMaxSizeInBytes, gfx::Size(),
      base::BindOnce(
          &ChromeScreenshotGrabber::OnScreenshotFileForPreviewDecoded,
          weak_factory_.GetWeakPtr(), screenshot_path));
}

void ChromeScreenshotGrabber::OnScreenshotFileForPreviewDecoded(
    const base::FilePath& screenshot_path,
    const SkBitmap& decoded_image) {
  OnReadScreenshotFileForPreviewCompleted(
      ui::ScreenshotGrabberObserver::SCREENSHOT_SUCCESS, screenshot_path,
      gfx::Image::CreateFrom1xBitmap(decoded_image));
}

void ChromeScreenshotGrabber::OnReadScreenshotFileForPreviewCompleted(
    ui::ScreenshotGrabberObserver::Result result,
    const base::FilePath& screenshot_path,
    gfx::Image image) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const std::string notification_id(kNotificationId);
  // We cancel a previous screenshot notification, if any, to ensure we get
  // a fresh notification pop-up.
  NotificationDisplayService::GetForProfile(GetProfile())
      ->Close(NotificationHandler::Type::TRANSIENT, notification_id);

  const bool success =
      (result == ui::ScreenshotGrabberObserver::SCREENSHOT_SUCCESS);

  message_center::RichNotificationData optional_field;
  if (success) {
    // The order in which these buttons are added must be reflected by
    // ScreenshotGrabberNotificationDelegate::ButtonIndex.
    message_center::ButtonInfo copy_button(l10n_util::GetStringUTF16(
        IDS_SCREENSHOT_NOTIFICATION_BUTTON_COPY_TO_CLIPBOARD));
    copy_button.icon = ui::ResourceBundle::GetSharedInstance().GetImageNamed(
        IDR_NOTIFICATION_SCREENSHOT_COPY_TO_CLIPBOARD);
    optional_field.buttons.push_back(copy_button);

    if (chromeos::NoteTakingHelper::Get()->IsAppAvailable(GetProfile())) {
      message_center::ButtonInfo annotate_button(l10n_util::GetStringUTF16(
          IDS_SCREENSHOT_NOTIFICATION_BUTTON_ANNOTATE));
      annotate_button.icon =
          ui::ResourceBundle::GetSharedInstance().GetImageNamed(
              IDR_NOTIFICATION_SCREENSHOT_ANNOTATE);
      optional_field.buttons.push_back(annotate_button);
    }

    // Assign image for notification preview. It might be empty.
    optional_field.image = image;
  }

  std::unique_ptr<message_center::Notification> notification =
      message_center::Notification::CreateSystemNotification(
          image.IsEmpty() ? message_center::NOTIFICATION_TYPE_SIMPLE
                          : message_center::NOTIFICATION_TYPE_IMAGE,
          kNotificationId,
          l10n_util::GetStringUTF16(GetScreenshotNotificationTitle(result)),
          l10n_util::GetStringUTF16(GetScreenshotNotificationText(result)),
          gfx::Image(),
          l10n_util::GetStringUTF16(IDS_SCREENSHOT_NOTIFICATION_NOTIFIER_NAME),
          GURL(kNotificationOriginUrl),
          message_center::NotifierId(
              message_center::NotifierId::SYSTEM_COMPONENT,
              kNotifierScreenshot),
          optional_field,
          new ScreenshotGrabberNotificationDelegate(success, GetProfile(),
                                                    screenshot_path),
          kNotificationImageIcon,
          message_center::SystemNotificationWarningLevel::NORMAL);
  notification->set_clickable(success);

  NotificationDisplayService::GetForProfile(GetProfile())
      ->Display(NotificationHandler::Type::TRANSIENT, *notification);
}

Profile* ChromeScreenshotGrabber::GetProfile() {
  return ProfileManager::GetActiveUserProfile();
}
