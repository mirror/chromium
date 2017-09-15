(async function(testRunner) {
  var {session, dp} = await testRunner.startBlank('Tests reading service worker sync registrations.');

  async function installServiceWorker(url) {
    var registration = await navigator.serviceWorker.register(url)
    await registration.update();
    var permission = await navigator.permissions.query({name: 'background-sync'});
    if (permission.state !== 'granted') {
      await new Promise(resolve => {
        permission.onchange = function() {
          permission.onchange = null;
          resolve();
        }
        testRunner.setPermission('background-sync', 'granted', window.location.origin, window.location.origin);
      })
    }
  }

  function triggerSync(tag) {
    return navigator.serviceWorker.ready
        .then(registration => registration.sync.register(tag));
  }

  function uninstallServiceWorker() {
    return navigator.serviceWorker.ready
        .then(registration => registration.uninstall());
  }

  async function waitForServiceWorkerActivation() {
    var versions;
    do {
      var result = await dp.ServiceWorker.onceWorkerVersionUpdated();
      versions = result.params.versions;
    } while (!versions.length || versions[0].status !== "activated");
    return versions[0].registrationId;
  }

  function dumpSyncStatus(status) {
    var tags = status.registrations.map(registration => `${registration.tag} ${registration.numAttempts} ${registration.backgroundSyncState}`).sort();
    testRunner.log(`Status: ${status.status}, tags: [${tags.join(', ')}]`);
  }

  async function waitForSyncRegistrations(id, count) {
    var status;
    do {
      status = (await dp.ServiceWorker.getSyncStatus({registrationId: id})).result.status;
    } while (status.registrations.length !== count);
    return status;
  }

  setTimeout(() => testRunner.completeTest(), 5000);
  var swActivatedPromise = waitForServiceWorkerActivation();

  await dp.Runtime.enable();
  await dp.ServiceWorker.enable();
  dp.Runtime.onConsoleAPICalled(m => testRunner.log('#Console# ' + JSON.stringify(m.params.args[0].value)));

  await session.evaluateAsync(installServiceWorker, testRunner.url('../resources/service-worker-fails-to-sync.js'));
  testRunner.log('installed!');


  var id = await swActivatedPromise;
  testRunner.log('No sync registrations expected');

  dumpSyncStatus(await waitForSyncRegistrations(id, 0));
  testRunner.log('Triggering syncs');

  await session.evaluateAsync(triggerSync, 'tag1');
  await session.evaluateAsync(triggerSync, 'tag2');
  testRunner.log('Should be a single registration');
  dumpSyncStatus(await waitForSyncRegistrations(id, 2));

  await session.evaluateAsync(uninstallServiceWorker);

  testRunner.completeTest();
});
