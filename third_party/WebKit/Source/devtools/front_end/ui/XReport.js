// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @extends {UI.XElement}
 */
UI.XReport = class extends UI.XElement {
  constructor() {
    super();
    this._shadowRoot = UI.createShadowRootWithCoreStylesV1(this);
    this._fragment = UI.Fragment.cached`
      <x-vbox style='background: white'>
        <x-vbox $=header style='border-bottom: 1px solid rgb(230, 230, 230); padding: 12px 24px;'>
          <x-hbox $=title style='font-size: 15px'></x-hbox>
          <x-hbox $=subtitle style='font-size: 12px; margin-top: 10px' hidden></x-hbox>
          <x-hbox $=url style='font-size: 12px; margin-top: 10px' hidden></x-hbox>
          <x-hbox $=toolbar style='margin: 5px 0 -8px -8px;' hidden></x-hbox>
        </x-vbox>
        <x-vbox $=sections><slot></slot></x-vbox>
      </x-vbox>
    `;
    this._shadowRoot.appendChild(this._fragment.element());
    this.style.setProperty('background', '#f9f9f9');
    this.style.setProperty('overflow', 'auto');
  }

  /**
   * @return {!Array<string>}
   */
  static get observedAttributes() {
    // TODO(dgozman): should be super.observedAttributes, but it does not compile.
    return UI.XElement.observedAttributes.concat(['x-noscroll', 'x-noheader', 'x-title', 'x-subtitle']);
  }

  /**
   * @param {string} attr
   * @param {?string} oldValue
   * @param {?string} newValue
   * @override
   */
  attributeChangedCallback(attr, oldValue, newValue) {
    if (attr === 'x-noscroll') {
      this.style.setProperty('overflow', newValue ? 'visible' : 'auto');
      return;
    }
    if (attr === 'x-noheader') {
      this._fragment.$('header').hidden = !newValue;
      return;
    }
    if (attr === 'x-title') {
      this._fragment.$('title').textContent = newValue || '';
      return;
    }
    if (attr === 'x-subtitle') {
      this._fragment.$('subtitle').textContent = newValue || '';
      this._fragment.$('subtitle').hidden = !newValue;
      return;
    }
    super.attributeChangedCallback(attr, oldValue, newValue);
  }

  /**
   * @param {?Element} link
   */
  setURL(link) {
    this._fragment.$('url').removeChildren();
    if (link)
      this._fragment.$('url').appendChild(link);
    this._fragment.$('url').hidden = !link;
  }

  /**
   * @param {?UI.Toolbar} toolbar
   */
  setToolbar(toolbar) {
    this._fragment.$('toolbar').removeChildren();
    if (toolbar)
      this._fragment.$('toolbar').appendChild(toolbar.element);
    this._fragment.$('toolbar').hidden = !toolbar;
  }

  /**
   * @param {function(!Element, !Element): number} comparator
   */
  sortSections(comparator) {
    var sections = Array.prototype.slice.call(this._fragment.$('sections').children);
    var sorted = sections.every((e, i, a) => !i || comparator(a[i - 1], a[i]) <= 0);
    if (sorted)
      return;

    this._fragment.$('sections').removeChildren();
    sections.sort(comparator);
    for (var section of sections)
      this._fragment.$('sections').appendChild(section);
  }
};

/**
 * @extends {UI.XElement}
 */
UI.XReport.Section = class extends UI.XElement {
  constructor() {
    super();
    this._shadowRoot = UI.createShadowRootWithCoreStylesV1(this);
    this._fragment = UI.Fragment.cached`
      <x-vbox style='padding: 12px; border-bottom: 1px solid rgb(230, 230, 230);'>
        <x-hbox $=header style='align-items: center; margin-left: 18px'>
          <x-hbox $=title style='flex: auto; font-weight: bold; color: #555; white-space: nowrap; text-overflow: ellipsis; overflow: hidden;'></x-hbox>
        </x-hbox>
        <x-vbox><slot></slot></x-vbox>
      </x-vbox>
    `;
    this._shadowRoot.appendChild(this._fragment.element());
    this._toolbar = null;
  }

  /**
   * @return {!Array<string>}
   */
  static get observedAttributes() {
    // TODO(dgozman): should be super.observedAttributes, but it does not compile.
    return UI.XElement.observedAttributes.concat(['x-title']);
  }

  /**
   * @param {string} attr
   * @param {?string} oldValue
   * @param {?string} newValue
   * @override
   */
  attributeChangedCallback(attr, oldValue, newValue) {
    if (attr === 'x-title') {
      this._fragment.$('title').textContent = newValue || '';
      return;
    }
    super.attributeChangedCallback(attr, oldValue, newValue);
  }

  /**
   * @param {?UI.Toolbar} toolbar
   */
  setToolbar(toolbar) {
    if (this._toolbar)
      this._toolbar.element.remove();
    this._toolbar = toolbar;
    if (this._toolbar)
      this._fragment.$('header').appendChild(this._toolbar.element);
  }
};

/**
 * @extends {UI.XElement}
 */
UI.XReport.Field = class extends UI.XElement {
  constructor() {
    super();
    this._shadowRoot = UI.createShadowRootWithCoreStylesV1(this);
    this._fragment = UI.Fragment.cached`
      <x-hbox style='margin-top: 8px; line-height: 28px;'>
        <x-hbox $=title style='flex: 0 0 128px; white-space: pre; justify-content: flex-end; color: #888; padding: 0 6px;'></x-hbox>
        <x-div style='flex: auto; white-space: pre; padding: 0 6px;'><slot></slot></x-div>
      </x-hbox>
    `;
    this._shadowRoot.appendChild(this._fragment.element());
  }

  /**
   * @return {!Array<string>}
   */
  static get observedAttributes() {
    // TODO(dgozman): should be super.observedAttributes, but it does not compile.
    return UI.XElement.observedAttributes.concat(['x-title']);
  }

  /**
   * @param {string} attr
   * @param {?string} oldValue
   * @param {?string} newValue
   * @override
   */
  attributeChangedCallback(attr, oldValue, newValue) {
    if (attr === 'x-title') {
      this._fragment.$('title').textContent = newValue || '';
      return;
    }
    super.attributeChangedCallback(attr, oldValue, newValue);
  }
};

/**
 * @extends {UI.XHBox}
 */
UI.XReport.Row = class extends UI.XHBox {
  constructor() {
    super();
    this.style.setProperty('margin', '10px 0 2px 18px');
  }
};

// self.customElements.define('x-report', UI.XReport);
// self.customElements.define('x-report-section', UI.XReport.Section);
// self.customElements.define('x-report-field', UI.XReport.Field);
// self.customElements.define('x-report-row', UI.XReport.Row);
