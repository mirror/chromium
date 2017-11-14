/**
 * @class PortProcessor
 * @extends AudioWorkletProcessor
 *
 * This processor class demonstrates the message port functionality.
 */
class PortProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.port.onmessage = this.handleMessage.bind(this);
    this.port.postMessage({
      state: 'created',
      timeStamp: currentTime
    });
  }

  handleMessage(event) {
    this.port.postMessage({
      message: event.data,
      timeStamp: currentTime
    });
  }

  process(inputs, outputs, parameters) {
    // Doesn't do anything here.
    return true;
  }
}

registerProcessor('port-processor', PortProcessor);
