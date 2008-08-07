function TryCall(x) {
  var caught = [];
  try {
    x();
  } catch (e) {
    caught.push(e);
  }

  try {
    new x();
  } catch (e) {
    caught.push(e);
  }

  assertTrue(caught[0] instanceof TypeError);
  assertTrue(caught[1] instanceof TypeError);
};


TryCall(this);
TryCall(Math);
TryCall(true);
TryCall(1234);
TryCall("hest");



