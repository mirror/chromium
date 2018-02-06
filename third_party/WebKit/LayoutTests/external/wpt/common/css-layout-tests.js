// To make sure that we take the snapshot at the right time, we do double
// requestAnimationFrame. In the second frame, we take a screenshot, that makes
// sure that we already have a full frame.
function importLayoutWorkletAndTerminateTestAfterAsyncPaint(code) {
    if (typeof CSS.layoutWorklet == "undefined") {
        takeScreenshot();
    } else {
        var blob = new Blob([code], {type: 'text/javascript'});
        CSS.layoutWorklet.addModule(URL.createObjectURL(blob)).then(function() {
            requestAnimationFrame(function() {
                requestAnimationFrame(function() {
                    takeScreenshot();
                });
            });
        });
    }
}

