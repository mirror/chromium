// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_navigation_finished_observer.h"

#include "base/bind.h"
#include "base/run_loop.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class TestNavigationFinishedObserver::TestWebContentsObserver
    : public WebContentsObserver {
 public:
  TestWebContentsObserver(WebContents* web_contents,
                          TestNavigationFinishedObserver* parent)
      : WebContentsObserver(web_contents), parent_(parent) {}

  void DidFinishNavigation(NavigationHandle* navigation_handle) override {
    parent_->OnDidFinishNavigation(navigation_handle);
  }

  void WebContentsDestroyed() override {
    parent_->OnWebContentsDestroyed(this);
  }

  TestNavigationFinishedObserver* parent_;

  DISALLOW_COPY_AND_ASSIGN(TestWebContentsObserver);
};

TestNavigationFinishedObserver::TestNavigationFinishedObserver(
    const GURL& target_url)
    : target_url_(target_url),
      navigation_finished_(false),
      navigation_succeeded_(false),
      web_contents_created_callback_(
          // We remove the callback in the destructor so it will never be
          // called after |this| is destroyed.
          base::BindRepeating(
              &TestNavigationFinishedObserver::OnWebContentsCreated,
              base::Unretained(this))) {
  for (auto* web_contents : WebContentsImpl::GetAllWebContents())
    RegisterObserver(web_contents);

  WebContentsImpl::FriendWrapper::AddCreatedCallbackForTesting(
      web_contents_created_callback_);
}

TestNavigationFinishedObserver::~TestNavigationFinishedObserver() {
  WebContentsImpl::FriendWrapper::RemoveCreatedCallbackForTesting(
      web_contents_created_callback_);
}

void TestNavigationFinishedObserver::Wait() {
  if (navigation_finished_)
    return;

  base::RunLoop run_loop;
  quit_closure_ = run_loop.QuitClosure();
  run_loop.Run();
}

void TestNavigationFinishedObserver::OnDidFinishNavigation(
    NavigationHandle* navigation_handle) {
  if (navigation_handle->GetURL() != target_url_ ||
      !navigation_handle->HasCommitted())
    return;

  navigation_finished_ = true;
  navigation_succeeded_ = !navigation_handle->IsErrorPage();

  // If quit closure is invalid then a navigation finished before Wait() was
  // called.
  if (!quit_closure_)
    return;

  std::move(quit_closure_).Run();
}

void TestNavigationFinishedObserver::OnWebContentsCreated(
    WebContents* web_contents) {
  RegisterObserver(web_contents);
}

void TestNavigationFinishedObserver::OnWebContentsDestroyed(
    TestWebContentsObserver* observer) {
  web_contents_observers_.erase(std::find_if(
      web_contents_observers_.begin(), web_contents_observers_.end(),
      [observer](const std::unique_ptr<TestWebContentsObserver>& ptr) {
        return ptr.get() == observer;
      }));
}

void TestNavigationFinishedObserver::RegisterObserver(
    WebContents* web_contents) {
  web_contents_observers_.insert(
      std::make_unique<TestWebContentsObserver>(web_contents, this));
}

}  // namespace content
