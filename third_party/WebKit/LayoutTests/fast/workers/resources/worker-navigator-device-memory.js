function log(message)
{
    postMessage(message);
}

onmessage = function(event)
{
    log(navigator.deviceMemory > 0 ? 'PASS' : 'FAIL');
    log('DONE');
}
