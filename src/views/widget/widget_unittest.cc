// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/widget/native_widget_views.h"

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "views/test/test_views_delegate.h"
#include "views/views_delegate.h"

#if defined(OS_WIN)
#include "views/widget/native_widget_win.h"
#elif defined(TOOLKIT_USES_GTK)
#include "views/widget/native_widget_gtk.h"
#endif

namespace views {
namespace {

class WidgetTestViewsDelegate : public TestViewsDelegate {
 public:
  WidgetTestViewsDelegate() : default_parent_view_(NULL) {
  }
  virtual ~WidgetTestViewsDelegate() {}

  void set_default_parent_view(View* default_parent_view) {
    default_parent_view_ = default_parent_view;
  }

  // Overridden from TestViewsDelegate:
  virtual View* GetDefaultParentView() OVERRIDE {
    return default_parent_view_;
  }

 private:
  View* default_parent_view_;

  DISALLOW_COPY_AND_ASSIGN(WidgetTestViewsDelegate);
};

class WidgetTest : public testing::Test {
 public:
  WidgetTest() {
#if defined(OS_WIN)
    OleInitialize(NULL);
#endif
  }
  virtual ~WidgetTest() {
#if defined(OS_WIN)
    OleUninitialize();
#endif
  }

  virtual void TearDown() {
    // Flush the message loop because we have pending release tasks
    // and these tasks if un-executed would upset Valgrind.
    RunPendingMessages();
  }

  void RunPendingMessages() {
    message_loop_.RunAllPending();
  }

 protected:
  WidgetTestViewsDelegate views_delegate;

 private:
  MessageLoopForUI message_loop_;

  DISALLOW_COPY_AND_ASSIGN(WidgetTest);
};

NativeWidget* CreatePlatformNativeWidget(
    internal::NativeWidgetDelegate* delegate) {
#if defined(OS_WIN)
  return new NativeWidgetWin(delegate);
#elif defined(TOOLKIT_USES_GTK)
  return new NativeWidgetGtk(delegate);
#endif
}

Widget* CreateTopLevelPlatformWidget() {
  Widget* toplevel = new Widget;
  Widget::InitParams toplevel_params(Widget::InitParams::TYPE_WINDOW);
  toplevel_params.native_widget = CreatePlatformNativeWidget(toplevel);
  toplevel->Init(toplevel_params);
  return toplevel;
}

Widget* CreateChildPlatformWidget(gfx::NativeView parent_native_view) {
  Widget* child = new Widget;
  Widget::InitParams child_params(Widget::InitParams::TYPE_CONTROL);
  child_params.native_widget = CreatePlatformNativeWidget(child);
  child_params.parent = parent_native_view;
  child->Init(child_params);
  return child;
}

Widget* CreateChildNativeWidgetViews() {
  Widget* child = new Widget;
  Widget::InitParams child_params(Widget::InitParams::TYPE_CONTROL);
  child_params.native_widget = new NativeWidgetViews(child);
  child->Init(child_params);
  return child;
}

TEST_F(WidgetTest, GetTopLevelWidget_Native) {
  // Create a hierarchy of native widgets.
  Widget* toplevel = CreateTopLevelPlatformWidget();
#if defined(OS_WIN)
  gfx::NativeView parent = toplevel->GetNativeView();
#elif defined(TOOLKIT_USES_GTK)
  NativeWidgetGtk* native_widget =
      static_cast<NativeWidgetGtk*>(toplevel->native_widget());
  gfx::NativeView parent = native_widget->window_contents();
#endif
  Widget* child = CreateChildPlatformWidget(parent);

  EXPECT_EQ(toplevel, toplevel->GetTopLevelWidget());
  EXPECT_EQ(toplevel, child->GetTopLevelWidget());

  toplevel->CloseNow();
  // |child| should be automatically destroyed with |toplevel|.
}

TEST_F(WidgetTest, GetTopLevelWidget_Synthetic) {
  // Create a hierarchy consisting of a top level platform native widget and a
  // child NativeWidgetViews.
  Widget* toplevel = CreateTopLevelPlatformWidget();
  views_delegate.set_default_parent_view(toplevel->GetRootView());
  Widget* child = CreateChildNativeWidgetViews();

  EXPECT_EQ(toplevel, toplevel->GetTopLevelWidget());
  EXPECT_EQ(child, child->GetTopLevelWidget());

  toplevel->CloseNow();
  // |child| should be automatically destroyed with |toplevel|.
}

// TODO(beng): write test cases for child NativeWidgetViews parented to
//             arbitrary views that aren't the default parent view.

////////////////////////////////////////////////////////////////////////////////
// Widget ownership tests.
//
// Tests various permutations of Widget ownership specified in the
// InitParams::Ownership param.

// A WidgetTest that supplies a toplevel widget for NativeWidgetViews to parent
// to.
class WidgetOwnershipTest : public WidgetTest {
 public:
  WidgetOwnershipTest() {}
  virtual ~WidgetOwnershipTest() {}

  virtual void SetUp() {
    desktop_widget_ = CreateTopLevelPlatformWidget();
    views_delegate.set_default_parent_view(desktop_widget_->GetRootView());
  }

  virtual void TearDown() {
    desktop_widget_->CloseNow();
    WidgetTest::TearDown();
  }

 private:
  Widget* desktop_widget_;

  DISALLOW_COPY_AND_ASSIGN(WidgetOwnershipTest);
};

// A bag of state to monitor destructions.
struct OwnershipTestState {
  OwnershipTestState() : widget_deleted(false), native_widget_deleted(false) {}

  bool widget_deleted;
  bool native_widget_deleted;
};

// A platform NativeWidget subclass that updates a bag of state when it is
// destroyed.
class OwnershipTestNativeWidget :
#if defined(OS_WIN)
    public NativeWidgetWin {
#elif defined(TOOLKIT_USES_GTK)
    public NativeWidgetGtk {
#endif
public:
  OwnershipTestNativeWidget(internal::NativeWidgetDelegate* delegate,
                            OwnershipTestState* state)
#if defined(OS_WIN)
    : NativeWidgetWin(delegate),
#elif defined(TOOLKIT_USES_GTK)
    : NativeWidgetGtk(delegate),
#endif
      state_(state) {
  }
  virtual ~OwnershipTestNativeWidget() {
    state_->native_widget_deleted = true;
  }

 private:
  OwnershipTestState* state_;

  DISALLOW_COPY_AND_ASSIGN(OwnershipTestNativeWidget);
};

// A views NativeWidget subclass that updates a bag of state when it is
// destroyed.
class OwnershipTestNativeWidgetViews : public NativeWidgetViews {
 public:
  OwnershipTestNativeWidgetViews(internal::NativeWidgetDelegate* delegate,
                                 OwnershipTestState* state)
      : NativeWidgetViews(delegate),
        state_(state) {
  }
  virtual ~OwnershipTestNativeWidgetViews() {
    state_->native_widget_deleted = true;
  }

 private:
  OwnershipTestState* state_;

  DISALLOW_COPY_AND_ASSIGN(OwnershipTestNativeWidgetViews);
};

// A Widget subclass that updates a bag of state when it is destroyed.
class OwnershipTestWidget : public Widget {
 public:
  OwnershipTestWidget(OwnershipTestState* state) : state_(state) {}
  virtual ~OwnershipTestWidget() {
    state_->widget_deleted = true;
  }

 private:
  OwnershipTestState* state_;

  DISALLOW_COPY_AND_ASSIGN(OwnershipTestWidget);
};

// Widget owns its NativeWidget, part 1: NativeWidget is a platform-native
// widget.
TEST_F(WidgetOwnershipTest, Ownership_WidgetOwnsPlatformNativeWidget) {
  OwnershipTestState state;

  scoped_ptr<Widget> widget(new OwnershipTestWidget(&state));
  Widget::InitParams params(Widget::InitParams::TYPE_POPUP);
  params.native_widget = new OwnershipTestNativeWidget(widget.get(), &state);
  params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  widget->Init(params);

  // Now delete the Widget, which should delete the NativeWidget.
  widget.reset();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);

  // TODO(beng): write test for this ownership scenario and the NativeWidget
  //             being deleted out from under the Widget.
}

// Widget owns its NativeWidget, part 2: NativeWidget is a NativeWidgetViews.
TEST_F(WidgetOwnershipTest, Ownership_WidgetOwnsViewsNativeWidget) {
  OwnershipTestState state;

  scoped_ptr<Widget> widget(new OwnershipTestWidget(&state));
  Widget::InitParams params(Widget::InitParams::TYPE_POPUP);
  params.native_widget =
      new OwnershipTestNativeWidgetViews(widget.get(), &state);
  params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  widget->Init(params);

  // Now delete the Widget, which should delete the NativeWidget.
  widget.reset();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);

  // TODO(beng): write test for this ownership scenario and the NativeWidget
  //             being deleted out from under the Widget.
}

// NativeWidget owns its Widget, part 1: NativeWidget is a platform-native
// widget.
TEST_F(WidgetOwnershipTest, Ownership_PlatformNativeWidgetOwnsWidget) {
  OwnershipTestState state;

  Widget* widget = new OwnershipTestWidget(&state);
  Widget::InitParams params(Widget::InitParams::TYPE_POPUP);
  params.native_widget = new OwnershipTestNativeWidget(widget, &state);
  widget->Init(params);

  // Now destroy the native widget.
  widget->CloseNow();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);
}

// NativeWidget owns its Widget, part 2: NativeWidget is a NativeWidgetViews.
TEST_F(WidgetOwnershipTest, Ownership_ViewsNativeWidgetOwnsWidget) {
  OwnershipTestState state;

  Widget* widget = new OwnershipTestWidget(&state);
  Widget::InitParams params(Widget::InitParams::TYPE_POPUP);
  params.native_widget = new OwnershipTestNativeWidgetViews(widget, &state);
  widget->Init(params);

  // Now destroy the native widget.
  widget->CloseNow();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);
}

// NativeWidget owns its Widget, part 3: NativeWidget is a platform-native
// widget, destroyed out from under it by the OS.
TEST_F(WidgetOwnershipTest,
       Ownership_PlatformNativeWidgetOwnsWidget_NativeDestroy) {
  OwnershipTestState state;

  Widget* widget = new OwnershipTestWidget(&state);
  Widget::InitParams params(Widget::InitParams::TYPE_POPUP);
  params.native_widget = new OwnershipTestNativeWidget(widget, &state);
  widget->Init(params);

  // Now simulate a destroy of the platform native widget from the OS:
#if defined(OS_WIN)
  DestroyWindow(widget->GetNativeView());
#elif defined(TOOLKIT_USES_GTK)
  gtk_widget_destroy(widget->GetNativeView());
#endif

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);
}

// NativeWidget owns its Widget, part 4: NativeWidget is a NativeWidgetViews,
// destroyed by the view hierarchy that contains it.
TEST_F(WidgetOwnershipTest,
       Ownership_ViewsNativeWidgetOwnsWidget_NativeDestroy) {
  OwnershipTestState state;

  Widget* widget = new OwnershipTestWidget(&state);
  Widget::InitParams params(Widget::InitParams::TYPE_POPUP);
  params.native_widget = new OwnershipTestNativeWidgetViews(widget, &state);
  widget->Init(params);

  // Now simulate a destroy of the NativeWidgetView from its parent.
  NativeWidgetViews* native_widget =
      static_cast<NativeWidgetViews*>(widget->native_widget());
  delete native_widget->GetView();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);
}

}  // namespace
}  // namespace views
