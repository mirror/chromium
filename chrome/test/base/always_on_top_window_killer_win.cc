// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/always_on_top_window_killer_win.h"

#include <Windows.h>

#include <ios>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/numerics/safe_conversions.h"
#include "base/process/process.h"
#include "base/strings/stringprintf.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image.h"
#include "ui/snapshot/snapshot_win.h"

namespace {

constexpr char kDialogFoundBeforeTest[] =
    "There is an always on top dialog on the desktop. This was most likely "
    "caused by a previous test and may cause this test to fail. Trying to "
    "close it.";

constexpr char kDialogFoundPostTest[] =
    "There is an always on top dialog on the desktop after running this test. "
    "This was most likely caused by this test and may cause future tests to "
    "fail, trying to close it.";

constexpr char kWindowFoundBeforeTest[] =
    "There is an always on top window on the desktop before running the test. "
    "This may have been caused by a previous test and may cause this test to "
    "fail, class-name=";

constexpr char kWindowFoundPostTest[] =
    "There is an always on top window on the desktop after running the test. "
    "This may have been caused by this test or a previous test and may cause "
    "flake, class-name=";

constexpr char kSnapshotOutputDir[] = "snapshot-output-dir";

// Saves a snapshot of |hwnd|'s client area to a file in |output_dir|, returning
// the path to the file if created. An empty path is returned if no new snapshot
// is created. The name of the file is generated based on the tuple (|hwnd|,
// |process_id|, |thread_id|). No new snapshot is created if a snapshot for a
// given tuple already exists.
base::FilePath SaveSnapshot(HWND hwnd,
                            DWORD process_id,
                            DWORD thread_id,
                            const base::FilePath& output_dir) {
  // Create the output file.
  base::FilePath output_path(output_dir.Append(base::FilePath(
      base::StringPrintf(L"ss_%u_%u_%u.png", hwnd, process_id, thread_id))));
  base::File file(output_path, base::File::FLAG_CREATE |
                                   base::File::FLAG_WRITE |
                                   base::File::FLAG_SHARE_DELETE |
                                   base::File::FLAG_CAN_DELETE_ON_CLOSE);
  if (!file.IsValid()) {
    if (file.error_details() == base::File::FILE_ERROR_EXISTS) {
      LOG(INFO) << "Skipping window snapshot since it is already present: "
                << output_path.BaseName();
    } else {
      LOG(ERROR) << "Failed to create snapshot output file \"" << output_path
                 << "\" with error " << file.error_details();
    }
    return base::FilePath();
  }

  // Delete the output file in case of any error.
  file.DeleteOnClose(true);

  // Take the snapshot.
  gfx::Image image;
  RECT tmp_rect;
  ::GetClientRect(hwnd, &tmp_rect);
  gfx::Rect bounds(display::win::ScreenWin::DIPToClientRect(
      hwnd, gfx::Rect(gfx::Rect(tmp_rect).size())));
  if (!ui::internal::GrabHwndSnapshot(hwnd, bounds, bounds, &image) ||
      image.IsEmpty()) {
    LOG(ERROR) << "Failed to grab snapshot of window " << std::hex << hwnd;
    return base::FilePath();
  }

  // Encode it.
  std::vector<unsigned char> encoded;
  if (!gfx::PNGCodec::EncodeBGRASkBitmap(*image.ToSkBitmap(), false,
                                         &encoded)) {
    LOG(ERROR) << "Failed to PNG encode window snapshot.";
    return base::FilePath();
  }

  // Write it to disk.
  const int to_write = base::checked_cast<int>(encoded.size());
  int written =
      file.WriteAtCurrentPos(reinterpret_cast<char*>(encoded.data()), to_write);
  if (written != to_write) {
    LOG(ERROR) << "Failed to write entire snapshot to file";
    return base::FilePath();
  }

  // Keep the output file.
  file.DeleteOnClose(false);
  return output_path;
}

BOOL CALLBACK AlwaysOnTopWindowProc(HWND hwnd, LPARAM l_param) {
  const BOOL kContinueIterating = TRUE;

  if (!IsWindowVisible(hwnd) || IsIconic(hwnd))
    return kContinueIterating;

  const LONG ex_styles = GetWindowLong(hwnd, GWL_EXSTYLE);
  if (ex_styles & WS_EX_TOPMOST) {
    wchar_t class_name_chars[512];
    if (GetClassName(hwnd, class_name_chars, arraysize(class_name_chars))) {
      const std::wstring class_name(class_name_chars);
      const RunType run_type = *reinterpret_cast<RunType*>(l_param);
      // "#32770" is used for system dialogs, such as happens if a child
      // process triggers an assert().
      if (class_name == L"#32770") {
        LOG(ERROR) << (run_type == RunType::BEFORE_TEST ? kDialogFoundBeforeTest
                                                        : kDialogFoundPostTest);
        // We don't own the dialog, so we can't destroy it. CloseWindow()
        // results in iconifying the window. An alternative may be to focus it,
        // then send return and wait for close. As we reboot machines running
        // interactive ui tests at least every 12 hours we're going with the
        // simple for now.
        CloseWindow(hwnd);
      } else if (class_name != L"Button" && class_name != L"Shell_TrayWnd" &&
                 class_name != L"Shell_SecondaryTrayWnd") {
        // 'Button' is the start button, 'Shell_TrayWnd' the taskbar, and
        // 'Shell_SecondaryTrayWnd' is the taskbar on non-primary displays.
        //
        // These windows may be problematic as well, but in theory tests should
        // not be creating an always on top window that outlives the test. Log
        // the window in case there are problems.
        DWORD process_id = 0;
        DWORD thread_id = GetWindowThreadProcessId(hwnd, &process_id);

        base::Process process = base::Process::Open(process_id);
        base::string16 process_path(MAX_PATH, L'\0');
        if (process.IsValid()) {
          // It's possible that the actual process owning |hwnd| has gone away
          // and that a new process using the same PID has appeared. If this
          // turns out to be an issue, we could fetch the process start time
          // here and compare it with the time just before getting |thread_id|
          // above. This is likely overkill for diagnostic purposes.
          DWORD str_len = process_path.size();
          if (!::QueryFullProcessImageName(process.Handle(), 0,
                                           &process_path[0], &str_len) ||
              str_len >= MAX_PATH) {
            str_len = 0;
          }
          process_path.resize(str_len);
        }
        LOG(ERROR) << (run_type == RunType::BEFORE_TEST ? kWindowFoundBeforeTest
                                                        : kWindowFoundPostTest)
                   << class_name << " process_id=" << process_id
                   << " thread_id=" << thread_id
                   << " process_path=" << process_path;

        // Save a snapshot of the window if the process was run with an output
        // directory.
        base::FilePath output_dir =
            base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
                kSnapshotOutputDir);
        if (!output_dir.empty()) {
          base::FilePath snapshot_file =
              SaveSnapshot(hwnd, process_id, thread_id, output_dir);
          if (!snapshot_file.empty()) {
            LOG(ERROR) << "Wrote snapshot to file " << snapshot_file;
          }
        }

        return kContinueIterating;
      }
    }
  }
  return kContinueIterating;
}

}  // namespace

void KillAlwaysOnTopWindows(RunType run_type) {
  EnumWindows(AlwaysOnTopWindowProc, reinterpret_cast<LPARAM>(&run_type));
}
