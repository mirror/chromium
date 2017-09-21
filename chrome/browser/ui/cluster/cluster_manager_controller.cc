#include "chrome/browser/ui/cluster/cluster_manager_controller.h"

ClusterManagerController::ClusterManagerController(ClusterManager* manager)
    : manager_(manager) {
}

ClusterManagerController::~ClusterManagerController() {
}

void ClusterManagerController::NewTab() {
  (void)manager_;
}
