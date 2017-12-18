// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/web_interface_broker.h"

#include <memory>

#include "content/common/service_manager/service_manager_connection_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_interface_broker_builder.h"
#include "content/public/browser/web_interface_filter.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {
namespace {

class OriginInterface {
 public:
  static constexpr char Name_[] = "origin interface";
};
constexpr char OriginInterface::Name_[];

class FrameInterface {
 public:
  static constexpr char Name_[] = "frame interface";
};
constexpr char FrameInterface::Name_[];

class BothInterface {
 public:
  static constexpr char Name_[] = "both interface";
};
constexpr char BothInterface::Name_[];

class ServiceInterface {
 public:
  static constexpr char Name_[] = "service interface";
};
constexpr char ServiceInterface::Name_[];

class WebContentsObserverInterface {
 public:
  static constexpr char Name_[] = "WebContentsObserver interface";
};
constexpr char WebContentsObserverInterface::Name_[];

class WebInterfaceBrokerTest : public RenderViewHostTestHarness {
 public:
  enum class InterfaceRoute {
    kNone,
    kFrame,
    kOrigin,
    kBothFrame,
    kBothOrigin,
    kWebContentsObserver,
    kFrameDelegate,
    kWorkerDelegate,
    kBadMessage,
  };

  void SetUp() override {
    service_manager::mojom::ServicePtr service;
    ServiceManagerConnection::SetForProcess(
        std::make_unique<ServiceManagerConnectionImpl>(
            mojo::MakeRequest(&service),
            BrowserThread::GetTaskRunnerForThread(BrowserThread::IO)));
    RenderViewHostTestHarness::SetUp();

    NavigateAndCommit(GURL("http://www.example.com"));
    new TestWebContentsObserver(web_contents(), this);

    auto builder = WebInterfaceBrokerBuilder::Create(
        base::BindRepeating(&WebInterfaceBrokerTest::OnUnknownWorkerInterface,
                            base::Unretained(this)),
        base::BindRepeating(&WebInterfaceBrokerTest::OnUnknownFrameInterface,
                            base::Unretained(this)));

    builder->AddInterface(
        std::make_unique<TestWebInterfaceFilter>(this),
        base::BindRepeating(&WebInterfaceBrokerTest::BindOriginInterface,
                            base::Unretained(this)));
    builder->AddInterface(
        std::make_unique<TestWebInterfaceFilter>(this),
        base::BindRepeating(&WebInterfaceBrokerTest::BindFrameOnlyInterface,
                            base::Unretained(this)));
    builder->AddInterface(
        std::make_unique<TestWebInterfaceFilter>(this),
        base::BindRepeating(&WebInterfaceBrokerTest::BindBothInterfaceForFrame,
                            base::Unretained(this)),
        base::BindRepeating(&WebInterfaceBrokerTest::BindBothInterfaceForWorker,
                            base::Unretained(this)));
    builder->AddWebContentsObserverInterface<WebContentsObserverInterface>(
        std::make_unique<TestWebInterfaceFilter>(this));
    builder->AddInterface<ServiceInterface>(
        std::make_unique<TestWebInterfaceFilter>(this), "fake service");

    broker_ = builder->Build();
  }

  void TearDown() override {
    RenderViewHostTestHarness::TearDown();
    ServiceManagerConnection::DestroyForProcess();
  }

  void SetFilterResult(WebInterfaceFilter::Result result) {
    filter_result_ = result;
  }

  const std::string& bad_message() { return bad_message_; }

  InterfaceRoute BindWorkerInterface(const std::string& interface_name) {
    mojo::MessagePipe pipe;
    broker_->BindInterfaceForWorker(
        WebContextType::kDedicatedWorker, interface_name,
        std::move(pipe.handle0), process(),
        main_rfh()->GetLastCommittedOrigin(),
        base::BindOnce(&WebInterfaceBrokerTest::ReportBadMessage,
                       base::Unretained(this)));
    return route_;
  }

  InterfaceRoute BindFrameInterface(const std::string& interface_name) {
    mojo::MessagePipe pipe;
    broker_->BindInterfaceForFrame(
        interface_name, std::move(pipe.handle0), main_rfh(),
        base::BindOnce(&WebInterfaceBrokerTest::ReportBadMessage,
                       base::Unretained(this)));
    return route_;
  }

 private:
  class TestWebInterfaceFilter : public WebInterfaceFilter {
   public:
    TestWebInterfaceFilter(WebInterfaceBrokerTest* test) : test_(test) {}

    Result FilterInterface(WebContextType context_type,
                           RenderProcessHost* process,
                           RenderFrameHost* frame,
                           const url::Origin& origin) override {
      return test_->FilterInterface(context_type, process, frame, origin);
    }

   private:
    WebInterfaceBrokerTest* const test_;
  };

  class TestWebContentsObserver : public WebContentsObserver {
   public:
    TestWebContentsObserver(WebContents* web_contents,
                            WebInterfaceBrokerTest* test)
        : WebContentsObserver(web_contents), test_(test) {}

    void WebContentsDestroyed() override { delete this; }

    void OnInterfaceRequestFromFrame(
        RenderFrameHost* frame,
        const std::string& interface_name,
        mojo::ScopedMessagePipeHandle* interface_pipe) override {
      test_->OnWebContentsObserverInterfaceRequest(frame, interface_name,
                                                   std::move(*interface_pipe));
    }

   private:
    WebInterfaceBrokerTest* const test_;
  };

  void SetInterfaceRoute(InterfaceRoute route) {
    EXPECT_EQ(InterfaceRoute::kNone, route_);
    route_ = route;
  }

  void OnUnknownFrameInterface(const std::string& interface_name,
                               mojo::ScopedMessagePipeHandle interface_pipe,
                               RenderFrameHost* frame,
                               mojo::ReportBadMessageCallback) {
    EXPECT_EQ(main_rfh(), frame);
    EXPECT_EQ("fake interface", interface_name);
    SetInterfaceRoute(InterfaceRoute::kFrameDelegate);
  }

  void OnUnknownWorkerInterface(WebContextType,
                                const std::string& interface_name,
                                mojo::ScopedMessagePipeHandle interface_pipe,
                                RenderProcessHost* process_host,
                                const url::Origin& origin,
                                mojo::ReportBadMessageCallback) {
    EXPECT_EQ(process(), process_host);
    EXPECT_EQ(main_rfh()->GetLastCommittedOrigin(), origin);
    EXPECT_EQ("fake interface", interface_name);
    SetInterfaceRoute(InterfaceRoute::kWorkerDelegate);
  }

  void ReportBadMessage(const std::string& message) {
    DCHECK(!message.empty());
    bad_message_ = message;
    SetInterfaceRoute(InterfaceRoute::kBadMessage);
  }

  void BindOriginInterface(mojo::InterfaceRequest<OriginInterface>,
                           RenderProcessHost* process_host,
                           const url::Origin& origin) {
    SetInterfaceRoute(InterfaceRoute::kOrigin);
    EXPECT_EQ(process(), process_host);
    EXPECT_EQ(main_rfh()->GetLastCommittedOrigin(), origin);
  }

  void BindFrameOnlyInterface(mojo::InterfaceRequest<FrameInterface>,
                              RenderFrameHost* frame) {
    SetInterfaceRoute(InterfaceRoute::kFrame);
    EXPECT_EQ(main_rfh(), frame);
  }

  void BindBothInterfaceForWorker(mojo::InterfaceRequest<BothInterface>,
                                  RenderProcessHost* process_host,
                                  const url::Origin& origin) {
    SetInterfaceRoute(InterfaceRoute::kBothOrigin);
    EXPECT_EQ(process(), process_host);
    EXPECT_EQ(main_rfh()->GetLastCommittedOrigin(), origin);
  }

  void BindBothInterfaceForFrame(mojo::InterfaceRequest<BothInterface>,
                                 RenderFrameHost* frame) {
    SetInterfaceRoute(InterfaceRoute::kBothFrame);
    EXPECT_EQ(main_rfh(), frame);
  }

  void OnWebContentsObserverInterfaceRequest(
      RenderFrameHost* frame,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle interface_pipe) {
    SetInterfaceRoute(InterfaceRoute::kWebContentsObserver);
    EXPECT_EQ(WebContentsObserverInterface::Name_, interface_name);
    EXPECT_EQ(main_rfh(), frame);
  }

  WebInterfaceFilter::Result FilterInterface(WebContextType context_type,
                                             RenderProcessHost* process_host,
                                             RenderFrameHost* frame,
                                             const url::Origin& origin) {
    if (context_type == WebContextType::kFrame)
      EXPECT_EQ(main_rfh(), frame);
    EXPECT_EQ(process(), process_host);
    EXPECT_EQ(main_rfh()->GetLastCommittedOrigin(), origin);
    return filter_result_;
  }

  std::unique_ptr<WebInterfaceBroker> broker_;

  InterfaceRoute route_ = InterfaceRoute::kNone;
  std::string bad_message_;

  WebInterfaceFilter::Result filter_result_ =
      WebInterfaceFilter::Result::kAllow;
};

TEST_F(WebInterfaceBrokerTest, Worker_FilterAllow) {
  EXPECT_EQ(InterfaceRoute::kOrigin,
            BindWorkerInterface(OriginInterface::Name_));
}

TEST_F(WebInterfaceBrokerTest, Worker_FilterDeny) {
  SetFilterResult(WebInterfaceFilter::Result::kDeny);
  EXPECT_EQ(InterfaceRoute::kNone, BindWorkerInterface(OriginInterface::Name_));
}

TEST_F(WebInterfaceBrokerTest, Worker_FilterBadMessage) {
  SetFilterResult(WebInterfaceFilter::Result::kBadMessage);
  EXPECT_EQ(InterfaceRoute::kBadMessage,
            BindWorkerInterface(OriginInterface::Name_));
  EXPECT_EQ(bad_message(), "Bad request for interface origin interface");
}

TEST_F(WebInterfaceBrokerTest, Worker_UnknownInterface) {
  EXPECT_EQ(InterfaceRoute::kWorkerDelegate,
            BindWorkerInterface("fake interface"));
}

TEST_F(WebInterfaceBrokerTest, Worker_FrameInterface) {
  EXPECT_EQ(InterfaceRoute::kBadMessage,
            BindWorkerInterface(FrameInterface::Name_));
  EXPECT_EQ(bad_message(), "Bad request for interface frame interface");
}

TEST_F(WebInterfaceBrokerTest, Worker_WebContentsObserverInterface) {
  EXPECT_EQ(InterfaceRoute::kBadMessage,
            BindWorkerInterface(WebContentsObserverInterface::Name_));
  EXPECT_EQ(bad_message(),
            "Bad request for interface WebContentsObserver interface");
}

TEST_F(WebInterfaceBrokerTest, Worker_ServiceInterface) {
  EXPECT_EQ(InterfaceRoute::kNone,
            BindWorkerInterface(ServiceInterface::Name_));
}

TEST_F(WebInterfaceBrokerTest, Worker_FrameAndWorkerPicksWorker) {
  EXPECT_EQ(InterfaceRoute::kBothOrigin,
            BindWorkerInterface(BothInterface::Name_));
}

TEST_F(WebInterfaceBrokerTest, Frame_FilterAllow) {
  EXPECT_EQ(InterfaceRoute::kOrigin,
            BindFrameInterface(OriginInterface::Name_));
}

TEST_F(WebInterfaceBrokerTest, Frame_FrameInterface_FilterAllow) {
  EXPECT_EQ(InterfaceRoute::kFrame, BindFrameInterface(FrameInterface::Name_));
}

TEST_F(WebInterfaceBrokerTest, Frame_FilterDeny) {
  SetFilterResult(WebInterfaceFilter::Result::kDeny);
  EXPECT_EQ(InterfaceRoute::kNone, BindFrameInterface(OriginInterface::Name_));
}

TEST_F(WebInterfaceBrokerTest, Frame_FrameInterface_FilterDeny) {
  SetFilterResult(WebInterfaceFilter::Result::kDeny);
  EXPECT_EQ(InterfaceRoute::kNone, BindFrameInterface(FrameInterface::Name_));
}

TEST_F(WebInterfaceBrokerTest, Frame_FilterBadMessage) {
  SetFilterResult(WebInterfaceFilter::Result::kBadMessage);
  EXPECT_EQ(InterfaceRoute::kBadMessage,
            BindFrameInterface(OriginInterface::Name_));
  EXPECT_EQ(bad_message(), "Bad request for interface origin interface");
}

TEST_F(WebInterfaceBrokerTest, Frame_FrameInterface_FilterBadMessage) {
  SetFilterResult(WebInterfaceFilter::Result::kBadMessage);
  EXPECT_EQ(InterfaceRoute::kBadMessage,
            BindFrameInterface(FrameInterface::Name_));
  EXPECT_EQ(bad_message(), "Bad request for interface frame interface");
}

TEST_F(WebInterfaceBrokerTest, Frame_UnknownInterface) {
  EXPECT_EQ(InterfaceRoute::kFrameDelegate,
            BindFrameInterface("fake interface"));
}

TEST_F(WebInterfaceBrokerTest, Frame_WebContentsObserverInterface) {
  EXPECT_EQ(InterfaceRoute::kWebContentsObserver,
            BindFrameInterface(WebContentsObserverInterface::Name_));
}

TEST_F(WebInterfaceBrokerTest, Frame_ServiceInterface) {
  EXPECT_EQ(InterfaceRoute::kNone, BindFrameInterface(ServiceInterface::Name_));
}

TEST_F(WebInterfaceBrokerTest, Frame_FrameAndWorkerPicksFrame) {
  EXPECT_EQ(InterfaceRoute::kBothFrame,
            BindFrameInterface(BothInterface::Name_));
}

}  // namespace
}  // namespace content
