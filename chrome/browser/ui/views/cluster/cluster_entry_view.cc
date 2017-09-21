
#include "chrome/browser/ui/views/cluster/cluster_entry_view.h"

#include "chrome/browser/cluster/cluster_entry.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/render_text.h"

ClusterEntryView::ClusterEntryView(views::ButtonListener* listener,
                                   ClusterEntry* entry)
    : LabelButton(listener, base::string16()),
      entry_(entry) {
  SetText(entry_->title());
}

ClusterEntryView::ClusterEntryView() {}
