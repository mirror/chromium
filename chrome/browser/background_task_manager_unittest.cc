// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if ENABLE_BACKGROUND_TASK == 1

#include "chrome/browser/background_task_manager.h"
#include "chrome/browser/navigation_entry.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/web_contents.h"
#include "chrome/test/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
// TODO(jianli): remove DISABLED_ as a prefix.
class DISABLED_BackgroundTaskManagerTest : public testing::Test {
 public:
  DISABLED_BackgroundTaskManagerTest() : source_(NULL) {}

  // testing::Test methods:

  virtual void SetUp() {
    profile_.reset(new TestingProfile());
    source_ = new WebContents(profile_.get(), NULL, NULL,
                              MSG_ROUTING_NONE, NULL);
    source_->SetupController(profile_.get());

    NavigationEntry entry(TAB_CONTENTS_WEB);
    const GURL url("http://www.google.com");
    entry.set_url(url);
    // TODO(jianli): fix the following.
    // See http://codereview.chromium.org/6311
    // It may have been replaced by AddTransientEntry.
    // source_->controller()->AddDummyEntryForInterstitial(entry);
  }

  virtual void TearDown() {
    source_->CloseContents();
  }

 protected:
  scoped_ptr<TestingProfile> profile_;
  WebContents* source_;

 private:
  MessageLoopForUI message_loop_;

  DISALLOW_COPY_AND_ASSIGN(DISABLED_BackgroundTaskManagerTest);
};
}

TEST_F(DISABLED_BackgroundTaskManagerTest, RegisterAndUnregister) {
  scoped_ptr<BackgroundTaskManager> background_task_manager(
      new BackgroundTaskManager(NULL, profile_.get()));

  // Cross origin is not allowed.
  EXPECT_FALSE(background_task_manager->RegisterTask(
      source_, L"Task1", L"http://foo.com"));

  // Registers a task.
  EXPECT_TRUE(background_task_manager->RegisterTask(
      source_, L"Task1", L"http://www.google.com/test_bgtask"));

  // Tries to register a task with the same ID.
  EXPECT_FALSE(background_task_manager->RegisterTask(
      source_, L"Task1", L"http://www.google.com/test_bgtask_again"));

  // Tries to unregisters a non-existent task.
  EXPECT_FALSE(background_task_manager->UnregisterTask(
      source_, L"Task-none"));

  // Unregisters a task.
  EXPECT_TRUE(background_task_manager->UnregisterTask(
      source_, L"Task1"));

  // Registers a task with the same ID again.
  EXPECT_TRUE(background_task_manager->RegisterTask(
      source_, L"Task1", L"http://www.google.com/test_bgtask_again"));

  // Tests if the task is persisted and restored.
  background_task_manager.reset(
      new BackgroundTaskManager(NULL, profile_.get()));
  EXPECT_FALSE(background_task_manager->RegisterTask(
      source_, L"Task1", L"http://www.google.com/test_bgtask_again"));
}

#endif  // ENABLE_BACKGROUND_TASK == 1
