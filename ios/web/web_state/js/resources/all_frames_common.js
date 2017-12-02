// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides common methods that can be shared by other JavaScripts.

goog.provide('__crWeb.common');

goog.require('__crWeb.base');


/** @typedef {HTMLInputElement|HTMLTextAreaElement|HTMLSelectElement} */
var FormControlElement;

if (!__gCrWeb['common']) {

/**
 * Namespace for this file. It depends on |__gCrWeb| having already been
 * injected. String 'common' is used in |__gCrWeb['common']| as it needs to be
 * accessed in Objective-C code.
 */
__gCrWeb.common = {};

// Store common namespace object in a global __gCrWeb object referenced by a
// string, so it does not get renamed by closure compiler during the
// minification.
__gCrWeb['common'] = __gCrWeb.common;
}

/* Beginning of anonymous object. */
(function() {

  /**
   * JSON safe object to protect against custom implementation of Object.toJSON
   * in host pages.
   * @constructor
   */
  __gCrWeb.common.JSONSafeObject = function JSONSafeObject() {
  };

  /**
   * Protect against custom implementation of Object.toJSON in host pages.
   */
  __gCrWeb.common.JSONSafeObject.prototype['toJSON'] = null;

  /**
   * Retain the original JSON.stringify method where possible to reduce the
   * impact of sites overriding it
   */
  __gCrWeb.common.JSONStringify = JSON.stringify;

  /**
   * Returns a string that is formatted according to the JSON syntax rules.
   * This is equivalent to the built-in JSON.stringify() function, but is
   * less likely to be overridden by the website itself.  Prefer the private
   * {@code __gcrWeb.common.JSONStringify} whenever possible instead of using
   * this public function. The |__gCrWeb| object itself does not use it; it uses
   * its private counterpart instead.
   */
  __gCrWeb['stringify'] = function(value) {
    if (value === null)
      return 'null';
    if (value === undefined)
       return 'undefined';
    if (typeof(value.toJSON) == 'function') {
      // Prevents websites from changing stringify's behavior by adding the
      // method toJSON() by temporarily removing it.
      var originalToJSON = value.toJSON;
      value.toJSON = undefined;
      var stringifiedValue = __gCrWeb.common.JSONStringify(value);
      value.toJSON = originalToJSON;
      return stringifiedValue;
    }
    return __gCrWeb.common.JSONStringify(value);
  };

  /**
   * Prefix used in references to form elements that have no 'id' or 'name'
   */
  __gCrWeb.common.kNamelessFormIDPrefix = 'gChrome~form~';

  /**
   * Prefix used in references to field elements that have no 'id' or 'name' but
   * are included in a form.
   */
  __gCrWeb.common.kNamelessFieldIDPrefix = 'gChrome~field~';


  /**
   * Trims any whitespace from the start and end of a string.
   * Used in preference to String.prototype.trim as this can be overridden by
   * sites.
   *
   * @param {string} str The string to be trimmed.
   * @return {string} The string after trimming.
   */
  __gCrWeb.common.trim = function(str) {
    return str.replace(/^\s+|\s+$/g, '');
  };

  /**
   * Returns the form's |name| attribute if not space only; otherwise the
   * form's |id| attribute.
   *
   * It is the name that should be used for the specified |element| when
   * storing Autofill data. Various attributes are used to attempt to identify
   * the element, beginning with 'name' and 'id' attributes. If both name and id
   * are empty and the field is in a form, returns
   * __gCrWeb.common.kNamelessFieldIDPrefix + index of the field in the form.
   * Providing a uniquely reversible identifier for any element is a non-trivial
   * problem; this solution attempts to satisfy the majority of cases.
   *
   * It aims to provide the logic in
   *     WebString nameForAutofill() const;
   * in chromium/src/third_party/WebKit/Source/WebKit/chromium/public/
   *  WebFormControlElement.h
   *
   * @param {Element} element An element of which the name for Autofill will be
   *     returned.
   * @return {string} the name for Autofill.
   */
  __gCrWeb.common.getFieldIdentifier = function(element) {
    if (!element) {
      return '';
    }
    var trimmedName = element.name;
    if (trimmedName) {
      trimmedName = __gCrWeb.common.trim(trimmedName);
      if (trimmedName.length > 0) {
        return trimmedName;
      }
    }
    trimmedName = element.id;
    if (trimmedName) {
      return __gCrWeb.common.trim(trimmedName);
    }
    if (element.form) {
      var elements = __gCrWeb.common.getFormControlElements(element.form);
      for (var index = 0; index < elements.length; index++) {
        if (elements[index] === element) {
          return __gCrWeb.common.kNamelessFieldIDPrefix + index;
        }
      }
    }
    return '';
  };

  /**
   * Returns the form's |name| attribute if non-empty; otherwise the form's |id|
   * attribute, or the index of the form (with prefix) in document.forms.
   *
   * It is partially based on the logic in
   *     const string16 GetFormIdentifier(const blink::WebFormElement& form)
   * in chromium/src/components/autofill/renderer/form_autofill_util.h.
   *
   * @param {Element} form An element for which the identifier is returned.
   * @return {string} a string that represents the element's identifier.
   */
  __gCrWeb.common.getFormIdentifier = function(form) {
    if (!form)
      return '';
    var name = form.getAttribute('name');
    if (name && name.length != 0 &&
        form.ownerDocument.forms.namedItem(name) === form) {
      return name;
    }
    name = form.getAttribute('id');
    if (name) {
      return name;
    }
    // A form name must be supplied, because the element will later need to be
    // identified from the name. A last resort is to take the index number of
    // the form in document.forms. ids are not supposed to begin with digits (by
    // HTML 4 spec) so this is unlikely to match a true id.
    for (var idx = 0; idx != document.forms.length; idx++) {
      if (document.forms[idx] == form) {
        return __gCrWeb.common.kNamelessFormIDPrefix + idx;
      }
    }
    return '';
  };
}());  // End of anonymous object
