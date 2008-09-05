const SMI_MAX = (1 << 30) - 1;
const SMI_MIN = -(1 << 30);
const ONE = 1;
const ONE_HUNDRED = 100;

const OBJ_42 = new (function() {
  this.valueOf = function() { return 42; };
})();

assertEquals(42, OBJ_42.valueOf()); 


function Add1(x) {
  return x + 1;
}

function Add100(x) {
  return x + 100;
}

function Add1Reversed(x) {
  return 1 + x;
}

function Add100Reversed(x) {
  return 100 + x;
}


assertEquals(1, Add1(0));  // fast case
assertEquals(1, Add1Reversed(0));  // fast case
assertEquals(SMI_MAX + ONE, Add1(SMI_MAX));  // overflow
assertEquals(SMI_MAX + ONE, Add1Reversed(SMI_MAX));  // overflow
assertEquals(42 + ONE, Add1(OBJ_42));  // non-smi
assertEquals(42 + ONE, Add1Reversed(OBJ_42));  // non-smi

assertEquals(100, Add100(0));  // fast case
assertEquals(100, Add100Reversed(0));  // fast case
assertEquals(SMI_MAX + ONE_HUNDRED, Add100(SMI_MAX));  // overflow
assertEquals(SMI_MAX + ONE_HUNDRED, Add100Reversed(SMI_MAX));  // overflow
assertEquals(42 + ONE_HUNDRED, Add100(OBJ_42));  // non-smi
assertEquals(42 + ONE_HUNDRED, Add100Reversed(OBJ_42));  // non-smi



function Sub1(x) {
  return x - 1;
}

function Sub100(x) {
  return x - 100;
}

function Sub1Reversed(x) {
  return 1 - x;
}

function Sub100Reversed(x) {
  return 100 - x;
}


assertEquals(0, Sub1(1));  // fast case
assertEquals(-1, Sub1Reversed(2));  // fast case
assertEquals(SMI_MIN - ONE, Sub1(SMI_MIN));  // overflow
assertEquals(ONE - SMI_MIN, Sub1Reversed(SMI_MIN));  // overflow
assertEquals(42 - ONE, Sub1(OBJ_42));  // non-smi
assertEquals(ONE - 42, Sub1Reversed(OBJ_42));  // non-smi

assertEquals(0, Sub100(100));  // fast case
assertEquals(1, Sub100Reversed(99));  // fast case
assertEquals(SMI_MIN - ONE_HUNDRED, Sub100(SMI_MIN));  // overflow
assertEquals(ONE_HUNDRED - SMI_MIN, Sub100Reversed(SMI_MIN));  // overflow
assertEquals(42 - ONE_HUNDRED, Sub100(OBJ_42));  // non-smi
assertEquals(ONE_HUNDRED - 42, Sub100Reversed(OBJ_42));  // non-smi
