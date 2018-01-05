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
   * @param {string} tagName
   * @param {!Array<string>} params
   * @param {function(!Element, !Object, !Array<!Node>):(!Element|!DocumentFragment|!UI.Fragment)} processor
   */
  static registerTemplate(tagName, params, processor) {
    var paramsMap = new Map();
    for (var param of params)
      paramsMap.set(param.toUpperCase(), param);
    UI.Fragment._registry.set(tagName.toUpperCase(), {params: paramsMap, processor: processor});
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
      html += insideText ? UI.Fragment._textMarker(i) : UI.Fragment._attributeMarker(i);
    }
    html += strings[strings.length - 1];

    var template = createElement('template');
    template.innerHTML = html;
    var content = /** @type {!DocumentFragment} */ (UI.Fragment._preprocess(template.content));
    var walker =
        template.ownerDocument.createTreeWalker(content, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, null, false);
    var emptyTextNodes = [];
    var binds = [];
    var nodesToMark = [];
    while (walker.nextNode()) {
      var node = walker.currentNode;
      if (node.nodeType === Node.ELEMENT_NODE && node.hasAttributes()) {
        if (node.hasAttribute('$')) {
          nodesToMark.push(node);
          binds.push({elementId: node.getAttribute('$')});
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
            nodesToMark.push(node);
            binds.push({state: {name: state, attribute: attr, value: node.attributes[i].value}});
            continue;
          }

          var names = name.split(UI.Fragment._attributeMarkerRegex);
          var values = node.attributes[i].value.split(UI.Fragment._attributeMarkerRegex);
          if (names.length === 1 && values.length === 1)
            continue;

          attributesToRemove.push(name);
          nodesToMark.push(node);
          binds.push({attr: {name: UI.Fragment._splitResult(names), value: UI.Fragment._splitResult(values)}});
        }
        for (var i = 0; i < attributesToRemove.length; i++)
          node.removeAttribute(attributesToRemove[i]);
      }

      if (node.nodeType === Node.TEXT_NODE) {
        var texts = node.data.split(UI.Fragment._textMarkerRegex);
        if (texts.length > 1) {
          var split = UI.Fragment._splitResult(texts);
          node.data = split.strings[split.strings.length - 1];
          for (var i = 0; i < split.strings.length - 1; i++) {
            if (split.strings[i])
              node.parentNode.insertBefore(createTextNode(split.strings[i]), node);
            var nodeToReplace = createElement('span');
            nodesToMark.push(nodeToReplace);
            binds.push({replaceNodeIndex: split.indices[i]});
            node.parentNode.insertBefore(nodeToReplace, node);
          }
        }
      }

      if (node.nodeType === Node.TEXT_NODE &&
          (!node.previousSibling || node.previousSibling.nodeType === Node.ELEMENT_NODE) &&
          (!node.nextSibling || node.nextSibling.nodeType === Node.ELEMENT_NODE) && /^\s*$/.test(node.data))
        emptyTextNodes.push(node);
    }

    for (var i = 0; i < nodesToMark.length; i++)
      nodesToMark[i].classList.add(UI.Fragment._class(i));

    for (var emptyTextNode of emptyTextNodes)
      emptyTextNode.remove();
    return {content: content, document: template.ownerDocument, binds: binds};
  }

  /**
   * @param {!Array<string>} a
   * @return {!UI.Fragment._Split}
   */
  static _splitResult(a) {
    var strings = [];
    var indices = [];
    for (var i = 0; i < a.length; i += 2) {
      strings.push(a[i]);
      if (i + 1 < a.length)
        indices.push(Number(a[i + 1]));
    }
    return {strings: strings, indices: indices};
  }

  /**
   * @param {!Element|!DocumentFragment} node
   * @return {!Element|!DocumentFragment}
   */
  static _preprocess(node) {
    var template;
    if (node.nodeType === Node.ELEMENT_NODE)
      template = UI.Fragment._registry.get(node.tagName);
    var params = template ? template.params : null;
    var processor = template ? template.processor : UI.Fragment._defaultProcessor;
    var children = [];
    var paramsObject = {};

    var child = node.firstChild;
    while (child) {
      var next = child.nextSibling;
      if (child.nodeType === Node.ELEMENT_NODE) {
        var tagName = child.tagName.toUpperCase();
        if (params && params.has(tagName)) {
          var paramChildren = [];
          for (var paramChild of child.childNodes) {
            if (paramChild.nodeType === Node.ELEMENT_NODE || paramChild.nodeType === Node.DOCUMENT_FRAGMENT_NODE)
              paramChildren.push(UI.Fragment._preprocess(/** @type {!Element|!DocumentFragment} */ (paramChild)));
            else
              paramChildren.push(paramChild);
          }
          if (paramChildren.length === 1) {
            paramsObject[params.get(tagName)] = paramChildren[0];
          } else {
            var fragment = createDocumentFragment();
            for (var paramChild of paramChildren)
              fragment.appendChild(paramChild);
            paramsObject[params.get(tagName)] = fragment;
          }
        } else {
          children.push(UI.Fragment._preprocess(/** @type {!Element} */ (child)));
        }
      } else {
        children.push(child);
      }
      child = next;
    }
    node.removeChildren();

    var result = processor.call(null, node, paramsObject, children);
    var resultNode = result instanceof UI.Fragment ? result._element : result;
    if (resultNode !== node && resultNode.nodeType === Node.ELEMENT_NODE) {
      for (var i = 0; i < node.attributes.length; i++)
        resultNode.setAttribute(node.attributes[i].name, node.attributes[i].value);
    }
    return resultNode;
  }

  /**
   * @param {!Element|!DocumentFragment} element
   * @param {!Object} params
   * @param {!Array<!Node>} children
   * @return {!Element|!DocumentFragment}
   */
  static _defaultProcessor(element, params, children) {
    for (var child of children)
      element.appendChild(child);
    return element;
  }

  /**
   * @param {!UI.Fragment._Template} template
   * @param {!Array<*>} values
   * @return {!UI.Fragment}
   */
  static _render(template, values) {
    var content = template.document.importNode(template.content, true);
    if (content.firstChild !== content.lastChild || content.firstChild.nodeType !== Node.ELEMENT_NODE)
      throw new Error('Fragment must have a single top-level element');
    var resultElement = /** @type {!Element} */ (content.firstChild);
    var result = new UI.Fragment(resultElement);

    var idByElement = new Map();
    var boundElements = [];
    for (var i = 0; i < template.binds.length; i++) {
      var className = UI.Fragment._class(i);
      var element = /** @type {!Element} */ (content.querySelector('.' + className));
      element.classList.remove(className);
      boundElements.push(element);
    }

    for (var bindIndex = 0; bindIndex < template.binds.length; bindIndex++) {
      var bind = template.binds[bindIndex];
      var element = boundElements[bindIndex];
      if ('elementId' in bind) {
        result._elementsById.set(/** @type {string} */ (bind.elementId), element);
        idByElement.set(element, bind.elementId);
      } else if ('replaceNodeIndex' in bind) {
        var value = values[/** @type {number} */ (bind.replaceNodeIndex)];
        this._insertNodesBefore(value, element);
        element.remove();
      } else if ('state' in bind) {
        var list = result._states.get(bind.state.name) || [];
        list.push(
            {attributeName: bind.state.attribute, attributeValue: bind.state.value, element: element, toggled: false});
        result._states.set(bind.state.name, list);
      } else if ('attr' in bind) {
        var name = bind.attr.name;
        var value = bind.attr.value;
        if (name.indices.length === 1 && value.indices.length === 0 && typeof values[name.indices[0]] === 'function') {
          values[name.indices[0]].call(null, element);
        } else {
          var nameString = name.strings[0];
          for (var i = 1; i < name.strings.length; i++) {
            nameString += values[name.indices[i - 1]];
            nameString += name.strings[i];
          }
          if (nameString) {
            var valueString = value.strings[0];
            for (var i = 1; i < value.strings.length; i++) {
              valueString += values[value.indices[i - 1]];
              valueString += value.strings[i];
            }
            element.setAttribute(nameString, valueString);
          }
        }
      } else {
        throw new Error('Unexpected bind');
      }
    }

    // We do this after binds so that querySelector works.
    var shadows = content.querySelectorAll('x-shadow');
    for (var shadow of shadows) {
      if (!shadow.parentElement)
        throw 'There must be a parent element here';
      var shadowRoot = UI.createShadowRootWithCoreStylesV1(shadow.parentElement);
      if (shadow.parentElement.tagName === 'X-WIDGET')
        shadow.parentElement._shadowRoot = shadowRoot;
      var children = [];
      while (shadow.lastChild) {
        children.push(shadow.lastChild);
        shadow.lastChild.remove();
      }
      for (var i = children.length - 1; i >= 0; i--)
        shadowRoot.appendChild(children[i]);
      var id = idByElement.get(shadow);
      if (id)
        result._elementsById.set(id, /** @type {!Element} */ (/** @type {!Node} */ (shadowRoot)));
      shadow.remove();
    }

    return result;
  }

  /**
   * @param {*} value
   * @param {!Element} element
   */
  static _insertNodesBefore(value, element) {
    if (Array.isArray(value)) {
      for (var item of value)
        UI.Fragment._insertNodesBefore(item, element);
      return;
    }
    var node = null;
    if (value instanceof Node)
      node = value;
    else if (value instanceof UI.Fragment)
      node = value._element;
    else if (value === null || value === undefined)
      node = createTextNode('');
    else
      node = createTextNode('' + value);
    element.parentNode.insertBefore(node, element);
  }
};

/**
 * @typedef {!{
 *   content: !DocumentFragment,
 *   document: !Document,
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
 *   strings: !Array<string>,
 *   indices: !Array<number>
 * }}
 */
UI.Fragment._Split;

/**
 * @typedef {!{
 *   elementId: (string|undefined),
 *
 *   state: (!{
 *     name: string,
 *     attribute: string,
 *     value: string
 *   }|undefined),
 *
 *   attr: (!{
 *     name: !UI.Fragment._Split,
 *     value: !UI.Fragment._Split,
 *   }|undefined),
 *
 *   replaceNodeIndex: (number|undefined)
 * }}
 */
UI.Fragment._Bind;

UI.Fragment._textMarker = index => '{{template-text-' + index + '}}';
UI.Fragment._textMarkerRegex = /{{template-text-(\d+)}}/;

UI.Fragment._attributeMarker = index => 'template-attribute' + index;
UI.Fragment._attributeMarkerRegex = /template-attribute(\d+)/;

UI.Fragment._class = index => 'template-class-' + index;

UI.Fragment._templateCache = new Map();
UI.Fragment._registry = new Map();
