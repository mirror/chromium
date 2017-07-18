// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/webclipboard_impl.h"

#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"

namespace content {

namespace {

class WebClipboardImplTest : public ContentBrowserTest {
 public:
  WebClipboardImplTest() = default;
  ~WebClipboardImplTest() override = default;
};

IN_PROC_BROWSER_TEST_F(WebClipboardImplTest, PasteRTF) {
  BrowserTestClipboardScope clipboard;

  const std::string rtf_content = "{\\rtf1\\ansi Hello, {\\b world.}}";
  clipboard.SetRtf(rtf_content);

  // paste_listener.html takes RTF from the clipboard and sets the title.
  NavigateToURL(shell(), GetTestUrl(".", "paste_listener.html"));

  const base::string16 expected_title = base::UTF8ToUTF16(rtf_content);
  content::TitleWatcher title_watcher(shell()->web_contents(), expected_title);
  shell()->web_contents()->Paste();
  EXPECT_EQ(expected_title, title_watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(WebClipboardImplTest, ImageCopy) {
  BrowserTestClipboardScope clipboard;
  clipboard.SetText("");

  base::string16 expected_types;
#if !defined(OS_MACOSX)
  // See comments in WebClipboardImpl::WriteImage for why the expected types
  // are platform-specific.
  expected_types = base::ASCIIToUTF16("file;image/png string;text/html");
#else
  expected_types = base::ASCIIToUTF16("file;image/png");
#endif

  NavigateToURL(shell(), GetTestUrl(".", "image_copy_types.html"));
  NavigateIframeToURL(shell()->web_contents(), "copyme",
                      GetTestUrl(".", "media/blackwhite.png"));

  // Run script to copy image contents and wait for completion.
  {
    shell()
        ->web_contents()
        ->GetMainFrame()
        ->ExecuteJavaScriptWithUserGestureForTests(
            base::ASCIIToUTF16("var ok = document.getElementById('copyme')"
                               ".contentWindow.document.execCommand('copy');"
                               "document.title = ok ? 'copied' : 'failed';"));
    content::TitleWatcher title_watcher(shell()->web_contents(),
                                        base::ASCIIToUTF16("copied"));
    title_watcher.AlsoWaitForTitle(base::ASCIIToUTF16("failed"));
    EXPECT_EQ(base::ASCIIToUTF16("copied"), title_watcher.WaitAndGetTitle());
  }

  // Paste and check the types.
  {
    shell()->web_contents()->GetMainFrame()->ExecuteJavaScriptForTests(
        base::ASCIIToUTF16("setTimeout(()=>document.title='timeout', 200)"));
    shell()->web_contents()->Paste();
    content::TitleWatcher title_watcher(shell()->web_contents(),
                                        expected_types);
    title_watcher.AlsoWaitForTitle(base::ASCIIToUTF16("-empty-"));
    title_watcher.AlsoWaitForTitle(base::ASCIIToUTF16("timeout"));
    EXPECT_EQ(expected_types, title_watcher.WaitAndGetTitle());
  }
}
}

} // namespace content
