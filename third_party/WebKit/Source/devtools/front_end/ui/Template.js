// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

UI.Template = class {
  /**
   * @param {!Array<string>} strings
   * @param {!Array<*>} values
   */
  constructor(strings, values) {
    var html = '';
    var insideText = false;
    for (var i = 0; i < values.length; i++) {
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
    html += strings[values.length];

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
        var count = 0;
        for (var i = node.attributes.length - 1; i >= 0; i--) {
          if (UI.Template._attributeMarkerRegex.test(node.attributes[i].name))
            ++count;
          if (UI.Template._attributeMarkerRegex.test(node.attributes[i].value))
            ++count;
        }
        valueIndex += count;
        count = 0;
        // TODO(dgozman): support multuple values inside attribute names/values.
        for (var i = node.attributes.length - 1; i >= 0; i--) {
          if (UI.Template._attributeMarkerRegex.test(node.attributes[i].name) &&
              UI.Template._attributeMarkerRegex.test(node.attributes[i].value)) {
            count += 2;
            this._binds.push({nodeIndex, attrNameIndex: valueIndex - count, attrValueIndex: valueIndex - count + 1});
            node.removeAttribute(node.attributes[i].name);
            continue;
          }
          if (UI.Template._attributeMarkerRegex.test(node.attributes[i].name)) {
            ++count;
            if (typeof values[valueIndex - count] === 'function')
              this._binds.push({nodeIndex, funcIndex: valueIndex - count});
            else
              this._binds.push({nodeIndex, attrNameIndex: valueIndex - count, attrValue: node.attributes[i].value});
            node.removeAttribute(node.attributes[i].name);
            continue;
          }
          if (UI.Template._attributeMarkerRegex.test(node.attributes[i].value)) {
            ++count;
            this._binds.push({nodeIndex, attrName: node.attributes[i].name, attrValueIndex: valueIndex - count});
            node.removeAttribute(node.attributes[i].name);
            continue;
          }
        }
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
        } else if ('funcIndex' in bind) {
          values[bind.funcIndex].call(null, result, node);
        } else if ('replaceNodeIndex' in bind) {
          var value = values[bind.replaceNodeIndex];
          if (value instanceof Node) {
            node.parentNode.insertBefore(value, node);
            replacedNodes.push(node);
          } else {
            node.data = '' + value;
          }
        } else {
          var name = 'attrNameIndex' in bind ? '' + values[bind.attrNameIndex] : bind.attrName;
          var value = 'attrValueIndex' in bind ? '' + values[bind.attrValueIndex] : bind.attrValue;
          node.setAttribute(name, value);
        }
      }
      if (node.nodeType === Node.ELEMENT_NODE && node.hasAttributes()) {
        for (var cssAttribute of UI.Template._cssAttributes) {
          if (node.hasAttribute(cssAttribute)) {
            var style = node.getAttribute('style') || '';
            style = cssAttribute + ': ' + node.getAttribute(cssAttribute) + '; ' + style;
            node.setAttribute('style', style);
            node.removeAttribute(cssAttribute);
          }
        }
      }
      ++nodeIndex;
    }
    for (var replacedNode of replacedNodes)
      replacedNode.parentNode.removeChild(replacedNode);
    return result;
  }
};

UI.Template._textMarker = '{{template-text}}';
UI.Template._textMarkerRegex = /{{template-text}}/;

UI.Template._attributeMarker = index => 'template-attribute' + index + '';
UI.Template._attributeMarkerRegex = /^template-attribute\d+$/;

UI.Template._symbol = Symbol('UI.Template');
UI.Template._cssAttributes = ['flex'];

UI.Template._templateCache = new Map();

/**
 * @param {!Array<string>} strings
 * @param {!Array<*>} values
 * @return {!Element}
 */
self.html = function(strings, ...values) {
  var template = UI.Template._templateCache.get(strings);
  if (!template) {
    template = new UI.Template(strings, values);
    UI.Template._templateCache.set(strings, template);
  }
  return template.render(values);
}

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
