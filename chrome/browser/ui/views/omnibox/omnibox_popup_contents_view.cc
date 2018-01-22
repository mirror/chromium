// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/omnibox/omnibox_popup_contents_view.h"

#include <algorithm>
#include <memory>

#include "base/feature_list.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_result_view.h"
#include "chrome/browser/ui/views/theme_copying_widget.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "third_party/skia/include/core/SkDrawLooper.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/theme_provider.h"
#include "ui/compositor/clip_recorder.h"
#include "ui/compositor/content_shadow.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/safe_integer_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/path.h"
#include "ui/gfx/shadow_util.h"
#include "ui/gfx/shadow_value.h"
#include "ui/views/background.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/painter.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"

namespace {

// Cache the shadow images so that potentially expensive shadow drawing isn't
// repeated.
base::LazyInstance<gfx::ImageSkia>::DestructorAtExit g_top_shadow =
    LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<gfx::ImageSkia>::DestructorAtExit g_bottom_shadow =
    LAZY_INSTANCE_INITIALIZER;

constexpr int kPopupVerticalPadding = 4;
constexpr int kTouchableBackbufferHeight = 500;
constexpr int kTouchableRadius = 8;
constexpr int kTouchableElevation = 16;
constexpr int kTouchableAnimationDurationMs = 250;
constexpr int kTouchableMinHeight = 20;

bool IsTouchable() {
  return true;
}

bool IsNarrow() {
  return IsTouchable() ||
         base::FeatureList::IsEnabled(omnibox::kUIExperimentNarrowDropdown);
}

}  // namespace

class OmniboxPopupContentsView::AutocompletePopupWidget
    : public ThemeCopyingWidget,
      public base::SupportsWeakPtr<AutocompletePopupWidget> {
 public:
  explicit AutocompletePopupWidget(views::Widget* role_model)
      : ThemeCopyingWidget(role_model) {}
  ~AutocompletePopupWidget() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(AutocompletePopupWidget);
};

class OmniboxPopupContentsView::AutocompleteClipFrame : public views::View {
 public:
  // Inserts a AutocompleteClipFrame into the view hierarchy and returns a ref.
  static AutocompleteClipFrame* Create(AutocompletePopupWidget* widget,
                                       OmniboxPopupContentsView* contents) {
    AutocompleteClipFrame* frame = new AutocompleteClipFrame(contents);
    frame->Init();
    widget->SetContentsView(frame);
    return frame;
  }

  static gfx::Insets GetBorderInsets() {
    // gfx::ShadowDetails::Get() is cached.
    return gfx::ShadowValue::GetBlurRegion(
               gfx::ShadowDetails::Get(kTouchableElevation, kTouchableRadius)
                   .values) +
           gfx::Insets(kTouchableRadius);
  }

  int content_inset() const { return content_inset_; }

  void Repaint() { contents_->layer()->ScheduleDraw(); }

  void SetTargetBounds(const gfx::Rect& window_bounds) {
    gfx::Rect content_bounds = gfx::Rect(window_bounds.size());
    background_host_->SetSize(content_bounds.size());
    content_bounds.Inset(insets_);
    contents_->layer()->SchedulePaint(gfx::Rect(content_bounds.size()));

    const gfx::Rect old_shadow_bounds = contents_shadow_->content_bounds();
    if (old_shadow_bounds == content_bounds)
      return;

    contents_shadow_->SetContentBounds(content_bounds);
    background_->SetBoundsRect(content_bounds);

    // Animate the background (rounded rect and shadow) with a transform.
    float scale_x = 1.0 * old_shadow_bounds.width() / content_bounds.width();
    float scale_y = 1.0 *
                    std::max(old_shadow_bounds.height(), kTouchableMinHeight) /
                    content_bounds.height();

    // Start at the current transform, which might not be the identity matrix
    // if an animation is in progress.
    gfx::Transform transform = background_host_->layer()->transform();
    transform.Translate(-insets_.left(), insets_.top());
    // Animate from a height of at least one row.
    transform.Scale(scale_x, scale_y);
    transform.Translate(insets_.left(), -insets_.top());
    background_host_->layer()->SetTransform(transform);
    ui::ScopedLayerAnimationSettings transition(
        background_host_->layer()->GetAnimator());
    transition.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(kTouchableAnimationDurationMs));
    transition.SetTweenType(gfx::Tween::FAST_OUT_SLOW_IN_2);
    // Always animate towards the identity matrix.
    background_host_->layer()->SetTransform(gfx::Transform());

    // Inset the clip to avoid poking outside the rounded corners.
    content_bounds.Inset(content_inset_, content_inset_);
    const gfx::Rect target_bounds = content_bounds;
    // Always animate from the current height.
    content_bounds.set_height(clip_view_->layer()->bounds().height());
    clip_view_->SetBoundsRect(target_bounds);
    clip_view_->layer()->SetBounds(content_bounds);

    ui::ScopedLayerAnimationSettings clip_transition(
        clip_view_->layer()->GetAnimator());
    clip_transition.SetTransitionDuration(base::TimeDelta::FromMilliseconds(
        kTouchableAnimationDurationMs + 50));  // S
    clip_transition.SetTweenType(gfx::Tween::FAST_OUT_SLOW_IN_2);
    clip_view_->layer()->SetBounds(target_bounds);
  }

  // views::View:
  void Layout() override {
    gfx::Rect rect = GetLocalBounds();
    background_->SetBoundsRect(rect);
    rect.Inset(insets_);
    clip_view_->SetBounds(content_inset_, content_inset_,
                          rect.width() - 2 * content_inset_,
                          kTouchableMinHeight);
    contents_->SetBounds(0, 0, rect.width() - 2 * content_inset_,
                         kTouchableBackbufferHeight);

    // Set a dummy contents size to seed the first animation.
    rect.set_height(kTouchableMinHeight);
    contents_shadow_->SetContentBounds(rect);
  }

 private:
  AutocompleteClipFrame(OmniboxPopupContentsView* contents)
      : contents_(contents),
        shadow_(gfx::ShadowDetails::Get(kTouchableElevation, kTouchableRadius)),
        insets_(GetBorderInsets()) {}

  void Init() {
    // Paint the contents to a layer, so it can be clipped.
    contents_->SetPaintToLayer();
    contents_->layer()->SetFillsBoundsOpaquely(false);

    // Note |contents_| can't be masked directly since the mask must match the
    // size of the layer it's hosted in.
    clip_view_ = new views::View();
    clip_view_->SetPaintToLayer();
    clip_view_->AddChildView(contents_);
    clip_view_->SetBounds(insets_.left() + content_inset_,
                          insets_.top() + content_inset_, 0, 0);
    clip_view_->layer()->SetMasksToBounds(true);

    background_host_ = new views::View();
    background_host_->SetPaintToLayer();
    background_host_->layer()->SetFillsBoundsOpaquely(false);

    background_ = new views::View();
    background_->SetBounds(insets_.left(), insets_.top(), 0, 0);
    background_->SetBackground(views::CreateBackgroundFromPainter(
        views::Painter::CreateSolidRoundRectPainter(SK_ColorWHITE,
                                                    kTouchableRadius)));
    background_host_->AddChildView(background_);

    contents_shadow_ = new ui::Shadow();
    contents_shadow_->SetRoundedCornerRadius(kTouchableRadius);
    contents_shadow_->Init(kTouchableElevation);

    background_host_->layer()->Add(contents_shadow_->layer());

    // Add sibling views. Note the background is not clipped.
    AddChildView(background_host_);
    AddChildView(clip_view_);
  }

  OmniboxPopupContentsView* contents_;
  const gfx::ShadowDetails shadow_;
  const gfx::Insets insets_;

  // The minimum inset to avoid corners poking outside the rounded rectangle.
  const int content_inset_ = std::ceil(
      kTouchableRadius - std::sqrt(kTouchableRadius * kTouchableRadius / 2.0));

  ui::Shadow* contents_shadow_ = nullptr;  // Fix ownership.
  views::View* clip_view_ = nullptr;
  views::View* background_host_ = nullptr;
  views::View* background_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AutocompleteClipFrame);
};

////////////////////////////////////////////////////////////////////////////////
// OmniboxPopupContentsView, public:

OmniboxPopupContentsView::OmniboxPopupContentsView(
    const gfx::FontList& font_list,
    OmniboxView* omnibox_view,
    OmniboxEditModel* edit_model,
    LocationBarView* location_bar_view)
    : model_(new OmniboxPopupModel(this, edit_model)),
      omnibox_view_(omnibox_view),
      location_bar_view_(location_bar_view),
      font_list_(font_list),
      size_animation_(this),
      start_margin_(0),
      end_margin_(0) {
  // The contents is owned by the LocationBarView.
  set_owned_by_client();

  const bool narrow_popup = IsNarrow();

  if (g_top_shadow.Get().isNull() && !narrow_popup) {
    std::vector<gfx::ShadowValue> shadows;
    // Blur by 1dp. See comment below about blur accounting.
    shadows.emplace_back(gfx::Vector2d(), 2, SK_ColorBLACK);
    g_top_shadow.Get() =
        gfx::ImageSkiaOperations::CreateHorizontalShadow(shadows, false);
  }
  if (g_bottom_shadow.Get().isNull() && !narrow_popup) {
    const int kSmallShadowBlur = 3;
    const int kLargeShadowBlur = 8;
    const int kLargeShadowYOffset = 3;

    std::vector<gfx::ShadowValue> shadows;
    // gfx::ShadowValue counts blur pixels both inside and outside the shape,
    // whereas these blur values only describe the outside portion, hence they
    // must be doubled.
    shadows.emplace_back(gfx::Vector2d(), 2 * kSmallShadowBlur,
                         SK_ColorBLACK);
    shadows.emplace_back(gfx::Vector2d(0, kLargeShadowYOffset),
                         2 * kLargeShadowBlur, SK_ColorBLACK);

    g_bottom_shadow.Get() =
        gfx::ImageSkiaOperations::CreateHorizontalShadow(shadows, true);
  }

  for (size_t i = 0; i < AutocompleteResult::GetMaxMatches(); ++i) {
    OmniboxResultView* result_view = new OmniboxResultView(this, i, font_list_);
    result_view->SetVisible(false);
    AddChildViewAt(result_view, static_cast<int>(i));
  }
}

OmniboxPopupContentsView::~OmniboxPopupContentsView() {
  // We don't need to do anything with |popup_| here.  The OS either has already
  // closed the window, in which case it's been deleted, or it will soon, in
  // which case there's nothing we need to do.
}

gfx::Rect OmniboxPopupContentsView::GetPopupBounds() const {
  if (!size_animation_.is_animating())
    return target_bounds_;

  gfx::Rect current_frame_bounds = start_bounds_;
  int total_height_delta = target_bounds_.height() - start_bounds_.height();
  // Round |current_height_delta| instead of truncating so we won't leave single
  // white pixels at the bottom of the popup as long when animating very small
  // height differences.
  int current_height_delta = static_cast<int>(
      size_animation_.GetCurrentValue() * total_height_delta - 0.5);
  current_frame_bounds.set_height(
      current_frame_bounds.height() + current_height_delta);
  return current_frame_bounds;
}

void OmniboxPopupContentsView::OpenMatch(size_t index,
                                         WindowOpenDisposition disposition) {
  DCHECK(HasMatchAt(index));

  omnibox_view_->OpenMatch(model_->result().match_at(index), disposition,
                           GURL(), base::string16(), index);
}

gfx::Image OmniboxPopupContentsView::GetMatchIcon(
    const AutocompleteMatch& match,
    SkColor vector_icon_color) const {
  return model_->GetMatchIcon(match, vector_icon_color);
}

void OmniboxPopupContentsView::SetSelectedLine(size_t index) {
  DCHECK(HasMatchAt(index));

  model_->SetSelectedLine(index, false, false);
}

bool OmniboxPopupContentsView::IsSelectedIndex(size_t index) const {
  return index == model_->selected_line();
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxPopupContentsView, OmniboxPopupView overrides:

bool OmniboxPopupContentsView::IsOpen() const {
  return popup_ != nullptr;
}

void OmniboxPopupContentsView::InvalidateLine(size_t line) {
  OmniboxResultView* result = result_view_at(line);
  result->Invalidate();
  result->SchedulePaint();

  if (HasMatchAt(line) && GetMatchAtIndex(line).associated_keyword.get()) {
    result->ShowKeyword(IsSelectedIndex(line) &&
        model_->selected_line_state() == OmniboxPopupModel::KEYWORD);
  }
}

void OmniboxPopupContentsView::OnLineSelected(size_t line) {
  result_view_at(line)->OnSelected();
}

void OmniboxPopupContentsView::UpdatePopupAppearance() {
  if (model_->result().empty() || omnibox_view_->IsImeShowingPopup()) {
    // No matches or the IME is showing a popup window which may overlap
    // the omnibox popup window.  Close any existing popup.
    if (popup_) {
      size_animation_.Stop();

      // NOTE: Do NOT use CloseNow() here, as we may be deep in a callstack
      // triggered by the popup receiving a message (e.g. LBUTTONUP), and
      // destroying the popup would cause us to read garbage when we unwind back
      // to that level.
      popup_->Close();  // This will eventually delete the popup.
      popup_.reset();
    }
    return;
  }

  // Fix-up any matches due to tail suggestions, before display below.
  model_->autocomplete_controller()->InlineTailPrefixes();

  // Update the match cached by each row, in the process of doing so make sure
  // we have enough row views.
  const size_t result_size = model_->result().size();
  for (size_t i = 0; i < result_size; ++i) {
    OmniboxResultView* view = result_view_at(i);
    const AutocompleteMatch& match = GetMatchAtIndex(i);
    view->SetMatch(match);
    view->SetVisible(true);
    if (match.answer && !model_->answer_bitmap().isNull()) {
      view->SetAnswerImage(
          gfx::ImageSkia::CreateFrom1xBitmap(model_->answer_bitmap()));
    }
  }

  for (size_t i = result_size; i < AutocompleteResult::GetMaxMatches(); ++i)
    child_at(i)->SetVisible(false);

  int top_edge_overlap = 0;
  bool narrow_popup = IsNarrow();
  if (!narrow_popup) {
    // We want the popup to appear to overlay the bottom of the toolbar. So we
    // shift the popup to completely cover the client edge, and then draw an
    // additional semitransparent shadow above that.
    top_edge_overlap = g_top_shadow.Get().height() +
                       views::NonClientFrameView::kClientEdgeThickness;
  }

  gfx::Point top_left_screen_coord;
  int width;
  location_bar_view_->GetOmniboxPopupPositioningInfo(
      &top_left_screen_coord, &width, &start_margin_,
      &end_margin_, top_edge_overlap);
  gfx::Rect new_target_bounds(top_left_screen_coord,
                              gfx::Size(width, CalculatePopupHeight()));

  SkColor background_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_ResultsTableNormalBackground);
  if (IsTouchable()) {
    new_target_bounds.Inset(-AutocompleteClipFrame::GetBorderInsets());
    // A background is necessary to paint the text properly, but it is inset,
    // clipped, and composited over a background that has rounded rectangles.
    SetBackground(views::CreateSolidBackground(background_color));
  } else if (narrow_popup) {
    auto border = std::make_unique<views::BubbleBorder>(
        views::BubbleBorder::NONE, views::BubbleBorder::SMALL_SHADOW,
        background_color);

    // Outdent the popup to factor in the shadow size.
    new_target_bounds.Inset(-border->GetInsets());

    SetBackground(std::make_unique<views::BubbleBackground>(border.get()));
    SetBorder(std::move(border));
  }

  // If we're animating and our target height changes, reset the animation.
  // NOTE: If we just reset blindly on _every_ update, then when the user types
  // rapidly we could get "stuck" trying repeatedly to animate shrinking by the
  // last few pixels to get to one visible result.
  if (new_target_bounds.height() != target_bounds_.height())
    size_animation_.Reset();
  target_bounds_ = new_target_bounds;

  if (!popup_) {
    views::Widget* popup_parent = location_bar_view_->GetWidget();

    // If the popup is currently closed, we need to create it.
    popup_ = (new AutocompletePopupWidget(popup_parent))->AsWeakPtr();

    views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
#if defined(OS_WIN)
    // On Windows use the software compositor to ensure that we don't block
    // the UI thread blocking issue during command buffer creation. We can
    // revert this change once http://crbug.com/125248 is fixed.
    params.force_software_compositing = true;
#endif
    params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
    params.parent = popup_parent->GetNativeView();
    params.bounds = GetPopupBounds();
    if (IsTouchable()) {
      params.bounds.set_height(kTouchableBackbufferHeight);
      params.shadow_type = views::Widget::InitParams::SHADOW_TYPE_NONE;
    }

    params.context = popup_parent->GetNativeWindow();
    popup_->Init(params);
    // Third-party software such as DigitalPersona identity verification can
    // hook the underlying window creation methods and use SendMessage to
    // synchronously change focus/activation, resulting in the popup being
    // destroyed by the time control returns here.  Bail out in this case to
    // avoid a NULL dereference.
    if (!popup_)
      return;
    popup_->SetVisibilityAnimationTransition(views::Widget::ANIMATE_NONE);
    if (IsTouchable()) {
      clip_frame_ = AutocompleteClipFrame::Create(popup_.get(), this);
    } else {
      popup_->SetContentsView(this);
    }
    popup_->StackAbove(omnibox_view_->GetRelativeWindowForPopup());
    if (!popup_) {
      // For some IMEs GetRelativeWindowForPopup triggers the omnibox to lose
      // focus, thereby closing (and destroying) the popup.
      // TODO(sky): this won't be needed once we close the omnibox on input
      // window showing.
      return;
    }
    popup_->ShowInactive();
  } else if (IsTouchable()) {
    UpdatePopupBounds();  // Don't start a UI animation.
  } else {
    // Animate the popup shrinking, but don't animate growing larger since that
    // would make the popup feel less responsive.
    start_bounds_ = GetWidget()->GetWindowBoundsInScreen();
    if (target_bounds_.height() < start_bounds_.height())
      size_animation_.Show();
    else
      start_bounds_ = target_bounds_;
    UpdatePopupBounds();
  }

  Layout();
}

void OmniboxPopupContentsView::OnMatchIconUpdated(size_t match_index) {
  result_view_at(match_index)->OnMatchIconUpdated();
}

gfx::Rect OmniboxPopupContentsView::GetTargetBounds() {
  return target_bounds_;
}

void OmniboxPopupContentsView::PaintUpdatesNow() {
  // TODO(beng): remove this from the interface.
}

void OmniboxPopupContentsView::OnDragCanceled() {
  SetMouseHandler(nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxPopupContentsView, AnimationDelegate implementation:

void OmniboxPopupContentsView::AnimationProgressed(
    const gfx::Animation* animation) {
  // Touchable Chrome doesn't use UI-thread animations.
  DCHECK(!IsTouchable());
  // We should only be running the animation when the popup is already visible.
  DCHECK(popup_);
  popup_->SetBounds(GetPopupBounds());
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxPopupContentsView, views::View overrides:

void OmniboxPopupContentsView::Layout() {
  // Size our children to the available content area.
  LayoutChildren();

  // We need to manually schedule a paint here since we are a layered window and
  // won't implicitly require painting until we ask for one.
  SchedulePaint();
}

views::View* OmniboxPopupContentsView::GetTooltipHandlerForPoint(
    const gfx::Point& point) {
  return nullptr;
}

bool OmniboxPopupContentsView::OnMouseDragged(const ui::MouseEvent& event) {
  size_t index = GetIndexForPoint(event.location());

  // If the drag event is over the bounds of one of the result views, pass
  // control to that view.
  if (HasMatchAt(index)) {
    SetMouseHandler(result_view_at(index));
    return false;
  }

  // If the drag event is not over any of the result views, that means that it
  // has passed outside the bounds of the popup view. Return true to keep
  // receiving the drag events, as the drag may return in which case we will
  // want to respond to it again.
  return true;
}

void OmniboxPopupContentsView::OnGestureEvent(ui::GestureEvent* event) {
  const size_t event_location_index = GetIndexForPoint(event->location());
  if (!HasMatchAt(event_location_index))
    return;

  switch (event->type()) {
    case ui::ET_GESTURE_TAP_DOWN:
    case ui::ET_GESTURE_SCROLL_BEGIN:
    case ui::ET_GESTURE_SCROLL_UPDATE:
      SetSelectedLine(event_location_index);
      break;
    case ui::ET_GESTURE_TAP:
    case ui::ET_GESTURE_SCROLL_END:
      OpenMatch(event_location_index, WindowOpenDisposition::CURRENT_TAB);
      break;
    default:
      return;
  }
  event->SetHandled();
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxPopupContentsView, private:

int OmniboxPopupContentsView::CalculatePopupHeight() {
  DCHECK_GE(static_cast<size_t>(child_count()), model_->result().size());
  int popup_height = 0;
  for (size_t i = 0; i < model_->result().size(); ++i)
    popup_height += child_at(i)->GetPreferredSize().height();

  // Add enough space on the top and bottom so it looks like there is the same
  // amount of space between the text and the popup border as there is in the
  // interior between each row of text.
  int height = popup_height + kPopupVerticalPadding * 2;
  if (IsTouchable())
    height += std::ceil(kTouchableRadius -
                        std::sqrt(kTouchableRadius * kTouchableRadius / 2.0)) *
              2;
  else
    height += g_top_shadow.Get().height() + g_bottom_shadow.Get().height();
  return height;
}

void OmniboxPopupContentsView::UpdatePopupBounds() {
  if (IsTouchable()) {
    clip_frame_->SetTargetBounds(GetPopupBounds());
  } else {
    popup_->SetBounds(GetPopupBounds());
  }
}

void OmniboxPopupContentsView::LayoutChildren() {
  gfx::Rect contents_rect = GetContentsBounds();
  contents_rect.Inset(gfx::Insets(kPopupVerticalPadding, 0));
  if (!IsTouchable()) {
    contents_rect.Inset(start_margin_, g_top_shadow.Get().height(), end_margin_,
                        0);
  }

  int top = contents_rect.y();
  for (size_t i = 0; i < AutocompleteResult::GetMaxMatches(); ++i) {
    View* v = child_at(i);
    if (v->visible()) {
      v->SetBounds(contents_rect.x(), top, contents_rect.width(),
                   v->GetPreferredSize().height());
      top = v->bounds().bottom();
    }
  }
}

bool OmniboxPopupContentsView::HasMatchAt(size_t index) const {
  return index < model_->result().size();
}

const AutocompleteMatch& OmniboxPopupContentsView::GetMatchAtIndex(
    size_t index) const {
  return model_->result().match_at(index);
}

size_t OmniboxPopupContentsView::GetIndexForPoint(const gfx::Point& point) {
  if (!HitTestPoint(point))
    return OmniboxPopupModel::kNoMatch;

  int nb_match = model_->result().size();
  DCHECK(nb_match <= child_count());
  for (int i = 0; i < nb_match; ++i) {
    views::View* child = child_at(i);
    gfx::Point point_in_child_coords(point);
    View::ConvertPointToTarget(this, child, &point_in_child_coords);
    if (child->visible() && child->HitTestPoint(point_in_child_coords))
      return i;
  }
  return OmniboxPopupModel::kNoMatch;
}

OmniboxResultView* OmniboxPopupContentsView::result_view_at(size_t i) {
  return static_cast<OmniboxResultView*>(child_at(static_cast<int>(i)));
}

void OmniboxPopupContentsView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  node_data->role = ui::AX_ROLE_LIST_BOX;
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxPopupContentsView, views::View overrides, private:

const char* OmniboxPopupContentsView::GetClassName() const {
  return "OmniboxPopupContentsView";
}

void OmniboxPopupContentsView::OnPaint(gfx::Canvas* canvas) {
  if (IsNarrow()) {
    View::OnPaint(canvas);
    if (clip_frame_)
      clip_frame_->Repaint();
    return;
  }

  canvas->TileImageInt(g_top_shadow.Get(), 0, 0, width(),
                       g_top_shadow.Get().height());
  canvas->TileImageInt(g_bottom_shadow.Get(), 0,
                       height() - g_bottom_shadow.Get().height(), width(),
                       g_bottom_shadow.Get().height());
}

void OmniboxPopupContentsView::PaintChildren(
    const views::PaintInfo& paint_info) {
  if (IsNarrow()) {
    View::PaintChildren(paint_info);
    return;
  }

  gfx::Rect contents_bounds = GetContentsBounds();
  contents_bounds.Inset(0, g_top_shadow.Get().height(), 0,
                        g_bottom_shadow.Get().height());

  ui::ClipRecorder clip_recorder(paint_info.context());
  clip_recorder.ClipRect(gfx::ScaleToRoundedRect(
      contents_bounds, paint_info.paint_recording_scale_x(),
      paint_info.paint_recording_scale_y()));
  {
    ui::PaintRecorder recorder(paint_info.context(), size());
    SkColor background_color = result_view_at(0)->GetColor(
        OmniboxResultView::NORMAL, OmniboxResultView::BACKGROUND);
    recorder.canvas()->DrawColor(background_color);
  }
  View::PaintChildren(paint_info);
}
