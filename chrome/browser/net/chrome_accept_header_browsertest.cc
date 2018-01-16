// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/bind_test_util.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/http_request.h"

class ChromeAcceptHeaderTest : public InProcessBrowserTest {
 protected:
  ChromeAcceptHeaderTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeAcceptHeaderTest);
};

IN_PROC_BROWSER_TEST_F(ChromeAcceptHeaderTest, AcceptHeader) {
  net::EmbeddedTestServer server(net::EmbeddedTestServer::TYPE_HTTP);
  server.ServeFilesFromSourceDirectory("chrome/test/data");
  std::string plugin_accept_header, favicon_accept_header;
  base::RunLoop favicon_loop;
  server.RegisterRequestMonitor(base::BindLambdaForTesting(
      [&](const net::test_server::HttpRequest& request) {
        if (request.relative_url == "/pdf/test.pdf") {
          auto it = request.headers.find("Accept");
          if (it != request.headers.end())
            plugin_accept_header = it->second;
        } else if (request.relative_url == "/favicon.ico") {
          auto it = request.headers.find("Accept");
          if (it != request.headers.end())
            favicon_accept_header = it->second;
          favicon_loop.QuitClosure().Run();
        }
      }));
  ASSERT_TRUE(server.Start());
  GURL url = server.GetURL("/pdf/pdf_embed.html");
  ui_test_utils::NavigateToURL(browser(), url);

  ASSERT_EQ("*/*", plugin_accept_header);

  favicon_loop.Run();
  ASSERT_EQ("image/webp,image/apng,image/*,*/*;q=0.8", favicon_accept_header);
}
