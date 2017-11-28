// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/callback.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "components/printing/service/pdf_compositor_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace printing {

class PdfCompositorImplTest : public testing::Test {
 public:
  PdfCompositorImplTest()
      : task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        run_loop_(new base::RunLoop()),
        status_(false) {}

  void OnIsReadyToCompositeCallback(bool status) {
    status_ = status;
    run_loop_->Quit();
  }

  bool ResultFromCallback() {
    run_loop_->Run();
    run_loop_.reset(new base::RunLoop());
    return status_;
  }

 protected:
  base::test::ScopedTaskEnvironment task_environment_;
  std::unique_ptr<base::RunLoop> run_loop_;
  bool status_;
};

// Class used for testing various timeouts.
class PdfCompositorImplWithTimeout : public PdfCompositorImpl {
 public:
  // When |secs| is not given, set it to a large number so the tests
  // won't trigger timeout.
  explicit PdfCompositorImplWithTimeout(int secs = 10000)
      : PdfCompositorImpl("unittest", nullptr), timeout_secs_(secs) {}
  ~PdfCompositorImplWithTimeout() override{};

  void SetTimeout(int secs) { timeout_secs_ = secs; }

 private:
  base::TimeDelta Timeout() const override {
    return base::TimeDelta::FromSeconds(timeout_secs_);
  }

  int timeout_secs_;
};

TEST_F(PdfCompositorImplTest, IsReadyToComposite) {
  PdfCompositorImplWithTimeout impl;
  // Frame 1 contains content 3.
  // Content 3 corresponds to frame 2.
  // Both frame 1, 2 and 3 are all painted.
  impl.AddSubframeMap(2u, 3u);
  impl.AddSubframeContent(2u, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>());
  impl.AddSubframeContent(1u, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>(3u));
  impl.AddSubframeContent(3u, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>());

  // Frame 1 with content 3 should be ready.
  impl.IsReadyToComposite(
      1, 0, std::vector<uint32_t>({3u}),
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());

  // Frame 4 with content 2 should not be ready since frame 4 is not painted
  // yet.
  impl.IsReadyToComposite(
      4, 0, std::vector<uint32_t>({2u}),
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_FALSE(ResultFromCallback());

  // Add content of frame 4. Frame 4 with content 2 is still not ready since
  // content 2 is not known as painted.
  impl.AddSubframeContent(4u, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>(2u));
  impl.IsReadyToComposite(
      4, 0, std::vector<uint32_t>({2u}),
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_FALSE(ResultFromCallback());

  // Content 2 corresponds to frame 3. Now frame 4 is ready since content 2 is
  // painted also.
  impl.AddSubframeMap(3u, 2u);
  impl.IsReadyToComposite(
      4, 0, std::vector<uint32_t>({2u}),
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());
}

TEST_F(PdfCompositorImplTest, ImmediateTimeout) {
  PdfCompositorImplWithTimeout impl(0);
  // We have an immediate timeout, so it is always ready to composite.
  impl.IsReadyToComposite(
      1, 0, std::vector<uint32_t>({3u}),
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_FALSE(ResultFromCallback());
  impl.IsReadyToComposite(
      1, 0, std::vector<uint32_t>({3u}),
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());
  impl.IsReadyToComposite(
      4, 0, std::vector<uint32_t>({2u}),
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_FALSE(ResultFromCallback());
  impl.IsReadyToComposite(
      4, 0, std::vector<uint32_t>({2u}),
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());
}

}  // namespace printing
