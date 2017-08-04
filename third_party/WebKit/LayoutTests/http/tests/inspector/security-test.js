// This file is being deprecated and is moving to front_end/security_test_runner/SecurityTestRunner.js
// Please see crbug.com/667560 for more details

var initialize_SecurityTest = function() {

InspectorTest.preloadPanel("security");

InspectorTest.dumpSecurityPanelSidebarOrigins = function() {
    for (var key in Security.SecurityPanelSidebarTree.OriginGroupName) {
        var originGroupName = Security.SecurityPanelSidebarTree.OriginGroupName[key];
        var originGroup = Security.SecurityPanel._instance()._sidebarTree._originGroups.get(originGroupName);
        if (originGroup.hidden)
            continue;
        InspectorTest.addResult("Group: " + originGroupName);
        var originTitles = originGroup.childrenListElement.getElementsByClassName("title");
        for (var originTitle of originTitles)
            InspectorTest.dumpDeepInnerHTML(originTitle);
    }
}

/**
 * @param {!SDK.NetworkRequest} request
 */
InspectorTest.dispatchRequestFinished = function(request) {
    InspectorTest.networkManager.dispatchEventToListeners(SDK.NetworkManager.Events.RequestFinished, request);
}

}
