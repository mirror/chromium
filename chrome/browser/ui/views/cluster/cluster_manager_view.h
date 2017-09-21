
#ifndef CHROME_BROWSER_UI_VIEWS_CLUSTER_CLUSTER_MANAGER_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_CLUSTER_CLUSTER_MANAGER_VIEW_H_

#include "chrome/browser/cluster/cluster_manager.h"
#include "chrome/browser/ui/cluster/cluster_manager_controller.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

class ClusterManagerView
    : public views::View,
      public views::ButtonListener {
 public:
  ClusterManagerView();
  ~ClusterManagerView() override;

  void Init();

  // views::View overrides:
  void Layout() override;
  //void PaintChildren(const views::PaintInfo& paint_info) override;
  void OnPaint(gfx::Canvas* canvas) override;
  const char* GetClassName() const override;
  gfx::Size CalculatePreferredSize() const override;

  // views::ButtonListener implementation:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

 private:
  //void PaintCluster(const Cluster& cluster, gfx::Canvas* canvas, int top);
  //void PaintClusterEntry(const ClusterEntry& entry, gfx::Canvas* canvas, int top);

  // Currently this owns the cluster manager just to have somewhere to put it.
  // This should instead be some shared service.
  ClusterManager manager_;
  ClusterManagerController controller_;

  views::LabelButton* new_tab_button_ = nullptr;
};

#endif  // CHROME_BROWSER_UI_VIEWS_CLUSTER_CLUSTER_MANAGER_VIEW_H_
