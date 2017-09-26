
(async function() {
  await TestRunner.showPanel('layers');

  UI.panels.layers._update();
  var transformController = UI.panels.layers._layers3DView._transformController;
  transformController.element.focus();

  TestRunner.addResult("Initial")
  dumpTransform();

  TestRunner.addResult("Pan Up")
  eventSender.keyDown('w');
  eventSender.keyDown('ArrowUp');
  dumpTransform();


  TestRunner.addResult("Pan Left")
  eventSender.keyDown('ArrowLeft');
  eventSender.keyDown('a');
  dumpTransform();

  TestRunner.addResult("Rotate Left")
  eventSender.keyDown('v');
  eventSender.keyDown('ArrowLeft');
  eventSender.keyDown('a');
  dumpTransform();

  TestRunner.addResult("Rotate Down")
  eventSender.keyDown('ArrowDown');
  eventSender.keyDown('s');
  dumpTransform();

  TestRunner.addResult("Switch to Pan and Pan Right")
  eventSender.keyDown('x');
  eventSender.keyDown('ArrowRight');
  eventSender.keyDown('d');
  dumpTransform();


  TestRunner.addResult("Zoom in")
  eventSender.keyDown('+', ['shiftKey']);
  dumpTransform();


  TestRunner.addResult("Reset")
  eventSender.keyDown('0');
  dumpTransform();

  TestRunner.addResult("Zoom out")
  eventSender.keyDown('-', ['shiftKey']);
  dumpTransform();


  TestRunner.addResult("Toggle and Rotate Down")
  eventSender.keyDown('ShiftLeft', ['shiftKey']);
  eventSender.keyDown('ArrowDown', ['shiftKey']);
  eventSender.keyDown('S', ['shiftKey']);
  dumpTransform();

  TestRunner.completeTest();

  function dumpTransform() {
    TestRunner.addResult('scale: ' + transformController.scale());
    TestRunner.addResult('offsetX: ' + transformController.offsetX());
    TestRunner.addResult('offsetY: ' + transformController.offsetY());
    TestRunner.addResult('rotateX: ' + transformController.rotateX());
    TestRunner.addResult('rotateY: ' + transformController.rotateY());
    TestRunner.addResult('');
  }
})().catch(e => {
  TestRunner.addResult(e);
  TestRunner.completeTest();
});
