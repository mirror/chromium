let port;
let received = false;

function reportFailure(details) {
  port.postMessage('FAIL: ' + details);
}

onmessage = event => {
  port = event.source;

//  port.postMessage('wss://{{host}}:{{ports[ws][0]}}/echo');
  const ws = new WebSocket('ws://{{host}}:{{ports[ws][0]}}/echo');
  ws.onopen = () => {
    ws.send('Hello');
  };
  ws.onmessage = msg => {
    if (msg.data !== 'Hello') {
      reportFailure('Unexpected reply: ' + msg.data);
      return;
    }

    received = true;
    ws.close();
  };
  ws.onclose = () => {
    if (!received) {
      reportFailure('Closed before receiving reply');
      return;
    }

    port.postMessage('PASS');
  };
  ws.onerror = () => {
    reportFailure('Got an error event');
  };
};
