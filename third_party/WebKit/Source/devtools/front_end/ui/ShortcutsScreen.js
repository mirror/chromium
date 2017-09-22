/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @unrestricted
 */
UI.ShortcutsScreen = class extends UI.SimpleView {
  constructor() {
    super(Common.UIString('Shortcuts'));
    /** @type {!Map<string, !UI.ShortcutsSection>} */
    this._sections = new Map();
    UI.shortcutRegistry.addEventListener(UI.ShortcutRegistry.Events.ShortcutsChanged, this._updateShortcuts.bind(this));
    this.element.classList.add('settings-tab-container');

    this._updateShortcuts();
  }

  _updateShortcuts() {
    // set order of some sections explicitly
    this.section(Common.UIString('Elements Panel'));
    this.section(Common.UIString('Styles Pane'));
    this.section(Common.UIString('Debugger'));
    this.section(Common.UIString('Console'));

    self.runtime.extensions('action').forEach(extension => {
      var descriptor = extension.descriptor();
      if (!descriptor['helpSection'])
        return;
      var keys = UI.shortcutRegistry.shortcutDescriptorsForAction(descriptor['actionId']);
      if (!keys.length)
        return;
      var section = this.section(descriptor['helpSection']);
      section.addAlternateKeys(keys, descriptor['helpDescription'] || descriptor['title']);
    });

    // Elements panel
    var elementsSection = this.section(Common.UIString('Elements Panel'));

    elementsSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.NavigateUp, Common.UIString('Navigate elements'));
    elementsSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.NavigateDown, Common.UIString('Navigate elements'));

    elementsSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.Expand, Common.UIString('Expand/collapse'));
    elementsSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.Collapse, Common.UIString('Expand/collapse'));

    elementsSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.EditAttribute, Common.UIString('Edit attribute'));
    elementsSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('elements.hide-element'), Common.UIString('Hide element'));
    elementsSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('elements.edit-as-html'),
        Common.UIString('Toggle edit as HTML'));

    var stylesPaneSection = this.section(Common.UIString('Styles Pane'));

    stylesPaneSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.NextProperty, Common.UIString('Next/previous property'));
    stylesPaneSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.PreviousProperty, Common.UIString('Next/previous property'));

    stylesPaneSection.addAlternateKeys(UI.ShortcutsScreen.ElementsPanelShortcuts.IncrementValue, ('Increment value'));
    stylesPaneSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.DecrementValue, Common.UIString('Decrement value'));

    stylesPaneSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.IncrementBy10, Common.UIString('Increment by %f', 10));
    stylesPaneSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.DecrementBy10, Common.UIString('Decrement by %f', 10));

    stylesPaneSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.IncrementBy100, Common.UIString('Increment by %f', 100));
    stylesPaneSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.DecrementBy100, Common.UIString('Decrement by %f', 100));

    stylesPaneSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.IncrementBy01, Common.UIString('Increment by %f', 0.1));
    stylesPaneSection.addAlternateKeys(
        UI.ShortcutsScreen.ElementsPanelShortcuts.DecrementBy01, Common.UIString('Decrement by %f', 0.1));

    // Editing
    var textEditorSection = UI.shortcutsScreen.section(Common.UIString('Text Editor'));
    textEditorSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('sources.go-to-member'), Common.UIString('Go to member'));
    textEditorSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('editor.autocomplete'), Common.UIString('Autocompletion'));
    textEditorSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('sources.go-to-line'), Common.UIString('Go to line'));
    textEditorSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('sources.jump-to-previous-location'),
        Common.UIString('Jump to previous editing location'));
    textEditorSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('sources.jump-to-next-location'),
        Common.UIString('Jump to next editing location'));
    textEditorSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('editor.toggle-comment'), Common.UIString('Toggle comment'));
    textEditorSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('editor.select-next-occurrence'),
        Common.UIString('Select next occurrence'));
    textEditorSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('editor.undo-last-selection'), Common.UIString('Soft undo'));
    textEditorSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('editor.go-to-matching-bracket'),
        Common.UIString('Go to matching bracket'));
    textEditorSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('sources.close-editor-tab'),
        Common.UIString('Close editor tab'));
    textEditorSection.addAlternateKeys(
        UI.shortcutRegistry.shortcutDescriptorsForAction('sources.switch-file'),
        Common.UIString('Switch between files with the same name and different extensions.'));

    // Console
    var consoleSection = UI.shortcutsScreen.section(Common.UIString('Console'));

    consoleSection.addAlternateKeys(
        [
          UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Tab),
          UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Right)
        ],
        Common.UIString('Accept suggestion'));

    consoleSection.addAlternateKeys(
        [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Down)], Common.UIString('Next/previous line'));
    consoleSection.addAlternateKeys(
        [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Up)], Common.UIString('Next/previous line'));

    consoleSection.addAlternateKeys(
        [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Enter)], Common.UIString('Execute command'));
  }

  /**
   * @param {string} name
   * @return {!UI.ShortcutsSection}
   */
  section(name) {
    var section = this._sections.get(name);
    if (!section)
      this._sections.set(name, section = new UI.ShortcutsSection(name));
    return section;
  }
};

/**
 * We cannot initialize it here as localized strings are not loaded yet.
 * @type {!UI.ShortcutsScreen}
 */
UI.shortcutsScreen;

/**
 * @unrestricted
 */
UI.ShortcutsSection = class {
  /**
   * @param {string} name
   */
  constructor(name) {
    this.name = name;
    this._lines = /** @type {!Array.<!{key: !Node, text: string}>} */ ([]);
  }

  /**
   * @param {!Array.<!UI.KeyboardShortcut.Descriptor>} keys
   * @param {string} description
   */
  addAlternateKeys(keys, description) {
    var keysNode = this._joinNodes(
        keys.map(this._renderKey.bind(this)), this._createSpan('help-key-delimiter', Common.UIString('or')));
    for (var line of this._lines) {
      if (line.text === description) {
        line.key = this._joinNodes([line.key, keysNode], this._createSpan('help-key-delimiter', '/'));
        return;
      }
    }
    this._lines.push({key: keysNode, text: description});
  }

  /**
   * @param {!Element} container
   */
  renderSection(container) {
    var parent = container.createChild('div', 'help-block');

    var headLine = parent.createChild('div', 'help-line');
    headLine.createChild('div', 'help-key-cell');
    headLine.createChild('div', 'help-section-title help-cell').textContent = this.name;

    for (var i = 0; i < this._lines.length; ++i) {
      var line = parent.createChild('div', 'help-line');
      var keyCell = line.createChild('div', 'help-key-cell');
      keyCell.appendChild(this._lines[i].key);
      keyCell.appendChild(this._createSpan('help-key-delimiter', ':'));
      line.createChild('div', 'help-cell').textContent = this._lines[i].text;
    }
  }

  /**
   * @param {!UI.KeyboardShortcut.Descriptor} key
   * @return {!Node}
   */
  _renderKey(key) {
    var keyName = key.name;
    var plus = this._createSpan('help-combine-keys', '+');
    return this._joinNodes(keyName.split(' + ').map(this._createSpan.bind(this, 'help-key')), plus);
  }

  /**
   * @param {string} className
   * @param {string} textContent
   * @return {!Element}
   */
  _createSpan(className, textContent) {
    var node = createElement('span');
    node.className = className;
    node.textContent = textContent;
    return node;
  }

  /**
   * @param {!Array.<!Element>} nodes
   * @param {!Element} delimiter
   * @return {!Node}
   */
  _joinNodes(nodes, delimiter) {
    var result = createDocumentFragment();
    for (var i = 0; i < nodes.length; ++i) {
      if (i > 0)
        result.appendChild(delimiter.cloneNode(true));
      result.appendChild(nodes[i]);
    }
    return result;
  }
};


UI.ShortcutsScreen.ElementsPanelShortcuts = {
  NavigateUp: [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Up)],

  NavigateDown: [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Down)],

  Expand: [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Right)],

  Collapse: [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Left)],

  EditAttribute: [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Enter)],

  NextProperty: [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Tab)],

  PreviousProperty:
      [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Tab, UI.KeyboardShortcut.Modifiers.Shift)],

  IncrementValue: [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Up)],

  DecrementValue: [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Down)],

  IncrementBy10: [
    UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.PageUp),
    UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Up, UI.KeyboardShortcut.Modifiers.Shift)
  ],

  DecrementBy10: [
    UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.PageDown),
    UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Down, UI.KeyboardShortcut.Modifiers.Shift)
  ],

  IncrementBy100:
      [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.PageUp, UI.KeyboardShortcut.Modifiers.Shift)],

  DecrementBy100:
      [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.PageDown, UI.KeyboardShortcut.Modifiers.Shift)],

  IncrementBy01: [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Up, UI.KeyboardShortcut.Modifiers.Alt)],

  DecrementBy01: [UI.KeyboardShortcut.makeDescriptor(UI.KeyboardShortcut.Keys.Down, UI.KeyboardShortcut.Modifiers.Alt)]
};

