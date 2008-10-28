// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK

#include "chrome/browser/background_task/background_task_manager.h"
#include "chrome/browser/navigation_entry.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/web_contents.h"
#include "chrome/test/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
class BackgroundTaskManagerTest : public testing::Test {
 public:
  BackgroundTaskManagerTest() : source_(NULL) {}

  // testing::Test methods:

  virtual void SetUp() {
    profile_.reset(new TestingProfile());
    source_ = new WebContents(profile_.get(), NULL, NULL,
                              MSG_ROUTING_NONE, NULL);
    source_->SetupController(profile_.get());

    NavigationEntry* entry = new NavigationEntry(TAB_CONTENTS_WEB);
    entry->set_url(GURL("http://www.google.com"));
    source_->controller()->AddTransientEntry(entry);
  }

  virtual void TearDown() {
    source_->CloseContents();
  }

 protected:
  scoped_ptr<TestingProfile> profile_;
  WebContents* source_;

 private:
  MessageLoopForUI message_loop_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundTaskManagerTest);
};
}

TEST_F(BackgroundTaskManagerTest, RegisterAndUnregister) {
  scoped_ptr<BackgroundTaskManager> background_task_manager(
      new BackgroundTaskManager(profile_.get()));

  // Cross origin is not allowed.
  EXPECT_FALSE(background_task_manager->RegisterTask(
      source_, L"Task1", GURL("http://foo.com"),
      START_BACKGROUND_TASK_ON_DEMAND));

  // Registers a task.
  EXPECT_TRUE(background_task_manager->RegisterTask(
      source_, L"Task1", GURL("http://www.google.com/test_bgtask"),
      START_BACKGROUND_TASK_ON_DEMAND));

  // Tries to register a task with the same ID.
  EXPECT_FALSE(background_task_manager->RegisterTask(
      source_, L"Task1", GURL("http://www.google.com/test_bgtask_again"),
      START_BACKGROUND_TASK_ON_DEMAND));

  // Tries to unregisters a non-existent task.
  EXPECT_FALSE(background_task_manager->UnregisterTask(
      source_, L"Task-none"));

  // Unregisters a task.
  EXPECT_TRUE(background_task_manager->UnregisterTask(
      source_, L"Task1"));

  // Registers a task with the same ID again.
  EXPECT_TRUE(background_task_manager->RegisterTask(
      source_, L"Task1", GURL("http://www.google.com/test_bgtask_again"),
      START_BACKGROUND_TASK_ON_DEMAND));

  // Tests if the task is persisted and restored.
  background_task_manager.reset(
      new BackgroundTaskManager(profile_.get()));
  background_task_manager->LoadAndStartAllRegisteredTasks();
  EXPECT_FALSE(background_task_manager->RegisterTask(
      source_, L"Task1", GURL("http://www.google.com/test_bgtask_again"),
      START_BACKGROUND_TASK_ON_DEMAND));
}

#endif  // ENABLE_BACKGROUND_TASK
