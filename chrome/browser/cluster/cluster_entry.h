#ifndef CHROME_BROWSER_CLUSTER_CLUSTER_ENTRY_H_
#define CHROME_BROWSER_CLUSTER_CLUSTER_ENTRY_H_

#include "content/public/browser/navigation_entry.h"

class ClusterEntry {
 public:
  ClusterEntry();
  ClusterEntry(const ClusterEntry&);
  ClusterEntry(ClusterEntry&&) noexcept;
  ~ClusterEntry();

  const base::string16& title() const { return serialized_.title(); }

  sessions::SerializedNavigationEntry& serialized() { return serialized_; }

 private:
  sessions::SerializedNavigationEntry serialized_;
};

#endif  // CHROME_BROWSER_CLUSTER_CLUSTER_ENTRY_H_
