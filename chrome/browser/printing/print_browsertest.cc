// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/auto_reset.h"
#include "base/run_loop.h"
#include "chrome/browser/printing/print_view_manager_common.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace printing {

namespace {

class PrintPreviewObserver : PrintPreviewUI::TestingDelegate {
 public:
  PrintPreviewObserver() { PrintPreviewUI::SetDelegateForTesting(this); }
  ~PrintPreviewObserver() { PrintPreviewUI::SetDelegateForTesting(nullptr); }

  void WaitUntilPreviewIsReady() {
    if (rendered_page_count_ >= total_page_count_)
      return;

    base::RunLoop run_loop;
    base::AutoReset<base::RunLoop*> auto_reset(&run_loop_, &run_loop);
    run_loop.Run();
  }

 private:
  // PrintPreviewUI::TestingDelegate implementation.
  void DidGetPreviewPageCount(int page_count) override {
    total_page_count_ = page_count;
  }

  // PrintPreviewUI::TestingDelegate implementation.
  void DidRenderPreviewPage(content::WebContents* preview_dialog) override {
    ++rendered_page_count_;
    CHECK(rendered_page_count_ <= total_page_count_);
    if (rendered_page_count_ == total_page_count_ && run_loop_) {
      run_loop_->Quit();
    }
  }

  int total_page_count_ = 1;
  int rendered_page_count_ = 0;
  base::RunLoop* run_loop_ = nullptr;
};

}  // namespace

class PrintBrowserTest : public InProcessBrowserTest {
 public:
  PrintBrowserTest() {}
  ~PrintBrowserTest() override {}

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void PrintAndWaitUntilPreviewIsReady(bool print_only_selection) {
    PrintPreviewObserver print_preview_observer;

    printing::StartPrint(browser()->tab_strip_model()->GetActiveWebContents(),
                         /*print_preview_disabled=*/false,
                         print_only_selection);

    print_preview_observer.WaitUntilPreviewIsReady();
  }
};

class TestPrintFrameContentMsgFilter : public content::BrowserMessageFilter {
 public:
  explicit TestPrintFrameContentMsgFilter(int document_cookie,
                                          int page_number,
                                          base::OnceClosure quit_closure)
      : content::BrowserMessageFilter(PrintMsgStart),
        document_cookie_(document_cookie),
        page_number_(page_number),
        task_runner_(base::SequencedTaskRunnerHandle::Get()),
        quit_closure_(std::move(quit_closure)) {}

  bool OnMessageReceived(const IPC::Message& message) override {
    // Only expect this particular message.
    bool handled = true;
    IPC_BEGIN_MESSAGE_MAP(TestPrintFrameContentMsgFilter, message)
      IPC_MESSAGE_HANDLER(PrintHostMsg_DidPrintFrameContent, CheckMessage)
      IPC_MESSAGE_UNHANDLED(handled = false)
    IPC_END_MESSAGE_MAP()
    EXPECT_TRUE(handled);
    task_runner_->PostTask(FROM_HERE, std::move(quit_closure_));
    return true;
  }

 private:
  ~TestPrintFrameContentMsgFilter() override {}

  void CheckMessage(int document_cookie,
                    int page_number,
                    const PrintHostMsg_DidPrintContent_Params& param) {
    EXPECT_EQ(document_cookie, document_cookie_);
    EXPECT_EQ(page_number, page_number_);
    EXPECT_TRUE(param.metafile_data_handle.IsValid());
    EXPECT_GT(param.data_size, 0u);
  }

  int document_cookie_;
  int page_number_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::OnceClosure quit_closure_;
};

// Printing only a selection containing iframes is partially supported.
// Iframes aren't currently displayed. This test passes whenever the print
// preview is rendered (i.e. no timeout in the test).
// This test shouldn't crash. See https://crbug.com/732780.
IN_PROC_BROWSER_TEST_F(PrintBrowserTest, SelectionContainsIframe) {
  ASSERT_TRUE(embedded_test_server()->Started());
  GURL url(embedded_test_server()->GetURL("/printing/selection_iframe.html"));
  ui_test_utils::NavigateToURL(browser(), url);

  PrintAndWaitUntilPreviewIsReady(/*print_only_selection=*/true);
}

// Printing frame content for the main frame of a generic webpage.
// This test passes when the printed result is sent back and checked in
// TestPrintFrameContentMsgFilter::CheckMessage().
IN_PROC_BROWSER_TEST_F(PrintBrowserTest, PrintFrameContent) {
  ASSERT_TRUE(embedded_test_server()->Started());
  GURL url(embedded_test_server()->GetURL("/printing/test1.html"));
  ui_test_utils::NavigateToURL(browser(), url);

  content::WebContents* original_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(original_contents);

  content::RenderFrameHost* rfh =
      browser()->tab_strip_model()->GetActiveWebContents()->GetMainFrame();
  constexpr int cookie = -345;
  constexpr int page_number = 98;
  base::RunLoop run_loop;
  auto filter = base::MakeRefCounted<TestPrintFrameContentMsgFilter>(
      cookie, page_number, run_loop.QuitWhenIdleClosure());
  rfh->GetProcess()->AddFilter(filter.get());

  PrintMsg_PrintFrame_Params params;
  params.printable_area = gfx::Rect(800, 600);
  params.document_cookie = cookie;
  params.page_number = page_number;
  rfh->Send(new PrintMsg_PrintFrameContent(rfh->GetRoutingID(), params));

  // The printed result will be received and checked in
  // TestPrintFrameContentMsgFilter.
  run_loop.Run();
}

// Printing frame content for an iframe with cross-site content.
// This test passes when the child render frame responds to the print message.
// The response is checked in TestPrintFrameContentMsgFilter::CheckMessage().
IN_PROC_BROWSER_TEST_F(PrintBrowserTest, PrintSubFrameContent) {
  ASSERT_TRUE(embedded_test_server()->Started());
  GURL url(
      embedded_test_server()->GetURL("/printing/content_with_iframe.html"));
  ui_test_utils::NavigateToURL(browser(), url);

  content::WebContents* original_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_EQ(2u, original_contents->GetAllFrames().size());
  content::RenderFrameHost* child_frame = original_contents->GetAllFrames()[1];
  ASSERT_TRUE(child_frame);

  constexpr int cookie = 1234;
  constexpr int page_number = 0;
  base::RunLoop run_loop;
  auto filter = base::MakeRefCounted<TestPrintFrameContentMsgFilter>(
      cookie, page_number, run_loop.QuitWhenIdleClosure());
  child_frame->GetProcess()->AddFilter(filter.get());

  PrintMsg_PrintFrame_Params params;
  params.printable_area = gfx::Rect(800, 600);
  params.document_cookie = cookie;
  params.page_number = page_number;
  child_frame->Send(
      new PrintMsg_PrintFrameContent(child_frame->GetRoutingID(), params));

  // The printed result will be received and checked in
  // TestPrintFrameContentMsgFilter.
  run_loop.Run();
}

}  // namespace printing
