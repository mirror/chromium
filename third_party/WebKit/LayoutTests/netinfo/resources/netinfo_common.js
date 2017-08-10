window.jsTestIsAsync = true;

var connection = navigator.connection;
var initialType = "bluetooth";
var initialDownlinkMax = 1.0;
var newConnectionType = "ethernet";
var newDownlinkMax = 2.0;
var initialEffectiveType = "3g";
var initialRtt = 50.0;
// Up to 10% noise  may be added to rtt and downlink. Use 11% as the buffer
// below to include any mismatches due to floating point calculations.
var initialRttMaxNoise = initialRtt * 0.11;
var initialDownlink = 5.0;
var initialDownlinkMaxNoise = initialDownlink * 0.11;
var newEffectiveType = "4g";
var newRtt = 50.0;
var newRttMaxNoise = newRtt * 0.11;
var newDownlink = 10.0;
var newDownlinkMaxNoise = newDownlink * 0.11;

// Suppress connection messages information from the host.
if (window.internals) {
    internals.setNetworkConnectionInfoOverride(true, initialType, initialDownlinkMax);
    internals.setNetworkQualityInfoOverride(initialEffectiveType, initialRtt, initialDownlink);

    // Reset the state of the singleton network state notifier.
    window.addEventListener('beforeunload', function() {
        internals.clearNetworkConnectionInfoOverride();
    }, false);
}

function isTypeOnline(type) {
    return type != 'none';
}

function verifyOnChange(data, type, downlinkMax, effectiveType, rtt, downlink) {
    var parsed = data.toString().split(",");
    if(parsed[0] != type)
        testFailed("type mismatch");
    if(parsed[2] != effectiveType)
        testFailed("effectiveType mismatch");
    // Up to 10% noise  may be added to rtt and downlink. rtt or downlink from
    // two different hosts may have a difference of up to 20%. Use 21% as the
    // buffer below to include any mismatches due to floating point
    // calculations.
    if(Math.abs(parsed[3] -rtt) > 0.21 *rtt)
        testFailed("rtt mismatch");
    if(Math.abs(parsed[4] -downlink) > 0.21 *downlink)
        testFailed("downlink mismatch");
}