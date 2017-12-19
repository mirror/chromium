// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/app_list_folder_view.h"

#include <algorithm>

#include "ash/app_list/model/app_list_folder_item.h"
#include "ash/app_list/model/app_list_model.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/app_list/app_list_constants.h"
#include "ui/app_list/app_list_features.h"
#include "ui/app_list/pagination_model.h"
#include "ui/app_list/views/app_list_item_view.h"
#include "ui/app_list/views/app_list_main_view.h"
#include "ui/app_list/views/apps_container_view.h"
#include "ui/app_list/views/apps_grid_view.h"
#include "ui/app_list/views/contents_view.h"
#include "ui/app_list/views/folder_background_view.h"
#include "ui/app_list/views/folder_header_view.h"
#include "ui/app_list/views/page_switcher_fullscreen.h"
#include "ui/app_list/views/search_box_view.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/background.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/view_model.h"
#include "ui/views/view_model_utils.h"

namespace app_list {

namespace {

// The preferred width/height for AppListFolderView.
constexpr int kAppsFolderPreferredWidth = 576;
constexpr int kAppsFolderPreferredHeight = 504;

// Constants for fullscreen app list.
constexpr int kFullscreenBackgroundCornerRadius = 4;
constexpr int kFullscreenItemGridsBottomPadding = 24;
constexpr int kFullscreenFolderPadding = 12;

// Indexes of interesting views in ViewModel of AppListFolderView.
const int kIndexChildItems = 0;
const int kIndexFolderHeader = 1;
const int kIndexPageSwitcher = 2;

// Threshold for the distance from the center of the item to the circle of the
// folder container ink bubble, beyond which, the item is considered dragged
// out of the folder boundary.
const int kOutOfFolderContainerBubbleDelta = 30;

// A background with rounded corner for fullscreen app list.
class FolderBackground : public views::Background {
 public:
  FolderBackground(SkColor color, int corner_radius)
      : color_(color), corner_radius_(corner_radius) {}
  ~FolderBackground() override {}

 private:
  // views::Background overrides:
  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    gfx::Rect bounds = view->GetContentsBounds();
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    flags.setColor(color_);
    canvas->DrawRoundRect(bounds, corner_radius_, flags);
  }

  const SkColor color_;
  const int corner_radius_;

  DISALLOW_COPY_AND_ASSIGN(FolderBackground);
};

}  // namespace

AppListFolderView::AppListFolderView(AppsContainerView* container_view,
                                     AppListModel* model,
                                     AppListMainView* app_list_main_view)
    : container_view_(container_view),
      app_list_main_view_(app_list_main_view),
      folder_header_view_(new FolderHeaderView(this)),
      view_model_(new views::ViewModel),
      model_(model),
      folder_item_(NULL),
      hide_for_reparent_(false),
      is_fullscreen_app_list_enabled_(features::IsFullscreenAppListEnabled()),
      is_app_list_focus_enabled_(features::IsAppListFocusEnabled()) {
  if (is_fullscreen_app_list_enabled_) {
    // In fullscreen app list, the items grid is on top of the folder header and
    // the page switcher.
    items_grid_view_ =
        new AppsGridView(app_list_main_view_->contents_view(), this);
    items_grid_view_->SetModel(model);
    AddChildView(items_grid_view_);
    view_model_->Add(items_grid_view_, kIndexChildItems);

    AddChildView(folder_header_view_);
    view_model_->Add(folder_header_view_, kIndexFolderHeader);

    page_switcher_fullscreen_ = new PageSwitcherFullscreen(
        items_grid_view_->pagination_model(), false /* vertical */);
    AddChildView(page_switcher_fullscreen_);
    view_model_->Add(page_switcher_fullscreen_, kIndexPageSwitcher);

    SetPaintToLayer();
    SetBackground(std::make_unique<FolderBackground>(
        kCardBackgroundColor, kFullscreenBackgroundCornerRadius));
  } else {
    AddChildView(folder_header_view_);
    view_model_->Add(folder_header_view_, kIndexFolderHeader);

    items_grid_view_ =
        new AppsGridView(app_list_main_view_->contents_view(), this);
    items_grid_view_->SetLayout(
        container_view->apps_grid_view()->cols(),
        container_view->apps_grid_view()->rows_per_page());
    items_grid_view_->SetModel(model);
    AddChildView(items_grid_view_);
    view_model_->Add(items_grid_view_, kIndexChildItems);

    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
  }

  model_->AddObserver(this);
}

AppListFolderView::~AppListFolderView() {
  model_->RemoveObserver(this);

  // This prevents the AppsGridView's destructor from calling the now-deleted
  // AppListFolderView's methods if a drag is in progress at the time.
  items_grid_view_->set_folder_delegate(nullptr);
}

void AppListFolderView::SetAppListFolderItem(AppListFolderItem* folder) {
  accessible_name_ = ui::ResourceBundle::GetSharedInstance().GetLocalizedString(
      IDS_APP_LIST_FOLDER_OPEN_FOLDER_ACCESSIBILE_NAME);
  NotifyAccessibilityEvent(ui::AX_EVENT_ALERT, true);

  folder_item_ = folder;
  items_grid_view_->SetItemList(folder_item_->item_list());
  folder_header_view_->SetFolderItem(folder_item_);
}

void AppListFolderView::ScheduleShowHideAnimation(bool show,
                                                  bool hide_for_reparent) {
  hide_for_reparent_ = hide_for_reparent;

  // Stop any previous animation.
  layer()->GetAnimator()->StopAnimating();

  if (show) {
    // Hide the top items temporarily if showing the view for opening the
    // folder.
    items_grid_view_->SetTopItemViewsVisible(false);

    // Reset page if showing the view.
    items_grid_view_->pagination_model()->SelectPage(0, false);
  }

  // Set initial state.
  layer()->SetOpacity(show ? 0.0f : 1.0f);
  SetVisible(true);
  UpdateFolderNameVisibility(true);

  ui::ScopedLayerAnimationSettings animation(layer()->GetAnimator());
  animation.SetTweenType(show ? kFolderFadeInTweenType
                              : kFolderFadeOutTweenType);
  animation.AddObserver(this);
  animation.SetTransitionDuration(base::TimeDelta::FromMilliseconds(
      show ? kFolderTransitionInDurationMs : kFolderTransitionOutDurationMs));

  layer()->SetOpacity(show ? 1.0f : 0.0f);
  if (!is_fullscreen_app_list_enabled_) {
    // Don't show back button in fullscreen app list.
    app_list_main_view_->search_box_view()->ShowBackOrGoogleIcon(show);
  }
}

gfx::Size AppListFolderView::CalculatePreferredSize() const {
  if (!is_fullscreen_app_list_enabled_)
    return gfx::Size(kAppsFolderPreferredWidth, kAppsFolderPreferredHeight);

  gfx::Size size = items_grid_view_->GetTileGridSizeWithoutPadding();
  size.Enlarge(0, kFullscreenItemGridsBottomPadding +
                      folder_header_view_->GetPreferredSize().height());
  size.Enlarge(kFullscreenFolderPadding * 2, kFullscreenFolderPadding * 2);
  return size;
}

void AppListFolderView::Layout() {
  CalculateIdealBounds();
  views::ViewModelUtils::SetViewBoundsToIdealBounds(*view_model_);
}

bool AppListFolderView::OnKeyPressed(const ui::KeyEvent& event) {
  if (is_app_list_focus_enabled_) {
    // TODO(weidongg/766807) Remove this function when the flag is enabled by
    // default.
    return false;
  }
  // Process TAB if focus should go to header; otherwise, AppsGridView will do
  // the right thing.
  if (event.key_code() == ui::VKEY_TAB) {
    if (items_grid_view_->has_selected_view() == event.IsShiftDown() &&
        !folder_header_view_->HasTextFocus()) {
      folder_header_view_->SetTextFocus();
      items_grid_view_->ClearAnySelectedView();
      return true;
    } else {
      GiveBackFocusToSearchBox();
    }
  }

  // This will select an app in the list, so we need to relinquish focus.
  if (event.key_code() == ui::VKEY_DOWN)
    GiveBackFocusToSearchBox();

  return items_grid_view_->OnKeyPressed(event);
}

void AppListFolderView::OnAppListItemWillBeDeleted(AppListItem* item) {
  if (item == folder_item_) {
    items_grid_view_->OnFolderItemRemoved();
    folder_header_view_->OnFolderItemRemoved();
    folder_item_ = NULL;

    // Do not change state if it is hidden.
    if (hide_for_reparent_ || layer()->opacity() == 0.0f)
      return;

    // If the folder item associated with this view is removed from the model,
    // (e.g. the last item in the folder was deleted), reset the view and signal
    // the container view to show the app list instead.
    // Pass NULL to ShowApps() to avoid triggering animation from the deleted
    // folder.
    container_view_->ShowApps(NULL);
  }
}

void AppListFolderView::OnImplicitAnimationsCompleted() {
  // Show the top items when the opening folder animation is done.
  if (layer()->opacity() == 1.0f)
    items_grid_view_->SetTopItemViewsVisible(true);

  // If the view is hidden for reparenting a folder item, it has to be visible,
  // so that drag_view_ can keep receiving mouse events.
  if (layer()->opacity() == 0.0f && !hide_for_reparent_)
    SetVisible(false);

  // Set the view bounds to a small rect, so that it won't overlap the root
  // level apps grid view during folder item reprenting transitional period.
  if (hide_for_reparent_)
    SetBoundsRect(gfx::Rect(bounds().x(), bounds().y(), 1, 1));
}

void AppListFolderView::CalculateIdealBounds() {
  gfx::Rect rect(GetContentsBounds());
  if (rect.IsEmpty())
    return;
  if (is_fullscreen_app_list_enabled_) {
    rect.Inset(kFullscreenFolderPadding, kFullscreenFolderPadding);

    gfx::Rect grid_frame(rect);
    grid_frame.Inset(items_grid_view_->GetTilePadding());
    grid_frame.set_height(items_grid_view_->GetPreferredSize().height());
    view_model_->set_ideal_bounds(kIndexChildItems, grid_frame);

    gfx::Rect header_frame(rect);
    header_frame.set_y(grid_frame.bottom() +
                       items_grid_view_->GetTilePadding().bottom() +
                       kFullscreenItemGridsBottomPadding);
    header_frame.set_height(folder_header_view_->GetPreferredSize().height());
    view_model_->set_ideal_bounds(kIndexFolderHeader, header_frame);

    gfx::Rect page_switcher_frame(rect);
    gfx::Size page_switcher_size =
        page_switcher_fullscreen_->GetPreferredSize();
    page_switcher_frame.set_x(page_switcher_frame.right() -
                              page_switcher_size.width());
    page_switcher_frame.set_y(header_frame.y());
    page_switcher_frame.set_size(page_switcher_size);
    view_model_->set_ideal_bounds(kIndexPageSwitcher, page_switcher_frame);
    return;
  }

  gfx::Rect header_frame(rect);
  gfx::Size size = folder_header_view_->GetPreferredSize();
  header_frame.set_height(size.height());
  view_model_->set_ideal_bounds(kIndexFolderHeader, header_frame);

  gfx::Rect grid_frame(rect);
  grid_frame.Subtract(header_frame);
  view_model_->set_ideal_bounds(kIndexChildItems, grid_frame);
}

void AppListFolderView::StartSetupDragInRootLevelAppsGridView(
    AppListItemView* original_drag_view,
    const gfx::Point& drag_point_in_root_grid,
    bool has_native_drag) {
  // Converts the original_drag_view's bounds to the coordinate system of
  // root level grid view.
  gfx::RectF rect_f(original_drag_view->bounds());
  views::View::ConvertRectToTarget(items_grid_view_,
                                   container_view_->apps_grid_view(), &rect_f);
  gfx::Rect rect_in_root_grid_view = gfx::ToEnclosingRect(rect_f);

  container_view_->apps_grid_view()
      ->InitiateDragFromReparentItemInRootLevelGridView(
          original_drag_view, rect_in_root_grid_view, drag_point_in_root_grid,
          has_native_drag);
}

gfx::Rect AppListFolderView::GetItemIconBoundsAt(int index) {
  AppListItemView* item_view = items_grid_view_->GetItemViewAt(index);
  // Icon bounds relative to AppListItemView.
  const gfx::Rect icon_bounds = item_view->GetIconBounds();
  gfx::Rect to_apps_grid_view = item_view->ConvertRectToParent(icon_bounds);
  gfx::Rect to_folder =
      items_grid_view_->ConvertRectToParent(to_apps_grid_view);

  // Get the icon image's bound.
  to_folder.ClampToCenteredSize(
      gfx::Size(kGridIconDimension, kGridIconDimension));

  return to_folder;
}

void AppListFolderView::UpdateFolderViewBackground(bool show_bubble) {
  if (is_fullscreen_app_list_enabled_)
    return;
  if (hide_for_reparent_)
    return;

  // Before showing the folder container inking bubble, hide the folder name.
  if (show_bubble)
    UpdateFolderNameVisibility(false);

  container_view_->folder_background_view()->UpdateFolderContainerBubble(
      show_bubble ? FolderBackgroundView::SHOW_BUBBLE
                  : FolderBackgroundView::HIDE_BUBBLE);
}

void AppListFolderView::UpdateFolderNameVisibility(bool visible) {
  if (is_fullscreen_app_list_enabled_)
    return;
  folder_header_view_->UpdateFolderNameVisibility(visible);
}

void AppListFolderView::SetBackButtonLabel(bool folder) {
  app_list_main_view_->search_box_view()->SetBackButtonLabel(folder);
}

bool AppListFolderView::IsPointOutsideOfFolderBoundary(
    const gfx::Point& point) {
  if (!GetLocalBounds().Contains(point))
    return true;

  gfx::Point center = GetLocalBounds().CenterPoint();
  float delta = (point - center).Length();
  return delta >
         kFolderBackgroundBubbleRadius + kOutOfFolderContainerBubbleDelta;
}

// When user drags a folder item out of the folder boundary ink bubble, the
// folder view UI will be hidden, and switch back to top level AppsGridView.
// The dragged item will seamlessly move on the top level AppsGridView.
// In order to achieve the above, we keep the folder view and its child grid
// view visible with opacity 0, so that the drag_view_ on the hidden grid view
// will keep receiving mouse event. At the same time, we initiated a new
// drag_view_ in the top level grid view, and keep it moving with the hidden
// grid view's drag_view_, so that the dragged item can be engaged in drag and
// drop flow in the top level grid view. During the reparenting process, the
// drag_view_ in hidden grid view will dispatch the drag and drop event to
// the top level grid view, until the drag ends.
void AppListFolderView::ReparentItem(
    AppListItemView* original_drag_view,
    const gfx::Point& drag_point_in_folder_grid,
    bool has_native_drag) {
  // Convert the drag point relative to the root level AppsGridView.
  gfx::Point to_root_level_grid = drag_point_in_folder_grid;
  ConvertPointToTarget(items_grid_view_, container_view_->apps_grid_view(),
                       &to_root_level_grid);
  StartSetupDragInRootLevelAppsGridView(original_drag_view, to_root_level_grid,
                                        has_native_drag);
  container_view_->ReparentFolderItemTransit(folder_item_);
}

void AppListFolderView::DispatchDragEventForReparent(
    AppsGridView::Pointer pointer,
    const gfx::Point& drag_point_in_folder_grid) {
  AppsGridView* root_grid = container_view_->apps_grid_view();
  gfx::Point drag_point_in_root_grid = drag_point_in_folder_grid;
  ConvertPointToTarget(items_grid_view_, root_grid, &drag_point_in_root_grid);
  root_grid->UpdateDragFromReparentItem(pointer, drag_point_in_root_grid);
}

void AppListFolderView::DispatchEndDragEventForReparent(
    bool events_forwarded_to_drag_drop_host,
    bool cancel_drag) {
  container_view_->apps_grid_view()->EndDragFromReparentItemInRootLevel(
      events_forwarded_to_drag_drop_host, cancel_drag);
  container_view_->ReparentDragEnded();
}

void AppListFolderView::HideViewImmediately() {
  SetVisible(false);
  hide_for_reparent_ = false;
}

void AppListFolderView::CloseFolderPage() {
  accessible_name_ = ui::ResourceBundle::GetSharedInstance().GetLocalizedString(
      IDS_APP_LIST_FOLDER_CLOSE_FOLDER_ACCESSIBILE_NAME);
  NotifyAccessibilityEvent(ui::AX_EVENT_ALERT, true);

  GiveBackFocusToSearchBox();
  if (items_grid_view()->dragging())
    items_grid_view()->EndDrag(true);
  items_grid_view()->ClearAnySelectedView();
  container_view_->ShowApps(folder_item_);
}

bool AppListFolderView::IsOEMFolder() const {
  return folder_item_->folder_type() == AppListFolderItem::FOLDER_TYPE_OEM;
}

void AppListFolderView::SetRootLevelDragViewVisible(bool visible) {
  container_view_->apps_grid_view()->SetDragViewVisible(visible);
}

void AppListFolderView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ui::AX_ROLE_GENERIC_CONTAINER;
  node_data->SetName(accessible_name_);
}

void AppListFolderView::NavigateBack(AppListFolderItem* item,
                                     const ui::Event& event_flags) {
  CloseFolderPage();
}

void AppListFolderView::GiveBackFocusToSearchBox() {
  app_list_main_view_->search_box_view()->search_box()->RequestFocus();
}

void AppListFolderView::SetItemName(AppListFolderItem* item,
                                    const std::string& name) {
  model_->SetItemName(item, name);
}

}  // namespace app_list
