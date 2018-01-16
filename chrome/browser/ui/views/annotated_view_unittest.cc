// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/annotated_view.h"

#include "testing/gtest/include/gtest/gtest.h"

class AnnotatedViewTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto contents_a = std::make_unique<views::View>();
    auto contents_b = std::make_unique<views::View>();
    auto contents_c = std::make_unique<views::View>();

    contents_a_ = contents_a.get();
    contents_b_ = contents_b.get();
    contents_c_ = contents_c.get();

    auto annotation_a = std::make_unique<views::View>();
    auto annotation_b = std::make_unique<views::View>();

    annotation_a_ = annotation_a.get();
    annotation_b_ = annotation_b.get();

    annotated_a_ = std::make_unique<AnnotatedView>(std::move(contents_a),
                                                   std::move(annotation_a));
    annotated_b_ = std::make_unique<AnnotatedView>(std::move(contents_b),
                                                   std::move(annotation_b));
    annotated_c_ =
        std::make_unique<AnnotatedView>(std::move(contents_c), nullptr);

    std::vector<AnnotatedView*> siblings = {
        annotated_a_.get(), annotated_b_.get(), annotated_c_.get(),
    };

    annotated_a_->SetSiblings(siblings);
    annotated_b_->SetSiblings(siblings);
    annotated_c_->SetSiblings(siblings);
  }

  views::View* contents_a() { return contents_a_; }
  views::View* contents_b() { return contents_b_; }
  views::View* contents_c() { return contents_c_; }

  views::View* annotation_a() { return annotation_a_; }
  views::View* annotation_b() { return annotation_b_; }

  views::View* annotated_a() { return annotated_a_.get(); }
  views::View* annotated_b() { return annotated_b_.get(); }
  views::View* annotated_c() { return annotated_c_.get(); }

 private:
  views::View* contents_a_;
  views::View* contents_b_;
  views::View* contents_c_;

  views::View* annotation_a_;
  views::View* annotation_b_;

  std::unique_ptr<AnnotatedView> annotated_a_;
  std::unique_ptr<AnnotatedView> annotated_b_;
  std::unique_ptr<AnnotatedView> annotated_c_;
};

TEST_F(AnnotatedViewTest, HeightUpdates) {
  EXPECT_NE(annotated_a(), annotated_b());
}
