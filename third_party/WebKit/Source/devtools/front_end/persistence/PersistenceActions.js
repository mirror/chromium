// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Persistence.PersistenceActions = {};

/**
 * @implements {UI.ContextMenu.Provider}
 * @unrestricted
 */
Persistence.PersistenceActions.ContextMenuProvider = class {
  /**
   * @override
   * @param {!Event} event
   * @param {!UI.ContextMenu} contextMenu
   * @param {!Object} target
   */
  appendApplicableItems(event, contextMenu, target) {
    var contentProvider = /** @type {!Common.ContentProvider} */ (target);
    if (!contentProvider.contentURL())
      return;
    if (contentProvider instanceof SDK.NetworkRequest)
      return;
    if (!contentProvider.contentType().isDocumentOrScriptOrStyleSheet())
      return;

    async function saveAs() {
      if (contentProvider instanceof Workspace.UISourceCode) {
        var uiSourceCode = /** @type {!Workspace.UISourceCode} */ (contentProvider);
        uiSourceCode.commitWorkingCopy();
      }
      var content = await contentProvider.requestContent();
      var url = contentProvider.contentURL();
      Workspace.fileManager.save(url, /** @type {string} */ (content), true);
      Workspace.fileManager.close(url);
    }

    contextMenu.appendSeparator();

    if (contentProvider instanceof Workspace.UISourceCode) {
      var uiSourceCode = /** @type {!Workspace.UISourceCode} */ (contentProvider);
      if (!uiSourceCode.project().canSetFileContent())
        contextMenu.appendItem(Common.UIString('Save as...'), () => saveAs());
    }
  }
};
