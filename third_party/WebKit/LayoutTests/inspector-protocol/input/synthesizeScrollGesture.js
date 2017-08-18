(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests Input.synthesizeScrollGesture method.`);

  await dp.Emulation.setTouchEmulationEnabled({enabled: true, maxTouchPoints: 2});
  await session.reload();
  await session.evaluate(`
    document.body.style.margin = '0px';

    var div1 = document.createElement('div');
    div1.style.height = '300px';
    div1.style.overflow = 'auto';
    document.body.appendChild(div1);

    var innerDiv1 = document.createElement('div');
    innerDiv1.style.height = '1000px';
    div1.appendChild(innerDiv1);

    var div2 = document.createElement('div');
    div2.style.height = '300px';
    div2.style.overflow = 'auto';
    document.body.appendChild(div2);

    var innerDiv2 = document.createElement('div');
    innerDiv2.style.height = '1000px';
    div2.appendChild(innerDiv2);
  `);

  async function dumpScrollTops() {
    testRunner.log(`div1.scrollTop = ${await session.evaluate('div1.scrollTop')}`);
    testRunner.log(`div2.scrollTop = ${await session.evaluate('div2.scrollTop')}`);
  }

  await dumpScrollTops();

  testRunner.log('Scrolling div1 by 100');
  await dp.Input.synthesizeScrollGesture({x: 10, y: 280, yDistance: -100, gestureSourceType: 'touch'});
  await dumpScrollTops();

  testRunner.log('Scrolling div2 by 220');
  await dp.Input.synthesizeScrollGesture({x: 10, y: 580, yDistance: -220, gestureSourceType: 'touch'});
  await dumpScrollTops();

  testRunner.completeTest();
})
