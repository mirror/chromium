#!/usr/bin/python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This is a helper tool for extracting a list of C++ methods referenced from IDL
# files.  This list is used as an auxiliary input to rewrite_to_chrome_style
# clang tool (see --method-blocklist command line parameter).

# pylint: disable=W0403
import os
import sys
from optparse import OptionParser

from idl_reader import IdlReader
from idl_definitions import Visitor
from v8_utilities import (capitalize,
                          extended_attribute_value_as_list,
                          has_extended_attribute,
                          uncapitalize)
from v8_types import (is_explicit_nullable, use_output_parameter_for_result)


def get_name(node):
    """ Returns the name of |node|.
    """
    if has_extended_attribute(node, ["ImplementedAs"]):
        return node.extended_attributes["ImplementedAs"]
    # PrefixGet is not used in production code right now.
    return node.name


def get_extra_param_count_for_return_value(return_idl_type):
    """ Returns number of extra parameters for handling the return value.
    """
    extra_param_count = 0
    if use_output_parameter_for_result(return_idl_type):
        extra_param_count += 1
    if is_explicit_nullable(return_idl_type):
        extra_param_count += 1
    return extra_param_count


def get_extra_param_count(node, special_name=""):
    """ Returns number of extra parameters injected by extended attributes.

    Some extended attributes (like [RaisesException], [CallWith], etc.) require
    the C++ method to accept extra parameters.  This function helps count them.

    |special_name| if present will be used to match both [RaisesException]
    and [RaisesException=<special_name>] (e.g. [RaisesException=Getter]),
    but not [RaisesException=<different name>].
    """
    extra_param_count = 0

    if has_extended_attribute(node, ["PostMessage"]):
        extra_param_count += 1

    if has_extended_attribute(node, ["RaisesException"]):
        specific_names = extended_attribute_value_as_list(node, "RaisesException")
        if specific_names == [None]:
            # [RaisesException]
            extra_param_count += 1
        elif special_name in specific_names:
            # [RaisesException=(foo,bar)] and |special_name| matches foo or bar.
            extra_param_count += 1

    # Helper for handling CallWith, GetterCallWith, etc.
    def get_extra_param_count_for_call_with(node, ext_attr_name):
        if not has_extended_attribute(node, [ext_attr_name]):
            return 0
        list_of_extra_args = extended_attribute_value_as_list(node, ext_attr_name)
        return len(list_of_extra_args)

    extra_param_count += get_extra_param_count_for_call_with(node, "CallWith")
    if special_name:
        extra_param_count += get_extra_param_count_for_call_with(node, special_name + "CallWith")

    return extra_param_count


class ExtractingVisitor(Visitor):
    def __init__(self, filename):
        self.__interface = None
        self.__filename = filename

    def __print_method(self, method_name, number_of_parameters, is_static_method=False):
        is_classname_from_filename = False
        if (has_extended_attribute(self.__interface, ["LegacyTreatAsPartialInterface"]) or
                self.__interface.is_partial):
            if not is_static_method:
                number_of_parameters += 1
            if not has_extended_attribute(self.__interface, ["ImplementedAs"]):
                is_classname_from_filename = True

        if is_classname_from_filename:
            class_name = os.path.splitext(os.path.basename(self.__filename))[0]
        else:
            class_name = get_name(self.__interface)

        print "%s:::%s:::%d" % (class_name, method_name, number_of_parameters)
        if self.__interface.name == "Window":
            print "%s:::%s:::%d" % ("LocalDOMWindow", method_name, number_of_parameters)

    def visit_interface(self, interface):
        self.__interface = interface
        if has_extended_attribute(interface, ["SetWrapperReferenceFrom"]):
            self.__print_method(interface.extended_attributes["SetWrapperReferenceFrom"], 0)
        if has_extended_attribute(interface, ["SetWrapperReferenceTo"]):
            self.__print_method(interface.extended_attributes["SetWrapperReferenceTo"].name, 0)
        if interface.serializer:
          serializer_implemented_as = None
          if interface.serializer.operation:
            seralizer_implemented_as = interface.serializer.operation.name
          if not serializer_implemented_as:
            self.__print_method('toJSONForBinding', 0)
        if interface.stringifier:
          if not interface.stringifier.attribute and not interface.stringifier.operation:
            self.__print_method('toString', 0)

    def visit_operation(self, operation):
        if operation.is_constructor:
            return

        operation_name = get_name(operation)
        number_of_extra_parameters = get_extra_param_count(operation)
        number_of_extra_parameters += get_extra_param_count_for_return_value(operation.idl_type)
        number_of_mandatory_parameters = len([
            arg for arg in operation.arguments if not arg.is_optional])
        for number_of_parameters in range(number_of_mandatory_parameters, len(operation.arguments) + 1):
            self.__print_method(
                operation_name, number_of_parameters + number_of_extra_parameters,
                is_static_method=operation.is_static)

        # Bindings generator assumes that if an implementation supports
        # anonymousIndexedGetter(...) method, then the implementations also
        # supports length() method.
        if 'getter' in operation.specials and str(operation.arguments[0].idl_type) == 'unsigned long':
            self.__print_method("length", 0,)

    def visit_attribute(self, attribute):
        if has_extended_attribute(attribute, ["Reflect"]):
            return  # No attribute-specific method.

        # CachedAttribute.
        if has_extended_attribute(attribute, ["CachedAttribute"]):
            self.__print_method(attribute.extended_attributes["CachedAttribute"], 0)

        # Getter.
        attribute_name = uncapitalize(get_name(attribute))
        self.__print_method(attribute_name,
                            get_extra_param_count_for_return_value(attribute.idl_type) +
                            get_extra_param_count(attribute, "Getter"),
                            is_static_method=attribute.is_static)

        # Setter.
        if (not attribute.is_read_only and
                not has_extended_attribute(attribute, ["PutForwards"])):
            self.__print_method("set" + capitalize(attribute_name),
                                1 + get_extra_param_count(attribute, "Setter"),
                                is_static_method=attribute.is_static)

    def visit_dictionary(self, dictionary):
        # No need to worry about IDL dictionaries - their header files are in
        # generated code (and consequently their C++ methods will not be renamed by
        # the rewrite_to_chrome_style clang tool).
        pass


def parse_options():
    parser = OptionParser()
    options, args = parser.parse_args()
    if len(args) != 1:
        parser.error('Must specify exactly 1 input file as argument, but %d given.' % len(args))
    idl_filename = os.path.realpath(args[0])
    return options, idl_filename


def print_base_implementations():
    """ Prints IDL-related C++ methods implemented by *base* class.

    When seeing getElementsByTagName in Document.idl, the ExtractingVisitor above
    will print out an entry for blocking Document::getElementsByTagName.  This is
    not quite right - getElementsByTagName is not implemented by blink::Document,
    but by one of its base classes - by blink::ContainerNode.  There are many
    examples like this - they are currently manually gathered below.
    """
    print """
# Methods declared in a base class:
BaseAudioContext:::getOutputTimestamp:::1
BaseAudioContext:::suspendContext:::1
BaseRenderingContext2D:::beginPath:::0
BaseRenderingContext2D:::clearRect:::4
BaseRenderingContext2D:::clip:::1
BaseRenderingContext2D:::clip:::2
BaseRenderingContext2D:::createImageData:::2
BaseRenderingContext2D:::createImageData:::3
BaseRenderingContext2D:::createLinearGradient:::4
BaseRenderingContext2D:::createPattern:::4
BaseRenderingContext2D:::createRadialGradient:::7
BaseRenderingContext2D:::currentTransform:::0
BaseRenderingContext2D:::drawImage:::11
BaseRenderingContext2D:::drawImage:::5
BaseRenderingContext2D:::drawImage:::7
BaseRenderingContext2D:::fill:::1
BaseRenderingContext2D:::fill:::2
BaseRenderingContext2D:::fillRect:::4
BaseRenderingContext2D:::fillStyle:::1
BaseRenderingContext2D:::filter:::0
BaseRenderingContext2D:::getImageData:::5
BaseRenderingContext2D:::getLineDash:::0
BaseRenderingContext2D:::globalAlpha:::0
BaseRenderingContext2D:::globalCompositeOperation:::0
BaseRenderingContext2D:::imageSmoothingEnabled:::0
BaseRenderingContext2D:::imageSmoothingQuality:::0
BaseRenderingContext2D:::isContextLost:::0
BaseRenderingContext2D:::isPointInPath:::3
BaseRenderingContext2D:::isPointInPath:::4
BaseRenderingContext2D:::isPointInStroke:::2
BaseRenderingContext2D:::isPointInStroke:::3
BaseRenderingContext2D:::lineCap:::0
BaseRenderingContext2D:::lineDashOffset:::0
BaseRenderingContext2D:::lineJoin:::0
BaseRenderingContext2D:::lineWidth:::0
BaseRenderingContext2D:::miterLimit:::0
BaseRenderingContext2D:::putImageData:::4
BaseRenderingContext2D:::putImageData:::8
BaseRenderingContext2D:::resetTransform:::0
BaseRenderingContext2D:::restore:::0
BaseRenderingContext2D:::rotate:::1
BaseRenderingContext2D:::save:::0
BaseRenderingContext2D:::scale:::2
BaseRenderingContext2D:::setCurrentTransform:::1
BaseRenderingContext2D:::setFillStyle:::1
BaseRenderingContext2D:::setFilter:::1
BaseRenderingContext2D:::setGlobalAlpha:::1
BaseRenderingContext2D:::setGlobalCompositeOperation:::1
BaseRenderingContext2D:::setImageSmoothingEnabled:::1
BaseRenderingContext2D:::setImageSmoothingQuality:::1
BaseRenderingContext2D:::setLineCap:::1
BaseRenderingContext2D:::setLineDash:::1
BaseRenderingContext2D:::setLineDashOffset:::1
BaseRenderingContext2D:::setLineJoin:::1
BaseRenderingContext2D:::setLineWidth:::1
BaseRenderingContext2D:::setMiterLimit:::1
BaseRenderingContext2D:::setShadowBlur:::1
BaseRenderingContext2D:::setShadowColor:::1
BaseRenderingContext2D:::setShadowOffsetX:::1
BaseRenderingContext2D:::setShadowOffsetY:::1
BaseRenderingContext2D:::setStrokeStyle:::1
BaseRenderingContext2D:::setTransform:::6
BaseRenderingContext2D:::shadowBlur:::0
BaseRenderingContext2D:::shadowColor:::0
BaseRenderingContext2D:::shadowOffsetX:::0
BaseRenderingContext2D:::shadowOffsetY:::0
BaseRenderingContext2D:::stroke:::0
BaseRenderingContext2D:::stroke:::1
BaseRenderingContext2D:::strokeRect:::4
BaseRenderingContext2D:::strokeStyle:::1
BaseRenderingContext2D:::transform:::6
BaseRenderingContext2D:::translate:::2
Body:::body:::1
Body:::bodyWithUseCounter:::1
CanvasRenderingContext:::canvas:::0
CanvasRenderingContext:::clearRect:::4
CanvasRenderingContext:::isContextLost:::0
CanvasRenderingContext:::offscreenCanvas:::0
CanvasRenderingContext:::setFont:::1
ContainerNode:::getElementById:::1
ContainerNode:::getElementsByClassName:::1
ContainerNode:::getElementsByName:::1
ContainerNode:::getElementsByTagName:::1
ContainerNode:::getElementsByTagNameNS:::2
CSSRule:::cssRules:::0
CSSStyleSheet:::item:::1
Document:::all:::0
DOMFileSystemBase:::name:::0
DOMMatrixReadOnly:::a:::0
DOMMatrixReadOnly:::b:::0
DOMMatrixReadOnly:::c:::0
DOMMatrixReadOnly:::d:::0
DOMMatrixReadOnly:::e:::0
DOMMatrixReadOnly:::f:::0
DOMMatrixReadOnly:::m11:::0
DOMMatrixReadOnly:::m12:::0
DOMMatrixReadOnly:::m13:::0
DOMMatrixReadOnly:::m14:::0
DOMMatrixReadOnly:::m21:::0
DOMMatrixReadOnly:::m22:::0
DOMMatrixReadOnly:::m23:::0
DOMMatrixReadOnly:::m24:::0
DOMMatrixReadOnly:::m31:::0
DOMMatrixReadOnly:::m32:::0
DOMMatrixReadOnly:::m33:::0
DOMMatrixReadOnly:::m34:::0
DOMMatrixReadOnly:::m41:::0
DOMMatrixReadOnly:::m42:::0
DOMMatrixReadOnly:::m43:::0
DOMMatrixReadOnly:::m44:::0
DOMURLUtilsReadOnly:::hash:::0
DOMURLUtilsReadOnly:::host:::0
DOMURLUtilsReadOnly:::hostname:::0
DOMURLUtilsReadOnly:::href:::0
DOMURLUtilsReadOnly:::origin:::0
DOMURLUtilsReadOnly:::password:::0
DOMURLUtilsReadOnly:::pathname:::0
DOMURLUtilsReadOnly:::port:::0
DOMURLUtilsReadOnly:::protocol:::0
DOMURLUtilsReadOnly:::search:::0
DOMURLUtilsReadOnly:::username:::0
DOMURLUtils:::setHash:::1
DOMURLUtils:::setHost:::1
DOMURLUtils:::setHostname:::1
DOMURLUtils:::setHref:::1
DOMURLUtils:::setPassword:::1
DOMURLUtils:::setPathname:::1
DOMURLUtils:::setPort:::1
DOMURLUtils:::setProtocol:::1
DOMURLUtils:::setSearch:::1
DOMURLUtils:::setUsername:::1
DOMWindowBase64:::atob:::2
DOMWindowBase64:::btoa:::2
Element:::blur:::0
Element:::dataset:::0
Element:::focus:::1
Element:::innerText:::0
Element:::outerText:::0
Element:::setTabIndex:::1
Element:::style:::0
Element:::styleMap:::0
Element:::title:::0
Element:::willValidate:::0
EntryBase:::filesystem:::0
EntryBase:::fullPath:::0
EntryBase:::isDirectory:::0
EntryBase:::isFile:::0
EntryBase:::name:::0
EntryBase:::toURL:::0
FileWriterBase:::length:::0
FileWriterBase:::position:::0
FileWriterBaseCallback:::handleEvent:::1
HTMLCollection:::length:::0
HTMLElement:::formOwner:::0
HTMLFormControlElement:::checkValidity:::2
HTMLFormControlElement:::formAction:::0
HTMLFormControlElement:::formEnctype:::0
HTMLFormControlElement:::formMethod:::0
HTMLFormControlElement:::formOwner:::0
HTMLFormControlElement:::reportValidity:::0
HTMLFormControlElement:::setCustomValidity:::1
HTMLFormControlElement:::setFormAction:::1
HTMLFormControlElement:::setFormEnctype:::1
HTMLFormControlElement:::setFormMethod:::1
HTMLFormControlElement:::type:::0
HTMLFrameOwnerElement:::contentDocument:::0
HTMLFrameOwnerElement:::contentWindow:::0
HTMLFrameOwnerElement:::getSVGDocument:::1
HTMLMediaSource:::duration:::0
IDBCursor:::isValueDirty:::0
IDBCursor:::value:::1
Image:::height:::0
Image:::width:::0
InProcessWorkerBase:::postMessage:::4
InProcessWorkerBase:::terminate:::0
InsertionPoint:::getDistributedNodes:::0
LabelableElement:::labels:::0
ListedElement:::setCustomValidity:::1
ListedElement:::validationMessage:::0
ListedElement:::validity:::0
LiveNodeListBase:::ownerNode:::0
LiveNodeList:::item:::1
MIDIPort:::midiAccess:::0
MIDIPortMap:::size:::0
MouseEvent:::getDataTransfer:::0
Node:::assignedSlotForBinding:::0
Node:::getDestinationInsertionPoints:::0
Node:::remove:::1
Node:::tabIndex:::0
NodeIteratorBase:::filter:::0
NodeIteratorBase:::root:::0
NodeIteratorBase:::whatToShow:::0
PerformanceBase:::clearFrameTimings:::0
PerformanceBase:::clearMarks:::1
PerformanceBase:::clearMeasures:::1
PerformanceBase:::clearResourceTimings:::0
PerformanceBase:::getEntries:::0
PerformanceBase:::getEntriesByName:::2
PerformanceBase:::getEntriesByType:::1
PerformanceBase:::mark:::2
PerformanceBase:::measure:::4
PerformanceBase:::now:::0
PerformanceBase:::setFrameTimingBufferSize:::1
PerformanceBase:::setResourceTimingBufferSize:::1
PerformanceBase:::timing:::0
ScriptElementBase:::nonce:::0
ScriptElementBase:::setNonce:::1
ScriptLoaderClient:::nonce:::0
ScriptLoaderClient:::setNonce:::1
SecurityContext:::addressSpaceForBindings:::0
ShapeDetector:::detect:::2
StyleElement:::media:::0
StyleElement:::sheet:::0
StyleElement:::type:::0
StyleSheet:::ownerRule:::0
SVGAnimatedProperty:::animVal:::0
SVGAnimatedProperty:::baseVal:::0
SVGAnimatedProperty:::setBaseVal:::2
SVGAnimatedPropertyBase:::contextElement:::0
SVGFELightElement:::azimuth:::0
SVGFELightElement:::elevation:::0
SVGFELightElement:::limitingConeAngle:::0
SVGFELightElement:::pointsAtX:::0
SVGFELightElement:::pointsAtY:::0
SVGFELightElement:::pointsAtZ:::0
SVGFELightElement:::specularExponent:::0
SVGFELightElement:::x:::0
SVGFELightElement:::y:::0
SVGFELightElement:::z:::0
SVGListPropertyTearOffHelper:::appendItem:::2
SVGListPropertyTearOffHelper:::clear:::1
SVGListPropertyTearOffHelper:::getItem:::2
SVGListPropertyTearOffHelper:::initialize:::2
SVGListPropertyTearOffHelper:::insertItemBefore:::3
SVGListPropertyTearOffHelper:::length:::0
SVGListPropertyTearOffHelper:::removeItem:::2
SVGListPropertyTearOffHelper:::replaceItem:::3
SVGPolyElement:::animatedPoints:::0
SVGPolyElement:::pointsFromJavascript:::0
SVGPropertyTearOffBase:::contextElement:::0
SVGSMILElement:::targetElement:::0
TextControlElement:::autocapitalize:::0
TextControlElement:::maxLength:::0
TextControlElement:::minLength:::0
TextControlElement:::select:::0
TextControlElement:::selectionDirection:::0
TextControlElement:::selectionEnd:::0
TextControlElement:::selectionStart:::0
TextControlElement:::setAutocapitalize:::1
TextControlElement:::setMaxLength:::2
TextControlElement:::setMinLength:::2
TextControlElement:::setRangeText:::2
TextControlElement:::setRangeText:::5
TextControlElement:::setSelectionDirection:::1
TextControlElement:::setSelectionEnd:::1
TextControlElement:::setSelectionRangeForBinding:::3
TextControlElement:::setSelectionStart:::1
TextControlElement:::setValue:::2
TextControlElement:::value:::0
TrackBase:::id:::0
TrackBase:::kind:::0
TrackBase:::label:::0
TrackBase:::language:::0
TrackBase:::owner:::0
TrackListBase:::getTrackById:::1
TrackListBase:::length:::0
TrackListBase:::owner:::0
UIEventWithKeyState:::altKey:::0
UIEventWithKeyState:::ctrlKey:::0
UIEventWithKeyState:::getModifierState:::1
UIEventWithKeyState:::metaKey:::0
UIEventWithKeyState:::shiftKey:::0
WebGL2RenderingContextBase:::bufferData:::3
WebGLExtension:::canvas:::0
WorkerGlobalScope:::close:::0

# Things straddling gen and non-gn code:
WrapperTypeInfo:::domTemplate:::2
      """


def print_known_issues():
    """ Prints methods that are not currently properly picked up by ExtractingVisitor.
    """
    print """
# TODO(lukasza): No idea why use_output_parameter_for_result incorrectly returns
# False for operations below (i.e. why it doesn't see that the IDL interface
# types involved are unions and/or dictionaries):
AnimationEffectReadOnly:::getComputedTiming:::1
Document:::currentScriptForBinding:::1
FormData:::get:::2

# TODO(lukasza): Despite being defined in NavigatorStorageUtils.idl (without
# ImplementedAs extended attribute), the method below is exposed via a different
# C++ class - via blink::Navigator.
Navigator:::cookieEnabled:::0
      """


def main():
    _, input_filename = parse_options()

    # Print C++ methods associated with IDL definition in |input_filename|.
    idl_reader = IdlReader()
    idl_definitions = idl_reader.read_idl_file(input_filename)
    idl_definitions.accept(ExtractingVisitor(input_filename))

    # Print hardcoded lists of C++ methods associated with IDL definitions.
    print_base_implementations()
    print_known_issues()


if __name__ == '__main__':
    sys.exit(main())
