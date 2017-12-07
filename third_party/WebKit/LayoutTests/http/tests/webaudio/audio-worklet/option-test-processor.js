/**
 * @class OptionTestProcessor
 * @extends AudioWorkletProcessor
 *
 * This processor class demonstrates the message port functionality.
 */
class OptionTestProcessor extends AudioWorkletProcessor {
  constructor(processorData) {
    super();
    this.port.postMessage(processorData);
  }

  process() {
    return true;
  }
}

registerProcessor('option-test-processor', OptionTestProcessor);
