(async function(testRunner) {
  let {page, session, dp} = await testRunner.startURL(
      'resources/service-worker.html',
      `Tests reading cached response from the protocol.`);

  async function dumpResponse(cacheId, path) {
    var result = await dp.CacheStorage.requestCachedResponse({"cacheId": cacheId, "requestURL": path});
    if (result["error"]) {
      var error = result["error"];
      testRunner.log(`Error: ${error.message} ${error.data || ""}`);
      return;
    }
    var response = result["result"]["response"];
    testRunner.log(response.headers["Content-Type"]);
    testRunner.log("Type of body: " + (typeof response.body));
  }

  async function waitForServiceWorkerActivation() {
    do {
      var result = await dp.ServiceWorker.onceWorkerVersionUpdated();
      var versions = result["params"]["versions"];
    } while (!versions.length || versions[0]["status"] !== "activated");
  }

  var swActivatedPromise = waitForServiceWorkerActivation();

  await dp.Runtime.enable();
  await dp.ServiceWorker.enable();
  await swActivatedPromise;

  result = await dp.CacheStorage.requestCacheNames({securityOrigin: "http://127.0.0.1:8000"});
  var cacheId = result["result"]["caches"][0]["cacheId"];
  result = await dp.CacheStorage.requestEntries({cacheId, skipCount: 0, pageSize: 5});
  var requests = result["result"]["cacheDataEntries"].map(entry => entry["request"]).sort();
  testRunner.log("Cached requests:");

  for (var entry of requests)
    await dumpResponse(cacheId, entry);

  testRunner.log('Trying without specifying all the arguments:')
  await dumpResponse(null, null);
  testRunner.log('Trying without specifying the request path:')
  await dumpResponse(cacheId, null);
  testRunner.log('Trying with non existant cache:')
  await dumpResponse("bogus", requests[0]);
  testRunner.log('Trying with non existant request path:')
  await dumpResponse(cacheId, "http://localhost:8080/bogus");

  testRunner.completeTest()
});
