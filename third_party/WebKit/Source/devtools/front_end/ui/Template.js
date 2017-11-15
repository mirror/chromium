// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

UI.Template = class {
  /**
   * @param {!UI.Template.Options} options
   * @return {function(!Array<string>, !Array<*>):!Element}
   */
  static factory(options) {
    /**
     * @param {!Array<string>} strings
     * @param {!Array<*>} values
     * @return {!Element}
     */
    function create(strings, ...values) {
      var template = options.cache ? UI.Template._templateCache.get(strings) : undefined;
      if (!template) {
        template = new UI.Template(strings);
        if (options.cache)
          UI.Template._templateCache.set(strings, template);
      }
      return template.render(values);
    }
    return create;
  }

  /**
   * @param {!Array<string>} strings
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
      html += insideText ? UI.Template._textMarker : UI.Template._attributeMarker(i);
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
        if (node.hasAttribute('bind')) {
          this._binds.push({nodeIndex, bindName: node.getAttribute('bind')});
          node.removeAttribute('bind');
        }
        var attributesToRemove = [];
        for (var i = 0; i < node.attributes.length; i++) {
          if (!UI.Template._attributeMarkerRegex.test(node.attributes[i].name) &&
              !UI.Template._attributeMarkerRegex.test(node.attributes[i].value)) {
            continue;
          }
          attributesToRemove.push(node.attributes[i].name);
          var bind = {nodeIndex, attrIndex: valueIndex};
          bind.attrNames = node.attributes[i].name.split(UI.Template._attributeMarkerRegex);
          valueIndex += bind.attrNames.length - 1;
          bind.attrValues = node.attributes[i].value.split(UI.Template._attributeMarkerRegex);
          valueIndex += bind.attrValues.length - 1;
          this._binds.push(bind);
        }
        for (var i = 0; i < attributesToRemove.length; i++)
          node.removeAttribute(attributesToRemove[i]);
        ++nodeIndex;
      } else if (node.nodeType === Node.TEXT_NODE && node.nodeValue.indexOf(UI.Template._textMarker) !== -1) {
        var texts = node.nodeValue.split(UI.Template._textMarkerRegex);
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
   */
  render(values) {
    var bindIndex = 0;
    var nodeIndex = 0;
    var content = document.importNode(this._template.content, true);
    var result = content.firstChild === content.lastChild ? content.firstChild : content;
    var walker = document.createTreeWalker(content, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, null, false);
    var replacedNodes = [];
    while (walker.nextNode()) {
      var node = walker.currentNode;
      while (bindIndex < this._binds.length && this._binds[bindIndex].nodeIndex === nodeIndex) {
        var bind = this._binds[bindIndex++];
        if ('bindName' in bind) {
          result[bind.bindName] = node;
        } else if ('replaceNodeIndex' in bind) {
          var value = values[bind.replaceNodeIndex];
          if (value instanceof Node) {
            node.parentNode.insertBefore(value, node);
            replacedNodes.push(node);
          } else {
            node.data = '' + value;
          }
        } else if ('attrIndex' in bind) {
          if (bind.attrNames.length === 2 && bind.attrValues.length === 1 && typeof values[bind.attrIndex] === 'function') {
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
            node.setAttribute(name, value);
          }
        } else {
          throw 'Unexpected bind';
        }
      }
      if (node.nodeType === Node.ELEMENT_NODE && node.hasAttributes()) {
        for (var cssAttribute of UI.Template._cssAttributes) {
          if (node.hasAttribute(cssAttribute)) {
          }
        }
        for (var i = node.attributes.length - 1; i >= 0; i--) {
          var name = node.attributes[i].name;
          if (UI.Template._cssAttributes.has(name)) {
            var style = node.getAttribute('style') || '';
            style = name + ': ' + node.attributes[i].value + '; ' + style;
            node.setAttribute('style', style);
            node.removeAttribute(name);
            continue;
          }
          if (name.startsWith('toggle-')) {
            if (!result[UI.Template._toggles])
              result[UI.Template._toggles] = new Set();
            var toggle = name.substring(7);
            var style = '/*' + toggle + '*/' + node.attributes[i].value + '/*-' + toggle + '*/';
            Object.defineProperty(result, toggle, {
              configurable: true,
              enumerable: true,
              get: () => result[UI.Template._toggles].has(toggle),
              set: UI.Template._toggle.bind(null, node, result[UI.Template._toggles], toggle, style)
            });
            node.removeAttribute(name);
            continue;
          }
        }
      }
      ++nodeIndex;
    }
    for (var replacedNode of replacedNodes)
      replacedNode.parentNode.removeChild(replacedNode);
    return result;
  }

  /**
   * @param {!Node} node
   * @param {!Set<string>} set
   * @param {string} name
   * @param {string} toggleStyle
   * @param {boolean} value
   */
  static _toggle(node, set, name, toggleStyle, value) {
    if (set.has(name) === value)
      return;
    var style = node.getAttribute('style') || '';
    if (value) {
      set.add(name);
      style += toggleStyle;
    } else {
      set.delete(name);
      var index = style.indexOf(toggleStyle);
      if (index === -1)
        throw 'Inconsistent toggle';
      style = style.substring(0, index) + style.substring(index + toggleStyle.length);
    }
    node.setAttribute('style', style);
  }
};

UI.Template._textMarker = '{{template-text}}';
UI.Template._textMarkerRegex = /{{template-text}}/;

UI.Template._attributeMarker = index => 'template-attribute' + index;
UI.Template._attributeMarkerRegex = /template-attribute\d+/;

UI.Template._symbol = Symbol('UI.Template');
UI.Template._toggles = Symbol('UI.Template.toggles');
UI.Template._cssAttributes = new Set(['flex']);

UI.Template._templateCache = new Map();

/**
 * @typedef {{
 *    cache: (boolean|undefined)
 * }}
 */
UI.Template.Options;

/**
 * @param {!Array<string>} strings
 * @param {!Array<*>} values
 * @return {!Element}
 */
UI.html = UI.Template.factory({cache: false});

/**
 * @param {!Array<string>} strings
 * @param {!Array<*>} values
 * @return {!Element}
 */
UI.cachedHtml = UI.Template.factory({cache: true});

/**
 * @param {!Array<string>} strings
 * @param {!Array<*>} values
 * @return {string}
 */
self.s = function(strings, ...values) {
  if (!values.length)
    return Common.localize(strings[0]);
  var result = '';
  for (var i = 0; i < values.length; i++) {
    result += Common.localize(strings[i]);
    result += '' + values[i];
  }
  return result + Common.localize(strings[values.length]);
};
