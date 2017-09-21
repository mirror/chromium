

#include "chrome/browser/cluster/cluster_manager.h"

ClusterManager::ClusterManager() {
  Cluster cluster;
  cluster.set_title("Main");

  ClusterEntry entry;
  entry.serialized().
  cluster.entries().push_back(std::move(entry));

  root_clusters.push_back(std::move(cluster));
}

ClusterManager::~ClusterManager() = default;
