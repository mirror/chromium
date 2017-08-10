window.jsTestIsAsync = true;

var connection = navigator.connection;
var initialType = "bluetooth";
var initialDownlinkMax = 1.0;
var newConnectionType = "ethernet";
var newDownlinkMax = 2.0;
var initialEffectiveType = "3g";
var initialRtt = 50.0;
var initialRttMaxNoise = 0;
var initialDownlink = 5.0;
var initialDownlinkMaxNoise = 0.5;
var newEffectiveType = "4g";
var newRtt = 50.0;
var newRttMaxNoise = 0;
var newDownlink = 10.0;
var newDownlinkMaxNoise = 1.0;

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

function parse_query(data, type, downlinkMax, effectiveType, rtt, downlink) {
    var parsed = data.toString().split(",");
    if(parsed[0] != type)
        testFailed("type error");
    if(parsed[1] != downlinkMax)
        testFailed("downlinkMax error");
    if(parsed[2] != effectiveType)
        testFailed("effectiveType error");
    if(Math.abs(parsed[3] -rtt) > 0.2 *rtt)
        testFailed("rtt error");
    if(Math.abs(parsed[4] -downlink) > 0.2 *downlink)
        testFailed("downlink error");
}