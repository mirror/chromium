// Copyright 2017 The Chromium Authors. All
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview using private properties isn't a Closure violation in tests.
 * @suppress {accessControls}
 */

/**
 * In theory, the browser should reset its state between tests, but in practice
 * application panel tests suffer a lot of flakiness because it resources such as
 * IndexedDB don't get properly cleared between tests.
 *
 * This is a temporary solution to fix flakiness by resetting all storages at
 * the start of application panel tests. The better solution would be to clear these
 * storages between every test in the back-end.
 */
(function resetState() {
  var clearStorageView = new Resources.ClearStorageView();
  clearStorageView._clear();
})();

ApplicationTestRunner.createWebSQLDatabase = function(name) {
  return TestRunner.evaluateInPageAsync(`_openWebSQLDatabase("${name}")`);
};

ApplicationTestRunner.requestURLComparer = function(r1, r2) {
  return r1.request.url.localeCompare(r2.request.url);
};

ApplicationTestRunner.runAfterCachedResourcesProcessed = function(callback) {
  if (!TestRunner.resourceTreeModel._cachedResourcesProcessed)
    TestRunner.resourceTreeModel.addEventListener(SDK.ResourceTreeModel.Events.CachedResourcesLoaded, callback);
  else
    callback();
};

ApplicationTestRunner.runAfterResourcesAreFinished = function(resourceURLs, callback) {
  var resourceURLsMap = new Set(resourceURLs);

  function checkResources() {
    for (var url of resourceURLsMap) {
      var resource = ApplicationTestRunner.resourceMatchingURL(url);

      if (resource)
        resourceURLsMap.delete(url);
    }

    if (!resourceURLsMap.size) {
      TestRunner.resourceTreeModel.removeEventListener(SDK.ResourceTreeModel.Events.ResourceAdded, checkResources);
      callback();
    }
  }

  checkResources();

  if (resourceURLsMap.size)
    TestRunner.resourceTreeModel.addEventListener(SDK.ResourceTreeModel.Events.ResourceAdded, checkResources);
};

ApplicationTestRunner.showResource = function(resourceURL, callback) {
  var reported = false;

  function callbackWrapper(sourceFrame) {
    if (reported)
      return;

    callback(sourceFrame);
    reported = true;
  }

  function showResourceCallback() {
    var resource = ApplicationTestRunner.resourceMatchingURL(resourceURL);

    if (!resource)
      return;

    UI.panels.resources.showResource(resource, 1);
    var sourceFrame = UI.panels.resources._resourceViewForResource(resource);

    if (sourceFrame.loaded)
      callbackWrapper(sourceFrame);
    else
      TestRunner.addSniffer(sourceFrame, 'onTextEditorContentSet', callbackWrapper.bind(null, sourceFrame));
  }

  ApplicationTestRunner.runAfterResourcesAreFinished([resourceURL], showResourceCallback);
};

ApplicationTestRunner.resourceMatchingURL = function(resourceURL) {
  var result = null;
  TestRunner.resourceTreeModel.forAllResources(visit);

  function visit(resource) {
    if (resource.url.indexOf(resourceURL) !== -1) {
      result = resource;
      return true;
    }
  }

  return result;
};

ApplicationTestRunner.databaseModel = function() {
  return TestRunner.mainTarget.model(Resources.DatabaseModel);
};

ApplicationTestRunner.domStorageModel = function() {
  return TestRunner.mainTarget.model(Resources.DOMStorageModel);
};

ApplicationTestRunner.indexedDBModel = function() {
  return TestRunner.mainTarget.model(Resources.IndexedDBModel);
};

TestRunner.deprecatedInitAsync(`
  function _openWebSQLDatabase(name) {
    return new Promise(resolve => openDatabase(name, '1.0', '', 1024 * 1024, resolve));
  }
`);
