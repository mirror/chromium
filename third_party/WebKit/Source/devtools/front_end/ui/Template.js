// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

UI.Template = class {
  /**
   * @param {!Array<string>} strings
   * @param {!Array<*>} values
   * @return {!Element}
   */
  static create(strings, ...values) {
    strings = strings.slice(0);
    for (var i = values.length - 1; i >= 0; i--) {
      var special = typeof values[i] === 'function' || values[i] instanceof Node;
      if (!special) {
        strings.splice(i, 2, strings[i] + values[i] + strings[i + 1]);
        values.splice(i, 1);
      }
    }

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
      html += insideText ? UI.Template._textMarker : UI.Template._attributeMarker + i;
    }
    html += strings[values.length];

    var template = createElement('template');
    template.innerHTML = html;
    var walker =
        document.createTreeWalker(template.content, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, null, false);
    var index = 0;
    var emptyTextNodes = [];
    var binds = [];
    while (walker.nextNode()) {
      var node = walker.currentNode;
      if (node.nodeType === Node.ELEMENT_NODE && node.hasAttributes()) {
        if (node.hasAttribute('bind')) {
          binds.push({key: node.getAttribute('bind'), node});
          node.removeAttribute('bind');
        }
        if (node.hasAttribute('flex')) {
          var style = node.getAttribute('style') || '';
          style = 'flex: ' + node.getAttribute('flex') + '; ' + style;
          node.setAttribute('style', style);
          node.removeAttribute('flex');
        }
        var count = 0;
        for (var i = 0; i < node.attributes.length; i++) {
          if (node.attributes[i].name.startsWith(UI.Template._attributeMarker))
            count++;
        }
        index += count;
        count = 0;
        for (var i = node.attributes.length - 1; i >= 0; i--) {
          if (!node.attributes[i].name.startsWith(UI.Template._attributeMarker))
            continue;
          count++;
          if (typeof values[index - count] !== 'function')
            throw 'Only bind functions are supported in attributes';
          binds.push({func: values[index - count], node});
          node.removeAttribute(node.attributes[i].name);
        }
      } else if (node.nodeType === Node.TEXT_NODE && node.nodeValue.indexOf(UI.Template._textMarker) !== -1) {
        var texts = node.nodeValue.split(UI.Template._textMarkerRegex);
        node.data = texts[texts.length - 1];
        for (var i = 0; i < texts.length - 1; i++) {
          node.parentNode.insertBefore(createTextNode(texts[i]), node);
          var value = values[index++];
          if (!(value instanceof Node))
            throw 'Only nodes are supported in text content';
          node.parentNode.insertBefore(value, node);
        }
      } else if (node.nodeType === Node.TEXT_NODE) {
        if ((!node.previousSibling || node.previousSibling.nodeType === Node.ELEMENT_NODE) &&
            (!node.nextSibling || node.nextSibling.nodeType === Node.ELEMENT_NODE) && /^\s*$/.test(node.nodeValue))
          emptyTextNodes.push(node);
      }
    }
    for (var emptyTextNode of emptyTextNodes)
      emptyTextNode.parentNode.removeChild(emptyTextNode);

    var result = template.content.firstChild === template.content.lastChild ? template.content.firstChild : template.content;
    for (var bind of binds) {
      if (bind.func)
        bind.func.call(null, result, bind.node);
      else
        result[bind.key] = bind.node;
    }
    return result;
  }
};

UI.Template._textMarker = `{{template-text}}`;
UI.Template._attributeMarker = `template-attribute`;
UI.Template._textMarkerRegex = new RegExp(UI.Template._textMarker);

/**
 * @param {!Array<string>} strings
 * @param {!Array<*>} values
 * @return {!UI.Template}
 */
self.html = UI.Template.create;

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
