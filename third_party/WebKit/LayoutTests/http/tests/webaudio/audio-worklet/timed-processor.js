/**
 * @class TimedProcessor
 * @extends AudioWorkletProcessor
 *
 * This processor class is for the life cycle and the processor state event.
 */
class TimedProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.createdAt_ = currentTime;

    // Lifetime in second.
    this.lifetime_ = options ? options.lifetime || 1 : 1;
  }

  process() {
    return currentTime - this.createdAt_ > this.lifetime_ ? false : true;
  }
}

registerProcessor('timed', TimedProcessor);
