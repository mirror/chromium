#ifndef CHROME_BROWSER_UI_CLUSTER_MANAGER_CONTROLLER_H_
#define CHROME_BROWSER_UI_CLUSTER_MANAGER_CONTROLLER_H_

class ClusterManager;

class ClusterManagerController {
 public:
  // Does not take ownership of the opinter, it must outlive this class.
  ClusterManagerController(ClusterManager* manager);
  ~ClusterManagerController();

  void NewTab();

 private:
  ClusterManager* manager_;  // Non-owning.
};

#endif  // CHROME_BROWSER_UI_CLUSTER_MANAGER_CONTROLLER_H_
