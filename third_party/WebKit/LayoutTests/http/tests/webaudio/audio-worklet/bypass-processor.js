/**
 * @class Bypasser
 * @extends AudioWorkletProcessor
 *
 * This processor class demosntrates a simple bypass node.
 */
class Bypasser extends AudioWorkletProcessor {
  constructor() {
    super();
  }

  process(inputs, outputs) {
    let inputBuffer = inputs[0];
    let outputBuffer = outputs[0];
    for (var i = 0; i < outputBuffer.numberOfChannels; ++i) {
      outputBuffer.getChannelData(i).set(inputBuffer.getChannelData(i));
    }
  }
}

registerProcessor('bypasser', Bypasser);
