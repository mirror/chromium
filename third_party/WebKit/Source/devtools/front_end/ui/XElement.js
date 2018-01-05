// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @extends {HTMLElement}
 */
UI.XElement = class extends HTMLElement {
  static get observedAttributes() {
    return [
      'flex',          'padding',     'padding-top',      'padding-bottom', 'padding-left',
      'padding-right', 'margin',      'margin-top',       'margin-bottom',  'margin-left',
      'margin-right',  'overflow',    'overflow-x',       'overflow-y',     'font-size',
      'color',         'background',  'background-color', 'border',         'border-top',
      'border-bottom', 'border-left', 'border-right',     'max-width',      'max-height',
      'line-height', 'text', 'hidden', 'font-weight', 'justify-content'
    ];
  }

  /**
   * @param {string} attr
   * @param {?string} oldValue
   * @param {?string} newValue
   * @override
   */
  attributeChangedCallback(attr, oldValue, newValue) {
    if (attr === 'hidden') {
      if (newValue !== null) {
        this._display = this.style.getPropertyValue('display');
        this.style.setProperty('display', 'none');
      } else {
        if (this._display)
          this.style.setProperty('display', this._display);
        this._display = null;
      }
    }
    if (attr === 'flex') {
      if (newValue === null)
        this.style.removeProperty('flex');
      else if (newValue === 'initial' || newValue === 'auto' || newValue === 'none' || newValue.indexOf(' ') !== -1)
        this.style.setProperty('flex', newValue);
      else
        this.style.setProperty('flex', '0 0 ' + newValue);
      return;
    }
    if (attr === 'text') {
      if (oldValue === 'ellipsis') {
        this.style.removeProperty('white-space');
        this.style.removeProperty('text-overflow');
        this.style.removeProperty('overflow');
      } else if (oldValue === 'pre') {
        this.style.setProperty('white-space', 'pre');
      }
      if (newValue === 'ellipsis') {
        this.style.setProperty('white-space', 'nowrap');
        this.style.setProperty('text-overflow', 'ellipsis');
        this.style.setProperty('overflow', 'hidden');
      } else if (newValue === 'pre') {
        this.style.setProperty('white-space', 'pre');
      }
      return;
    }
    if (newValue === null) {
      this.style.removeProperty(attr);
      if (attr.startsWith('padding-') || attr.startsWith('margin-') || attr.startsWith('border-') ||
          attr.startsWith('background-') || attr.startsWith('overflow-')) {
        var shorthand = attr.substring(0, attr.indexOf('-'));
        var shorthandValue = this.getAttribute(shorthand);
        if (shorthandValue !== null)
          this.style.setProperty(shorthand, shorthandValue);
      }
    } else {
      this.style.setProperty(attr, newValue);
    }
  }

  /**
   * @return {boolean}
   */
  get hidden() {
    return this.hasAttribute('hidden');
  }

  /**
   * @param {boolean} value
   */
  set hidden(value) {
    if (value)
      this.setAttribute('hidden', '');
    else
      this.removeAttribute('hidden');
  }
};

/**
 * @extends {UI.XElement}
 */
UI._XBox = class extends UI.XElement {
  /**
   * @param {string} direction
   */
  constructor(direction) {
    super();
    this.style.setProperty('display', 'flex');
    this.style.setProperty('flex-direction', direction);
    this.style.setProperty('justify-content', 'flex-start');
  }

  static get observedAttributes() {
    // TODO(dgozman): should be super.observedAttributes, but does not compile.
    return UI.XElement.observedAttributes.concat(['x-start', 'x-center', 'x-stretch', 'x-baseline', 'justify-content']);
  }

  /**
   * @param {string} attr
   * @param {?string} oldValue
   * @param {?string} newValue
   * @override
   */
  attributeChangedCallback(attr, oldValue, newValue) {
    if (attr === 'x-start' || attr === 'x-center' || attr === 'x-stretch' || attr === 'x-baseline') {
      if (newValue === null)
        this.style.removeProperty('align-items');
      else
        this.style.setProperty('align-items', attr === 'x-start' ? 'flex-start' : attr.substr(2));
      return;
    }
    super.attributeChangedCallback(attr, oldValue, newValue);
  }
};

/**
 * @extends {UI._XBox}
 */
UI.XVBox = class extends UI._XBox {
  constructor() {
    super('column');
  }
};

/**
 * @extends {UI._XBox}
 */
UI.XHBox = class extends UI._XBox {
  constructor() {
    super('row');
  }
};

/**
 * @extends {UI.XElement}
 */
UI.XCBox = class extends UI.XElement {
  constructor() {
    super();
    this.style.setProperty('display', 'flex');
    this.style.setProperty('flex-direction', 'column');
    this.style.setProperty('justify-content', 'center');
    this.style.setProperty('align-items', 'center');
  }
};

/**
 * @extends {UI.XElement}
 */
UI.XDiv = class extends UI.XElement {
  constructor() {
    super();
    this.style.setProperty('display', 'block');
  }
};

/**
 * @extends {UI.XElement}
 */
UI.XSpan = class extends UI.XElement {
  constructor() {
    super();
    this.style.setProperty('display', 'inline');
  }
};

// self.customElements.define('x-vbox', UI.XVBox);
// self.customElements.define('x-hbox', UI.XHBox);
// self.customElements.define('x-cbox', UI.XCBox);
// self.customElements.define('x-div', UI.XDiv);
// self.customElements.define('x-span', UI.XSpan);
