#include "chrome/browser/ui/views/cluster/cluster_manager_view.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"

ClusterManagerView::ClusterManagerView()
    : manager_(),
      controller_(&manager_) {
}

ClusterManagerView::~ClusterManagerView() {
}

void ClusterManagerView::Init() {
  new_tab_button_ = new views::LabelButton(this, base::ASCIIToUTF16("New tab"));
  AddChildView(new_tab_button_);
}

void ClusterManagerView::Layout() {
  new_tab_button_->SetBounds(5, 5, width() - 10, 20);
}

void ClusterManagerView::OnPaint(gfx::Canvas* canvas) {
  // White background, left border.
  canvas->FillRect(gfx::Rect(0, 0, width(), height()), 0xFFF2F2F2);
  canvas->DrawLine(gfx::Point(width() - 1, 0), gfx::Point(width() - 1, height()), 0xFF303030);
}

const char* ClusterManagerView::GetClassName() const {
  return "ClusterManager";
}

gfx::Size ClusterManagerView::CalculatePreferredSize() const {
  return gfx::Size(200, 200);
}

void ClusterManagerView::ButtonPressed(views::Button* sender,
                                       const ui::Event& event) {
  if (sender == new_tab_button_) {
    manager_.NewTab();
  }
}
