function test()
{
    applicationCache.oncached = function() { log("cached") }
    applicationCache.onnoupdate = function() { log("noupdate") }
    applicationCache.onerror = function() { log("error") }

  try {
        var cachedResourceUrl = "/resources/network-simulator.php?path=/appcache/resources/simple.txt";
        var redirectToCachedResourceUrl = "/resources/redirect.php?url=" + cachedResourceUrl;

        var req = new XMLHttpRequest;
        req.open("GET", cachedResourceUrl, false);
        req.send(null);
        if (req.responseText != "Hello, World!")
            throw "wrong response text for cachedResourceUrl";

        req = new XMLHttpRequest;
        req.open("GET", redirectToCachedResourceUrl, false);
        req.send(null);
        if (req.responseText != "Hello, World!")
            throw "wrong response text for redirectToCachedResourceUrl";

        parent.postMessage("done", "*");
    } catch (ex) {
        alert("FAIL, unexpected error: " + ex);
    }
}
