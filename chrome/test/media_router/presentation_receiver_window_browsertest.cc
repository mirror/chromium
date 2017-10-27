// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/presentation_receiver_window.h"

#include "base/path_service.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/timer/elapsed_timer.h"
#include "chrome/browser/media/router/offscreen_presentation_manager.h"
#include "chrome/browser/media/router/offscreen_presentation_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/presentation_connection_message.h"
#include "content/public/test/browser_test_utils.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/base/filename_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/WebKit/public/platform/modules/presentation/presentation.mojom.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace media_router {
namespace {

constexpr char kPresentationId[] = "test_id";
const base::FilePath::StringPieceType kResourcePath =
    FILE_PATH_LITERAL("media_router/browser_test_resources/");

bool ConditionalWait(base::TimeDelta timeout,
                     base::TimeDelta interval,
                     const base::Callback<bool()>& callback) {
  base::ElapsedTimer timer;
  do {
    if (callback.Run()) {
      return true;
    }

    base::RunLoop run_loop;
    PostDelayedTask(FROM_HERE, run_loop.QuitClosure(), interval);
    run_loop.Run();
  } while (timer.Elapsed() < timeout);
  return false;
}

base::FilePath GetResourceFile(base::FilePath::StringPieceType relative_path) {
  base::FilePath base_dir;
  // ASSERT_TRUE can only be used in void returning functions.
  // Use CHECK instead in non-void returning functions.
  CHECK(PathService::Get(base::DIR_MODULE, &base_dir));
  base::FilePath full_path =
      base_dir.Append(kResourcePath).Append(relative_path);
  {
    base::ScopedAllowBlockingForTesting scoped_allow_blocking;
    CHECK(PathExists(full_path));
  }
  return full_path;
}

class FakeControllerConnection : public blink::mojom::PresentationConnection {
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

  bool is_receiver_connection_bound() const {
    return receiver_connection_.is_bound();
  }

 private:
  mojo::Binding<blink::mojom::PresentationConnection> binding_;
  blink::mojom::PresentationConnectionPtr receiver_connection_;
};

class PresentationReceiverWindowBrowserTest : public InProcessBrowserTest {
 protected:
};

}  // namespace

IN_PROC_BROWSER_TEST_F(PresentationReceiverWindowBrowserTest, CreatesWindow) {
  PresentationReceiverWindow* receiver_window = new PresentationReceiverWindow(
      browser()->profile(), GURL("about:blank"), gfx::Rect(100, 100));
  receiver_window->Start(kPresentationId);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(receiver_window->window()->IsFullscreen());
  EXPECT_FALSE(
      receiver_window->SupportsWindowFeature(Browser::FEATURE_TABSTRIP));
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

  PresentationReceiverWindow* receiver_window = new PresentationReceiverWindow(
      browser()->profile(), GURL("about:blank"), target_display.bounds());
  receiver_window->Start(kPresentationId);
  base::RunLoop().RunUntilIdle();

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
}

IN_PROC_BROWSER_TEST_F(PresentationReceiverWindowBrowserTest,
                       NavigationClosesWindow) {
  class CloseObserver final : public content::WebContentsObserver {
   public:
    bool destroyed() { return destroyed_; }

    void WebContentsDestroyed() override { destroyed_ = true; }

    void Observe(content::WebContents* web_contents) {
      content::WebContentsObserver::Observe(web_contents);
    }

   private:
    bool destroyed_ = false;
  };

  // Start receiver window.
  const GURL presentation_url =
      net::FilePathToFileURL(media_router::GetResourceFile(
          FILE_PATH_LITERAL("presentation_receiver.html")));
  PresentationReceiverWindow* receiver_window = new PresentationReceiverWindow(
      browser()->profile(), presentation_url, gfx::Rect(100, 100));
  receiver_window->Start(kPresentationId);
  CloseObserver close_observer;
  close_observer.Observe(receiver_window->presentation_web_contents());
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(
      content::WaitForLoadStop(receiver_window->presentation_web_contents()));
  ASSERT_FALSE(close_observer.destroyed());

  ASSERT_TRUE(
      content::ExecuteScript(receiver_window->presentation_web_contents(),
                             "window.location = 'about:blank'"));
  const auto check_destroyed = [&close_observer]() {
    return close_observer.destroyed();
  };
  EXPECT_TRUE(ConditionalWait(base::TimeDelta::FromSeconds(10),
                              base::TimeDelta::FromSeconds(1),
                              base::Bind(&decltype(check_destroyed)::operator(),
                                         base::Unretained(&check_destroyed))));
}

IN_PROC_BROWSER_TEST_F(PresentationReceiverWindowBrowserTest,
                       PresentationApiCommunication) {
  // Start receiver window.
  const GURL presentation_url =
      net::FilePathToFileURL(media_router::GetResourceFile(
          FILE_PATH_LITERAL("presentation_receiver.html")));
  PresentationReceiverWindow* receiver_window = new PresentationReceiverWindow(
      browser()->profile(), presentation_url, gfx::Rect(100, 100));
  receiver_window->Start(kPresentationId);
  base::RunLoop().RunUntilIdle();

  // Register controller with OffscreenPresentationManager using test-local
  // implementation of blink::mojom::PresentationConnection.
  FakeControllerConnection controller_connection;
  auto controller_ptr = controller_connection.Bind();
  ASSERT_FALSE(controller_connection.is_receiver_connection_bound());
  OffscreenPresentationManagerFactory::GetOrCreateForBrowserContext(
      browser()->profile())
      ->RegisterOffscreenPresentationController(
          content::PresentationInfo(presentation_url, kPresentationId),
          RenderFrameHostId(0, 0), std::move(controller_ptr),
          controller_connection.MakeConnectionRequest(),
          MediaRoute("route", MediaSource(presentation_url), "sink", "desc",
                     true, "", true));
  ASSERT_TRUE(
      content::WaitForLoadStop(receiver_window->presentation_web_contents()));
  const auto check_connection = [&controller_connection]() {
    return controller_connection.is_receiver_connection_bound();
  };
  ASSERT_TRUE(ConditionalWait(
      base::TimeDelta::FromSeconds(10), base::TimeDelta::FromSeconds(1),
      base::Bind(&decltype(check_connection)::operator(),
                 base::Unretained(&check_connection))));

  // Test ping-pong message.
  const std::string message("turtles");
  bool received_message = false;
  EXPECT_CALL(controller_connection, OnMessageMock(::testing::_))
      .WillOnce(::testing::Invoke(
          [&message,
           &received_message](content::PresentationConnectionMessage response) {
            ASSERT_TRUE(response.message);
            EXPECT_EQ("Pong: " + message, *response.message);
            received_message = true;
          }));
  controller_connection.SendTextMessage(message);
  const auto check_received_message = [&received_message]() {
    return received_message == true;
  };
  ASSERT_TRUE(ConditionalWait(
      base::TimeDelta::FromSeconds(10), base::TimeDelta::FromSeconds(1),
      base::Bind(&decltype(check_received_message)::operator(),
                 base::Unretained(&check_received_message))));
}

}  // namespace media_router
