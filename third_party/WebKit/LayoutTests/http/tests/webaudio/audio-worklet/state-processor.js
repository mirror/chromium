/**
 * @class TimedProcessor
 * @extends AudioWorkletProcessor
 *
 * This processor class is for the life cycle and the processor state event.
 * It only lives for 1 render quantum.
 */
class TimedProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.createdAt_ = currentTime;
    this.lifetime_ = 128 / sampleRate;
  }

  process() {
    return currentTime - this.createdAt_ > this.lifetime_ ? false : true;
  }
}


/**
 * @class ConstructorErrorProcessor
 * @extends AudioWorkletProcessor
 */
class ConstructorErrorProcessor extends AudioWorkletProcessor {
  constructor() {
    throw 'ConstructorError';
  }

  process() {
    return true;
  }
}


/**
 * @class ProcessErrorProcessor
 * @extends AudioWorkletProcessor
 */
class ProcessErrorProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
  }

  process() {
    throw 'ProcessError';
    return true;
  }
}


registerProcessor('timed', TimedProcessor);
registerProcessor('constructor-error', ConstructorErrorProcessor);
registerProcessor('process-error', ProcessErrorProcessor);
