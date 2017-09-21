#ifndef CHROME_BROWSER_CLUSTER_CLUSTER_H_
#define CHROME_BROWSER_CLUSTER_CLUSTER_H_ 

#include <vector>

#include "base/strings/string16.h"

class ClusterEntry;

class Cluster {
 public:
  Cluster();
  ~Cluster();

  const base::string16& title() const { return title_; }
  void set_title(const base::string16& t) { title_ = t; }

  const std::vector<ClusterEntry>& entries() const { return entries_; }
  std::vector<ClusterEntry>& entries() { return entries_; }

 private:
  base::string16 title_;

  std::vector<ClusterEntry> entries_;
};

#endif  // CHROME_BROWSER_CLUSTER_CLUSTER_H_
