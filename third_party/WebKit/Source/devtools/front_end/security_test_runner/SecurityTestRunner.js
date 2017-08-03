
SecurityTestRunner.dumpSecurityPanelSidebarOrigins = function() {
  for (var key in Security.SecurityPanelSidebarTree.OriginGroupName) {
    var originGroupName = Security.SecurityPanelSidebarTree.OriginGroupName[key];
    var originGroup = Security.SecurityPanel._instance()._sidebarTree._originGroups.get(originGroupName);

    if (originGroup.hidden)
      continue;

    TestRunner.addResult('Group: ' + originGroupName);
    var originTitles = originGroup.childrenListElement.getElementsByClassName('title');

    for (var originTitle of originTitles)
      TestRunner.dumpDeepInnerHTML(originTitle);
  }
};

SecurityTestRunner.dispatchRequestFinished = function(request) {
  TestRunner.networkManager.dispatchEventToListeners(SDK.NetworkManager.Events.RequestFinished, request);
};
