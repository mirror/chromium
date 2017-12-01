// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

UI.Fragment = class extends Element {
  constructor() {
    super();
    throw 'Do not instantiate UI.Fragment';
  }

  /**
   * @param {string} tId
   * @return {!Element}
   */
  $(tId) {
    return this;
  }

  /**
   * @param {string} state
   * @param {boolean} toggled
   */
  setState(state, toggled) {
  }

  /**
   * @return {!UI._Widget}
   */
  asWidget() {
    return /** @type {!UI._Widget} */ (/** @type {!Element} */ (this));
  }

  /**
   * @param {!Array<string>} strings
   * @param {...*} vararg
   * @return {!UI.Fragment}
   */
  static build(strings, vararg) {
    var values = Array.prototype.slice.call(arguments, 1);
    return new UI._Template(strings).render(values);
  }

  /**
   * @param {!Array<string>} strings
   * @param {...*} vararg
   * @return {!UI.Fragment}
   */
  static cached(strings, vararg) {
    var values = Array.prototype.slice.call(arguments, 1);
    var template = UI._Template._templateCache.get(strings);
    if (!template) {
      template = new UI._Template(strings);
      UI._Template._templateCache.set(strings, template);
    }
    return template.render(values);
  }
};

UI._Template = class {
  /**
   * @param {!Array<string>} strings
   * @suppressGlobalPropertiesCheck
   */
  constructor(strings) {
    var html = '';
    var insideText = false;
    for (var i = 0; i < strings.length - 1; i++) {
      html += strings[i];
      var close = strings[i].lastIndexOf('>');
      if (close !== -1) {
        if (strings[i].indexOf('<', close + 1) === -1)
          insideText = true;
        else
          insideText = false;
      }
      html += insideText ? UI._Template._textMarker : UI._Template._attributeMarker(i);
    }
    html += strings[strings.length - 1];

    this._template = createElement('template');
    this._template.innerHTML = html;
    var walker =
        document.createTreeWalker(this._template.content, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, null, false);
    var valueIndex = 0;
    var nodeIndex = 0;
    var emptyTextNodes = [];
    this._binds = [];
    while (walker.nextNode()) {
      var node = walker.currentNode;
      if (node.nodeType === Node.ELEMENT_NODE && node.hasAttributes()) {
        if (node.hasAttribute('$')) {
          this._binds.push({nodeIndex, bindName: node.getAttribute('$')});
          node.removeAttribute('$');
        }

        var attributesToRemove = [];
        for (var i = 0; i < node.attributes.length; i++) {
          var name = node.attributes[i].name;

          if (name.startsWith('s-')) {
            attributesToRemove.push(name);
            name = name.substring(2);
            var state = name.substring(0, name.indexOf('-'));
            var attr = name.substring(state.length + 1);
            this._binds.push({nodeIndex, state, attr, value: node.attributes[i].value});
            continue;
          }

          if (!UI._Template._attributeMarkerRegex.test(name) &&
              !UI._Template._attributeMarkerRegex.test(node.attributes[i].value))
            continue;

          attributesToRemove.push(name);
          var bind = {nodeIndex, attrIndex: valueIndex};
          bind.attrNames = name.split(UI._Template._attributeMarkerRegex);
          valueIndex += bind.attrNames.length - 1;
          bind.attrValues = node.attributes[i].value.split(UI._Template._attributeMarkerRegex);
          valueIndex += bind.attrValues.length - 1;
          this._binds.push(bind);
        }
        for (var i = 0; i < attributesToRemove.length; i++)
          node.removeAttribute(attributesToRemove[i]);
        ++nodeIndex;
      } else if (node.nodeType === Node.TEXT_NODE && node.nodeValue.indexOf(UI._Template._textMarker) !== -1) {
        var texts = node.nodeValue.split(UI._Template._textMarkerRegex);
        node.data = texts[texts.length - 1];
        for (var i = 0; i < texts.length - 1; i++) {
          if (texts[i]) {
            ++nodeIndex;
            node.parentNode.insertBefore(createTextNode(texts[i]), node);
          }
          this._binds.push({nodeIndex, replaceNodeIndex: valueIndex++});
          ++nodeIndex;
          node.parentNode.insertBefore(createTextNode(''), node);
        }
        ++nodeIndex;
      } else if (node.nodeType === Node.TEXT_NODE) {
        if ((!node.previousSibling || node.previousSibling.nodeType === Node.ELEMENT_NODE) &&
            (!node.nextSibling || node.nextSibling.nodeType === Node.ELEMENT_NODE) && /^\s*$/.test(node.nodeValue))
          emptyTextNodes.push(node);
        else
          ++nodeIndex;
      } else {
        ++nodeIndex;
      }
    }
    for (var emptyTextNode of emptyTextNodes)
      emptyTextNode.parentNode.removeChild(emptyTextNode);
  }

  /**
   * @param {!Array<*>} values
   * @return {!UI.Fragment}
   */
  render(values) {
    var bindIndex = 0;
    var nodeIndex = 0;
    var content = this._template.ownerDocument.importNode(this._template.content, true);
    var result = /** @type {!Element} */ (content.firstChild === content.lastChild ? content.firstChild : content);
    result._states = new Map();

    // TODO(dgozman): switch to child index paths or querySelector if this is slow.
    var walker = this._template.ownerDocument.createTreeWalker(
        content, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, null, false);
    var replacedNodes = [];
    var idByElement = new Map();
    while (walker.nextNode()) {
      var node = walker.currentNode;
      while (bindIndex < this._binds.length && this._binds[bindIndex].nodeIndex === nodeIndex) {
        var bind = this._binds[bindIndex++];
        if ('bindName' in bind) {
          if (!result._elementsById)
            result._elementsById = new Map();
          result._elementsById.set(bind.bindName, node);
          idByElement.set(node, bind.bindName);
        } else if ('replaceNodeIndex' in bind) {
          var value = values[bind.replaceNodeIndex];
          if (value instanceof Node) {
            node.parentNode.insertBefore(value, node);
            replacedNodes.push(node);
          } else {
            node.data = '' + value;
          }
        } else if ('state' in bind) {
          var list = result._states.get(bind.state) || [];
          list.push({name: bind.attr, value: bind.value, node, toggled: false});
          result._states.set(bind.state, list);
        } else if ('attrIndex' in bind) {
          if (bind.attrNames.length === 2 && bind.attrValues.length === 1 &&
              typeof values[bind.attrIndex] === 'function') {
            values[bind.attrIndex].call(null, result, node);
          } else {
            var name = bind.attrNames[0];
            for (var i = 1; i < bind.attrNames.length; i++) {
              name += values[bind.attrIndex + i - 1];
              name += bind.attrNames[i];
            }
            var value = bind.attrValues[0];
            for (var i = 1; i < bind.attrValues.length; i++) {
              value += values[bind.attrIndex + bind.attrNames.length - 1 + i - 1];
              value += bind.attrValues[i];
            }
            if (name)
              node.setAttribute(name, value);
          }
        } else {
          throw 'Unexpected bind';
        }
      }

      ++nodeIndex;
    }

    for (var replacedNode of replacedNodes)
      replacedNode.parentNode.removeChild(replacedNode);

    // TODO(dgozman): move to constructor if this turns out to be slow.
    var shadows = result.querySelectorAll('x-shadow');
    for (var shadow of shadows) {
      if (!shadow.parentElement)
        throw 'There must be a parent element here';
      var shadowRoot = UI.createShadowRootWithCoreStyles(shadow.parentElement);
      if (shadow.parentElement.tagName === 'X-WIDGET')
        shadow.parentElement._shadowRoot = shadowRoot;
      while (shadow.firstChild)
        shadowRoot.appendChild(shadow.firstChild);
      var id = idByElement.get(shadow);
      if (id)
        result._elementsById.set(id, shadowRoot);
    }

    /** @type {function(string):!Element} */
    result.$ = UI._Template._byId.bind(null, result);

    /** @type {function(string, boolean)} */
    result.setState = UI._Template._setState.bind(null, result);

    /** @type {function():!UI._Widget} */
    result.asWidget = UI._Template._asWidget.bind(null, result);

    return /** @type {!UI.Fragment} */ (result);
  }

  /**
   * @param {!Element} element
   * @param {string} name
   * @return {!Element}
   */
  static _byId(element, name) {
    return element._elementsById.get(name);
  }

  /**
   * @param {!Element} element
   * @param {string} name
   * @param {boolean} toggled
   */
  static _setState(element, name, toggled) {
    var list = element._states.get(name);
    if (list === undefined) {
      console.error('Unknown state ' + name);
      return;
    }
    for (var state of list) {
      if (state.toggled === toggled)
        continue;
      state.toggled = toggled;
      var value = state.value;
      state.value = state.node.getAttribute(state.name);
      if (value === null)
        state.node.removeAttribute(state.name);
      else
        state.node.setAttribute(state.name, value);
    }
  }

  /**
   * @param {!Element} element
   * @return {!UI._Widget}
   */
  static _asWidget(element) {
    if (element.tagName !== 'X-WIDGET')
      throw `Cannot cast ${element.tagName} to x-widget`;
    return /** @type {!UI._Widget} */ (element);
  }
};

UI._Template._textMarker = '{{template-text}}';
UI._Template._textMarkerRegex = /{{template-text}}/;

UI._Template._attributeMarker = index => 'template-attribute' + index;
UI._Template._attributeMarkerRegex = /template-attribute\d+/;

UI._Template._templateCache = new Map();
