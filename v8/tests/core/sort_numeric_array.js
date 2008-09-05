/* This is not amitp's code.  He got it from the web.  amitp made some
   modifications: 1. numeric sort sorts descending by default.
   2. sorted columns have a unicode arrow instead of an image.  */

/* changed a dom-dependent script to a standard-alone one that can run
   by a JS engine.
   see topspf.html and tablesort.js.
*/

function sortArray(aSrc, bDesc, sType) {
	var a = aSrc;
	aSrc.sort(compareBy(bDesc,sType));
	return a;
}

function CaseInsensitiveString(s) {
	return String(s).toUpperCase();
}

function parseDate(s) {
	return Date.parse(s.replace(/\-/g, '/'));
}

/* alternative to number function
 * This one is slower but can handle non numerical characters in
 * the string allow strings like the follow (as well as a lot more)
 * to be used:
 *    "1,000,000"
 *    "1 000 000"
 *    "100cm"
 */

function toNumber(s) {
    return -Number(s.replace(/[^0-9\.]/g, ""));
}

function compareBy(bDescending, sType) {
	var d = bDescending;
	
	var fTypeCast = String;
	
	if (sType == "Number")
		fTypeCast = toNumber;
	else if (sType == "Date")
		fTypeCast = parseDate;
	else if (sType == "CaseInsensitiveString")
		fTypeCast = CaseInsensitiveString;

	return function (n1, n2) {
		if (fTypeCast(n1) < fTypeCast(n2))
			return d ? -1 : +1;
		if (fTypeCast(n1) > fTypeCast(n2))
			return d ? +1 : -1;
		return 0;
	};
}

function createNumericArray(nSize) {
  	var s = new Array(nSize);
  	for (var i = 0; i<s.length; i++) {
    		s[i] = String(i);
  	}
  	return s;
}

function createDateArray(nSize) {
	var s = new Array(nSize);
	for (var i=0; i<s.length; i++) {
	  	s[i] = String(new Date());
	}
	return s;
}
	
function createStringArray(nSize) {
	var s = new Array(nSize);
	for (var i=0; i<s.length; i++) {
		s[i] = 's' + i;
	}
	return s;
}


var reputations=[
'93.3',
'98.1',
'96.5',
'94.8',
'96.3',
'0.0',
'0.1',
'0.0',
'93.5',
'96.9',
'98.2',
'0.8',
'0.5',
'0.6',
'0.0',
'0.4',
'1.5',
'0.2',
'0.9',
'88.9',
'0.1',
'0.7',
'0.1',
'99.3',
'0.7',
'1.2',
'0.8',
'0.0',
'0.6',
'73.0',
'0.0',
'0.1',
'0.0',
'0.1',
'0.1',
'0.0',
'92.7',
'98.3',
'0.1',
'92.0',
'0.0',
'17.4',
'84.8',
'1.7',
'0.2',
'0.0',
'0.1',
'0.2',
'98.0',
'0.2',
'97.6',
'30.2',
'74.6',
'0.5',
'99.6',
'0.2',
'27.7',
'96.8',
'96.4',
'0.2',
'80.9',
'81.0',
'0.2',
'98.6',
'87.8',
'0.1',
'89.1',
'1.2',
'0.2',
'0.2',
'93.1',
'0.2',
'0.2',
'0.2',
'0.2',
'99.2',
'92.5',
'97.8',
'82.2',
'99.3',
'0.1',
'54.2',
'0.1',
'0.1',
'0.0',
'81.1',
'0.1',
'28.0',
'94.5',
'6.5',
'92.9',
'89.0',
'0.2',
'98.8',
'95.8',
'49.3',
'97.1',
'79.4',
'93.2',
'1.8',
'95.7',
'0.0',
'96.2',
'94.8',
'0.0',
'63.6',
'0.1',
'94.1',
'0.1',
'0.2',
'0.7',
'28.2',
'0.1',
'99.3',
'0.1',
'89.2',
'61.3',
'9.6',
'0.0',
'99.5',
'99.5',
'59.1',
'51.0',
'1.1',
'7.3',
'8.4',
'85.1',
'0.1',
'97.4',
'99.8',
'78.6',
'91.8',
'98.6',
'0.2',
'84.3',
'0.1',
'96.0',
'63.7',
'55.4',
'99.6',
'0.0',
'0.0',
'0.0',
'94.1',
'16.1',
'42.3',
'0.0',
'89.7',
'78.7',
'91.3',
'1.1',
'0.0',
'97.5',
'0.1',
'0.1',
'99.5',
'0.0',
'0.0',
'34.2',
'0.0',
'11.9',
'0.0',
'0.0',
'0.2',
'0.1',
'11.1',
'0.6',
'96.4',
'6.0',
'96.6',
'91.5',
'56.1',
'3.3',
'1.8',
'99.8',
'28.7',
'97.2',
'96.5',
'0.0',
'0.1',
'0.1',
'95.4',
'51.7',
'0.4',
'62.7',
'0.1',
'92.7',
'0.2',
'95.4',
'98.7',
'4.7',
'0.4',
'0.8',
'0.4',
'0.0',
'93.3',
'0.0',
'77.0',
'0.4',
'0.1'];

function sortColumn(event) {
  var startTime = new Date().getTime();
  sortArray(reputations, true, 'Number');
  var endTime = new Date().getTime();
  //print('sorting took ' + (endTime - startTime) + 'ms');
}

sortColumn();
