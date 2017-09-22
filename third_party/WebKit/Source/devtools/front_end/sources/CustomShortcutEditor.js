// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Sources.CustomShortcutsProject = class extends Bindings.ContentProviderBasedProject {
  constructor() {
    super(Workspace.workspace, 'shortcuts:', Workspace.projectTypes.Shortcuts, '', false /* isServiceProject */);
    this._setting = Common.settings.createSetting('customShortcutText', '{\n}');
    this._uiSourceCode = this.addContentProvider(
        'custom-shortcuts.json',
        Common.StaticContentProvider.fromString(
            'custom-shortcuts.json', Common.resourceTypes.Other, this._setting.get()),
        'application/json');
  }

  static show() {
    if (!this._instance)
      this._instance = new this();
    Common.Revealer.reveal(this._instance._uiSourceCode);
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
   * @param {function(string?)} callback
   */
  setFileContent(uiSourceCode, newContent, callback) {
    this._setting.set(newContent);
    try {
      var object = JSON.parse(newContent);
      if (typeof object !== 'object' || Array.isArray(object))
        return;
      UI.shortcutRegistry.setCustomKeyboardShortcuts(/** @type {!Object<string, string>} */ (object));
    } catch (error) {
    }
    callback('');
  }
};


/**
 * @implements {UI.ActionDelegate}
 */
Sources.CustomShortcutsProject.ActionDelegate = class {
  /**
   * @override
   * @return {boolean}
   */
  handleAction() {
    Sources.CustomShortcutsProject.show();
    return true;
  }
};

Sources.CustomShortcutsFrame = class extends SourceFrame.UISourceCodeFrame {
  /**
   * @param {!Workspace.UISourceCode} uiSourceCode
   */
  constructor(uiSourceCode) {
    super(uiSourceCode);
    this.configureAutocomplete(
        {suggestionsCallback: this._suggestions.bind(this), substituteRangeCallback: this._substituteRange.bind(this)});
    this._actionIds = self.runtime.extensions('action').map(extension => extension.descriptor()['actionId']);
  }

  /**
   * @param {!TextUtils.TextRange} queryRange
   * @param {!TextUtils.TextRange} substituteRange
   * @param {boolean=} force
   * @return {?Promise.<!UI.SuggestBox.Suggestions>}
   */
  _suggestions(queryRange, substituteRange, force) {
    if (!force && substituteRange.startColumn === substituteRange.endColumn)
      return Promise.resolve([]);
    var token = this.textEditor.tokenAtTextPosition(substituteRange.startLine, substituteRange.startColumn);
    if (!token || token.type.indexOf('js-property') === -1)
      return Promise.resolve([]);
    var query = this.textEditor.text(queryRange);
    query = query.replace(/[^A-z.-_]/g, '').toLowerCase();

    var prefixCompletions = [];
    var anywhereCompletions = [];

    this._actionIds.forEach(actionId => {
      var lower = actionId.toLowerCase();
      if (lower.startsWith(query))
        prefixCompletions.push(actionId);
      else if (lower.indexOf(query) !== -1)
        anywhereCompletions.push(actionId);
    });

    return Promise.resolve(
        prefixCompletions.concat(anywhereCompletions).map(completion => ({text: '"' + completion + '"'})));
  }

  /**
   * @param {number} lineNumber
   * @param {number} columnNumber
   * @return {?TextUtils.TextRange}
   */
  _substituteRange(lineNumber, columnNumber) {
    var lineText = this.textEditor.line(lineNumber);
    var start;
    for (start = columnNumber - 1; start >= 0; start--) {
      if (' :{}[],\t\r\n'.indexOf(lineText.charAt(start)) !== -1)
        break;
    }
    var end;
    for (end = columnNumber; end < lineText.length; end++) {
      if (' :{}[],\t\r\n'.indexOf(lineText.charAt(end)) !== -1)
        break;
    }
    return new TextUtils.TextRange(lineNumber, start + 1, lineNumber, end);
  }
};
