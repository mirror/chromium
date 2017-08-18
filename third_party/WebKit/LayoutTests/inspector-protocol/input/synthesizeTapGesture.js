(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests Input.synthesizeTapGesture method.`);

  await dp.Emulation.setTouchEmulationEnabled({enabled: true, maxTouchPoints: 2});
  await session.reload();
  await session.evaluate(`
    document.body.style.margin = '0px';

    var div = document.createElement('div');
    div.style.height = '2000px';
    div.addEventListener('click', event => {
      if (event.target !== div)
        return;
      event.preventDefault();
      document.scrollingElement.scrollTop = document.scrollingElement.scrollTop + 100;
    }, false);
    document.body.appendChild(div);

    var topA = document.createElement('a');
    topA.href = '#middle';
    topA.style.height = '50px';
    topA.style.width = '50px';
    topA.style.marginTop = '0px';
    topA.style.display = 'block';
    topA.style.background = 'red';
    div.appendChild(topA);

    var middleA = document.createElement('a');
    middleA.name = 'middle';
    middleA.style.height = '50px';
    middleA.style.width = '50px';
    middleA.style.marginTop = '200px';
    middleA.style.display = 'block';
    middleA.style.background = 'green';
    div.appendChild(middleA);
  `);

  testRunner.log(`scrollTop = ${await session.evaluate('document.scrollingElement.scrollTop')}`);

  testRunner.log('Tapping the link');
  await dp.Input.synthesizeTapGesture({x: 25, y: 25, gestureSourceType: 'touch'});
  testRunner.log(`scrollTop = ${await session.evaluate('document.scrollingElement.scrollTop')}`);

  testRunner.log('Tapping event handler');
  await dp.Input.synthesizeTapGesture({x: 300, y: 300, gestureSourceType: 'touch'});
  testRunner.log(`scrollTop = ${await session.evaluate('document.scrollingElement.scrollTop')}`);

  testRunner.log('Tapping 3 times');
  await dp.Input.synthesizeTapGesture({x: 100, y: 100, gestureSourceType: 'touch', tapCount: 3});
  testRunner.log(`scrollTop = ${await session.evaluate('document.scrollingElement.scrollTop')}`);

  testRunner.completeTest();
})
