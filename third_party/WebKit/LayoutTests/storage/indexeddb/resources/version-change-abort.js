if (this.importScripts) {
    importScripts('../../../resources/js-test.js');
    importScripts('shared.js');
}

description("Ensure that aborted VERSION_CHANGE transactions are completely rolled back");

self.isOnErrorTest = true;

indexedDBTest(prepareDatabase, openRequest1Complete);
function prepareDatabase()
{
    db = event.target.result;
    trans = event.target.transaction;
    shouldBeTrue("trans instanceof IDBTransaction");
    trans.onabort = unexpectedAbortCallback;
    trans.onerror = unexpectedErrorCallback;

    evalAndLog("db.createObjectStore('store1')");
}

function openRequest1Complete()
{
    debug("openRequest1 complete");
    shouldBe("db.version", "1");
    debug("");
    db.close();

    evalAndLog("vcreq = indexedDB.open(dbname, 2)");
    vcreq.onupgradeneeded = inSetVersion2;
    vcreq.onerror = openRequest2Error;
    vcreq.onblocked = unexpectedBlockedCallback;
    vcreq.onsuccess = unexpectedSuccessCallback;
}

function inSetVersion2()
{
    db = event.target.result;
    debug("openRequest2() callback");
    shouldBe("db.version", "2");
    shouldBeTrue("vcreq.transaction instanceof IDBTransaction");
    trans = vcreq.result;
    trans.onerror = unexpectedErrorCallback;
    trans.oncomplete = unexpectedCompleteCallback;
    trans.onabort = onTransactionAbort;

    evalAndLog("db.deleteObjectStore('store1')");
    evalAndLog("db.createObjectStore('store2')");

    // Throwing an exception during the callback should:
    // * fire window.onerror (uncaught within event dispatch)
    // * Cause the transaction to abort - fires 'abort' at transaction
    // * fires 'error' at the open request
    // * fires window.onerror (with request error)

    debug("raising exception");
    waitForError(/This should not be caught/, onGlobalErrorUncaughtException);
    throw new Error("This should not be caught");
}

function onGlobalErrorUncaughtException()
{
    sawGlobalErrorUncaughtException = true;
}

function onTransactionAbort()
{
    shouldBeTrue("sawGlobalErrorUncaughtException");
    sawTransactionAbort = true;
}

function openRequest2Error(evt)
{
    debug("");
    debug("openRequest2Error() callback");
    shouldBeTrue("sawGlobalErrorUncaughtException");
    shouldBeTrue("sawTransactionAbort");
    waitForError(/AbortError/, onGlobalErrorAbortError);
}

function onGlobalErrorAbortError()
{
    debug("");
    debug("Verify rollback:");
    evalAndLog("request = indexedDB.open(dbname)");
    request.onblocked = unexpectedBlockedCallback;
    request.onupgradeneeded = unexpectedUpgradeNeededCallback;
    request.onerror = unexpectedErrorCallback;
    request.onsuccess = function (e) {
        db = event.target.result;
        shouldBe("db.version", "1");
        shouldBeTrue("db.objectStoreNames.contains('store1')");
        shouldBeFalse("db.objectStoreNames.contains('store2')");

        finishJSTest();
    };
}
