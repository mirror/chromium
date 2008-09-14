// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef V8_CUSTOM_H__
#define V8_CUSTOM_H__

#include <v8.h>

struct NPObject;

namespace WebCore {

class Frame;
class V8Proxy;
class String;
class HTMLCollection;

class V8Custom {
 public:

  // Constants.
  static const int kDefaultWrapperInternalFieldCount = 2;
  static const int kDocumentMinimumInternalFieldCount = 3;
  static const int kDocumentImplementationIndex = 2;
  static const int kHTMLDocumentInternalFieldCount = 5;
  static const int kHTMLDocumentMarkerIndex = 3;
  static const int kHTMLDocumentShadowIndex = 4;

  static const int kStyleSheetOwnerNodeIndex =
                      kDefaultWrapperInternalFieldCount + 0;
  static const int kStyleSheetInternalFieldCount =
                      kDefaultWrapperInternalFieldCount + 1;

#define DECLARE_PROPERTY_ACCESSOR_GETTER(NAME) \
static v8::Handle<v8::Value> v8##NAME##AccessorGetter(\
    v8::Local<v8::String> name, const v8::AccessorInfo& info);

#define DECLARE_PROPERTY_ACCESSOR_SETTER(NAME)  \
static void v8##NAME##AccessorSetter(v8::Local<v8::String> name, \
                                     v8::Local<v8::Value> value, \
                                     const v8::AccessorInfo& info);

#define DECLARE_PROPERTY_ACCESSOR(NAME) \
  DECLARE_PROPERTY_ACCESSOR_GETTER(NAME) \
  DECLARE_PROPERTY_ACCESSOR_SETTER(NAME)


#define DECLARE_NAMED_PROPERTY_GETTER(NAME)  \
static v8::Handle<v8::Value> v8##NAME##NamedPropertyGetter(\
    v8::Local<v8::String> name, const v8::AccessorInfo& info);

#define DECLARE_NAMED_PROPERTY_SETTER(NAME)  \
static v8::Handle<v8::Value> v8##NAME##NamedPropertySetter(\
    v8::Local<v8::String> name, \
    v8::Local<v8::Value> value, \
    const v8::AccessorInfo& info);

#define DECLARE_NAMED_PROPERTY_DELETER(NAME)  \
static v8::Handle<v8::Boolean> v8##NAME##NamedPropertyDeleter(\
    v8::Local<v8::String> name, const v8::AccessorInfo& info);

#define USE_NAMED_PROPERTY_GETTER(NAME) \
  V8Custom::v8##NAME##NamedPropertyGetter

#define USE_NAMED_PROPERTY_SETTER(NAME) \
  V8Custom::v8##NAME##NamedPropertySetter

#define USE_NAMED_PROPERTY_DELETER(NAME) \
  V8Custom::v8##NAME##NamedPropertyDeleter

#define DECLARE_INDEXED_PROPERTY_GETTER(NAME)  \
static v8::Handle<v8::Value> v8##NAME##IndexedPropertyGetter(\
    uint32_t index, const v8::AccessorInfo& info);

#define DECLARE_INDEXED_PROPERTY_SETTER(NAME)  \
static v8::Handle<v8::Value> v8##NAME##IndexedPropertySetter(\
    uint32_t index, v8::Local<v8::Value> value, const v8::AccessorInfo& info);

#define DECLARE_INDEXED_PROPERTY_DELETER(NAME)  \
static v8::Handle<v8::Boolean> v8##NAME##IndexedPropertyDeleter(\
    uint32_t index, const v8::AccessorInfo& info);

#define USE_INDEXED_PROPERTY_GETTER(NAME) \
  V8Custom::v8##NAME##IndexedPropertyGetter

#define USE_INDEXED_PROPERTY_SETTER(NAME) \
  V8Custom::v8##NAME##IndexedPropertySetter

#define USE_INDEXED_PROPERTY_DELETER(NAME) \
  V8Custom::v8##NAME##IndexedPropertyDeleter

#define DECLARE_CALLBACK(NAME)                \
static v8::Handle<v8::Value> v8##NAME##Callback(const v8::Arguments& args);

#define USE_CALLBACK(NAME) \
  V8Custom::v8##NAME##Callback

#define DECLARE_NAMED_ACCESS_CHECK(NAME) \
static bool v8##NAME##NamedSecurityCheck(v8::Local<v8::Object> host, \
                                         v8::Local<v8::Value> key, \
                                         v8::AccessType type, \
                                         v8::Local<v8::Value> data);

#define DECLARE_INDEXED_ACCESS_CHECK(NAME) \
static bool v8##NAME##IndexedSecurityCheck(v8::Local<v8::Object> host, \
                                           uint32_t index, \
                                           v8::AccessType type, \
                                           v8::Local<v8::Value> data);

DECLARE_PROPERTY_ACCESSOR(CanvasRenderingContext2DStrokeStyle)
DECLARE_PROPERTY_ACCESSOR(CanvasRenderingContext2DFillStyle)
// Customized getter&setter of DOMWindow.location
DECLARE_PROPERTY_ACCESSOR(DOMWindowLocation)
// Customized setter of DOMWindow.opener
DECLARE_PROPERTY_ACCESSOR_SETTER(DOMWindowOpener)

DECLARE_PROPERTY_ACCESSOR(DocumentLocation)
DECLARE_PROPERTY_ACCESSOR(DocumentImplementation)
DECLARE_PROPERTY_ACCESSOR_GETTER(EventSrcElement)
DECLARE_PROPERTY_ACCESSOR(EventReturnValue)
DECLARE_PROPERTY_ACCESSOR_GETTER(EventDataTransfer)
DECLARE_PROPERTY_ACCESSOR_GETTER(EventClipboardData)

// Getter/Setter for window event handlers
DECLARE_PROPERTY_ACCESSOR(DOMWindowEventHandler)
// Getter/Setter for Element event handlers
DECLARE_PROPERTY_ACCESSOR(ElementEventHandler)

// Customized setter of src and location on HTMLFrameElement
DECLARE_PROPERTY_ACCESSOR_SETTER(HTMLFrameElementSrc)
DECLARE_PROPERTY_ACCESSOR_SETTER(HTMLFrameElementLocation)
// Customized setter of src on HTMLIFrameElement
DECLARE_PROPERTY_ACCESSOR_SETTER(HTMLIFrameElementSrc)
// Customized setter of Attr.value
DECLARE_PROPERTY_ACCESSOR_SETTER(AttrValue)

// Customized setter of HTMLOptionsCollection length
DECLARE_PROPERTY_ACCESSOR_SETTER(HTMLOptionsCollectionLength)

DECLARE_NAMED_ACCESS_CHECK(Location)
DECLARE_INDEXED_ACCESS_CHECK(History)

DECLARE_NAMED_ACCESS_CHECK(History)
DECLARE_INDEXED_ACCESS_CHECK(Location)

// HTMLCollection customized functions.
DECLARE_CALLBACK(HTMLCollectionItem)
DECLARE_CALLBACK(HTMLCollectionNamedItem)
// HTMLCollections are callable as functions.
DECLARE_CALLBACK(HTMLCollectionCallAsFunction)

// HTMLSelectElement customized functions.
DECLARE_CALLBACK(HTMLSelectElementRemove)

// HTMLOptionsCollection customized functions.
DECLARE_CALLBACK(HTMLOptionsCollectionRemove)
DECLARE_CALLBACK(HTMLOptionsCollectionAdd)

// HTMLDocument customized functions
DECLARE_CALLBACK(HTMLDocumentWrite)
DECLARE_CALLBACK(HTMLDocumentWriteln)
DECLARE_CALLBACK(HTMLDocumentOpen)
DECLARE_CALLBACK(HTMLDocumentClear)

// Document customized functions
DECLARE_CALLBACK(DocumentEvaluate)

// Window customized functions
DECLARE_CALLBACK(DOMWindowAddEventListener)
DECLARE_CALLBACK(DOMWindowRemoveEventListener)
DECLARE_CALLBACK(DOMWindowPostMessage)
DECLARE_CALLBACK(DOMWindowSetTimeout)
DECLARE_CALLBACK(DOMWindowSetInterval)
DECLARE_CALLBACK(DOMWindowAtob)
DECLARE_CALLBACK(DOMWindowBtoa)
DECLARE_CALLBACK(DOMWindowNOP)
DECLARE_CALLBACK(DOMWindowToString)
DECLARE_CALLBACK(DOMWindowShowModalDialog)
DECLARE_CALLBACK(DOMWindowOpen)

DECLARE_CALLBACK(DOMParserConstructor)
DECLARE_CALLBACK(XMLHttpRequestConstructor)
DECLARE_CALLBACK(XMLSerializerConstructor)
DECLARE_CALLBACK(XPathEvaluatorConstructor)
DECLARE_CALLBACK(XSLTProcessorConstructor)

// Implementation of custom XSLTProcessor methods.
DECLARE_CALLBACK(XSLTProcessorImportStylesheet)
DECLARE_CALLBACK(XSLTProcessorTransformToFragment)
DECLARE_CALLBACK(XSLTProcessorTransformToDocument)
DECLARE_CALLBACK(XSLTProcessorSetParameter)
DECLARE_CALLBACK(XSLTProcessorGetParameter)
DECLARE_CALLBACK(XSLTProcessorRemoveParameter)

// CSSPrimitiveValue customized functions
DECLARE_CALLBACK(CSSPrimitiveValueGetRGBColorValue)

// Canvas 2D customized functions
DECLARE_CALLBACK(CanvasRenderingContext2DSetStrokeColor)
DECLARE_CALLBACK(CanvasRenderingContext2DSetFillColor)
DECLARE_CALLBACK(CanvasRenderingContext2DStrokeRect)
DECLARE_CALLBACK(CanvasRenderingContext2DSetShadow)
DECLARE_CALLBACK(CanvasRenderingContext2DDrawImage)
DECLARE_CALLBACK(CanvasRenderingContext2DDrawImageFromRect)
DECLARE_CALLBACK(CanvasRenderingContext2DCreatePattern)

// Implementation of Clipboard methods.
DECLARE_CALLBACK(ClipboardClearData)
DECLARE_CALLBACK(ClipboardGetData)
DECLARE_CALLBACK(ClipboardSetData)

// Implementation of Element methods.
DECLARE_CALLBACK(ElementSetAttribute)
DECLARE_CALLBACK(ElementSetAttributeNode)
DECLARE_CALLBACK(ElementSetAttributeNS)
DECLARE_CALLBACK(ElementSetAttributeNodeNS)

// Implementation of EventTarget::addEventListener
// and EventTarget::removeEventListener
DECLARE_CALLBACK(EventTargetNodeAddEventListener)
DECLARE_CALLBACK(EventTargetNodeRemoveEventListener)

// Custom implementation of XMLHttpRequest properties
DECLARE_PROPERTY_ACCESSOR_SETTER(XMLHttpRequestOnreadystatechange)
DECLARE_PROPERTY_ACCESSOR_SETTER(XMLHttpRequestOnload)
DECLARE_CALLBACK(XMLHttpRequestAddEventListener)
DECLARE_CALLBACK(XMLHttpRequestRemoveEventListener)
DECLARE_CALLBACK(XMLHttpRequestOpen)
DECLARE_CALLBACK(XMLHttpRequestSend)
DECLARE_CALLBACK(XMLHttpRequestSetRequestHeader)
DECLARE_CALLBACK(XMLHttpRequestGetResponseHeader)
DECLARE_CALLBACK(XMLHttpRequestOverrideMimeType)

// Custom implementation of TreeWalker functions
DECLARE_CALLBACK(TreeWalkerParentNode)
DECLARE_CALLBACK(TreeWalkerFirstChild)
DECLARE_CALLBACK(TreeWalkerLastChild)
DECLARE_CALLBACK(TreeWalkerNextNode)
DECLARE_CALLBACK(TreeWalkerPreviousNode)
DECLARE_CALLBACK(TreeWalkerNextSibling)
DECLARE_CALLBACK(TreeWalkerPreviousSibling)

// Custom implementation of NodeIterator functions
DECLARE_CALLBACK(NodeIteratorNextNode)
DECLARE_CALLBACK(NodeIteratorPreviousNode)

// Custom implementation of NodeFilter function
DECLARE_CALLBACK(NodeFilterAcceptNode)

DECLARE_NAMED_PROPERTY_GETTER(DOMWindow)
DECLARE_INDEXED_PROPERTY_GETTER(DOMWindow)
DECLARE_NAMED_ACCESS_CHECK(DOMWindow)
DECLARE_INDEXED_ACCESS_CHECK(DOMWindow)

DECLARE_NAMED_PROPERTY_GETTER(HTMLFrameSetElement)
DECLARE_NAMED_PROPERTY_GETTER(HTMLFormElement)
DECLARE_NAMED_PROPERTY_GETTER(HTMLDocument)
DECLARE_NAMED_PROPERTY_SETTER(HTMLDocument)
DECLARE_NAMED_PROPERTY_DELETER(HTMLDocument)
DECLARE_NAMED_PROPERTY_GETTER(NodeList)
DECLARE_NAMED_PROPERTY_GETTER(NamedNodeMap)
DECLARE_NAMED_PROPERTY_GETTER(CSSStyleDeclaration)
DECLARE_NAMED_PROPERTY_SETTER(CSSStyleDeclaration)
DECLARE_NAMED_PROPERTY_GETTER(HTMLPlugInElement)
DECLARE_NAMED_PROPERTY_SETTER(HTMLPlugInElement)
DECLARE_INDEXED_PROPERTY_GETTER(HTMLPlugInElement)
DECLARE_INDEXED_PROPERTY_SETTER(HTMLPlugInElement)

// Plugin object can be called as function.
DECLARE_CALLBACK(HTMLPlugInElement)

DECLARE_NAMED_PROPERTY_GETTER(StyleSheetList)
DECLARE_INDEXED_PROPERTY_GETTER(NamedNodeMap)
DECLARE_INDEXED_PROPERTY_GETTER(HTMLFormElement)
DECLARE_INDEXED_PROPERTY_GETTER(HTMLOptionsCollection)
DECLARE_INDEXED_PROPERTY_SETTER(HTMLOptionsCollection)
DECLARE_INDEXED_PROPERTY_SETTER(HTMLSelectElementCollection)
DECLARE_NAMED_PROPERTY_GETTER(HTMLCollection)

// SVG custom properties and callbacks
#if ENABLE(SVG)
DECLARE_CALLBACK(SVGMatrixInverse)
DECLARE_CALLBACK(SVGMatrixRotateFromVector)
#endif

#undef DECLARE_INDEXED_ACCESS_CHECK
#undef DECLARE_NAMED_ACCESS_CHECK

#undef DECLARE_PROPERTY_ACCESSOR_SETTER
#undef DECLARE_PROPERTY_ACCESSOR_GETTER
#undef DECLARE_PROPERTY_ACCESSOR

#undef DECLARE_NAMED_PROPERTY_GETTER
#undef DECLARE_NAMED_PROPERTY_SETTER
#undef DECLARE_NAMED_PROPERTY_DELETER

#undef DECLARE_INDEXED_PROPERTY_GETTER
#undef DECLARE_INDEXED_PROPERTY_SETTER
#undef DECLARE_INDEXED_PROPERTY_DELETER

#undef DECLARE_CALLBACK

  // Returns the NPObject corresponding to an HTMLElement object.
  static NPObject* GetHTMLPlugInElementNPObject(v8::Handle<v8::Object> object);

  // Returns the owner frame pointer of a DOM wrapper object. It only works for
  // these DOM objects requiring cross-domain access check.
  static Frame* GetTargetFrame(v8::Local<v8::Object> host,
                               v8::Local<v8::Value> data);

  // Special case for downcasting SVG path segments
#if ENABLE(SVG)
  static V8ClassIndex::V8WrapperType DowncastSVGPathSeg(void* path_seg);
#endif

 private:
  static v8::Handle<v8::Value> WindowSetTimeoutImpl(const v8::Arguments& args,
                                                    bool single_shot);
};


// A template of named property accessor of collections.
template <class C>
static v8::Handle<v8::Value> CollectionNamedPropertyGetter(
    v8::Local<v8::String> name, const v8::AccessorInfo& info) {
  return GetNamedPropertyOfCollection<C>(name, info.Holder(), info.Data());
}


// Returns named property of a collection.
template <class C>
static v8::Handle<v8::Value> GetNamedPropertyOfCollection(
    v8::Local<v8::String> name,
    v8::Local<v8::Object> object,
    v8::Local<v8::Value> data) {
  // TODO: assert object is a collection type
  ASSERT(V8Proxy::MaybeDOMWrapper(object));

  V8ClassIndex::V8WrapperType t = V8Proxy::GetDOMWrapperType(object);
  C* collection = V8Proxy::FastToNativeObject<C>(t, object);
  String prop_name = ToWebCoreString(name);
  void* result = collection->namedItem(prop_name);
  if (!result) return v8::Handle<v8::Value>();
  V8ClassIndex::V8WrapperType type = V8ClassIndex::ToWrapperType(data);
  return V8Proxy::ToV8Object(type, result);
}


// A template returns whether a collection has a named property.
// This function does not cause JS heap allocation.
template <class C>
static bool HasNamedPropertyOfCollection(v8::Local<v8::String> name,
                                         v8::Local<v8::Object> object,
                                         v8::Local<v8::Value> data) {
  // TODO: assert object is a collection type
  ASSERT(V8Proxy::MaybeDOMWrapper(object));

  V8ClassIndex::V8WrapperType t = V8Proxy::GetDOMWrapperType(object);
  C* collection = V8Proxy::FastToNativeObject<C>(t, object);
  String prop_name = ToWebCoreString(name);
  void* result = collection->namedItem(prop_name);
  return result != NULL;
}


// A template of index interceptor of collections.
template <class C>
static v8::Handle<v8::Value> CollectionIndexedPropertyGetter(
    uint32_t index, const v8::AccessorInfo& info) {
  return GetIndexedPropertyOfCollection<C>(index, info.Holder(), info.Data());
}


// Returns the property at the index of a collection.
template <class C>
static v8::Handle<v8::Value> GetIndexedPropertyOfCollection(
    uint32_t index, v8::Local<v8::Object> object, v8::Local<v8::Value> data) {
  // TODO, assert that object must be a collection type
  ASSERT(V8Proxy::MaybeDOMWrapper(object));
  V8ClassIndex::V8WrapperType t = V8Proxy::GetDOMWrapperType(object);
  C* collection = V8Proxy::FastToNativeObject<C>(t, object);
  void* result = collection->item(index);
  if (!result) return v8::Handle<v8::Value>();
  V8ClassIndex::V8WrapperType type = V8ClassIndex::ToWrapperType(data);
  return V8Proxy::ToV8Object(type, result);
}


// Get an array containing the names of indexed properties in a collection.
template <class C>
static v8::Handle<v8::Array> CollectionIndexedPropertyEnumerator(
    const v8::AccessorInfo& info) {
  ASSERT(V8Proxy::MaybeDOMWrapper(info.Holder()));
  V8ClassIndex::V8WrapperType t = V8Proxy::GetDOMWrapperType(info.Holder());
  C* collection = V8Proxy::FastToNativeObject<C>(t, info.Holder());
  int length = collection->length();
  v8::Handle<v8::Array> properties = v8::Array::New(length);
  for (int i = 0; i < length; i++) {
    // TODO(ager): Do we need to check that the item function returns
    // a non-null value for this index?
    v8::Handle<v8::Integer> integer = v8::Integer::New(i);
    properties->Set(integer, integer);
  }
  return properties;
}


// Returns whether a collection has a property at a given index.
// This function does not cause JS heap allocation.
template <class C>
static bool HasIndexedPropertyOfCollection(uint32_t index,
                                           v8::Local<v8::Object> object,
                                           v8::Local<v8::Value> data) {
  // TODO, assert that object must be a collection type
  ASSERT(V8Proxy::MaybeDOMWrapper(object));
  V8ClassIndex::V8WrapperType t = V8Proxy::GetDOMWrapperType(object);
  C* collection = V8Proxy::FastToNativeObject<C>(t, object);
  void* result = collection->item(index);
  return result != NULL;
}


// A template for indexed getters on collections of strings that should return
// null if the resulting string is a null string.
template <class C>
static v8::Handle<v8::Value> CollectionStringOrNullIndexedPropertyGetter(
    uint32_t index, const v8::AccessorInfo& info) {
  // TODO, assert that object must be a collection type
  ASSERT(V8Proxy::MaybeDOMWrapper(info.Holder()));
  V8ClassIndex::V8WrapperType t = V8Proxy::GetDOMWrapperType(info.Holder());
  C* collection = V8Proxy::FastToNativeObject<C>(t, info.Holder());
  String result = collection->item(index);
  return v8StringOrNull(result);
}

}  // namespace WebCore

#endif  // V8_CUSTOM_H__
