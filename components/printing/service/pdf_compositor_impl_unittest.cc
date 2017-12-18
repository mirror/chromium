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
        run_loop_(std::make_unique<base::RunLoop>()),
        is_ready_(false) {}

  void OnIsReadyToCompositeCallback(bool is_ready) {
    is_ready_ = is_ready;
    run_loop_->Quit();
  }

  bool ResultFromCallback() {
    run_loop_->Run();
    run_loop_ = std::make_unique<base::RunLoop>();
    return is_ready_;
  }

 protected:
  base::test::ScopedTaskEnvironment task_environment_;
  std::unique_ptr<base::RunLoop> run_loop_;
  bool is_ready_;
};

// Class used for testing various timeouts.
class PdfCompositorImplWithTimeout : public PdfCompositorImpl {
 public:
  // |secs| is the timeout value used for waiting on subframes.
  explicit PdfCompositorImplWithTimeout(base::TimeDelta secs)
      : PdfCompositorImpl("unittest", nullptr), timeout_secs_(secs) {}
  ~PdfCompositorImplWithTimeout() override {}

  void SetTimeout(base::TimeDelta secs) { timeout_secs_ = secs; }

 private:
  base::TimeDelta Timeout() const override { return timeout_secs_; }

  base::TimeDelta timeout_secs_;
};

TEST_F(PdfCompositorImplTest, IsReadyToComposite) {
  PdfCompositorImplWithTimeout impl(base::TimeDelta::FromSeconds(10000));
  // Frame 1 contains content 3.
  // Content 3 corresponds to frame 2.
  // Both frame 1, 2 and 3 are all painted.
  impl.AddSubframeMap(2, 3u);
  impl.AddSubframeContent(2, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>());
  impl.AddSubframeContent(1, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>{3u});
  impl.AddSubframeContent(3, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>());

  // Frame 1 with content 3 should be ready.
  impl.IsReadyToComposite(
      1, 0, std::vector<uint32_t>{3u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());

  // Frame 4 with content 2 should not be ready since frame 4 is not painted
  // yet.
  impl.IsReadyToComposite(
      4, 0, std::vector<uint32_t>{2u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_FALSE(ResultFromCallback());

  // Add content of frame 4. Frame 4 with content 2 is still not ready since
  // content 2 is not known as painted.
  impl.AddSubframeContent(4, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>{2u});
  impl.IsReadyToComposite(
      4, 0, std::vector<uint32_t>{2u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_FALSE(ResultFromCallback());

  // Content 2 corresponds to frame 3. Now frame 4 is ready since content 2 is
  // painted also.
  impl.AddSubframeMap(3, 2u);
  impl.IsReadyToComposite(
      4, 0, std::vector<uint32_t>{2u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());
}

TEST_F(PdfCompositorImplTest, ImmediateTimeout) {
  PdfCompositorImplWithTimeout impl(base::TimeDelta::FromSeconds(0));
  // We have an immediate timeout, so it is always ready to composite.
  impl.IsReadyToComposite(
      1, 0, std::vector<uint32_t>{3u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_FALSE(ResultFromCallback());
  impl.IsReadyToComposite(
      1, 0, std::vector<uint32_t>{3u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());
  impl.IsReadyToComposite(
      4, 0, std::vector<uint32_t>{2u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_FALSE(ResultFromCallback());
  impl.IsReadyToComposite(
      4, 0, std::vector<uint32_t>{2u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());
}

TEST_F(PdfCompositorImplTest, ChangeOfTimeout) {
  PdfCompositorImplWithTimeout impl(base::TimeDelta::FromSeconds(0));
  impl.IsReadyToComposite(
      1, 0, std::vector<uint32_t>{3u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_FALSE(ResultFromCallback());
  // We have an immediate timeout, so it should be ready to composite.
  impl.IsReadyToComposite(
      1, 0, std::vector<uint32_t>{3u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());

  // Add timeout value so it won't be timed out.
  impl.SetTimeout(base::TimeDelta::FromSeconds(10000));

  // Now it won't be ready for composition.
  impl.IsReadyToComposite(
      1, 0, std::vector<uint32_t>{3u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_FALSE(ResultFromCallback());
  // We have an immediate timeout, so it should be ready to composite.
  impl.IsReadyToComposite(
      1, 0, std::vector<uint32_t>{3u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_FALSE(ResultFromCallback());
}

TEST_F(PdfCompositorImplTest, MultiLayerDependency) {
  PdfCompositorImplWithTimeout impl(base::TimeDelta::FromSeconds(10000));
  // Frame 1 corresponds to content 1.
  // Frame 3 corresponds to content 3, contains content 1.
  impl.AddSubframeMap(1, 1u);
  impl.AddSubframeMap(3, 3u);
  impl.AddSubframeContent(3, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>{1u});
  impl.AddSubframeContent(1, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>());

  // Both frame 1 and 3 are painted, frame 5 should be ready.
  impl.IsReadyToComposite(
      5, 0, std::vector<uint32_t>{3u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());

  // frame 6 should be ready since frame 1 is available.
  impl.IsReadyToComposite(
      6, 0, std::vector<uint32_t>{1u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());
}

TEST_F(PdfCompositorImplTest, DependencyLoop) {
  PdfCompositorImplWithTimeout impl(base::TimeDelta::FromSeconds(10000));
  // Frame 1 corresponds to content 1, contains content 3.
  // Frame 3 corresponds to content 3, contains content 1.
  impl.AddSubframeMap(1, 1u);
  impl.AddSubframeMap(3, 3u);
  impl.AddSubframeContent(1, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>{3u});
  impl.AddSubframeContent(3, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>{1u});

  // Both frame 1 and 3 are painted, frame 5 should be ready.
  impl.IsReadyToComposite(
      5, 0, std::vector<uint32_t>{1u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());

  impl.AddSubframeMap(6, 6u);
  impl.AddSubframeMap(7, 7u);
  impl.AddSubframeContent(6, mojo::SharedBufferHandle::Create(10),
                          std::vector<uint32_t>{7u});
  // frame 6 should be ready since frame 1 is available.
  impl.IsReadyToComposite(
      7, 0, std::vector<uint32_t>{6u},
      base::BindOnce(&PdfCompositorImplTest::OnIsReadyToCompositeCallback,
                     base::Unretained(this)));
  ASSERT_TRUE(ResultFromCallback());
}

}  // namespace printing
