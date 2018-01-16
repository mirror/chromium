// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_ANNOTATED_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_ANNOTATED_VIEW_H_

#include <memory>
#include <vector>

#include "ui/views/view.h"

// An annotated view is a composite view containing a contents view and an
// annotation view, which might be nullptr. An annotated view has a set of
// sibling views. When an annotated view's height changes, it changes the height
// of all its sibling views by the same amount, by growing or shrinking the
// annotated view if necessary. Annotated views with no supplied annotation have
// an empty  annotation view created for them.
class AnnotatedView : public views::View {
 public:
  AnnotatedView(std::unique_ptr<views::View> contents,
                std::unique_ptr<views::View> annotation);
  ~AnnotatedView() override;

  // Sets the siblings for this annotated view. It is legal for |this| to be in
  // |siblings| - it will be ignored during resizing.
  void SetSiblings(std::vector<AnnotatedView*> siblings);

 private:
  int GetAnnotationPreferredHeight();
  void UpdateHeight(int height);

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override;

  std::vector<AnnotatedView*> siblings_;
  views::View* supplied_annotation_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_ANNOTATED_VIEW_H_
