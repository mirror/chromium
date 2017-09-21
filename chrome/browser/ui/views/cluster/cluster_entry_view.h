#ifndef CHROME_BROWSER_UI_VIEWS_CLUSTER_CLUSTER_ENTRY_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_CLUSTER_CLUSTER_ENTRY_VIEW_H_

#include "ui/views/controls/button/label_button.h"

class ClusterEntry;

class ClusterEntryView : public views::LabelButton {
 public:
  ClusterEntryView(views::ButtonListener* listener,
                   ClusterEntry* entry);
  ~ClusterEntryView() override;

 private:
  ClusterEntry* entry_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_CLUSTER_CLUSTER_ENTRY_VIEW_H_
