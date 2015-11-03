// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/native_widget_mus.h"

#include "components/mus/public/cpp/window.h"
#include "components/mus/public/interfaces/window_manager.mojom.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/window.h"
#include "ui/native_theme/native_theme_aura.h"
#include "ui/views/mus/window_tree_host_mus.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/core/base_focus_rules.h"
#include "ui/wm/core/capture_controller.h"
#include "ui/wm/core/focus_controller.h"

namespace views {
namespace {

// TODO: figure out what this should be.
class FocusRulesImpl : public wm::BaseFocusRules {
 public:
  FocusRulesImpl() {}
  ~FocusRulesImpl() override {}

  bool SupportsChildActivation(aura::Window* window) const override {
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusRulesImpl);
};

class ContentWindowLayoutManager : public aura::LayoutManager {
 public:
   ContentWindowLayoutManager(aura::Window* outer, aura::Window* inner)
      : outer_(outer), inner_(inner) {}
   ~ContentWindowLayoutManager() override {}

 private:
  // aura::LayoutManager:
  void OnWindowResized() override { inner_->SetBounds(outer_->bounds()); }
  void OnWindowAddedToLayout(aura::Window* child) override {
    OnWindowResized();
  }
  void OnWillRemoveWindowFromLayout(aura::Window* child) override {}
  void OnWindowRemovedFromLayout(aura::Window* child) override {}
  void OnChildWindowVisibilityChanged(aura::Window* child,
                                      bool visible) override {}
  void SetChildBounds(aura::Window* child,
                      const gfx::Rect& requested_bounds) override {
    SetChildBoundsDirect(child, requested_bounds);
  }

  aura::Window* outer_;
  aura::Window* inner_;

  DISALLOW_COPY_AND_ASSIGN(ContentWindowLayoutManager);
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMus, public:

NativeWidgetMus::NativeWidgetMus(internal::NativeWidgetDelegate* delegate,
                                 mojo::Shell* shell,
                                 mus::Window* window,
                                 mus::mojom::SurfaceType surface_type)
    : window_(window),
      shell_(shell),
      native_widget_delegate_(delegate),
      surface_type_(surface_type),
      show_state_before_fullscreen_(mus::mojom::SHOW_STATE_RESTORED),
      content_(new aura::Window(this)) {}
NativeWidgetMus::~NativeWidgetMus() {}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMus, private:

void NativeWidgetMus::UpdateClientAreaInWindowManager() {
  NonClientView* non_client_view =
      native_widget_delegate_->AsWidget()->non_client_view();
  if (!non_client_view || !non_client_view->client_view())
    return;

  window_->SetClientArea(non_client_view->client_view()->bounds());
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMus, internal::NativeWidgetPrivate implementation:

void NativeWidgetMus::InitNativeWidget(const Widget::InitParams& params) {
  window_tree_host_.reset(
      new WindowTreeHostMus(shell_, window_, surface_type_));
  window_tree_host_->InitHost();

  focus_client_.reset(new wm::FocusController(new FocusRulesImpl));

  aura::client::SetFocusClient(window_tree_host_->window(),
                               focus_client_.get());
  aura::client::SetActivationClient(window_tree_host_->window(),
                                    focus_client_.get());
  window_tree_host_->window()->AddPreTargetHandler(focus_client_.get());
  window_tree_host_->window()->SetLayoutManager(
      new ContentWindowLayoutManager(window_tree_host_->window(), content_));
  capture_client_.reset(
      new aura::client::DefaultCaptureClient(window_tree_host_->window()));

  content_->SetType(ui::wm::WINDOW_TYPE_NORMAL);
  content_->Init(ui::LAYER_TEXTURED);
  content_->SetTransparent(true);
  content_->SetFillsBoundsCompletely(false);

  window_tree_host_->window()->AddChild(content_);
  // TODO(beng): much else, see [Desktop]NativeWidgetAura.
}

NonClientFrameView* NativeWidgetMus::CreateNonClientFrameView() {
  NOTIMPLEMENTED();
  return nullptr;
}

bool NativeWidgetMus::ShouldUseNativeFrame() const {
  NOTIMPLEMENTED();
  return false;
}

bool NativeWidgetMus::ShouldWindowContentsBeTransparent() const {
  NOTIMPLEMENTED();
  return true;
}

void NativeWidgetMus::FrameTypeChanged() {
  NOTIMPLEMENTED();
}

Widget* NativeWidgetMus::GetWidget() {
  return native_widget_delegate_->AsWidget();
}

const Widget* NativeWidgetMus::GetWidget() const {
  NOTIMPLEMENTED();
  return native_widget_delegate_->AsWidget();
}

gfx::NativeView NativeWidgetMus::GetNativeView() const {
  return content_;
}

gfx::NativeWindow NativeWidgetMus::GetNativeWindow() const {
  return content_;
}

Widget* NativeWidgetMus::GetTopLevelWidget() {
  return GetWidget();
}

const ui::Compositor* NativeWidgetMus::GetCompositor() const {
  return window_tree_host_->window()->layer()->GetCompositor();
}

const ui::Layer* NativeWidgetMus::GetLayer() const {
  return window_tree_host_->window()->layer();
}

void NativeWidgetMus::ReorderNativeViews() {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::ViewRemoved(View* view) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::SetNativeWindowProperty(const char* name, void* value) {
  // TODO(beng): push properties to mus::Window.
  NOTIMPLEMENTED();
}

void* NativeWidgetMus::GetNativeWindowProperty(const char* name) const {
  // TODO(beng): pull properties to mus::Window.
  NOTIMPLEMENTED();
  return nullptr;
}

TooltipManager* NativeWidgetMus::GetTooltipManager() const {
  NOTIMPLEMENTED();
  return nullptr;
}

void NativeWidgetMus::SetCapture() {
  content_->SetCapture();
}

void NativeWidgetMus::ReleaseCapture() {
  content_->ReleaseCapture();
}

bool NativeWidgetMus::HasCapture() const {
  return window_tree_host_->window()->HasCapture();
}

ui::InputMethod* NativeWidgetMus::GetInputMethod() {
  return window_tree_host_->GetInputMethod();
}

void NativeWidgetMus::CenterWindow(const gfx::Size& size) {
  // TODO(beng): clear user-placed property and set preferred size property.
  window_->SetPreferredSize(size);
}

void NativeWidgetMus::GetWindowPlacement(
      gfx::Rect* bounds,
      ui::WindowShowState* maximized) const {
  NOTIMPLEMENTED();
}

bool NativeWidgetMus::SetWindowTitle(const base::string16& title) {
  // TODO(beng): push title to window manager.
  NOTIMPLEMENTED();
  return false;
}

void NativeWidgetMus::SetWindowIcons(const gfx::ImageSkia& window_icon,
                                     const gfx::ImageSkia& app_icon) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::InitModalType(ui::ModalType modal_type) {
  NOTIMPLEMENTED();
}

gfx::Rect NativeWidgetMus::GetWindowBoundsInScreen() const {
  NOTIMPLEMENTED();
  return gfx::Rect();
}

gfx::Rect NativeWidgetMus::GetClientAreaBoundsInScreen() const {
  NOTIMPLEMENTED();
  return gfx::Rect();
}

gfx::Rect NativeWidgetMus::GetRestoredBounds() const {
  NOTIMPLEMENTED();
  return gfx::Rect();
}

void NativeWidgetMus::SetBounds(const gfx::Rect& bounds) {
  window_->RequestBoundsChange(bounds);
}

void NativeWidgetMus::SetSize(const gfx::Size& size) {
  window_->RequestBoundsChange(gfx::Rect(window_->bounds().size()));
}

void NativeWidgetMus::StackAbove(gfx::NativeView native_view) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::StackAtTop() {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::StackBelow(gfx::NativeView native_view) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::SetShape(SkRegion* shape) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::Close() {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::CloseNow() {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::Show() {
  ShowWithWindowState(ui::SHOW_STATE_NORMAL);
}

void NativeWidgetMus::Hide() {
  window_tree_host_->Hide();
  GetNativeWindow()->Hide();
}

void NativeWidgetMus::ShowMaximizedWithBounds(
    const gfx::Rect& restored_bounds) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::ShowWithWindowState(ui::WindowShowState state) {
  window_tree_host_->Show();
  GetNativeWindow()->Show();
}

bool NativeWidgetMus::IsVisible() const {
  NOTIMPLEMENTED();
  return true;
}

void NativeWidgetMus::Activate() {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::Deactivate() {
  NOTIMPLEMENTED();
}

bool NativeWidgetMus::IsActive() const {
  NOTIMPLEMENTED();
  return true;
}

void NativeWidgetMus::SetAlwaysOnTop(bool always_on_top) {
  NOTIMPLEMENTED();
}

bool NativeWidgetMus::IsAlwaysOnTop() const {
  NOTIMPLEMENTED();
  return false;
}

void NativeWidgetMus::SetVisibleOnAllWorkspaces(bool always_visible) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::Maximize() {
  window_tree_host_->SetShowState(mus::mojom::SHOW_STATE_MAXIMIZED);
}

void NativeWidgetMus::Minimize() {
  window_tree_host_->SetShowState(mus::mojom::SHOW_STATE_MINIMIZED);
}

bool NativeWidgetMus::IsMaximized() const {
  return window_tree_host_->show_state() == mus::mojom::SHOW_STATE_MAXIMIZED;
}

bool NativeWidgetMus::IsMinimized() const {
  return window_tree_host_->show_state() == mus::mojom::SHOW_STATE_MINIMIZED;
}

void NativeWidgetMus::Restore() {
  window_tree_host_->SetShowState(mus::mojom::SHOW_STATE_RESTORED);
}

void NativeWidgetMus::SetFullscreen(bool fullscreen) {
  if (fullscreen) {
    show_state_before_fullscreen_ = window_tree_host_->show_state();
    window_tree_host_->SetShowState(mus::mojom::SHOW_STATE_PRESENTATION);
  } else {
    window_tree_host_->SetShowState(show_state_before_fullscreen_);
  }
}

bool NativeWidgetMus::IsFullscreen() const {
  return window_tree_host_->show_state() == mus::mojom::SHOW_STATE_PRESENTATION;
}

void NativeWidgetMus::SetOpacity(unsigned char opacity) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::SetUseDragFrame(bool use_drag_frame) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::FlashFrame(bool flash_frame) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::RunShellDrag(
    View* view,
    const ui::OSExchangeData& data,
    const gfx::Point& location,
    int operation,
    ui::DragDropTypes::DragEventSource source) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::SchedulePaintInRect(const gfx::Rect& rect) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::SetCursor(gfx::NativeCursor cursor) {
  NOTIMPLEMENTED();
}

bool NativeWidgetMus::IsMouseEventsEnabled() const {
  NOTIMPLEMENTED();
  return true;
}

void NativeWidgetMus::ClearNativeFocus() {
  NOTIMPLEMENTED();
}

gfx::Rect NativeWidgetMus::GetWorkAreaBoundsInScreen() const {
  NOTIMPLEMENTED();
  return gfx::Rect();
}

Widget::MoveLoopResult NativeWidgetMus::RunMoveLoop(
    const gfx::Vector2d& drag_offset,
    Widget::MoveLoopSource source,
    Widget::MoveLoopEscapeBehavior escape_behavior) {
  NOTIMPLEMENTED();
  return Widget::MOVE_LOOP_CANCELED;
}

void NativeWidgetMus::EndMoveLoop() {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::SetVisibilityChangedAnimationsEnabled(bool value) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::SetVisibilityAnimationDuration(
    const base::TimeDelta& duration) {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::SetVisibilityAnimationTransition(
    Widget::VisibilityTransition transition) {
  NOTIMPLEMENTED();
}

ui::NativeTheme* NativeWidgetMus::GetNativeTheme() const {
  return ui::NativeThemeAura::instance();
}

void NativeWidgetMus::OnRootViewLayout() {
  NOTIMPLEMENTED();
}

bool NativeWidgetMus::IsTranslucentWindowOpacitySupported() const {
  NOTIMPLEMENTED();
  return true;
}

void NativeWidgetMus::OnSizeConstraintsChanged() {
  NOTIMPLEMENTED();
}

void NativeWidgetMus::RepostNativeEvent(gfx::NativeEvent native_event) {
  NOTIMPLEMENTED();
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMus, aura::WindowDelegate implementation:

gfx::Size NativeWidgetMus::GetMinimumSize() const {
  return native_widget_delegate_->GetMinimumSize();
}

gfx::Size NativeWidgetMus::GetMaximumSize() const {
  return native_widget_delegate_->GetMaximumSize();
}

void NativeWidgetMus::OnBoundsChanged(const gfx::Rect& old_bounds,
                                      const gfx::Rect& new_bounds) {
  // Assume that if the old bounds was completely empty a move happened. This
  // handles the case of a maximize animation acquiring the layer (acquiring a
  // layer results in clearing the bounds).
  if (old_bounds.origin() != new_bounds.origin() ||
      (old_bounds == gfx::Rect(0, 0, 0, 0) && !new_bounds.IsEmpty())) {
    native_widget_delegate_->OnNativeWidgetMove();
  }
  if (old_bounds.size() != new_bounds.size()) {
    native_widget_delegate_->OnNativeWidgetSizeChanged(new_bounds.size());
    UpdateClientAreaInWindowManager();
  }
}

gfx::NativeCursor NativeWidgetMus::GetCursor(const gfx::Point& point) {
  return gfx::NativeCursor();
}

int NativeWidgetMus::GetNonClientComponent(const gfx::Point& point) const {
  return native_widget_delegate_->GetNonClientComponent(point);
}

bool NativeWidgetMus::ShouldDescendIntoChildForEventHandling(
    aura::Window* child,
    const gfx::Point& location) {
  views::WidgetDelegate* widget_delegate = GetWidget()->widget_delegate();
  return !widget_delegate ||
      widget_delegate->ShouldDescendIntoChildForEventHandling(child, location);
}

bool NativeWidgetMus::CanFocus() {
  return true;
}

void NativeWidgetMus::OnCaptureLost() {
  native_widget_delegate_->OnMouseCaptureLost();
}

void NativeWidgetMus::OnPaint(const ui::PaintContext& context) {
  native_widget_delegate_->OnNativeWidgetPaint(context);
}

void NativeWidgetMus::OnDeviceScaleFactorChanged(float device_scale_factor) {
}

void NativeWidgetMus::OnWindowDestroying(aura::Window* window) {
  // Cleanup happens in OnHostClosed().
}

void NativeWidgetMus::OnWindowDestroyed(aura::Window* window) {
  // Cleanup happens in OnHostClosed(). We own |content_window_| (indirectly by
  // way of |dispatcher_|) so there should be no need to do any processing
  // here.
}

void NativeWidgetMus::OnWindowTargetVisibilityChanged(bool visible) {
}

bool NativeWidgetMus::HasHitTestMask() const {
  return native_widget_delegate_->HasHitTestMask();
}

void NativeWidgetMus::GetHitTestMask(gfx::Path* mask) const {
  native_widget_delegate_->GetHitTestMask(mask);
}

void NativeWidgetMus::OnKeyEvent(ui::KeyEvent* event) {
  if (event->is_char()) {
    // If a ui::InputMethod object is attached to the root window, character
    // events are handled inside the object and are not passed to this function.
    // If such object is not attached, character events might be sent (e.g. on
    // Windows). In this case, we just skip these.
    return;
  }
  // Renderer may send a key event back to us if the key event wasn't handled,
  // and the window may be invisible by that time.
  if (!content_->IsVisible())
    return;

  native_widget_delegate_->OnKeyEvent(event);
  if (event->handled())
    return;

  if (GetWidget()->HasFocusManager() &&
      !GetWidget()->GetFocusManager()->OnKeyEvent(*event))
    event->SetHandled();
}

void NativeWidgetMus::OnMouseEvent(ui::MouseEvent* event) {
  // TODO(sky): forward to tooltipmanager. See NativeWidgetDesktopAura.
  DCHECK(content_->IsVisible());
  native_widget_delegate_->OnMouseEvent(event);
  // WARNING: we may have been deleted.
}

void NativeWidgetMus::OnScrollEvent(ui::ScrollEvent* event) {
  if (event->type() == ui::ET_SCROLL) {
    native_widget_delegate_->OnScrollEvent(event);
    if (event->handled())
      return;

    // Convert unprocessed scroll events into wheel events.
    ui::MouseWheelEvent mwe(*static_cast<ui::ScrollEvent*>(event));
    native_widget_delegate_->OnMouseEvent(&mwe);
    if (mwe.handled())
      event->SetHandled();
  } else {
    native_widget_delegate_->OnScrollEvent(event);
  }
}

void NativeWidgetMus::OnGestureEvent(ui::GestureEvent* event) {
  native_widget_delegate_->OnGestureEvent(event);
}

}  // namespace views
