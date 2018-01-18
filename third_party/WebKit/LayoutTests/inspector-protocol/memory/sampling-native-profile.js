(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Test sampling native memory profiler.`);

  await dp.Memory.startSamplingNativeHeapProfile({
      samplingInterval: 100000, suppressRandomness: true});
  testRunner.log('Sampling started');
  await session.evaluate(`
    const canvas = document.createElement('canvas');
    canvas.width = 500;
    canvas.height = 200;
    const ctx = canvas.getContext('2d');
    ctx.fillStyle = 'green';
    ctx.fillRect(0, 0, 10, 10);
    document.body.appendChild(canvas);
    `);
  const message = await dp.Memory.getSamplingNativeHeapProfile();
  await dp.Memory.stopSamplingNativeHeapProfile();
  testRunner.log('Sampling stopped');

  const profile = message.result.profile;
  const foundTheSample = profile.samples.some(sample =>
    sample.size >= 500 * 200 && sample.stack.some(frame => frame.includes('HTMLCanvasElement')));
  testRunner.log('Found sample: ' + foundTheSample);

  testRunner.completeTest();
})
