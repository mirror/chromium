// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/presentation_receiver_window.h"

#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/timer/elapsed_timer.h"
#include "chrome/browser/media/router/local_presentation_manager.h"
#include "chrome/browser/media/router/local_presentation_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/media_router/presentation_receiver_window_factory.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/presentation_connection_message.h"
#include "content/public/common/presentation_info.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/script_executor.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/base/filename_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/WebKit/public/platform/modules/presentation/presentation.mojom.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace media_router {
namespace {

constexpr char kPresentationId[] = "test_id";
const base::FilePath::StringPieceType kResourcePath =
    FILE_PATH_LITERAL("media_router/browser_test_resources/");

base::FilePath GetResourceFile(base::FilePath::StringPieceType relative_path) {
  base::FilePath base_dir;
  if (!PathService::Get(base::DIR_MODULE, &base_dir))
    return base::FilePath();
  base::FilePath full_path =
      base_dir.Append(kResourcePath).Append(relative_path);
  {
    base::ScopedAllowBlockingForTesting scoped_allow_blocking;
    if (!PathExists(full_path))
      return base::FilePath();
  }
  return full_path;
}

// This class waits for a WebContents it is assigned via Observe to be destroyed
// and then quits a RunLoop it is given.  This is used in tests to wait for the
// receiver page to be torn down in the presentation window.
class CloseObserver final : public content::WebContentsObserver {
 public:
  explicit CloseObserver(base::RunLoop* run_loop) : run_loop_(run_loop) {}

  // content::WebContentsObserver overrides.
  void WebContentsDestroyed() override { run_loop_->Quit(); }

  using content::WebContentsObserver::Observe;

 private:
  base::RunLoop* const run_loop_;

  DISALLOW_COPY_AND_ASSIGN(CloseObserver);
};

// This class imitates a presentation controller page from a messaging
// standpoint.  It is registered as a controller connection for the appropriate
// presentation ID with the LocalPresentationManager to facilitate a
// presentation API communication test with the receiver window.
class FakeControllerConnection final
    : public blink::mojom::PresentationConnection {
 public:
  using OnMessageCallback = base::OnceCallback<void(bool)>;

  FakeControllerConnection() : binding_(this) {}

  void SendTextMessage(const std::string& message) {
    ASSERT_TRUE(receiver_connection_.is_bound());
    receiver_connection_->OnMessage(
        content::PresentationConnectionMessage(message),
        base::BindOnce([](bool success) { ASSERT_TRUE(success); }));
  }

  // blink::mojom::PresentationConnection implementation
  void OnMessage(content::PresentationConnectionMessage message,
                 OnMessageCallback callback) {
    OnMessageMock(message);
    std::move(callback).Run(true);
  }
  MOCK_METHOD1(OnMessageMock,
               void(content::PresentationConnectionMessage message));
  void DidChangeState(content::PresentationConnectionState state) override {}
  void RequestClose() override {}

  blink::mojom::PresentationConnectionRequest MakeConnectionRequest() {
    return mojo::MakeRequest(&receiver_connection_);
  }
  blink::mojom::PresentationConnectionPtr Bind() {
    blink::mojom::PresentationConnectionPtr connection;
    binding_.Bind(mojo::MakeRequest(&connection));
    return connection;
  }

 private:
  mojo::Binding<blink::mojom::PresentationConnection> binding_;
  blink::mojom::PresentationConnectionPtr receiver_connection_;

  DISALLOW_COPY_AND_ASSIGN(FakeControllerConnection);
};

void CloseWindow(PresentationReceiverWindow* receiver_window) {
  base::RunLoop run_loop;
  receiver_window->Terminate(run_loop.QuitClosure());
  run_loop.Run();
}

using PresentationReceiverWindowBrowserTest = InProcessBrowserTest;

}  // namespace

IN_PROC_BROWSER_TEST_F(PresentationReceiverWindowBrowserTest, CreatesWindow) {
  auto receiver_window =
      PresentationReceiverWindowFactory::CreateFromOriginalProfile(
          browser()->profile(), gfx::Rect(100, 100));
  receiver_window->Start(kPresentationId, GURL("about:blank"));
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(receiver_window->window()->IsFullscreen());

  CloseWindow(receiver_window.get());
}

IN_PROC_BROWSER_TEST_F(PresentationReceiverWindowBrowserTest,
                       MANUAL_CreatesWindowOnGivenDisplay) {
  // Pick specific display.
  auto* screen = display::Screen::GetScreen();
  const auto& displays = screen->GetAllDisplays();
  for (const auto& display : displays) {
    DVLOG(0) << display.ToString();
  }

  // Choose a non-default display to which to move the receiver window.
  ASSERT_LE(2ul, displays.size());
  const auto default_display =
      screen->GetDisplayNearestWindow(browser()->window()->GetNativeWindow());
  display::Display target_display;
  ASSERT_FALSE(target_display.is_valid());
  for (const auto& display : displays) {
    if (display.id() != default_display.id()) {
      target_display = display;
      break;
    }
  }
  ASSERT_TRUE(target_display.is_valid());

  auto receiver_window =
      PresentationReceiverWindowFactory::CreateFromOriginalProfile(
          browser()->profile(), target_display.bounds());
  receiver_window->Start(kPresentationId, GURL("about:blank"));
  ASSERT_TRUE(
      content::WaitForLoadStop(receiver_window->presentation_web_contents()));

  base::RunLoop run_loop;
  PostDelayedTask(FROM_HERE, run_loop.QuitClosure(),
                  base::TimeDelta::FromSeconds(5));
  run_loop.Run();

  // Check the window is on the correct display.
  const auto display = screen->GetDisplayNearestWindow(
      receiver_window->window()->GetNativeWindow());
  EXPECT_EQ(display.id(), target_display.id());

  // The inactive test won't work on single-display systems because fullscreen
  // forces it to have focus, so it must be part of the manual test with 2+
  // displays.
  EXPECT_FALSE(receiver_window->window()->IsActive());
}

IN_PROC_BROWSER_TEST_F(PresentationReceiverWindowBrowserTest,
                       NavigationClosesWindow) {
  // Start receiver window.
  auto file_path = media_router::GetResourceFile(
      FILE_PATH_LITERAL("presentation_receiver.html"));
  ASSERT_FALSE(file_path.empty());
  const GURL presentation_url = net::FilePathToFileURL(file_path);
  auto receiver_window =
      PresentationReceiverWindowFactory::CreateFromOriginalProfile(
          browser()->profile(), gfx::Rect(100, 100));
  receiver_window->Start(kPresentationId, presentation_url);
  ASSERT_TRUE(
      content::WaitForLoadStop(receiver_window->presentation_web_contents()));

  base::RunLoop run_loop;
  CloseObserver close_observer(&run_loop);
  close_observer.Observe(receiver_window->presentation_web_contents());

  ASSERT_TRUE(
      content::ExecuteScript(receiver_window->presentation_web_contents(),
                             "window.location = 'about:blank'"));
  run_loop.Run();

  CloseWindow(receiver_window.get());
}

IN_PROC_BROWSER_TEST_F(PresentationReceiverWindowBrowserTest,
                       PresentationApiCommunication) {
  // Start receiver window.
  auto file_path = media_router::GetResourceFile(
      FILE_PATH_LITERAL("presentation_receiver.html"));
  ASSERT_FALSE(file_path.empty());
  const GURL presentation_url = net::FilePathToFileURL(file_path);
  auto receiver_window =
      PresentationReceiverWindowFactory::CreateFromOriginalProfile(
          browser()->profile(), gfx::Rect(100, 100));
  receiver_window->Start(kPresentationId, presentation_url);

  // Register controller with LocalPresentationManager using test-local
  // implementation of blink::mojom::PresentationConnection.
  FakeControllerConnection controller_connection;
  auto controller_ptr = controller_connection.Bind();
  LocalPresentationManagerFactory::GetOrCreateForBrowserContext(
      browser()->profile())
      ->RegisterLocalPresentationController(
          content::PresentationInfo(presentation_url, kPresentationId),
          RenderFrameHostId(0, 0), std::move(controller_ptr),
          controller_connection.MakeConnectionRequest(),
          MediaRoute("route", MediaSource(presentation_url), "sink", "desc",
                     true, "", true));
  ASSERT_TRUE(
      content::WaitForLoadStop(receiver_window->presentation_web_contents()));

  // Test ping-pong message.
  const std::string message("turtles");
  base::RunLoop run_loop;
  EXPECT_CALL(controller_connection, OnMessageMock(::testing::_))
      .WillOnce(::testing::Invoke(
          [&run_loop,
           &message](content::PresentationConnectionMessage response) {
            ASSERT_TRUE(response.message);
            EXPECT_EQ("Pong: " + message, *response.message);
            run_loop.Quit();
          }));
  controller_connection.SendTextMessage(message);
  run_loop.Run();

  CloseWindow(receiver_window.get());
}

IN_PROC_BROWSER_TEST_F(PresentationReceiverWindowBrowserTest,
                       WindowClosingTerminatesPresentation) {
  // Start receiver window.
  auto receiver_window =
      PresentationReceiverWindowFactory::CreateFromOriginalProfile(
          browser()->profile(), gfx::Rect(100, 100));
  receiver_window->Start(kPresentationId, GURL("about:blank"));
  ASSERT_TRUE(
      content::WaitForLoadStop(receiver_window->presentation_web_contents()));

  base::RunLoop run_loop;
  CloseObserver close_observer(&run_loop);
  close_observer.Observe(receiver_window->presentation_web_contents());

  receiver_window->window()->Close();
  run_loop.Run();

  CloseWindow(receiver_window.get());
}

}  // namespace media_router
