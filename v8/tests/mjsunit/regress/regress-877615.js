// Copyright 2008 Google Inc.  All Rights Reserved.

Number.prototype.toLocaleString = function() { return 'invalid'};
assertEquals([1].toLocaleString(), 'invalid');  // invalid

Number.prototype.toLocaleString = 'invalid';
assertEquals([1].toLocaleString(), '1');  // 1

Number.prototype.toString = function() { return 'invalid' };
assertEquals([1].toLocaleString(), '1');  // 1
assertEquals([1].toString(), '1');        // 1

