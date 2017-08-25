// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @implements {Workspace.Project}
 */
Bindings.ChangeableNetworkProject = class extends Bindings.ContentProviderBasedProject {
  /**
   * @param {!Workspace.Workspace} workspace
   */
  constructor(workspace) {
    super(workspace, 'ChangeableNetworkProject', Workspace.projectTypes.Service, '', true /* isServiceProject */);

    workspace.addEventListener(Workspace.Workspace.Events.WorkingCopyChanged, this._uiSourceCodeChanged, this);
    workspace.addEventListener(Workspace.Workspace.Events.WorkingCopyCommitted, this._uiSourceCodeCommitted, this);
    workspace.addEventListener(Workspace.Workspace.Events.UISourceCodeRemoved, this._uiSourceCodeRemoved, this);
  }

  /**
   * @override
   * @return {!Workspace.projectTypes}
   */
  type() {
    return Workspace.projectTypes.ChangeableNetwork;
  }

  /**
   * @override
   * @return {boolean}
   */
  canSetFileContent() {
    return true;
  }

  /**
   * @override
   * @param {!Workspace.UISourceCode} uiSourceCode
   * @param {string} newContent
   * @param {function(?string)} callback
   */
  setFileContent(uiSourceCode, newContent, callback) {
    uiSourceCode.setWorkingCopy(newContent);
    callback(newContent);
  }

  /**
   * @param {!Common.Event} event
   */
  _uiSourceCodeRemoved(event) {
    var uiSourceCode = /** @type {!Workspace.UISourceCode} */ (event.data);
    var ownedUISourceCode = this.uiSourceCodeForURL(uiSourceCode.url());
    if (!ownedUISourceCode || ownedUISourceCode[Bindings.ChangeableNetworkProject._boundUISourceCode] !== uiSourceCode)
      return;
    ownedUISourceCode.setWorkingCopy(uiSourceCode.workingCopy());
    ownedUISourceCode.commitWorkingCopy();
    delete ownedUISourceCode[Bindings.ChangeableNetworkProject._boundUISourceCode];
  }

  /**
   * @param {!Common.Event} event
   */
  _uiSourceCodeCommitted(event) {
    var uiSourceCode = /** @type {!Workspace.UISourceCode} */ (event.data.uiSourceCode);
    var ownedUISourceCode = this.uiSourceCodeForURL(uiSourceCode.url());
    if (!ownedUISourceCode || ownedUISourceCode[Bindings.ChangeableNetworkProject._boundUISourceCode] !== uiSourceCode)
      return;
    ownedUISourceCode.setWorkingCopy(uiSourceCode.workingCopy());
    ownedUISourceCode.commitWorkingCopy();
  }

  /**
   * @param {!Common.Event} event
   */
  _uiSourceCodeChanged(event) {
    var uiSourceCode = /** @type {!Workspace.UISourceCode} */ (event.data.uiSourceCode);
    var url = uiSourceCode.url();
    var parsedURL = uiSourceCode.parsedURL();
    if (!parsedURL || parsedURL.isDataURL() || !parsedURL.isValid ||
        uiSourceCode.project().type() !== Workspace.projectTypes.Network)
      return;

    var newUISourceCode = this.uiSourceCodeForURL(url);
    if (!newUISourceCode) {
      var originalContentPromise = uiSourceCode.requestOriginalContent();
      var contentProvider =
          new Common.StaticContentProvider(url, uiSourceCode.contentType(), () => originalContentPromise);
      newUISourceCode = this.addContentProvider(url, contentProvider, uiSourceCode.mimeType());
    }
    newUISourceCode.setWorkingCopyGetter(() => uiSourceCode.workingCopy());
    newUISourceCode[Bindings.ChangeableNetworkProject._boundUISourceCode] = uiSourceCode;
  }
};

Bindings.ChangeableNetworkProject._boundUISourceCode = Symbol('ChangeableNetworkProject.BoundUISourceCode');
