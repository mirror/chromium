// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/tracing_controller.h"

#include <stdint.h>
#include <utility>

#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/run_loop.h"
#include "base/strings/pattern.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "content/browser/tracing/tracing_controller_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/trace_uploader.h"
#include "content/public/browser/tracing_controller.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/test_content_browser_client.h"

using base::trace_event::RECORD_CONTINUOUSLY;
using base::trace_event::RECORD_UNTIL_FULL;
using base::trace_event::TraceConfig;

namespace content {

namespace {

bool IsTraceEventArgsWhitelisted(
    const char* category_group_name,
    const char* event_name,
    base::trace_event::ArgumentNameFilterPredicate* arg_filter) {
  if (base::MatchPattern(category_group_name, "benchmark") &&
      base::MatchPattern(event_name, "whitelisted")) {
    return true;
  }
  return false;
}

}  // namespace

class TracingControllerTestEndpoint
    : public TracingController::TraceDataEndpoint {
 public:
  TracingControllerTestEndpoint(
      base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                          base::RefCountedString*)> done_callback)
      : done_callback_(done_callback) {}

  void ReceiveTraceChunk(std::unique_ptr<std::string> chunk) override {
    EXPECT_FALSE(chunk->empty());
    trace_ += *chunk;
  }

  void ReceiveTraceFinalContents(
      std::unique_ptr<const base::DictionaryValue> metadata) override {
    scoped_refptr<base::RefCountedString> chunk_ptr =
        base::RefCountedString::TakeString(&trace_);

    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(done_callback_, base::Passed(std::move(metadata)),
                   base::RetainedRef(chunk_ptr)));
  }

 protected:
  ~TracingControllerTestEndpoint() override {}

  std::string trace_;
  base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                      base::RefCountedString*)>
      done_callback_;
};

class TracingTestBrowserClient : public TestContentBrowserClient {
 public:
  TracingDelegate* GetTracingDelegate() override {
    return new TestTracingDelegate();
  };

 private:
  class TestTracingDelegate : public TracingDelegate {
   public:
    std::unique_ptr<TraceUploader> GetTraceUploader(
        net::URLRequestContextGetter* request_context) override {
      return nullptr;
    }
  };
};

class TracingControllerTest : public ContentBrowserTest {
 public:
  TracingControllerTest() {}

  void SetUp() override {
    get_categories_done_callback_count_ = 0;
    enable_recording_done_callback_count_ = 0;
    disable_recording_done_callback_count_ = 0;
    ContentBrowserTest::SetUp();
  }

  void TearDown() override { ContentBrowserTest::TearDown(); }

  void Navigate(Shell* shell) {
    NavigateToURL(shell, GetTestUrl("", "title.html"));
  }

  void GetCategoriesDoneCallbackTest(base::Closure quit_callback,
                                     const std::set<std::string>& categories) {
    get_categories_done_callback_count_++;
    EXPECT_TRUE(categories.size() > 0);
    quit_callback.Run();
  }

  void StartTracingDoneCallbackTest(base::Closure quit_callback) {
    enable_recording_done_callback_count_++;
    quit_callback.Run();
  }

  void StopTracingStringDoneCallbackTest(
      base::Closure quit_callback,
      std::unique_ptr<const base::DictionaryValue> metadata,
      base::RefCountedString* data) {
    disable_recording_done_callback_count_++;
    last_metadata_ = std::move(metadata);
    last_data_ = data->data();
    EXPECT_TRUE(data->size() > 0);
    quit_callback.Run();
  }

  void StopTracingFileDoneCallbackTest(base::Closure quit_callback,
                                            const base::FilePath& file_path) {
    disable_recording_done_callback_count_++;
    {
      base::ThreadRestrictions::ScopedAllowIO allow_io_for_test_verifications;
      EXPECT_TRUE(PathExists(file_path));
      int64_t file_size;
      base::GetFileSize(file_path, &file_size);
      EXPECT_GT(file_size, 0);
    }
    quit_callback.Run();
    last_actual_recording_file_path_ = file_path;
  }

    int get_categories_done_callback_count() const {
    return get_categories_done_callback_count_;
  }

  int enable_recording_done_callback_count() const {
    return enable_recording_done_callback_count_;
  }

  int disable_recording_done_callback_count() const {
    return disable_recording_done_callback_count_;
  }

  base::FilePath last_actual_recording_file_path() const {
    return last_actual_recording_file_path_;
  }

  const base::DictionaryValue* last_metadata() const {
    return last_metadata_.get();
  }

  const std::string& last_data() const {
    return last_data_;
  }

  void TestStartAndStopTracingString() {
    Navigate(shell());

    TracingController* controller = TracingController::GetInstance();

    {
      base::RunLoop run_loop;
      TracingController::StartTracingDoneCallback callback =
          base::Bind(&TracingControllerTest::StartTracingDoneCallbackTest,
                     base::Unretained(this),
                     run_loop.QuitClosure());
      bool result = controller->StartTracing(
          TraceConfig(), callback);
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(enable_recording_done_callback_count(), 1);
    }

    {
      base::RunLoop run_loop;
      base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                          base::RefCountedString*)>
          callback = base::Bind(
              &TracingControllerTest::StopTracingStringDoneCallbackTest,
              base::Unretained(this), run_loop.QuitClosure());
      bool result = controller->StopTracing(
          TracingController::CreateStringEndpoint(callback));
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(disable_recording_done_callback_count(), 1);
    }
  }

  void TestStartAndStopTracingStringWithFilter() {
    TracingTestBrowserClient client;
    ContentBrowserClient* old_client = SetBrowserClientForTesting(&client);
    Navigate(shell());

    base::trace_event::TraceLog::GetInstance()->SetArgumentFilterPredicate(
        base::Bind(&IsTraceEventArgsWhitelisted));

    TracingController* controller = TracingController::GetInstance();

    {
      base::RunLoop run_loop;
      TracingController::StartTracingDoneCallback callback =
          base::Bind(&TracingControllerTest::StartTracingDoneCallbackTest,
                     base::Unretained(this),
                     run_loop.QuitClosure());

      TraceConfig config = TraceConfig();
      config.EnableArgumentFilter();

      bool result = controller->StartTracing(config, callback);
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(enable_recording_done_callback_count(), 1);
    }

    {
      base::RunLoop run_loop;
      base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                          base::RefCountedString*)>
          callback = base::Bind(
              &TracingControllerTest::StopTracingStringDoneCallbackTest,
              base::Unretained(this), run_loop.QuitClosure());

      scoped_refptr<TracingController::TraceDataEndpoint> trace_data_endpoint =
          TracingController::CreateStringEndpoint(callback);

      bool result = controller->StopTracing(trace_data_endpoint);
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(disable_recording_done_callback_count(), 1);
    }
    SetBrowserClientForTesting(old_client);
  }

  void TestStartAndStopTracingCompressed() {
    Navigate(shell());

    TracingController* controller = TracingController::GetInstance();

    {
      base::RunLoop run_loop;
      TracingController::StartTracingDoneCallback callback =
          base::Bind(&TracingControllerTest::StartTracingDoneCallbackTest,
                     base::Unretained(this), run_loop.QuitClosure());
      bool result = controller->StartTracing(TraceConfig(), callback);
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(enable_recording_done_callback_count(), 1);
    }

    {
      base::RunLoop run_loop;
      base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                          base::RefCountedString*)>
          callback = base::Bind(
              &TracingControllerTest::StopTracingStringDoneCallbackTest,
              base::Unretained(this), run_loop.QuitClosure());
      bool result = controller->StopTracing(
          TracingControllerImpl::CreateCompressedStringEndpoint(
              new TracingControllerTestEndpoint(callback)));
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(disable_recording_done_callback_count(), 1);
    }
  }

  void TestStartAndStopTracingFile(
      const base::FilePath& result_file_path) {
    Navigate(shell());

    TracingController* controller = TracingController::GetInstance();

    {
      base::RunLoop run_loop;
      TracingController::StartTracingDoneCallback callback =
          base::Bind(&TracingControllerTest::StartTracingDoneCallbackTest,
                     base::Unretained(this),
                     run_loop.QuitClosure());
      bool result = controller->StartTracing(TraceConfig(), callback);
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(enable_recording_done_callback_count(), 1);
    }

    {
      base::RunLoop run_loop;
      base::Closure callback = base::Bind(
          &TracingControllerTest::StopTracingFileDoneCallbackTest,
          base::Unretained(this),
          run_loop.QuitClosure(),
          result_file_path);
      bool result = controller->StopTracing(
          TracingController::CreateFileEndpoint(result_file_path, callback));
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(disable_recording_done_callback_count(), 1);
    }
  }

 private:
  int get_categories_done_callback_count_;
  int enable_recording_done_callback_count_;
  int disable_recording_done_callback_count_;
  base::FilePath last_actual_recording_file_path_;
  std::unique_ptr<const base::DictionaryValue> last_metadata_;
  std::string last_data_;
};

IN_PROC_BROWSER_TEST_F(TracingControllerTest, GetCategories) {
  Navigate(shell());

  TracingController* controller = TracingController::GetInstance();

  base::RunLoop run_loop;
  TracingController::GetCategoriesDoneCallback callback =
      base::Bind(&TracingControllerTest::GetCategoriesDoneCallbackTest,
                 base::Unretained(this),
                 run_loop.QuitClosure());
  ASSERT_TRUE(controller->GetCategories(callback));
  run_loop.Run();
  EXPECT_EQ(get_categories_done_callback_count(), 1);
}

IN_PROC_BROWSER_TEST_F(TracingControllerTest, EnableAndStopTracing) {
  TestStartAndStopTracingString();
}

IN_PROC_BROWSER_TEST_F(TracingControllerTest,
                       EnableAndStopTracingWithFilePath) {
  base::FilePath file_path;
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io_for_creating_test_file;
    base::CreateTemporaryFile(&file_path);
  }
  TestStartAndStopTracingFile(file_path);
  EXPECT_EQ(file_path.value(), last_actual_recording_file_path().value());
}

IN_PROC_BROWSER_TEST_F(TracingControllerTest,
                       EnableAndStopTracingWithCompression) {
  TestStartAndStopTracingCompressed();
}

IN_PROC_BROWSER_TEST_F(TracingControllerTest,
                       EnableAndStopTracingWithEmptyFileAndNullCallback) {
  Navigate(shell());

  TracingController* controller = TracingController::GetInstance();
  EXPECT_TRUE(controller->StartTracing(
      TraceConfig(),
      TracingController::StartTracingDoneCallback()));
  EXPECT_TRUE(controller->StopTracing(NULL));
  base::RunLoop().RunUntilIdle();
}

}  // namespace content
