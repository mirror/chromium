// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

UI.Fragment = class {
  /**
   * @param {!Element} element
   */
  constructor(element) {
    this._element = element;

    /** @type {!Map<string, !Array<!UI.Fragment._State>>} */
    this._states = new Map();

    /** @type {!Map<string, !Element>} */
    this._elementsById = new Map();
  }

  /**
   * @return {!Element}
   */
  element() {
    return this._element;
  }

  /**
   * @param {string} elementId
   * @return {!Element}
   */
  $(elementId) {
    return this._elementsById.get(elementId);
  }

  /**
   * @param {string} name
   * @param {boolean} toggled
   */
  setState(name, toggled) {
    var list = this._states.get(name);
    if (list === undefined) {
      console.error('Unknown state ' + name);
      return;
    }
    for (var state of list) {
      if (state.toggled === toggled)
        continue;
      state.toggled = toggled;
      var value = state.attributeValue;
      state.attributeValue = state.element.getAttribute(state.attributeName);
      if (value === null)
        state.element.removeAttribute(state.attributeName);
      else
        state.element.setAttribute(state.attributeName, value);
    }
  }

  /**
   * @param {!Array<string>} strings
   * @param {...*} vararg
   * @return {!UI.Fragment}
   */
  static build(strings, vararg) {
    var values = Array.prototype.slice.call(arguments, 1);
    return UI.Fragment._render(UI.Fragment._template(strings), values);
  }

  /**
   * @param {!Array<string>} strings
   * @param {...*} vararg
   * @return {!UI.Fragment}
   */
  static cached(strings, vararg) {
    var values = Array.prototype.slice.call(arguments, 1);
    var template = UI.Fragment._templateCache.get(strings);
    if (!template) {
      template = UI.Fragment._template(strings);
      UI.Fragment._templateCache.set(strings, template);
    }
    return UI.Fragment._render(template, values);
  }

  /**
   * @param {!Array<string>} strings
   * @return {!UI.Fragment._Template}
   * @suppressGlobalPropertiesCheck
   */
  static _template(strings) {
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
      html += insideText ? UI.Fragment._textMarker : UI.Fragment._attributeMarker(i);
    }
    html += strings[strings.length - 1];

    var template = createElement('template');
    template.innerHTML = html;
    var walker =
        document.createTreeWalker(template.content, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, null, false);
    var valueIndex = 0;
    var nodeIndex = 0;
    var emptyTextNodes = [];
    var binds = [];
    while (walker.nextNode()) {
      var node = walker.currentNode;
      if (node.nodeType === Node.ELEMENT_NODE && node.hasAttributes()) {
        if (node.hasAttribute('$')) {
          binds.push({nodeIndex: nodeIndex, elementId: node.getAttribute('$')});
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
            binds.push({nodeIndex: nodeIndex, state: {name: state, attribute: attr, value: node.attributes[i].value}});
            continue;
          }

          if (!UI.Fragment._attributeMarkerRegex.test(name) &&
              !UI.Fragment._attributeMarkerRegex.test(node.attributes[i].value))
            continue;

          attributesToRemove.push(name);
          var bind = {nodeIndex: nodeIndex, attr: {index: valueIndex}};
          bind.attr.names = name.split(UI.Fragment._attributeMarkerRegex);
          valueIndex += bind.attr.names.length - 1;
          bind.attr.values = node.attributes[i].value.split(UI.Fragment._attributeMarkerRegex);
          valueIndex += bind.attr.values.length - 1;
          binds.push(bind);
        }
        for (var i = 0; i < attributesToRemove.length; i++)
          node.removeAttribute(attributesToRemove[i]);
        ++nodeIndex;
      } else if (node.nodeType === Node.TEXT_NODE && node.nodeValue.indexOf(UI.Fragment._textMarker) !== -1) {
        var texts = node.nodeValue.split(UI.Fragment._textMarkerRegex);
        node.data = texts[texts.length - 1];
        for (var i = 0; i < texts.length - 1; i++) {
          if (texts[i]) {
            ++nodeIndex;
            node.parentNode.insertBefore(createTextNode(texts[i]), node);
          }
          binds.push({nodeIndex: nodeIndex, replaceNodeIndex: valueIndex++});
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
    return {template: template, binds: binds};
  }

  /**
   * @param {!UI.Fragment._Template} template
   * @param {!Array<*>} values
   * @return {!UI.Fragment}
   */
  static _render(template, values) {
    var binds = template.binds;
    var document = template.template.ownerDocument;

    var bindIndex = 0;
    var nodeIndex = 0;
    var content = document.importNode(template.template.content, true);
    var resultElement =
        /** @type {!Element} */ (content.firstChild === content.lastChild ? content.firstChild : content);
    var result = new UI.Fragment(resultElement);

    // TODO(dgozman): switch to child index paths or querySelector if this is slow.
    var walker = document.createTreeWalker(content, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, null, false);
    var replacedNodes = [];
    var idByElement = new Map();
    while (walker.nextNode()) {
      var node = walker.currentNode;
      while (bindIndex < binds.length && binds[bindIndex].nodeIndex === nodeIndex) {
        var bind = binds[bindIndex++];
        if ('elementId' in bind) {
          result._elementsById.set(/** @type {string} */ (bind.elementId), node);
          idByElement.set(node, bind.elementId);
        } else if ('replaceNodeIndex' in bind) {
          var value = values[/** @type {number} */ (bind.replaceNodeIndex)];
          if (value instanceof Node) {
            node.parentNode.insertBefore(value, node);
            replacedNodes.push(node);
          } else {
            node.data = '' + value;
          }
        } else if ('state' in bind) {
          var list = result._states.get(bind.state.name) || [];
          list.push({
            attributeName: bind.state.attribute,
            attributeValue: bind.state.value,
            element: /** @type {!Element} */ (node),
            toggled: false
          });
          result._states.set(bind.state.name, list);
        } else if ('attr' in bind) {
          if (bind.attr.names.length === 2 && bind.attr.values.length === 1 &&
              typeof values[bind.attr.index] === 'function') {
            values[bind.attr.index].call(null, result, node);
          } else {
            var name = bind.attr.names[0];
            for (var i = 1; i < bind.attr.names.length; i++) {
              name += values[bind.attr.index + i - 1];
              name += bind.attr.names[i];
            }
            var value = bind.attr.values[0];
            for (var i = 1; i < bind.attr.values.length; i++) {
              value += values[bind.attr.index + bind.attr.names.length - 1 + i - 1];
              value += bind.attr.values[i];
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
    var shadows = result._element.querySelectorAll('x-shadow');
    for (var shadow of shadows) {
      if (!shadow.parentElement)
        throw 'There must be a parent element here';
      var shadowRoot = UI.createShadowRootWithCoreStyles(shadow.parentElement);
      while (shadow.firstChild)
        shadowRoot.appendChild(shadow.firstChild);
      var id = idByElement.get(shadow);
      if (id)
        result._elementsById.set(id, /** @type {!Element} */ (/** @type {!Node} */ (shadowRoot)));
    }

    return result;
  }
};

/**
 * @typedef {!{
 *   template: !Element,
 *   binds: !Array<!UI.Fragment._Bind>
 * }}
 */
UI.Fragment._Template;

/**
 * @typedef {!{
 *   attributeName: string,
 *   attributeValue: string,
 *   element: !Element,
 *   toggled: boolean
 * }}
 */
UI.Fragment._State;

/**
 * @typedef {!{
 *   nodeIndex: number,
 *
 *   elementId: (string|undefined),
 *
 *   state: (!{
 *     name: string,
 *     attribute: string,
 *     value: string
 *   }|undefined),
 *
 *   attr: (!{
 *     index: number,
 *     names: !Array<string>,
 *     values: !Array<string>
 *   }|undefined),
 *
 *   replaceNodeIndex: (number|undefined)
 * }}
 */
UI.Fragment._Bind;

UI.Fragment._textMarker = '{{template-text}}';
UI.Fragment._textMarkerRegex = /{{template-text}}/;

UI.Fragment._attributeMarker = index => 'template-attribute' + index;
UI.Fragment._attributeMarkerRegex = /template-attribute\d+/;

UI.Fragment._templateCache = new Map();
