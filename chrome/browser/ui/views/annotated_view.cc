// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/annotated_view.h"

AnnotatedView::AnnotatedView(std::unique_ptr<views::View> contents,
                             std::unique_ptr<views::View> annotation) {
  AddChildView(contents.release());
  if (annotation) {
    supplied_annotation_ = annotation.get();
    AddChildView(annotation.release());
  } else {
    AddChildView(new views::View());
  }
}

AnnotatedView::~AnnotatedView() {}

void AnnotatedView::SetSiblings(std::vector<AnnotatedView*> siblings) {
  siblings_ = siblings;
}

int AnnotatedView::GetAnnotationPreferredHeight() {
  return 0;
}

void AnnotatedView::UpdateHeight(int height) {}

void AnnotatedView::ChildPreferredSizeChanged(views::View* child) {}
