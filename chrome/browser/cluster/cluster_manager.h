#ifndef CHROME_BROWSER_CLUSTER_CLUSTER_MANAGER_H_
#define CHROME_BROWSER_CLUSTER_CLUSTER_MANAGER_H_

#include "chrome/browser/cluster/cluster.h"

class ClusterManager {
 public:
  ClusterManager();
  ~ClusterManager();

  const std::vector<Cluster>& root_clusters() const { return root_clusters_; }

 private:
  std::vector<Cluster> root_clusters_;
};

#endif  // CHROME_BROWSER_CLUSTER_CLUSTER_MANAGER_H_
