// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

UI.Template = class {
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
        if (node.hasAttribute('el')) {
          this._binds.push({nodeIndex, bindName: node.getAttribute('el')});
          node.removeAttribute('el');
        }
        var attributesToRemove = [];
        for (var i = 0; i < node.attributes.length; i++) {
          if (!UI.Template._attributeMarkerRegex.test(node.attributes[i].name) &&
              !UI.Template._attributeMarkerRegex.test(node.attributes[i].value))
            continue;

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
    var content = this._template.ownerDocument.importNode(this._template.content, true);
    var result = /** @type {!Element} */ (content.firstChild === content.lastChild ? content.firstChild : content);
    result._states = new Map();
    var walker = this._template.ownerDocument.createTreeWalker(
        content, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, null, false);
    var replacedNodes = [];
    while (walker.nextNode()) {
      var node = walker.currentNode;
      while (bindIndex < this._binds.length && this._binds[bindIndex].nodeIndex === nodeIndex) {
        var bind = this._binds[bindIndex++];
        if ('bindName' in bind) {
          if (!result._elements)
            result._elements = new Map();
          result._elements.set(bind.bindName, node);
        } else if ('replaceNodeIndex' in bind) {
          var value = values[bind.replaceNodeIndex];
          if (value instanceof Node) {
            node.parentNode.insertBefore(value, node);
            replacedNodes.push(node);
          } else {
            node.data = '' + value;
          }
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

      if (node.nodeType === Node.ELEMENT_NODE && node.hasAttributes()) {
        for (var i = node.attributes.length - 1; i >= 0; i--) {
          var name = node.attributes[i].name;
          if (!name.startsWith('x-'))
            continue;
          name = name.substring(2);
          var state = name.substring(0, name.indexOf('-'));
          var attr = name.substring(state.length + 1);
          var list = result._states.get(state) || [];
          list.push({name: attr, value: node.attributes[i].value, node, toggled: false});
          result._states.set(state, list);
          node.removeAttribute(name);
        }
      }

      ++nodeIndex;
    }

    for (var replacedNode of replacedNodes)
      replacedNode.parentNode.removeChild(replacedNode);

    /** @type {function(string):!Element} */
    result.el = UI.Template._el.bind(null, result);

    /** @type {function(string, boolean)} */
    result.toggle = UI.Template._toggle.bind(null, result);

    return result;
  }

  /**
   * @param {!Element} element
   * @param {string} name
   * @return {!Element}
   */
  static _el(element, name) {
    return element._elements.get(name);
  }

  /**
   * @param {!Element} element
   * @param {string} name
   * @param {boolean} toggled
   */
  static _toggle(element, name, toggled) {
    var list = element._states.get(name);
    if (list === undefined) {
      console.error('Unknown toggle ' + name);
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
};

UI.Template._textMarker = '{{template-text}}';
UI.Template._textMarkerRegex = /{{template-text}}/;

UI.Template._attributeMarker = index => 'template-attribute' + index;
UI.Template._attributeMarkerRegex = /template-attribute\d+/;

UI.Template._templateCache = new Map();

/**
 * @param {!Array<string>} strings
 * @param {...*} vararg
 * @return {!Element}
 */
UI.html = function(strings, vararg) {
  var values = Array.prototype.slice.call(arguments, 1);
  return new UI.Template(strings).render(values);
};

/**
 * @param {!Array<string>} strings
 * @param {...*} vararg
 * @return {!Element}
 */
UI.cachedHtml = function(strings, vararg) {
  var values = Array.prototype.slice.call(arguments, 1);
  var template = UI.Template._templateCache.get(strings);
  if (!template) {
    template = new UI.Template(strings);
    UI.Template._templateCache.set(strings, template);
  }
  return template.render(values);
};
