function waitUntilLoadedAndAutofocused(callback) {
    var loaded = false;
    var autofocused = false;
    window.addEventListener('load', function() {
        loaded = true;
        if (autofocused)
            callback();
    }, false);
    document.addEventListener('focusin', function() {
        if (autofocused)
            return;
        autofocused = true;
        if (loaded)
            callback();
    }, false);
}