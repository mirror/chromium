// * |globalObject| should be the global (usually |this|).
// * |propertyNamesInGlobal| should be a list of properties captured before
//   other scripts are loaded, using: Object.getOwnPropertyNames(this);
// * |platformSpecific| determines the platform-filtering of interfaces and
//   properties. Only platform-specific interfaces/properties will be tested if
//   set to true, and only all-platform interfaces/properties will be used
//   if set to false.
// * |outputFunc| is called back with each line of output.

function globalInterfaceListing(globalObject, propertyNamesInGlobal, platformSpecific, outputFunc) {

// List of builtin JS constructors; Blink is not controlling what properties these
// objects have, so exercising them in a Blink test doesn't make sense.
//
// If new builtins are added, please update this list along with the one in
// LayoutTests/http/tests/serviceworker/webexposed/resources/global-interface-listing-worker.js
var jsBuiltins = new Set([
    'Array',
    'ArrayBuffer',
    'Boolean',
    'Date',
    'Error',
    'EvalError',
    'Float32Array',
    'Float64Array',
    'Function',
    'Infinity',
    'Int16Array',
    'Int32Array',
    'Int8Array',
    'Intl',
    'JSON',
    'Map',
    'Math',
    'NaN',
    'Number',
    'Object',
    'Promise',
    'Proxy',
    'RangeError',
    'ReferenceError',
    'Reflect',
    'RegExp',
    'Set',
    'String',
    'Symbol',
    'SyntaxError',
    'TypeError',
    'URIError',
    'Uint16Array',
    'Uint32Array',
    'Uint8Array',
    'Uint8ClampedArray',
    'WeakMap',
    'WeakSet',
    'WebAssembly',
    'decodeURI',
    'decodeURIComponent',
    'encodeURI',
    'encodeURIComponent',
    'escape',
    'eval',
    'isFinite',
    'isNaN',
    'parseFloat',
    'parseInt',
    'undefined',
    'unescape',
]);

function isWebIDLConstructor(propertyKey) {
    if (jsBuiltins.has(propertyKey))
        return false;
    var descriptor = Object.getOwnPropertyDescriptor(this, propertyKey);
    if (descriptor.value == undefined || descriptor.value.prototype == undefined)
        return false;
    return descriptor.writable && !descriptor.enumerable && descriptor.configurable;
}

var wellKnownSymbols = new Map([
    [Symbol.hasInstance, "@@hasInstance"],
    [Symbol.isConcatSpreadable, "@@isConcatSpreadable"],
    [Symbol.iterator, "@@iterator"],
    [Symbol.match, "@@match"],
    [Symbol.replace, "@@replace"],
    [Symbol.search, "@@search"],
    [Symbol.species, "@@species"],
    [Symbol.split, "@@split"],
    [Symbol.toPrimitive, "@@toPrimitive"],
    [Symbol.toStringTag, "@@toStringTag"],
    [Symbol.unscopables, "@@unscopables"]
]);

// List of all platform-specific interfaces. Please update this list when adding
// a new platform-specific interface.
var platformSpecificInterfaces = new Set([
    'AbsoluteOrientationSensor',
    'Accelerometer',
    'AccessibleNode',
    'AccessibleNodeList',
    'AmbientLightSensor',
    'Animation',
    'AnimationEffectReadOnly',
    'AnimationEffectTiming',
    'AnimationEffectTimingReadOnly',
    'AnimationPlaybackEvent',
    'AnimationTimeline',
    'AudioParamMap',
    'AudioTrack',
    'AudioTrackList',
    'AuthenticatorAssertionResponse',
    'AuthenticatorAttestationResponse',
    'AuthenticatorResponse',
    'BackgroundFetchFetch',
    'BackgroundFetchManager',
    'BackgroundFetchRegistration',
    'BarcodeDetector',
    'Bluetooth',
    'BluetoothCharacteristicProperties',
    'BluetoothDevice',
    'BluetoothRemoteGATTCharacteristic',
    'BluetoothRemoteGATTDescriptor',
    'BluetoothRemoteGATTServer',
    'BluetoothRemoteGATTService',
    'BluetoothUUID',
    'BudgetState',
    'CSSImageValue',
    'CSSKeywordValue',
    'CSSMatrixComponent',
    'CSSNumericValue',
    'CSSPerspective',
    'CSSPositionValue',
    'CSSResourceValue',
    'CSSRotation',
    'CSSScale',
    'CSSSkew',
    'CSSStyleValue',
    'CSSTransformComponent',
    'CSSTransformValue',
    'CSSTranslation',
    'CSSURLImageValue',
    'CSSUnitValue',
    'CSSUnparsedValue',
    'CSSVariableReferenceValue',
    'CSSViewportRule',
    'DetectedBarcode',
    'DetectedFace',
    'DetectedText',
    'DocumentTimeline',
    'FaceDetector',
    'GamepadPose',
    'Gyroscope',
    'IDBObservation',
    'IDBObserver',
    'IDBObserverChanges',
    'KeyframeEffect',
    'KeyframeEffectReadOnly',
    'LinearAccelerationSensor',
    'Magnetometer',
    'MediaCapabilities',
    'MediaCapabilitiesInfo',
    'MediaKeysPolicy',
    'MediaMetadata',
    'MediaSession',
    'Mojo',
    'MojoHandle',
    'MojoInterfaceInterceptor',
    'MojoInterfaceRequestEvent',
    'MojoWatcher',
    'NFC',
    'OffscreenCanvas',
    'OffscreenCanvasRenderingContext2D',
    'OrientationSensor',
    'PaymentInstruments',
    'PaymentManager',
    'PerformanceServerTiming',
    'PublicKeyCredential',
    'RTCRtpSender',
    'RTCTrackEvent',
    'RelativeOrientationSensor',
    'ReportingObserver',
    'ResizeObserver',
    'ResizeObserverEntry',
    'ScrollTimeline',
    'Sensor',
    'SensorErrorEvent',
    'StylePropertyMap',
    'StylePropertyMapReadonly',
    'TextDetector',
    'TrackDefault',
    'TrackDefaultList',
    'TrustedHTML',
    'TrustedScriptURL',
    'TrustedURL',
    'VR',
    'VRDevice',
    'VRDeviceEvent',
    'VRDisplay',
    'VRDisplayCapabilities',
    'VRDisplayEvent',
    'VREyeParameters',
    'VRFrameData',
    'VRPose',
    'VRSession',
    'VRSessionEvent',
    'VRStageParameters',
    'VTTRegion',
    'VideoPlaybackQuality',
    'VideoTrack',
    'VideoTrackList',
    'Worklet',
    'WorkletAnimation',
]);

// List of all platform-specific properties on interfaces that appear on all
// platforms. Please update this list when adding a new platform-specific
// property to an all-platform interface.
var platformSpecificProperties = {
    BudgetService: new Set([
        'method getBudget',
        'method getCost',
    ]),
    CSS: new Set([
        'static getter paintWorklet',
        'static method Hz',
        'static method ch',
        'static method cm',
        'static method deg',
        'static method dpcm',
        'static method dpi',
        'static method dppx',
        'static method em',
        'static method ex',
        'static method fr',
        'static method grad',
        'static method in',
        'static method kHz',
        'static method mm',
        'static method ms',
        'static method number',
        'static method pc',
        'static method percent',
        'static method pt',
        'static method px',
        'static method rad',
        'static method registerProperty',
        'static method rem',
        'static method s',
        'static method turn',
        'static method vh',
        'static method vmax',
        'static method vmin',
        'static method vw',
    ]),
    CSSRule: new Set([
        'attribute VIEWPORT_RULE',
    ]),
    CanvasPattern: new Set([
        'method setTransform',
    ]),
    CanvasRenderingContext2D: new Set([
        'getter currentTransform',
        'getter direction',
        'method addHitRegion',
        'method clearHitRegions',
        'method getContextAttributes',
        'method isContextLost',
        'method removeHitRegion',
        'method scrollPathIntoView',
        'setter currentTransform',
        'setter direction',
    ]),
    Document: new Set([
        'getter fullscreenElement',
        'getter fullscreenEnabled',
        'getter onfullscreenchange',
        'getter onfullscreenerror',
        'getter onsecuritypolicyviolation',
        'getter rootScroller',
        'getter suborigin',
        'getter timeline',
        'method exitFullscreen',
        'method getAnimations',
        'setter onfullscreenchange',
        'setter onfullscreenerror',
        'setter onsecuritypolicyviolation',
        'setter rootScroller',
    ]),
    Element: new Set([
        'getter accessibleNode',
        'getter computedName',
        'getter computedRole',
        'getter onfullscreenchange',
        'getter onfullscreenerror',
        'getter styleMap',
        'method getAnimations',
        'method requestFullscreen',
        'setter onfullscreenchange',
        'setter onfullscreenerror',
    ]),
    Gamepad: new Set([
        'getter displayId',
        'getter hand',
        'getter pose',
    ]),
    GamepadButton: new Set([
        'getter touched',
    ]),
    HTMLCanvasElement: new Set([
        'method transferControlToOffscreen',
    ]),
    HTMLElement: new Set([
        'getter inert',
        'getter inputMode',
        'setter inert',
        'setter inputMode',
    ]),
    HTMLImageElement: new Set([
        'method decode',
    ]),
    HTMLLinkElement: new Set([
        'getter scope',
        'setter scope',
    ]),
    HTMLMediaElement: new Set([
        'getter audioTracks',
        'getter videoTracks',
    ]),
    HTMLVideoElement: new Set([
        'method getVideoPlaybackQuality',
    ]),
    Image: new Set([
        'method decode',
    ]),
    ImageData: new Set([
        'getter dataUnion',
        'method getColorSettings',
    ]),
    MediaKeys: new Set([
        'method getStatusForPolicy',
    ]),
    MediaStreamTrack: new Set([
        'getter contentHint',
        'setter contentHint',
    ]),
    MessageEvent: new Set([
        'getter suborigin',
    ]),
    MouseEvent: new Set([
        'getter region',
    ]),
    Navigator: new Set([
        'getter bluetooth',
        'getter clipboard',
        'getter deviceMemory',
        'getter mediaCapabilities',
        'getter mediaSession',
        'getter nfc',
        'getter vr',
        'method cancelKeyboardLock',
        'method getInstalledRelatedApps',
        'method getVRDisplays',
        'method isProtocolHandlerRegistered',
        'method requestKeyboardLock',
        'method share',
    ]),
    NetworkInformation: new Set([
        'getter downlinkMax',
        'getter ontypechange',
        'getter type',
        'setter ontypechange',
    ]),
    Notification: new Set([
        'getter image',
    ]),
    Path2D: new Set([
        'method addPath',
    ]),
    PerformanceResourceTiming: new Set([
        'getter serverTiming',
    ]),
    Permissions: new Set([
        'method request',
        'method requestAll',
        'method revoke',
    ]),
    RTCPeerConnection: new Set([
        'getter ontrack',
        'method addTrack',
        'method getSenders',
        'method removeTrack',
        'setter ontrack',
    ]),
    Request: new Set([
        'getter cache',
    ]),
    SVGImageElement: new Set([
        'method decode',
    ]),
    Screen: new Set([
        'getter keepAwake',
        'setter keepAwake',
    ]),
    ServiceWorkerRegistration: new Set([
        'getter backgroundFetch',
        'getter paymentManager',
    ]),
    ShadowRoot: new Set([
        'getter fullscreenElement',
    ]),
    SourceBuffer: new Set([
        'getter audioTracks',
        'getter trackDefaults',
        'getter videoTracks',
        'setter trackDefaults',
    ]),
    TextMetrics: new Set([
        'getter actualBoundingBoxAscent',
        'getter actualBoundingBoxDescent',
        'getter actualBoundingBoxLeft',
        'getter actualBoundingBoxRight',
        'getter alphabeticBaseline',
        'getter emHeightAscent',
        'getter emHeightDescent',
        'getter fontBoundingBoxAscent',
        'getter fontBoundingBoxDescent',
        'getter hangingBaseline',
        'getter ideographicBaseline',
    ]),
    Touch: new Set([
        'getter region',
    ]),
    VTTCue: new Set([
        'getter region',
        'setter region',
    ]),
    WebGL2RenderingContext: new Set([
        'method commit',
    ]),
    WebGLRenderingContext: new Set([
        'method commit',
    ]),
    WebGLTexture: new Set([
        'getter lastUploadedVideoFrameWasSkipped',
        'getter lastUploadedVideoHeight',
        'getter lastUploadedVideoTimestamp',
        'getter lastUploadedVideoWidth',
    ]),
    webkitRTCPeerConnection: new Set([
        'getter ontrack',
        'method addTrack',
        'method getSenders',
        'method removeTrack',
        'setter ontrack',
    ]),
    webkitSpeechRecognition: new Set([
        'getter audioTrack',
        'setter audioTrack',
    ]),
};

// List of all platform-specific global properties. Please update this list when
// adding a new platform-specific global property.
var platformSpecificGlobalProperties = new Set([
    'getter animationWorklet',
    'getter audioWorklet',
    'method getComputedStyleMap',
]);

function filterPlatformSpecificInterface(interfaceName) {
    return platformSpecificProperties.hasOwnProperty(interfaceName) ||
        platformSpecificInterfaces.has(interfaceName) == platformSpecific;
}

function filterPlatformSpecificProperty(interfaceName, property) {
    return (platformSpecificInterfaces.has(interfaceName) ||
            (platformSpecificProperties.hasOwnProperty(interfaceName) &&
             platformSpecificProperties[interfaceName].has(property))) ==
        platformSpecific;
}

function filterPlatformSpecificGlobalProperty(property) {
    return platformSpecificGlobalProperties.has(property) == platformSpecific;
}

function collectPropertyInfo(object, propertyKey, output) {
    var propertyString = wellKnownSymbols.get(propertyKey) || propertyKey.toString();
    var keywords = Object.prototype.hasOwnProperty.call(object, 'prototype') ? 'static ' : '';
    var descriptor = Object.getOwnPropertyDescriptor(object, propertyKey);
    if ('value' in descriptor) {
        var type = typeof descriptor.value === 'function' ? 'method' : 'attribute';
        output.push(keywords + type + ' ' + propertyString);
    } else {
        if (descriptor.get)
            output.push(keywords + 'getter ' + propertyString);
        if (descriptor.set)
            output.push(keywords + 'setter ' + propertyString);
    }
}

function ownEnumerableSymbols(object) {
    return Object.getOwnPropertySymbols(object).
        filter(function(name) {
            return Object.getOwnPropertyDescriptor(object, name).enumerable;
        });
}

function collectPropertyKeys(object) {
   if (Object.prototype.hasOwnProperty.call(object, 'prototype')) {
       // Skip properties that aren't static (e.g. consts), or are inherited.
       // TODO(caitp): Don't exclude non-enumerable properties
       var protoProperties = new Set(Object.keys(object.prototype).concat(
                                     Object.keys(object.__proto__),
                                     ownEnumerableSymbols(object.prototype),
                                     ownEnumerableSymbols(object.__proto__)));
       return Object.keys(object).
           concat(ownEnumerableSymbols(object)).
           filter(function(name) {
               return !protoProperties.has(name);
           });
   }
   return Object.getOwnPropertyNames(object).concat(Object.getOwnPropertySymbols(object));
}

  function outputProperty(property) {
    outputFunc('    ' + property);
}

// FIXME: List interfaces with NoInterfaceObject specified in their IDL file.
outputFunc('[INTERFACES]');
  var interfaceNames = Object.getOwnPropertyNames(this).filter(isWebIDLConstructor).filter(filterPlatformSpecificInterface);
interfaceNames.sort();
interfaceNames.forEach(function(interfaceName) {
    var inheritsFrom = this[interfaceName].__proto__.name;
    if (inheritsFrom)
        outputFunc('interface ' + interfaceName + ' : ' + inheritsFrom);
    else
        outputFunc('interface ' + interfaceName);
    // List static properties then prototype properties.
    [this[interfaceName], this[interfaceName].prototype].forEach(function(object) {
        var propertyKeys = collectPropertyKeys(object);
        var propertyStrings = [];
        propertyKeys.forEach(function(propertyKey) {
            collectPropertyInfo(object, propertyKey, propertyStrings);
        });

        propertyStrings.filter(filterPlatformSpecificProperty.bind(null, interfaceName)).sort().forEach(outputProperty);
    });
});

outputFunc('[GLOBAL OBJECT]');
var propertyStrings = [];
var memberNames = propertyNamesInGlobal.filter(function(propertyKey) {
    return !jsBuiltins.has(propertyKey) && !isWebIDLConstructor(propertyKey);
});
memberNames.forEach(function(propertyKey) {
    collectPropertyInfo(globalObject, propertyKey, propertyStrings);
});
propertyStrings.sort().filter(filterPlatformSpecificGlobalProperty).forEach(outputProperty);
}
