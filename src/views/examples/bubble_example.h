// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_EXAMPLES_BUBBLE_EXAMPLE_H_
#define VIEWS_EXAMPLES_BUBBLE_EXAMPLE_H_
#pragma once

#include "views/examples/example_base.h"
#include "views/controls/button/button.h"

namespace examples {

struct BubbleConfig;

// A Bubble example.
class BubbleExample : public ExampleBase,
                      public views::ButtonListener {
 public:
  explicit BubbleExample(ExamplesMain* main);
  virtual ~BubbleExample();

  // Overridden from ExampleBase.
  virtual void CreateExampleView(views::View* container) OVERRIDE;

 private:
  virtual void ButtonPressed(views::Button* sender,
                             const views::Event& event) OVERRIDE;

  views::Button* round_;
  views::Button* arrow_;
  views::Button* fade_in_;
  views::Button* fade_out_;

  DISALLOW_COPY_AND_ASSIGN(BubbleExample);
};

}  // namespace examples

#endif  // VIEWS_EXAMPLES_BUBBLE_EXAMPLE_H_
