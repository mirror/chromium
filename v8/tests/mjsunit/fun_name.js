function strip(s) {
  return s.replace(/\s/g, '');
}

assertEquals('function(){}', strip((function () { }).toString()));
assertEquals('functionanonymous(){}', strip(new Function().toString()));

