// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


WorkspaceDiff.WorkspaceDiff = class extends Common.Object {
  /**
   * @param {!Workspace.Workspace} workspace
   */
  constructor(workspace) {
    super();
    /** @type {!Map<!Workspace.UISourceCode, !WorkspaceDiff.WorkspaceDiff.UISourceCodeDiff>} */
    // this._uiSourceCodeDiffs = new Map();

    /** @type {!Map<!Workspace.UISourceCode, !Promise>} */
    //this._loadingUISourceCodes = new Map();


    /** @type {!Map<string, !WorkspaceDiff.WorkspaceDiff.UrlDiff>} */
    this._urlDiffsForUrl = new Map();
    this._persistChanges = true;


    /** @type {!Set<!Workspace.UISourceCode>} */
    //this._modifiedUISourceCodes = new Set();
    // workspace.addEventListener(Workspace.Workspace.Events.WorkingCopyChanged, this._uiSourceCodeChanged, this);
    workspace.addEventListener(Workspace.Workspace.Events.WorkingCopyCommitted, this._uiSourceCodeCommitted, this);
    //workspace.addEventListener(Workspace.Workspace.Events.UISourceCodeAdded, this._uiSourceCodeAdded, this);
    //workspace.addEventListener(Workspace.Workspace.Events.UISourceCodeRemoved, this._uiSourceCodeRemoved, this);
    //workspace.addEventListener(Workspace.Workspace.Events.ProjectRemoved, this._projectRemoved, this);
    //workspace.uiSourceCodes().forEach(this._updateModifiedState.bind(this));
  }

  /**
   * @param {boolean} enabled
   */
  setPersistChanges(enabled) {
    for (var urlDiff of this._urlDiffsForUrl.values())
      urlDiff.setEnableInterception(enabled);
    this._persistChanges = enabled;
    // if (!enabled) {
    //   for (var interceptor of this._requestInterceptors.values())
    //     interceptor.release();
    //   this._requestInterceptors.clear();
    // } else {
    //   for (var uiSourceCode of this._uiSourceCodeDiffs.keys()) {
    //     var interceptor = this._requestInterceptors.get(uiSourceCode.url());
    //     if (!interceptor) {
    //       var interceptor = WorkspaceDiff.WorkspaceDiff.PersistChangesInterceptor.maybeInterceptorForUISoureCode(uiSourceCode);
    //       if (interceptor)
    //         this._requestInterceptors.set(uiSourceCode.url(), interceptor);
    //     } else {
    //       interceptor._addUISourceCode(uiSourceCode);
    //     }
    //   }
    // }
  }

  /**
   * @param {string} url
   * @return {!Promise<?Diff.Diff.DiffArray>}
   */
  // requestDiff(url) {
  //   var urlDiff = this._urlDiffsForUrl.get(url);
  //   if (!urlDiff)
  //     return Promise.resolve(null);
  //   return urlDiff.requestDiff();
  // }

  /**
   * @param {!WorkspaceDiff.WorkspaceDiff.UrlDiff} urlDiff
   * @param {function(!Common.Event)} callback
   * @param {!Object=} thisObj
   */
  subscribeToDiffChange(urlDiff, callback, thisObj) {
    urlDiff.addEventListener(WorkspaceDiff.Events.DiffChanged, callback, thisObj);
  }

  /**
   * @param {!WorkspaceDiff.WorkspaceDiff.UrlDiff} urlDiff
   * @param {function(!Common.Event)} callback
   * @param {!Object=} thisObj
   */
  unsubscribeFromDiffChange(urlDiff, callback, thisObj) {
    urlDiff.removeEventListener(WorkspaceDiff.Events.DiffChanged, callback, thisObj);
  }

  /**
   * @return {!Array<!WorkspaceDiff.WorkspaceDiff.UrlDiff>}
   */
  modifiedUrlDiffs() {
    return Array.from(this._urlDiffsForUrl.values());
  }

  /**
   * @param {string} url
   * @return {?WorkspaceDiff.WorkspaceDiff.UISourceCodeDiff}
   */
  // _urlDiff(url) {
  //   return this._urlDiffsForUrl.get(url) || null;
    // if (!this._uiSourceCodeDiffs.has(uiSourceCode))
    //   this._uiSourceCodeDiffs.set(uiSourceCode, new WorkspaceDiff.WorkspaceDiff.UISourceCodeDiff(uiSourceCode));
    // if (this._persistChanges) {
    //   var interceptor = this._requestInterceptors.get(uiSourceCode.url());
    //   if (!interceptor) {
    //     interceptor = WorkspaceDiff.WorkspaceDiff.PersistChangesInterceptor.maybeInterceptorForUISoureCode(uiSourceCode);
    //     if (interceptor)
    //       this._requestInterceptors.set(uiSourceCode.url(), interceptor);
    //   } else {
    //     interceptor._addUISourceCode(uiSourceCode);
    //   }
    // }
    // return this._uiSourceCodeDiffs.get(uiSourceCode);
  //}

  /**
   * @param {!Workspace.UISourceCode} uiSourceCode
   * @return {!WorkspaceDiff.WorkspaceDiff.UrlDiff}
   */
  _urlDiffForUISourceCode(uiSourceCode) {
    var url = uiSourceCode.url();
    var urlDiff = this._urlDiffsForUrl.get(url);
    if (urlDiff)
      return urlDiff;
    urlDiff = new WorkspaceDiff.WorkspaceDiff.UrlDiff(
        url, WorkspaceDiff.WorkspaceDiff._originalContentForUISourceCode(uiSourceCode));
    if (this._persistChanges)
      urlDiff.setEnableInterception(true);
    this._urlDiffsForUrl.set(url, urlDiff);
    return urlDiff;
  }

  /**
   * @param {!Common.Event} event
   */
  async _uiSourceCodeCommitted(event) {
    var uiSourceCode = /** @type {!Workspace.UISourceCode} */ (event.data.uiSourceCode);
    var newContent = /** @type {string} */ (event.data.content);
    if (!uiSourceCode.url())
      return;

    var urlDiff = this._urlDiffForUISourceCode(uiSourceCode);
    urlDiff._setNewContent(newContent);
    this._updateModifiedState(urlDiff);
  }

  /**
   * @param {!Common.Event} event
   */
  _uiSourceCodeAdded(event) {
    var uiSourceCode = /** @type {!Workspace.UISourceCode} */ (event.data);

    if (!uiSourceCode.isDirty() && !uiSourceCode.history().length)
      return;

    var urlDiff = this._urlDiffForUISourceCode(uiSourceCode);
    this._updateModifiedState(urlDiff);
  }

  /**
   * @param {!Common.Event} event
   */
  _uiSourceCodeRemoved(event) {
    var uiSourceCode = /** @type {!Workspace.UISourceCode} */ (event.data);
    //this._removeUISourceCode(uiSourceCode);
  }

  /**
   * @param {!Common.Event} event
   */
  _projectRemoved(event) {
    // var project = /** @type {!Workspace.Project} */ (event.data);
    // for (var uiSourceCode of project.uiSourceCodes())
    //   this._removeUISourceCode(uiSourceCode);
  }

  /**
   * @param {!Workspace.UISourceCode} uiSourceCode
   */
  // _removeUISourceCode(uiSourceCode) {
  //   this._loadingUISourceCodes.delete(uiSourceCode);
  //   var uiSourceCodeDiff = this._uiSourceCodeDiffs.get(uiSourceCode);
  //   if (uiSourceCodeDiff)
  //     uiSourceCodeDiff._dispose = true;
  //   this._markAsUnmodified(uiSourceCode, true);
  //   this._uiSourceCodeDiffs.delete(uiSourceCode);
  // }

  /**
   * @param {!WorkspaceDiff.WorkspaceDiff.UrlDiff} urlDiff
   */
  _markAsUnmodified(urlDiff) {
    // this._uiSourceCodeProcessedForTest();
    this.dispatchEventToListeners(WorkspaceDiff.Events.ModifiedStatusChanged, {urlDiff: urlDiff, isModified: false});
  }

  /**
   * @param {!WorkspaceDiff.WorkspaceDiff.UrlDiff} urlDiff
   */
  _markAsModified(urlDiff) {
    // this._uiSourceCodeProcessedForTest();
    this.dispatchEventToListeners(WorkspaceDiff.Events.ModifiedStatusChanged, {urlDiff: urlDiff, isModified: true});
  }

  _uiSourceCodeProcessedForTest() {
  }

  /**
   * @param {!WorkspaceDiff.WorkspaceDiff.UrlDiff} urlDiff
   */
  async _updateModifiedState(urlDiff) {
    //this._loadingUISourceCodes.delete(uiSourceCode);

    // if (!this._requestInterceptors.has(uiSourceCode.url())) {
    //   if (uiSourceCode.isDirty()) {
    //     this._markAsModified(uiSourceCode);
    //     return;
    //   }
    //   if (!uiSourceCode.history().length) {
    //     this._markAsUnmodified(uiSourceCode);
    //     return;
    //   }
    // }

    var originalContent = await urlDiff._originalContent;
    if (originalContent === null || originalContent === urlDiff._newContent) {
      urlDiff.setEnableInterception(false);
      this._markAsUnmodified(urlDiff);
    } else {
      urlDiff.setEnableInterception(this._persistChanges);
      this._markAsModified(urlDiff);
    }


    // var contentsPromise = Promise.all([WorkspaceDiff.WorkspaceDiff._originalContentForUISourceCode(uiSourceCode), uiSourceCode.requestContent()]);
    // this._loadingUISourceCodes.set(uiSourceCode, contentsPromise);
    // var contents = await contentsPromise;
    // if (this._loadingUISourceCodes.get(uiSourceCode) !== contentsPromise)
    //   return;
    // this._loadingUISourceCodes.delete(uiSourceCode);

    // if (contents[0] !== null && contents[1] !== null && contents[0] !== contents[1])
    //   this._markAsModified(uiSourceCode);
    // else
    //   this._markAsUnmodified(uiSourceCode);
  }

  static _originalContentForUISourceCode(uiSourceCode) {
    var target = uiSourceCode.project()[Bindings.NetworkProject._targetSymbol];
    if (!target)
      return uiSourceCode.requestOriginalContent();
    var contentProvider = SDK.interceptedURLManager.contentProviderForTargetAndUrl(target, uiSourceCode.contentURL());
    if (contentProvider) {
      uiSourceCode._originalContentPromise = contentProvider.requestContent();
      return uiSourceCode._originalContentPromise;
    }
    return uiSourceCode.requestOriginalContent();
  }
};

WorkspaceDiff.WorkspaceDiff.UrlDiff = class extends Common.Object {
  /**
   * @param {string} url
   * @param {!Promise<?string>} originalContent
   */
  constructor(url, originalContent) {
    super();

    this._url = url;
    this._originalContent = originalContent;
    this._newContent = '';

    /** @type {?WorkspaceDiff.WorkspaceDiff.PersistChangesInterceptor} */
    this._requestInterceptor = null;
  }

  /**
   * @return {!Common.ParsedURL}
   */
  parsedURL() {
    return this._url.asParsedURL();
  }

  /**
   * @return {string}
   */
  mimeType() {
    return Common.ResourceType.mimeFromURL(this._url);
  }

  /**
   * @param {boolean} enabled
   */
  setEnableInterception(enabled) {
    if (enabled !== !!this._requestInterceptor)
    if (this._requestInterceptor)
      this._requestInterceptor.release();
    this._requestInterceptor = enabled ? new WorkspaceDiff.WorkspaceDiff.PersistChangesInterceptor(this._url, () => this._newContent, this._originalContent) : null;
  }

  /**
   * @param {string} newContent
   */
  _setNewContent(newContent) {
    this._newContent = newContent;
    this._requestDiffPromise = null;
    this.dispatchEventToListeners(WorkspaceDiff.Events.DiffChanged);
  }

  // /**
  //  * @return {!Promise<?Diff.Diff.DiffArray>}
  //  */
  requestDiff() {
    if (!this._requestDiffPromise)
      this._requestDiffPromise = this._innerRequestDiff();
    return this._requestDiffPromise;
  }

  /**
   * @return {!Promise<?Diff.Diff.DiffArray>}
   */
  async _innerRequestDiff() {
    var originalContent = await this._originalContent;
    if (originalContent === null || this._newContent === null)
      return null;
    return Diff.Diff.lineDiff(originalContent.split('\n'), this._newContent.split('\n'));
  }
};

/**
 * @enum {symbol}
 */
WorkspaceDiff.Events = {
  DiffChanged: Symbol('DiffChanged'),
  ModifiedStatusChanged: Symbol('ModifiedStatusChanged')
};

/**
 * @return {!WorkspaceDiff.WorkspaceDiff}
 */
WorkspaceDiff.workspaceDiff = function() {
  if (!WorkspaceDiff.WorkspaceDiff._instance)
    WorkspaceDiff.WorkspaceDiff._instance = new WorkspaceDiff.WorkspaceDiff(Workspace.workspace);
  return WorkspaceDiff.WorkspaceDiff._instance;
};

WorkspaceDiff.WorkspaceDiff.PersistChangesInterceptor = class extends SDK.RequestInterceptor {
  /**
   * @param {string} url
   * @param {function():string} newContentResolver
   * @param {string} mimeType
   * @param {string} originalContent
   */
  constructor(url, newContentResolver, mimeType, originalContent) {
    super(new Set([url]));

    this._newContentResolver = newContentResolver;
    this._mimeType = Common.ResourceType.mimeFromURL(url) || 'text/html';
    this._originalContent = originalContent;
    this._released = false;
  }

  // /**
  //  * @parma {!Workspace.UISourceCode} uiSourceCode
  //  * @return {?WorkspaceDiff.WorkspaceDiff.PersistChangesInterceptor}
  //  */
  // static maybeInterceptorForUISoureCode(uiSourceCode) {
  //   var target = uiSourceCode.project()[Bindings.NetworkProject._targetSymbol];
  //   if (!target)
  //     return null;
  //   var newContentResolver = uiSourceCode.requestContent.bind(uiSourceCode);
  //   var mimeType = uiSourceCode.mimeType();
  //   var originalContent = uiSourceCode.requestOriginalContent();
  //   var url = uiSourceCode.url();
  //   return new WorkspaceDiff.WorkspaceDiff.PersistChangesInterceptor(uiSourceCode, newContentResolver, mimeType, originalContent, url);
  // }

  /**
   * @override
   * @return {!Promise}
   */
  release() {
    this._released = true;
    return super.release();
  }

  /**
   * @override
   * @param {!SDK.InterceptedRequest} interceptedRequest
   * @return {!Promise}
   */
  async handle(interceptedRequest) {
    if (this._released)
      return;
    var content = await this._newContentResolver();
    interceptedRequest.continueRequestWithContent(content, this._mimeType, this._originalContent);
  }
};