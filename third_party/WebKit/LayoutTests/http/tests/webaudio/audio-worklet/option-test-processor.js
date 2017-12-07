/**
 * @class OptionTestProcessor
 * @extends AudioWorkletProcessor
 *
 * This processor class demonstrates the option passing feature by echoing the
 * received |processorData| to the node.
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
