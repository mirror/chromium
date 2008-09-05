// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

// This file contains infrastructure used by the API.  See
// v8natives.js for an explanation of these files are processed and
// loaded.


function CreateDate(time) {
  var date = new ORIGINAL_DATE();
  date.setTime(time);
  return date;
};


const kApiFunctionCache = {};
const functionCache = kApiFunctionCache;


function Instantiate(data) {
  if (!%IsTemplate(data)) return data;
  var tag = %GetTemplateField(data, kApiTagOffset);
  switch (tag) {
    case kFunctionTag:
      return InstantiateFunction(data);
    case kNewObjectTag:
      var Constructor = %GetTemplateField(data, kApiConstructorOffset);
      var result = Constructor ? new (Instantiate(Constructor))() : {};
      ConfigureTemplateInstance(result, data);
      return result;
    default:
      throw 'Unknown API tag <' + tag + '>';
  }
};


function InstantiateFunction(data) {
  var serialNumber = %GetTemplateField(data, kApiSerialNumberOffset);
  if (!(serialNumber in kApiFunctionCache)) {
    kApiFunctionCache[serialNumber] = null;
    var fun = %CreateApiFunction(data);
    kApiFunctionCache[serialNumber] = fun;
    var prototype = %GetTemplateField(data, kApiPrototypeTemplateOffset);
    fun.prototype = prototype ? Instantiate(prototype) : {};
    %AddProperty(fun.prototype, "constructor", fun, DONT_ENUM);
    var parent = %GetTemplateField(data, kApiParentTemplateOffset);
    if (parent) {
      var parent_fun = Instantiate(parent);
      fun.prototype.__proto__ = parent_fun.prototype;
    }
    ConfigureTemplateInstance(fun, data);
  }
  return kApiFunctionCache[serialNumber];
};


function ConfigureTemplateInstance(obj, data) {
  var properties = %GetTemplateField(data, kApiPropertyListOffset);
  if (properties) {
    for (var i = 0; i < properties[0]; i += 3) {
      var name = properties[i + 1];
      var prop_data = properties[i + 2];
      var attributes = properties[i + 3];
      var value = Instantiate(prop_data);
      %SetProperty(obj, name, value, attributes);
    }
  }
};
