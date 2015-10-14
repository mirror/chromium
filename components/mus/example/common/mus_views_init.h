// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MUS_EXAMPLE_COMMON_MUS_VIEWS_INIT_H_
#define COMPONENTS_MUS_EXAMPLE_COMMON_MUS_VIEWS_INIT_H_

#include "base/memory/scoped_ptr.h"
#include "components/mus/public/cpp/view_tree_delegate.h"
#include "ui/views/views_delegate.h"

namespace mandoline {
class AuraInit;
}

namespace mojo {
class ApplicationImpl;
}

// Does the necessary setup to use mus, views and the example wm.
class MUSViewsInit : public views::ViewsDelegate, public mus::ViewTreeDelegate {
 public:
  explicit MUSViewsInit(mojo::ApplicationImpl* app);
  ~MUSViewsInit() override;

 private:
  mus::View* CreateWindow();

  // views::ViewsDelegate:
  views::NativeWidget* CreateNativeWidget(
      views::internal::NativeWidgetDelegate* delegate) override;
  void OnBeforeWidgetInit(
      views::Widget::InitParams* params,
      views::internal::NativeWidgetDelegate* delegate) override;

  // mus::ViewTreeDelegate:
  void OnEmbed(mus::View* root) override;
  void OnConnectionLost(mus::ViewTreeConnection* connection) override;
#if defined(OS_WIN)
  HICON GetSmallWindowIcon() const override;
#endif

  mojo::ApplicationImpl* app_;

  scoped_ptr<mandoline::AuraInit> aura_init_;

  DISALLOW_COPY_AND_ASSIGN(MUSViewsInit);
};

#endif  // COMPONENTS_MUS_EXAMPLE_COMMON_MUS_VIEWS_INIT_H_
