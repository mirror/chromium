// copied from google3/java/com/google/caribou/antlers/fin/jsdata

//------------------------------------------------------------------------
// This file contains common utilities and basic javascript infrastructure.
//
// Notes:
// * Press 'D' to toggle debug mode.
//
// Functions:
//
// - Assertions
// DEPRECATED: Use assert.js
// AssertTrue(): assert an expression. Throws an exception if false.
// Fail(): Throws an exception. (Mark block of code that should be unreachable)
// AssertEquals(): assert that two values are equal.
// AssertNumArgs(): assert number of arguments for the function
// AssertType(): assert that a value has a particular type
//
// - Cookies
// SetCookie(): Sets a cookie.
// ExpireCookie(): Expires a cookie.
// GetCookie(): Gets a cookie value.
//
// - Dynamic HTML/DOM utilities
// MaybeGetElement(): get an element by its id
// GetElement(): get an element by its id
// GetParentNode(): Get the parent of an element
// GetAttribute(): Get attribute value of a DOM node
// SetInnerHTML(): set the inner HTML of a node
// GetInnerHTML(): get the inner HTML of a node
// ClearInnerHTML(): clear the inner HTML of a node
// SetCssStyle(): Sets a CSS property of a node.
// GetStyleProperty(): Get CSS property from a style attribute string
// ShowElement(): Show/hide element by setting the "display" css property.
// ShowBlockElement(): Show/hide block element
// SetButtonText(): Set the text of a button element.
// AppendNewElement(): Create and append a html element to a parent node.
// CreateDIV(): Create a DIV element and append to the document.
// CreateIFRAME(): Create an IFRAME and append to the document.
// CreateIMG(): Create an IMG.
// HasClass(): check if element has a given class
// AddClass(): add a class to an element
// RemoveClass(): remove a class from an element
//
// - Window/Screen utiltiies
// GetPageOffsetLeft(): get the X page offset of an element
// GetPageOffsetTop(): get the Y page offset of an element
// GetPageOffset(): get the X and Y page offsets of an element
// GetPageOffsetRight() : get X page offset of the right side of an element
// GetPageOffsetRight() : get Y page offset of the bottom of an element
// GetScrollTop(): get the vertical scrolling pos of a window.
// GetScrollLeft(): get the horizontal scrolling pos of a window
// IsScrollAtEnd():  check if window scrollbar has reached its maximum offset
// ScrollTo(): scroll window to a position
// ScrollIntoView(): scroll window so that an element is in view.
// GetWindowWidth(): get width of a window.
// GetWindowHeight(): get height of a window
// GetAvailScreenWidth(): get available screen width
// GetAvailScreenHeight(): get available screen height
// GetNiceWindowHeight(): get a nice height for a new browser window.
// Open{External/Internal}Window(): open a separate window
// CloseWindow(): close a window
//
// - DOM walking utilities
// AnnotateTerms(): find terms in a node and decorate them with some tag
// AnnotateText(): find terms in a text node and decorate them with some tag
//
// - String utilties
// HtmlEscape(): html escapes a string
// HtmlUnescape(): remove html-escaping.
// QuoteEscape(): escape " quotes.
// CollapseWhitespace(): collapse multiple whitespace into one whitespace.
// Trim(): trim whitespace on ends of string
// IsEmpty(): check if CollapseWhiteSpace(String) == ""
// IsLetterOrDigit(): check if a character is a letter or a digit
// ConvertEOLToLF(): normalize the new-lines of a string.
// HtmlEscapeInsertWbrs(): HtmlEscapes and inserts <wbr>s (word break tags)
//   after every n non-space chars and/or after or before certain special chars
//
// - TextArea utilities
// GetCursorPos(): finds the cursor position of a textfield
// SetCursorPos(): sets the cursor position in a textfield
//
// - Array utilities
// FindInArray(): do a linear search to find an element value.
// DeleteArrayElement(): return a new array with a specific value removed.
// CloneObject(): clone an object, copying its values recursively.
// CloneEvent(): clone an event; cannot use CloneObject because it
//               suffers from infinite recursion
//
// - Formatting utilities
// PrintArray(): used to print/generate HTML by combining static text
// and dynamic strings.
// ImageHtml(): create html for an img tag
// FormatJSLink(): formats a link that invokes js code when clicked.
// MakeId3(): formats an id that has two id numbers, eg, foo_3_7
//
// - Miscellaneous
// IsDefined(): returns true if argument is not undefined
//------------------------------------------------------------------------

Error.prototype.toString = function () {
  return this.message + ' @ ' + this.lineNumber;
};

// browser detection
var is_ie;
var is_ie5;
var is_ie6;
var is_ie7;
var is_konqueror;
var is_safari;
var is_nav;
var is_win;
var is_opera;
(function () {
  var agent = 'rhino';

  is_ie = (agent.indexOf('msie') != -1);
  is_ie5 = (agent.indexOf('msie 5') != -1 && document.all);
  is_ie6 = (agent.indexOf('msie 6') != -1 && document.all);
  is_ie7 = (agent.indexOf('msie 7') != -1 && document.all);
  is_konqueror = (agent.indexOf('konqueror') != -1);
  is_safari = (agent.indexOf('safari') != -1) || is_konqueror;
  is_nav = !is_ie && !is_safari && (agent.indexOf('mozilla') != -1);
  is_win = (agent.indexOf('win') != -1);
  is_opera = !!('opera' in this);
})();


var BACKSPACE_KEYCODE = 8;
var COMMA_KEYCODE = 188;                // ',' key
var DEBUG_KEYCODE = 68;                 // 'D' key
var DELETE_KEYCODE = 46;
var DOWN_KEYCODE = 40;                  // DOWN arrow key
var ENTER_KEYCODE = 13;                 // ENTER key
var ESC_KEYCODE = 27;                   // ESC key
var LEFT_KEYCODE = 37;                  // LEFT arrow key
var RIGHT_KEYCODE = 39;                 // RIGHT arrow key
var SPACE_KEYCODE = 32;                 // space bar
var TAB_KEYCODE = 9;                    // TAB key
var UP_KEYCODE = 38;                    // UP arrow key
var SHIFT_KEYCODE = 16;

// This is a "constant" but has different values depending on the browser
var semicolon_keycode = (is_ie ? 186 : 59);

var MAX_EMAIL_ADDRESS_LENGTH = 320;     // 64 + '@' + 255
var MAX_SIGNATURE_LENGTH = 1000;        // 1000 chars of maximum signature

//------------------------------------------------------------------------
// Assertions
// DEPRECATED: Use assert.js
//------------------------------------------------------------------------
/**
 * DEPRECATED: Use assert.js
 */

var WARN_ON = true;
var ASSERTS_ON = true;
var LOG_ON = true;

/** Used to warn about something. Shows an error in debug mode */
function warn(msg) {
  if (WARN_ON) {
    alert(msg);
  }
  log(msg);
}

/* TODO(msamuel) - this completely changes the semantics, since now
 * if (isBad) {
 *   raise('badness');
 * }
 * doSomething();
 *
 * doSomething will now execute.
 * Raise doesn't have the same "may not execute" clause that asserts do.
 */
function raise(msg) {
  if (FAIL_ON) {
    if (typeof Error != 'undefined') {
      var stack;
      try {
        throw new Error();
      } catch (e) {
        stack = '' + e.stack;
        if (stack.length > 1024) { stack = stack.substring(0, 1021) + '...'; }
      }
      // strip off the stack entries for assert and fail
      stack = stack.replace(/[^\n\r*]\/common.js:\d+[\r\n]+/g, '');
      log(stack);
      // include the stack so that firefox's error console displays it
      throw new Error((msg || 'Assertion Failed') + '\n\n' + stack);
    } else {
      throw (msg);
    }
  }
}

/**
 * DEPRECATED: Use assert.js
 *
 * Fail() is useful for marking logic paths that should
 * not be reached. For example, if you have a class that uses
 * ints for enums:
 *
 * MyClass.ENUM_FOO = 1;
 * MyClass.ENUM_BAR = 2;
 * MyClass.ENUM_BAZ = 3;
 *
 * And a switch statement elsewhere in your code that
 * has cases for each of these enums, then you can
 * "protect" your code as follows:
 *
 * switch(type) {
 *   case MyClass.ENUM_FOO: doFooThing(); break;
 *   case MyClass.ENUM_BAR: doBarThing(); break;
 *   case MyClass.ENUM_BAZ: doBazThing(); break;
 *   default:
 *     Fail("No enum in MyClass with value: " + type);
 * }
 *
 * This way, if someone introduces a new value for this enum
 * without noticing this switch statement, then the code will
 * fail if the logic allows it to reach the switch with the
 * new value, alerting the developer that he should add a
 * case to the switch to handle the new value he has introduced.
 *
 * @param {string} opt_msg to display for failure
 *                 DEFAULT: "Assertion failed"
 */
function Fail(opt_msg) {
  if (FAIL_ON) {
    if (opt_msg === undefined) opt_msg = 'Assertion failed';
    if (IsDefined(DumpError)) DumpError(opt_msg + '\n');
    raise(opt_msg);
  }
}

/**
 * DEPRECATED: Use assert.js
 *
 * Asserts that an expression is true (non-zero and non-null).
 *
 * Note that it is critical not to pass logic
 * with side-effects as the expression for AssertTrue
 * because if the assertions are removed by the
 * JSCompiler, then the expression will be removed
 * as well, in which case the side-effects will
 * be lost. So instead of this:
 *
 *  AssertTrue( criticalComputation() );
 *
 * Do this:
 *
 *  var result = criticalComputation();
 *  AssertTrue(result);
 *
 * @param expression to evaluate
 * @param {string}   opt_msg to display if the assertion fails
 *
 */
function AssertTrue(expression, opt_msg) {
  if (ASSERTS_ON && !expression) {
    if (opt_msg === undefined) opt_msg = 'Assertion failed';
    Fail(opt_msg);
  }
}

/**
 * DEPRECATED: Use assert.js
 *
 * Asserts that two values are the same.
 *
 * @param val1
 * @param val2
 * @param {string} opt_msg to display if the assertion fails
 */
function AssertEquals(val1, val2, opt_msg) {
  if (ASSERTS_ON) {
    if (val1 != val2) {
      if (opt_msg === undefined) {
        /* SAFE */
        opt_msg = "AssertEquals failed: <" + val1 + "> != <" + val2 + ">";
      }
      Fail(opt_msg);
    }
  }
}

/**
 * DEPRECATED: Use assert.js
 *
 * Asserts that a value is of the provided type.
 *
 *   AssertType(6, Number);
 *   AssertType("ijk", String);
 *   AssertType([], Array);
 *   AssertType({}, Object);
 *   AssertType(ICAL_Date.now(), ICAL_Date);
 *
 * @param value
 * @param {function} type
 * @param {string} opt_msg to display if the assertion fails
 */
function AssertType(value, type, opt_msg) {
  if (ASSERTS_ON) {
    // for backwards compatability only
    if (typeof value == type) return;

    if (value || value == "") {
      try {
        if (type == AssertTypeMap[typeof value] || value instanceof type) {
          return;
        }
      } catch (e) { /* failure, type was an illegal argument to instanceof */ }
    }
    if (opt_msg === undefined) {
      if (typeof type == 'function') {
        var match = type.toString().match(/^\s*function\s+([^\s\{]+)/);
        if (match) type = match[1];
      }
      /* SAFE */
      opt_msg = "AssertType failed: <" + value + "> not typeof "+ type;
    }
    Fail(opt_msg);
  }
}

/* SAFE? */
var AssertTypeMap = {
  'string'  : String,
  'number'  : Number,
  'boolean' : Boolean
};

/**
 * DEPRECATED: Use assert.js
 *
 * Asserts that the number of arguments to a
 * function is num. For example:
 *
 * function myFunc(one, two, three) [
 *   AssertNumArgs(3);
 *   ...
 * }
 *
 * myFunc(1, 2); // assertion fails!
 *
 * Note that AssertNumArgs does not take the function
 * as an argument; it is simply used in the context
 * of the function.
 *
 * @param {int} num arguments expected
 * @param {string} opt_msg to display if the assertion fails
 */
function AssertNumArgs(num, opt_msg) {
  if (ASSERTS_ON) {
    var caller = AssertNumArgs.caller;  // This is not supported in safari 1.0
    if (caller && caller.arguments.length != num) {
      if (opt_msg === undefined) {
        opt_msg = caller.name + ' expected ' + num + ' arguments '
                    + ' but received ' + caller.arguments.length;
      }
      Fail(opt_msg);
    }
  }
}

//------------------------------------------------------------------------
// Cookies
//------------------------------------------------------------------------
var ILLEGAL_COOKIE_CHARS_RE = /[\s;]/
/**
 * Sets a cookie.
 * The max_age can be -1 to set a session cookie. To expire cookies, use
 * ExpireCookie() instead.
 *
 * @param name The cookie name.
 * @param value The cookie value.
 * @param opt_max_age The max age in seconds (from now). Use -1 to set a
 *   session cookie. If not provided, the default is -1 (i.e. set a session
 *   cookie).
 * @param opt_path The path of the cookie, or null to not specify a path
 *   attribute (browser will use the full request path). If not provided, the
 *   default is '/' (i.e. path=/).
 * @param opt_domain The domain of the cookie, or null to not specify a domain
 *   attribute (brower will use the full request host name). If not provided,
 *   the default is null (i.e. let browser use full request host name).
 * @return Void.
 */
function SetCookie(name, value, opt_max_age, opt_path, opt_domain) {

  value = '' + value;
  AssertTrue((typeof name == /* SAFE */ 'string' &&
              typeof value == /* SAFE */ 'string' &&
              !name.match(ILLEGAL_COOKIE_CHARS_RE) &&
              !value.match(ILLEGAL_COOKIE_CHARS_RE)),
             'trying to set an invalid cookie');

  if (!IsDefined(opt_max_age)) opt_max_age = -1;
  if (!IsDefined(opt_path)) opt_path = '/';
  if (!IsDefined(opt_domain)) opt_domain = null;

  var domain_str = (opt_domain == null) ? '' : ';domain=' + opt_domain;
  var path_str = (opt_path == null) ? '' : ';path=' + opt_path;

  var expires_str;

  // Case 1: Set a session cookie.
  if (opt_max_age < 0) {
    expires_str = '';

  // Case 2: Expire the cookie.
  // Note: We don't tell people about this option in the function doc because
  // we prefer people to use ExpireCookie() to expire cookies.
  } else if (opt_max_age == 0) {
    // Note: Don't use Jan 1, 1970 for date because NS 4.76 will try to convert
    // it to local time, and if the local time is before Jan 1, 1970, then the
    // browser will ignore the Expires attribute altogether.
    var date = new Date(1970, 1 /*Feb*/, 1);  // Feb 1, 1970
    expires_str = ';expires=' + date.toUTCString();

  // Case 3: Set a persistent cookie.
  } else {
    var date = new Date( Now() + opt_max_age * 1000 );
    expires_str = ';expires=' + date.toUTCString();
  }

  document.cookie = name + '=' + value + domain_str + path_str + expires_str;
}

var EXPIRED_COOKIE_VALUE = 'EXPIRED';

/**
 * Expires a cookie.
 *
 * @param name The cookie name.
 * @param opt_path The path of the cookie, or null to expire a cookie set at
 *   the full request path. If not provided, the default is '/' (i.e. path=/).
 * @param opt_domain The domain of the cookie, or null to expire a cookie set
 *   at the full request host name. If not provided, the default is null (i.e.
 *   cookie at full request host name).
 * @return Void.
 */
function ExpireCookie(name, opt_path, opt_domain) {
  SetCookie(name, EXPIRED_COOKIE_VALUE, 0, opt_path, opt_domain);
}

/** Returns the value for the first cookie with the given name
 * @param {string} name
 * @return {string} or the empty string if no cookie found.
 */
function GetCookie(name) {
  var nameeq = name + "=";
  var cookie = String(document.cookie);
  for (var pos = -1; (pos = cookie.indexOf(nameeq, pos + 1)) >= 0;) {
    var i = pos;
    // walk back along string skipping whitespace and looking for a ; before
    // the name to make sure that we don't match cookies whose name contains
    // the given name as a suffix.
    while (--i >= 0) {
      var ch = cookie.charAt(i);
      if (ch == ';') {
        i = -1;  // indicate success
        break;
      } else if (' \t'.indexOf(ch) < 0) {
        break;
      }
    }
    if (-1 === i) {  // first cookie in the string or we found a ;
      var end = cookie.indexOf(';', pos);
      if (end < 0) { end = cookie.length; }
      return cookie.substring(pos + nameeq.length, end);
    }
  }
  return "";
}


//------------------------------------------------------------------------
// Time
//------------------------------------------------------------------------
function Now() {
  return (new Date()).getTime();
}

//------------------------------------------------------------------------
// Dynamic HTML/DOM utilities
//------------------------------------------------------------------------
// Gets a element by its id, may return null
function MaybeGetElement(win, id) {
  return win.document.getElementById(id);
}

// Same as MaybeGetElement except that it throws an exception if it's null
function GetElement(win, id) {
  var el = win.document.getElementById(id);
  if (!el) {
    DumpError("Element " + id + " not found.");
  }
  return el;
}

// Gets elements by its id/name
// IE treats getElementsByName as searching over ids, while Moz use names.
// so tags must have both id and name as the same string
function GetElements(win, id) {
  return win.document.getElementsByName(id);
}

// Gets the parent of a html element.
function GetParentNode(n) {
  try {
    return n.parentNode;
  } catch (e) {
    // n.parentNode may throw a permission-denied exception on mozilla
    // (e.g. on text element), ignore this exception.
    return n;
  }
}

function IsDescendant(parent, child) {
  do {
    if (parent === child) return true;
    child = GetParentNode(child);
  } while (child && child !== document.body);
  return false;
}

// Get attribute value of a DOM node
function GetAttribute(node, attribute) {
  if (!node.getAttribute) {
    return null;
  }
  var attr = node.getAttribute(attribute);
  if (is_ie && attribute == "style") {
    return attr.value;
  } else {
    return attr;
  }
}

// Sets inner html of a html element
function SetInnerHTML(win, id, html) {
  try {
    GetElement(win, id).innerHTML = html;
  } catch (ex) {
    DumpException(ex);
  }
}

// Gets inner-html of a html element
function GetInnerHTML(win, id) {
  try {
    return GetElement(win, id).innerHTML;
  } catch (ex) {
    DumpException(ex);
    return "";
  }
}

// Clears inner html of a html element
function ClearInnerHTML(win, id) {
  try {
    GetElement(win, id).innerHTML = "";
  } catch (ex) {
    DumpException(ex);
  }
}

// Sets a CSS style of an element
function SetCssStyle(win, id, name, value) {
  try {
    var elem = GetElement(win, id);
    elem.style[name] = value;
  } catch (ex) {
    DumpException(ex);
  }
}

// Get CSS property from a style attribute string
function GetStyleProperty(style, name) {
  var i = style.indexOf(name);
  if (i != -1) {
    var j = style.indexOf(";", i);
    if (j == -1) {
      j = style.length;
    }
    // the +1 below is for the colon following the attribute name
    return CollapseWhitespace(style.substring(i + name.length + 1, j));
  }
  return null;
}

// Show/hide an element.
function ShowElement(el, show) {
  el.style.display = show ? "" : "none";
}

// Show/hide a block element.
// ShowElement() doesn't work if object has an initial class with display:none
function ShowBlockElement(el, show) {
  el.style.display = show ? "block" : "none";
}

// Show/hide an inline element.
// ShowElement() doesn't work when an element starts off display:none.
function ShowInlineElement(el, show) {
  el.style.display = show ? "inline" : "none";
}

// Set the text of a button. This is to get around a bug in mozilla,
// where we can't set the text of a button by setting innerHTML.
function SetButtonText(button, text) {
  button.childNodes[0].nodeValue = text;
}

// Append a new HTML element to a HTML node.
function AppendNewElement(win, parent, tag) {
  var e = win.document.createElement(tag);
  parent.appendChild(e);
  return e;
}

// Finds the child with the given ID, or null if there is node.
// This does not search the children's children.
function FindChildWithID(parent, id) {
  var el;
  for (el = parent.firstChild; el && el.id != id; el = el.nextSibling) {
    // skip
  }
  return el;
}

// Adds a disabled option to the given menu
function AddMenuDisabledOption(win, menu, html) {
  var op = AppendNewElement(win, menu, 'OPTION');
  op.disabled = true;
  op.innerHTML = html;

  return op;
}

// Adds a option to the given menu
function AddMenuOption(win, menu, value, html) {
  var op = AppendNewElement(win, menu, 'OPTION');
  op.value = value;
  op.innerHTML = html;

  return op;
}

// Create a new DIV (append it to the end of the document)
function CreateDIV(win, id) {
  var div = MaybeGetElement(win, id);
  if (!div) {
    div = AppendNewElement(win, win.document.body, "div");
    div.id = id;
  }
  return div;
}

// Create a new IFRAME (append it to the end of the document)
function CreateIFRAME(win, id, url) {
  var iframe = MaybeGetElement(win, id);
  if (!iframe) {
    // We cannot create an IFRAME directly (IE doesn't allow it), so we
    // create a DIV and then insert an IFRAME.
    // We also give the IFRAME a name (same as id)
    var div = AppendNewElement(win, win.document.body, "div");
    div.innerHTML = "<iframe id=" + id + " name=" + id +
                    " src=" + url + "></iframe>";
    iframe = GetElement(win, id);
  }
  return iframe;
}

/**
 * CreateIMG is safe to call in any browser.
 * it works around a bug in Safari 2.0.3 with:
 * var a = new Image;
 * Safari doesn't define style, so a.style.width throws.
 */
function CreateIMG(width, height){
  if (!is_safari) {
      return new Image(width, height);
  }
  var image = document.createElement('img');
  if (width && height) {
    image.width = width;
    image.height = height;
  }
  return image;
}


// Create a new TR containing the given td's
function Tr(win, tds) {
  var tr = win.document.createElement("TR");
  for (var i = 0; i < tds.length; i++) {
    tr.appendChild(tds[i]);
  }
  return tr;
}

// Create a new TD, with an optional colspan
function Td(win, opt_colspan) {
  var td = win.document.createElement("TD");
  if (opt_colspan) {
    td.colSpan = opt_colspan;
  }
  return td;
}


// Check if an element has a given class
// !!!requires that the classes in the className for the node
// are delimited by spaces and not other wacky whitespace
// characters, such as newlines
function HasClass(el, cl) {
  var className;
  if (el == null || !(className = el.className)) return false;
  // normalize className so each class is surrounded by single spaces
  className = " " + className + " ";
  var index = className.indexOf(" " + cl + " ");
  return (index >= 0);
}

// Add a class to element
function AddClass(el, cl) {
  if (HasClass(el, cl)) return;
  el.className += " " + cl;
}

// Remove a class from an element
function RemoveClass(el, cl) {
  if (el.className == null) return;
  var classes = el.className.split(" ");
  var result = [];
  var changed = false;
  for (var i = 0; i < classes.length; i++) {
    if (classes[i] != cl) {
      if (classes[i]) { result.push(classes[i]); }
    } else {
      changed = true;
    }
  }
  if (changed) { el.className = result.join(" "); }
}

/**
 * Performs an in-order traversal of the tree rooted at the given node
 * (excluding the root node) and returns an array of nodes that match the
 * given selector. The selector must be a function that takes a node and returns
 * a boolean.
 *
 * This method is a generalization of the DOM method "getElementsByTagName"
 */
function GetElementsBySelector(root, selector) {
  var nodes = [];
  for (var child = root.firstChild; child; child = child.nextSibling) {
    AddElementBySelector_(child, selector, nodes);
  }
  return nodes;
}

/** Recursive helper for GetElementsBySelector() */
function AddElementBySelector_(root, selector, nodes) {
  // First test the parent
  if (selector(root)) {
    nodes.push(root);
  }

  // Then recurse through the children
  for (var child = root.firstChild; child; child = child.nextSibling) {
    AddElementBySelector_(child, selector, nodes);
  }
}

//------------------------------------------------------------------------
// Window/screen utilities
// TODO: these should be renamed (e.g. GetWindowWidth to GetWindowInnerWidth
// and moved to geom.js)
//------------------------------------------------------------------------
// Get page offset of an element
function GetPageOffsetLeft(el) {
  var x = el.offsetLeft;
  if (el.offsetParent != null)
    x += GetPageOffsetLeft(el.offsetParent);
  return x;
}

// Get page offset of an element
function GetPageOffsetTop(el) {
  var y = el.offsetTop;
  if (el.offsetParent != null)
    y += GetPageOffsetTop(el.offsetParent);
  return y;
}

// Get page offset of an element
function GetPageOffset(el) {
  var x = el.offsetLeft;
  var y = el.offsetTop;
  if (el.offsetParent != null) {
    var pos = GetPageOffset(el.offsetParent);
    x += pos.x;
    y += pos.y;
  }
  return {x: x, y: y};
}

function GetPageOffsetRight(el) {
  return GetPageOffsetLeft(el) + el.offsetWidth;
}

function GetPageOffsetBottom(el) {
  return GetPageOffsetTop(el) + el.offsetHeight;
}

// Get the y position scroll offset.
function GetScrollTop(win) {
  // all except Explorer
  if ("pageYOffset" in win) {
    return win.pageYOffset;
  }
  // Explorer 6 Strict Mode
  else if ("documentElement" in win.document &&
           /* SAFE */ "scrollTop" in win.document.documentElement) {
    return win.document.documentElement.scrollTop;
  }
  // other Explorers
  else if (/* SAFE */ "scrollTop" in win.document.body) {
    return win.document.body.scrollTop;
  }

  return 0;
}

// Get the x position scroll offset.
function GetScrollLeft(win) {
  // all except Explorer
  if ("pageXOffset" in win) {
    return win.pageXOffset;
  }
  // Explorer 6 Strict Mode
  else if ("documentElement" in win.document &&
           "scrollLeft" in win.document.documentElement) {
    return win.document.documentElement.scrollLeft;
  }
  // other Explorers
  else if ("scrollLeft" in win.document.body) {
    return win.document.body.scrollLeft;
  }

  return 0;
}

/**
 * Checks if window scrollbar has reached its maximum offset
 *
 * @param win a window object
 * @param opt_isHoriz true if horizontal bar, false if vertical
 */
function IsScrollAtEnd(win, opt_isHoriz) {
  var total =
    (opt_isHoriz) ? document.body.offsetWidth : document.body.offsetHeight;
  var inner =
    (opt_isHoriz) ? GetWindowWidth(win) : GetWindowHeight(win);
  var offset =
    (opt_isHoriz) ? GetScrollLeft(win) : GetScrollTop(win);

  return (inner + offset >= total || total < inner);
}

// Scroll window to pos
// position: 0 = top, 0.5 = middle, 1 = bottom
function ScrollTo(win, el, position) {
  var y = GetPageOffsetTop(el);
  y -= GetWindowHeight(win) * position;
  win.scrollTo(0, y);
}

// Scroll so that as far as possible the entire element is in view.
var ALIGN_BOTTOM = 'b';
var ALIGN_MIDDLE = 'm';
var ALIGN_TOP = 't';
function ScrollIntoView(win, el, alignment) {
  var el_top = GetPageOffsetTop(el);
  var el_bottom = el_top + el.offsetHeight;
  var win_top = GetScrollTop(win);
  var win_height = GetWindowHeight(win);
  var win_bottom = win_top + win_height;

  // Out of view?
  if (el_top < win_top ||
      el_bottom > win_bottom) {

    var scrollto_y;
    if (alignment == ALIGN_BOTTOM) {
      scrollto_y = el_bottom - win_height + 5;
    } else if (alignment == ALIGN_MIDDLE) {
      scrollto_y = (el_top + el_bottom) / 2 - win_height/2;
    } else {
      scrollto_y = el_top - 5;        // ALIGN_TOP
    }

    Debug("Scrolling to " + scrollto_y);
    win.scrollTo(0, scrollto_y);
  }
}

function IsElementVisible(win, id) {
  var el = MaybeGetElement(win, id);
  if (el == null) {
    return false;
  }
  var el_top = GetPageOffsetTop(el);
  var el_bottom = el_top + el.offsetHeight;
  var win_top = GetScrollTop(win);
  var win_bottom = win_top + GetWindowHeight(win);
  if (el_top >= win_top && el_bottom <= win_bottom) {
    return true;
  }
  return false;
}

function GetWindowWidth(win) {
  // all except Explorer
  if ("innerWidth" in win) {
    return win.innerWidth;
  }
  // Explorer 6 Strict Mode
  else if ("documentElement" in win.document &&
           "clientWidth" in win.document.documentElement) {
    return win.document.documentElement.clientWidth;
  }
  // other Explorers
  else if ("clientWidth" in win.document.body) {
    return win.document.body.clientWidth;
  }

  return 0;
}

function GetWindowHeight(win) {
  // all except Explorer
  if ("innerHeight" in win) {
    return win.innerHeight;
  }
  // Explorer 6 Strict Mode
  else if ("documentElement" in win.document &&
           "clientHeight" in win.document.documentElement) {
    return win.document.documentElement.clientHeight;
  }
  // other Explorers
  else if ("clientHeight" in win.document.body) {
    return win.document.body.clientHeight;
  }

  return 0;
}

function GetAvailScreenWidth(win) {
  return win.screen.availWidth;
}

function GetAvailScreenHeight(win) {
  return win.screen.availHeight;
}

// Returns a "nice" window height.
// Use the screen height. (Or should we use the height of the current window?)
function GetNiceWindowHeight(win) {
  return Math.floor(0.8 * GetAvailScreenHeight(win));
}

// Used for horizontally centering a new window of the given width in the
// available screen. Set the new window's distance from the left of the screen
// equal to this function's return value.
// Params: width: the width of the new window
// Returns: the distance from the left edge of the screen for the new window to
//   be horizontally centered
function GetCenteringLeft(win, width) {
  return (win.screen.availWidth - width) >> 1;
}

// Used for vertically centering a new window of the given height in the
// available screen. Set the new window's distance from the top of the screen
// equal to this function's return value.
// Params: height: the height of the new window
// Returns: the distance from the top edge of the screen for the new window to
//   be vertically aligned.
function GetCenteringTop(win, height) {
  return (win.screen.availHeight - height) >> 1;
}

/*
 * Opens a child popup window that has no browser toolbar/decorations.
 * (Copied from caribou's common.js library with small modifications.)
 *
 * @param url the URL for the new window (Note: this will be unique-ified)
 * @param opt_name the name of the new window
 * @param opt_width the width of the new window
 * @param opt_height the height of the new window
 * @param opt_center if true, the new window is centered in the available screen
 * @param opt_hide_scrollbars if true, the window hides the scrollbars
 * @param opt_noresize if true, makes window unresizable
 * @param opt_blocked_msg message warning that the popup has been blocked
 * @return a reference to the new child window
 */
function Popup(url, opt_name, opt_width, opt_height, opt_center,
               opt_hide_scrollbars, opt_noresize, opt_blocked_msg) {
  if (!opt_height) {
    opt_height = Math.floor(GetWindowHeight(window.top) * 0.8);
  }
  if (!opt_width) {
    opt_width = Math.min(GetAvailScreenWidth(window), opt_height);
  }

  /* SAFE */
  var features = "resizable=" + (opt_noresize ? "no" : "yes") + "," +
                 "scrollbars=" + (opt_hide_scrollbars ? "no" : "yes") + "," +
                 "width=" + opt_width + ",height=" + opt_height;
  if (opt_center) {
    /* SAFE */
    features += ",left=" + GetCenteringLeft(window, opt_width) + "," +
                "top=" + GetCenteringTop(window, opt_height);
  }
  return OpenWindow(window, url, opt_name, features, opt_blocked_msg);
}

/*
 * Opens a new window. Returns the new window handle. Tries to open the new
 * window using top.open() first. If that doesn't work, then tries win.open().
 * If that still doesn't work, prints an alert.
 * (Copied from caribou's common.js library with small modifications.)
 *
 * @param win the parent window from which to open the new child window
 * @param url the URL for the new window (Note: this will be unique-ified)
 * @param opt_name the name of the new window
 * @param opt_features the properties of the new window
 * @param opt_blocked_msg message warning that the popup has been blocked
 * @return a reference to the new child window
 */
function OpenWindow(win, url, opt_name, opt_features, opt_blocked_msg) {
  var newwin = OpenWindowHelper(top, url, opt_name, opt_features);
  if (!newwin || newwin.closed || !newwin.focus) {
    newwin = OpenWindowHelper(win, url, opt_name, opt_features);
  }
  if (!newwin || newwin.closed || !newwin.focus) {
    if (opt_blocked_msg) alert(opt_blocked_msg);
  } else {
    // Make sure that the window has the focus
    newwin.focus();
  }
  return newwin;
}

/*
 * Helper for OpenWindow().
 * (Copied from caribou's common.js library with small modifications.)
 */
function OpenWindowHelper(win, url, name, features) {
  var newwin;
  if (features) {
    newwin = win.open(url, name, features);
  } else if (name) {
    newwin = win.open(url, name);
  } else {
    newwin = win.open(url);
  }
  return newwin;
}

//------------------------------------------------------------------------
// DOM walking utilities
//------------------------------------------------------------------------

function MaybeEscape(str, escape) {
  return escape ? HtmlEscape(str) : str;
}


//------------------------------------------------------------------------
// Window data
//------------------------------------------------------------------------
// Gets an array, which can store data for the window. This data
// is deleted when the window is unloaded.
var windata = [];
function GetWindowData(win) {
  var data = windata[win.name];
  if (!data) {
    windata[win.name] = data = [];
  }
  return data;
}

// Clear js data for a window.
function ClearWindowData(win_name) {
  if (windata[win_name]) {
    windata[win_name] = null;
  }
}

//------------------------------------------------------------------------
// String utilities
//------------------------------------------------------------------------
// Do html escaping
var amp_re_ = /&/g;
var lt_re_ = /</g;
var gt_re_ = />/g;

// Convert text to HTML format. For efficiency, we just convert '&', '<', '>'
// characters.
// Note: Javascript >= 1.3 supports lambda expression in the replacement
// argument. But it's slower on IE.
// Note: we can also implement HtmlEscape by setting the value
// of a textnode and then reading the 'innerHTML' value, but that
// that turns out to be slower.
// Params: str: String to be escaped.
// Returns: The escaped string.
function HtmlEscape(str) {
  if (!str) return "";
  /* SAFE */
  return str.replace(amp_re_, "&amp;").replace(lt_re_, "&lt;").
    replace(gt_re_, "&gt;").replace(quote_re_, "&quot;");
}

/** converts html entities to plain text.  It covers the most common named
 * entities and numeric entities.
 * It does not cover all named entities -- it covers &{lt,gt,amp,quot,nbsp}; but
 * does not handle some of the more obscure ones like &{ndash,eacute};.
 */
function HtmlUnescape(str) {
  if (!str) return "";
  return str.
    replace(/&#(\d+);/g,
      function (_, n) { return String.fromCharCode(parseInt(n, 10)); }).
    replace(/&#x([a-f0-9]+);/gi,
      function (_, n) { return String.fromCharCode(parseInt(n, 16)); }).
    replace(/&(\w+);/g, function (_, entity) {
      entity = entity.toLowerCase();
      return entity in HtmlUnescape.unesc ? HtmlUnescape.unesc[entity] : '?';
    });
}
/* SAFE? */
HtmlUnescape.unesc = { lt: '<', gt: '>', quot: '"', nbsp: ' ', amp: '&' };

// Replace multiple spaces with &nbsp; to retain whitespace formatting
// in addition to escaping '&', '<', and '>'.
var dbsp_re_ = /  /g;
var ret_re_ = /\r/g;
var nl_re_ = /\n/g;
function HtmlWhitespaceEscape(str) {
  str = HtmlEscape(str);
  str = str.replace(dbsp_re_, /* SAFE */ "&nbsp;&nbsp;");
  str = str.replace(ret_re_, "");
  str = str.replace(nl_re_, "<br>");
  return str;
}

// Escape double quote '"' characters in addition to '&', '<', '>' so that a
// string can be included in an HTML tag attribute value within double quotes.
// Params: str: String to be escaped.
// Returns: The escaped string.
var quote_re_ = /\"/g;
function QuoteEscape(str) {
  return HtmlEscape(str).replace(quote_re_, /* SAFE */ "&quot;");
}

function QuoteUnescape(str) {
  return HtmlUnescape(str.replace(/&quot;/g, "\""));
}

/// Safari 2.0.3 can't handle \u syntax in RegExp, but can in strings
/// so this syntax gurantees that string parser will get called.
///
/// Do not remove the explicit "" + [...] construct or Safari 2.0.3 will break!
var JS_SPECIAL_RE_ =
    new RegExp("" + ["[\\\\'\r\n\b\"<>&\u0085\u2028\u2029]"], "g");

function JSEscOne_(s) {
  return JSEscOne_.js_escs_[s];
}

/** convert a string to a javascript string literal.  This function has the
  * property that the return value is also already html escaped, so the output
  * can be embedded in an html handler attribute.
  */
function ToJSString(s) {
  if (!JSEscOne_.js_escs_) {
    var escapes = {};
    escapes['\\'] = '\\\\';
    escapes['\''] = '\\047';
    escapes['\b'] = '\\b';
    escapes['\"'] = '\\042';
    escapes['<'] =  '\\074';
    escapes['>'] =  '\\076';
    escapes['&'] =  '\\046';
    // newline characters according to
    // http://www.mozilla.org/js/language/js20/formal/lexer-grammar.html
    escapes['\n'] = '\\n';
    escapes['\r'] = '\\r';
    escapes['\u0085'] = '\\205';
    escapes['\u2028'] = '\\u2028';
    escapes['\u2029'] = '\\u2029';

    JSEscOne_.js_escs_ = escapes;
  }

  return "'" + s.toString().replace(JS_SPECIAL_RE_, JSEscOne_) + "'";
}

// converts multiple ws chars to a single space, and strips
// leading and trailing ws
var spc_re_ = /\s+/g;
var beg_spc_re_ = /^ /;
var end_spc_re_ = / $/;
function CollapseWhitespace(str) {
  if (!str) return "";
  return str.replace(spc_re_, " ").replace(beg_spc_re_, "").
    replace(end_spc_re_, "");
}

var newline_re_ = /\r?\n/g;
var spctab_re_ = /[ \t]+/g;
var nbsp_re_ = /\xa0/g;
function StripNewlines(str) {
  if (!str) return "";
  return str.replace(newline_re_, " ");
}

function CanonicalizeNewlines(str) {
  if (!str) return "";
  return str.replace(newline_re_, '\n');
}

function HtmlifyNewlines(str) {
  if (!str) return "";
  return str.replace(newline_re_, "<br>");
}

function NormalizeSpaces(str) {
  if (!str) return "";
  return str.replace(spctab_re_, " ").replace(nbsp_re_, " ");
}

// URL encodes the string.
function UrlEncode(str) {
  return encodeURIComponent(str);
}

var trimRegex1_ = /^\s+/;
var trimRegex2_ = /\s+$/;

function Trim(str) {
  if (!str) return "";
  return str.replace(trimRegex1_, "").replace(trimRegex2_, "");
}

function EndsWith(str, suffix) {
  if (!str) return !suffix;
  return (str.lastIndexOf(suffix) == (str.length - suffix.length));
}

// Check if a string is empty
function IsEmpty(str) {
  return CollapseWhitespace(str) == "";
}

// Check if a character is a letter
function IsLetterOrDigit(ch) {
  return ((ch >= "a" && ch <= "z") ||
          (ch >= "A" && ch <= "Z") ||
          (ch >= '0' && ch <= '9'));
}

// Check if a character is a space character
function IsSpace(ch) {
  return (" \t\r\n".indexOf(ch) >= 0);
}

// Converts any instances of "\r" or "\r\n" style EOLs into "\n" (Line Feed),
// and also trim the extra newlines and whitespaces at the end.
var eol_re_ = /\r\n?/g;
var trailingspc_re_ = /[\n\t ]+$/;
function NormalizeText(str) {
  return str.replace(eol_re_, "\n").replace(trailingspc_re_, "");
}

// Inserts <wbr>s (word break tag) after every n non-space chars and/or
// after or before certain special chars. The input string should be plain
// text that has not yet been HTML-escaped.
// Params:
//   str: The string to insert <wbr>s into.
//   n: The maximum number of consecutive non-space characters to allow before
//     adding a <wbr>. To turn off this rule (i.e. if you only want to add
//     breaks based on special characters), pass in the value -1.
//   chars_to_break_after: The list of special characters (concatenated into a
//     string) after which a <wbr> should be added, if there is no natural
//     break at that point. To turn off this rule, pass in the empty string.
//   chars_to_break_before: The list of special characters (concatenated into a
//     string) before which a <wbr> should be added, if there is no natural
//     break at that point. To turn off this rule, pass in the empty string.
// Returns: The string str htmlescaped, and with <wbr>s inserted according to
//   the rules specified by the other arguments.
function HtmlEscapeInsertWbrs(str, n, chars_to_break_after,
                              chars_to_break_before) {
  AssertNumArgs(4);

  var out = '';
  var strpos = 0;
  var spc = 0;

  for (var i = 1; i < str.length; ++i) {
    var prev_char = str.charAt(i - 1);
    var next_char = str.charAt(i);
    if (IsSpace(next_char)) {
      spc = i;
    } else if (i - spc == n ||
               chars_to_break_after.indexOf(prev_char) != -1 ||
               chars_to_break_before.indexOf(next_char) != -1) {
      out += HtmlEscape(str.substring(strpos, i)) + '<wbr>';
      strpos = i;
      spc = i;
    }
  }
  out += HtmlEscape(str.substr(strpos));
  return out;
}

// Converts a string to its canonicalized label form.
var illegal_chars_re_ = /[ \/(){}&|\\\"\000]/g;
function CanonicalizeLabel(str, lowercase) {
  var uppercase = str.replace(illegal_chars_re_, '-');
  return lowercase ? uppercase.toLowerCase() : uppercase;
}

// Locale & case-insensitive string comparator
function CompareStringsIgnoreCase(s1, s2) {
  if (s1.localeCompare) return s1.localeCompare(s2)

  s1 = s1.toLowerCase();
  s2 = s2.toLowerCase();

  if (s1 < s2) {
    return -1;
  } else if (s1 == s2) {
    return 0;
  } else {
    return 1;
  }
}

//------------------------------------------------------------------------
// TextArea utilities
//------------------------------------------------------------------------

// Gets the cursor pos in a text area. Returns -1 if the cursor pos cannot
// be determined or if the cursor out of the textfield.
function GetCursorPos(win, textfield) {
  try {
    if (IsDefined(textfield.selectionEnd)) {
      // Mozilla directly supports this
      return textfield.selectionEnd;

    } else if (win.document.selection && win.document.selection.createRange) {
      // IE doesn't export an accessor for the endpoints of a selection.
      // Instead, it uses the TextRange object, which has an extremely obtuse
      // API. Here's what seems to work:

      // (1) Obtain a textfield from the current selection (cursor)
      var tr = win.document.selection.createRange();

      // Check if the current selection is in the textfield
      if (tr.parentElement() != textfield) {
        return -1;
      }

      // (2) Make a text range encompassing the textfield
      var tr2 = tr.duplicate();
      tr2.moveToElementText(textfield);

      // (3) Move the end of the copy to the beginning of the selection
      tr2.setEndPoint("EndToStart", tr);

      // (4) The span of the textrange copy is equivalent to the cursor pos
      var cursor = tr2.text.length;

      // Finally, perform a sanity check to make sure the cursor is in the
      // textfield. IE sometimes screws this up when the window is activated
      if (cursor > textfield.value.length) {
        return -1;
      }
      return cursor;
    } else {
      Debug(/* SAFE */ "Unable to get cursor position for: " +
            navigator.userAgent);

      // Just return the size of the textfield
      // TODO: Investigate how to get cursor pos in Safari!
      return textfield.value.length;
    }
  } catch (e) {
    DumpException(e, /* SAFE */ "Cannot get cursor pos");
  }
}

function SetCursorPos(win, textfield, pos) {
  if (IsDefined(textfield.selectionEnd) &&
      IsDefined(textfield.selectionStart)) {
    // Mozilla directly supports this
    textfield.selectionStart = pos;
    textfield.selectionEnd = pos;

  } else if (win.document.selection && textfield.createTextRange) {
    // IE has textranges. A textfield's textrange encompasses the
    // entire textfield's text by default
    var sel = textfield.createTextRange();

    sel.collapse(true);
    sel.move("character", pos);
    sel.select();
  }
}

//------------------------------------------------------------------------
// Array utilities
//------------------------------------------------------------------------
// Find an item in an array, returns the key, or -1 if not found
function FindInArray(array, x) {
  for (var i = 0; i < array.length; i++) {
    if (array[i] == x) {
      return i;
    }
  }
  return -1;
}

// Inserts an item into an array, if it's not already in the array
function InsertArray(array, x) {
  if (FindInArray(array, x) == -1) {
    array[array.length] = x;
  }
}

// Delete an element from an array
function DeleteArrayElement(array, x) {
  var i = 0;
  while (i < array.length && array[i] != x)
    i++;
  array.splice(i, 1);
}

// Copies a flat array
function CopyArray(array) {
  var copy = [];
  for (var i = 0; i < array.length; i++) {
    copy[i] = array[i];
  }
  return copy;
}

// Clone an object (recursively)
function CloneObject(x) {
  if ((typeof x) == "object") {
    var y = [];
    for (var i in x) {
      y[i] = CloneObject(x[i]);
    }
    return y;
  }
  return x;
}

/**
 * Clone an event; cannot use CloneObject(event)
 * because it suffers from infinite recursion.
 * Thus, only a subset of the event properties are
 * cloned -- if you need others, just add them
 * to this function (just don't remove any!)
 */
function CloneEvent(ev) {
  var clone = {};
  clone.clientX = ev.clientX;
  clone.clientY = ev.clientY;
  clone.pageX = ev.pageX;
  clone.pageY = ev.pageY;
  clone.type = ev.type;
  clone.srcElement = ev.srcElement;
  clone.target = ev.target;
  clone.cancelBubble = ev.cancelBubble;
  clone.explicitOriginalTarget = ev.explicitOriginalTarget;
  // add more properties here

  return clone;
}

function GetEventTarget(/*Event*/ ev) {
// Event is not a type in IE; IE uses Object for events
//  AssertType(ev, Event, 'arg passed to GetEventTarget not an Event');
  return ev.srcElement || ev.target;
}

/** cancels the event */
// from http://www.quirksmode.org/js/events_order.html
function CancelEvent(/*Event*/ ev) {
  if (is_ie) {
    ev.cancelBubble = true;
  } else if (ev.stopPropagation) {
    ev.stopPropagation();
  }
}

//------------------------------------------------------------------------
// Formatting utilities
//------------------------------------------------------------------------
// A simple printf type function that takes in a template array, and a data
// array. e.g. PrintArray(["a",,"b",,"c"], ["x", "y"]) => axbyc
function PrintArray(array, data) {
  // Check that the argument count is correct.
  AssertEquals(array.length, data.length * 2 + 1);

  for (var i = 0, idx = 1; i < data.length; i++, idx += 2) {
    array[idx] = data[i];
  }
  return array.join("");
}

function ImageHtml(url, attributes) {
  return "<img " + attributes + " src=" + url + ">";
}

// Formats an object id that has two id numbers, eg, foo_3_7
function MakeId3(idprefix, m, n) {
  return idprefix + m + "_" + n;
}

//------------------------------------------------------------------------
// Email address parsing
//------------------------------------------------------------------------
// Parse an email address of the form "name" <address> into [name, address]
function ParseAddress(addr) {
  var name = "";
  var address = "";
  for (var i = 0; i < addr.length;) {
    var token = GetEmailToken(addr, i);
    if (token.charAt(0) == '<') {
      var end = token.indexOf(">");
      address = token.substring(1, (end != -1) ? end : token.length);
    } else if (address == "") {
      name += token;
    }
    i += token.length;
  }

  // Check if it's a simple email address of the form "jlim@google.com"
  if (address == "" && name.indexOf("@") != -1) {
    address = name;
    name = "";
  }

  name = CollapseWhitespace(name);
  name = StripQuotes(name, "'");
  name = StripQuotes(name, "\"");
  address = CollapseWhitespace(address);
  return [name, address];
}

// Given an email address, get the address part
function GetAddress(address) {
  return ParseAddress(address)[1];
}

// Get the username part of an email address
function GetAddressUsername(emailAddress) {
  var address = GetAddress(emailAddress);
  if (address == "") { address = emailAddress; } // was of the form "aarati"
  // Take the last "@", as "foo@bar.com"@baz.com is a valid address
  var at = address.lastIndexOf("@");
  return (at == -1) ? address : address.substr(0, at);
}

// Given an email address, get the personal part
function GetPersonal(address) {
  return ParseAddress(address)[0];
}

// Given an address, get a short name
function GetPersonalElseUsername(address) {
  var personal = GetPersonal(address);
  if (personal != "") {
    return personal;
  } else {
    return GetAddressUsername(address);
  }
}

// Strip ' or " chars around a string
function StripQuotes(str, quotechar) {
  var len = str.length;
  if (str.charAt(0) == quotechar &&
      str.charAt(len - 1) == quotechar) {
    return str.substring(1, len - 1);
  }
  return str;
}

// NOTE - this is depricated by emailvalidation.js [Email_SplitAddresses]
// Convert a string containing list of email addresses into an array
// of strings
function EmailsToArray(str) {
  var result = [];
  var email = "";
  var token;

  for (var i = 0; i < str.length; ) {
    token = GetEmailToken(str, i);
    if (token == ",") {
      AddEmailAddress(result, email);
      email = "";
      i++;
      continue;
    }
    email += token;
    i += token.length;
  }

  // Add last
  if (email !="" || token == ",") {
    AddEmailAddress(result, email);
  }
  return result;
}

// Get the next token from a position in an address string
var openers_ = "\"<([";
var closers_ = "\">)]";
function GetEmailToken(str, pos) {
  var ch = str.charAt(pos);
  var p = openers_.indexOf(ch);
  if (p == -1)
    return ch;
  var end_pos = str.indexOf(closers_.charAt(p), pos + 1);
  var token = (end_pos >= 0) ? str.substring(pos, end_pos + 1) :
              str.substr(pos);
  return token;
}

// Add an email address to the result array.
function AddEmailAddress(result, email) {
  email = CleanEmailAddress(email);
  result[result.length] = email;
}

// TODO(msamuel) - I don't think this function is called anymore,
// and should be removed
// TODO(msamuel) - it also seems broken since it searches for strings
// with a quote anywhere, not just at the begining
// Clean up email address:
// - remove extra spaces
// - Surround name with quotes if it contains special characters
// to check if we need " quotes
// Note: do not use /g in the regular expression, otherwise the
// regular expression cannot be reusable.
var specialchars_re_ = /[()<>@,;:\\\".\[\]]/;

function CleanEmailAddress(str) {
  var name_address = ParseAddress(str);
  var name = name_address[0];
  var address = name_address[1];

  if (name.indexOf("\"") == -1) {  // If there's no "
    var quote_needed = specialchars_re_.test(name);
    if (quote_needed) {
      name = "\"" + name + "\"";
    }
  }

  if (name == "")
    return address;
  else if (address == "")
    return name;
  else
    return name + " <" + address + ">";
}

// TODO(msamuel) - This probably can be refactored out when we
// rewrite the Person Store.
// TODO(msamuel) - this is somewhat broken since it assumes that the
// name is already escaped, and contains no control characters, <, >, ", etc
// make CleanEmailParts properly handle the case
// commonName='Mike "Middle-Name is my middle name" Samuel'
//
// Faster version if we already have the name and address
function CleanEmailParts(name, address) {
  if (name.indexOf("\"") == -1) {  // If there's no "
    var quote_needed = specialchars_re_.test(name);
    if (quote_needed) {
      name = "\"" + name + "\"";
    }
  }

  if (name == "")
    return address;
  else if (address == "")
    return name;
  else
    return name + " <" + address + ">";
}

//------------------------------------------------------------------------
// Misc
//------------------------------------------------------------------------
// Compare long hex strings
function CompareID(a, b) {
  if (a.length != b.length) {
    return (a.length - b.length);
  } else {
    return (a < b) ? -1 : (a > b) ? 1 : 0;
  }
}

// Check if a value is defined
function IsDefined(value) {
  return (typeof value) != 'undefined';
}

/**
* SafariKeydownFix - fix bug where safari incorrectly triggers duplicate
* keydown/keyup events when there is a focused text input area and an
* adjacent div with a higher z-coordinate, such as our implementation of
* comboboxes and date pickers. This comment refers to comboboxes and date
* pickers as 'popupmenus'.
*
* Returns true if this should consume the event, so further processing
* by the caller isn't necessary.
* Caller should return if this returns true. Safe to call in any browser.
*
* Safari has a bug in popupmenus where we've registered a keydown eventhandler,
* and a single keydown is reported as 2 events with different timestamps.
*
* Workaround:
* Caller must install a keyUp handler, that calls this.
* Caller must also call this from keyDown handler.
*
* This looks for the specific pattern of the bug:
* keyDown keyDown keyUp keyUp
* and ignores the redundant downs and ups.
*
* Fixes: http://b/issue?id=507167
*/
function SafariKeydownFix(e, owner) {
  if (!is_safari) return false; // only functional in Safari.
  if ('keyup' == e.type) {
    owner.keydown_ = 0;
    return true;
  } else if ('keydown' == e.type) {
    var keycode = GetKeyCode(e);
    if (owner.keydown_ == keycode) {
      return true;
    }
    owner.keydown_ = keycode;
  }
  return false;
}


/**
 * !!!Warning: If event.type == 'keypress', then the keyCode
 * may be incorrect! This happens on both IE6 and Firefox 1.0.x (Fx).
 * Consider an event, E, that is received by a keypress listener
 * when the letter 'u' is typed:
 *
 * On Fx, E.which and E.charCode are 117 (E.keyCode is 0)
 * On IE, E.keyCode is 117 (E.which and E.charCode are undefined)
 *
 * Now, at first, this may seem sensible since 117 is the ASCII
 * value for lowercase u. Unfortunately, this conflicts with
 * the fact that the Unicode key code for F6 is also 117.
 * (On Firefox, you will see that E.DOM_VK_F6 is 117.)
 *
 * So if E.type == 'keypress' && E.keyCode == 117, which key was pressed?
 * Well, on IE, this ambiguity appears to be removed because hitting
 * a function key (F1-F12) does not generate a keypress event (but
 * keydown and keyup events are still generated).
 *
 * So how can this ambiguity be resolved on Firefox? Unfortunately, E.isChar
 * is false when both F6 and u are typed, which is too bad, since it would seem
 * like a logical way to distinguish between letter and function keys.
 *
 * The one notable difference is in the value of the "which" field, which is
 * specific to Fx. When 'u' is pressed, then E.which is 117 while E.keyCode is
 * 0.
 * However, when F6 is pressed, E.which is 0 and E.keyCode is 117.
 *
 * This function has been refactored to identify the cases where the key code
 * may differ from what the client expects. Its behavior has not been changed
 * as that may break existing code that depends upon GetKeyCode's functionality.
 * The safest thing to do is to use onKeyDown listeners instead of onKeyPress
 * listeners whenever possible.
 */
function GetKeyCode(event) {
  var code;
  if (event.type === 'keypress') {
    if (is_ie) {
      code = event.keyCode;
      // May be the ASCII value; up to client to convert to unicode key code
    } else if (is_nav) {
      code = event.which;
      if (code) {
        // May be the ASCII value; up to client to convert to unicode key code
      } else {
        code = event.keyCode;
      }
    } else {
      // Fail("GetKeyCode() only tested on IE and Mozilla");
      return event.keyCode || event.which;
    }
  } else {
    if (event.keyCode) {
      code = event.keyCode;
    } else if (event.which) {
      code = event.which;
    }
  }
  return code;
}

// define a forid function to fetch a DOM node by id.
function forid_1(id) {
  return document.getElementById(id);
}
function forid_2(id) {
  return document.all[id];
}

/**
 * Fetch an HtmlElement by id.
 * DEPRECATED: use $ in dom.js
 */
//var forid = document.getElementById ? forid_1 : forid_2;

/**
 * GetFnName returns the name of the provided function and
 * adds a property to the function called "name" with the name of the
 * function, if the property is not already present.
 *
 * GetFnName can help reduce the size of the code generated by JSCompiler.
 * For example, if you have code that refers to a function, MyFun, in a string:
 *
 * '<a href="javascript:MyFun()">' + linkLabel + '</a>'
 *
 * MyFun cannot be renamed by JSCompiler because it is inlined in a string.
 * GetFnName can be used to remove the inlining:
 *
 * '<a href="javascript:' + GetFnName(MyFun) + '()">' + linkLabel + '</a>'
 *
 * When written this way, MyFun can be renamed by JS Compiler.
 *
 * @throws an Error if any of the following are true:
 *         (1) the name of the function cannot be extracted,
 *         (2) the name of the function is the empty string, null, or undefined
 *         (3) the name of the function is "anonymous"
 *   An Error is thrown in cases (2) and (3) because neither "" nor "anonymous"
 *   is a name that can be used to invoke the function.
 *
 *   Examples of valid functions that produce these errors are:
 *   f = function(a) {return a;};        // name is ""
 *   g = new Function('a', 'return a');  // name is "anonymous"
 *
 */
function GetFnName(func) {
  // AssertType(func, Function) fails for window.alert on IE, which is why
  // a weaker assertion, AssertTrue, is used in place of AssertType
  AssertTrue(func, "func passed to GetFnName() is undefined");
  var name;
  if (!('name' in func)) {
    var match = /\W*function\s+([\w\$]+)\(/.exec(func);
    if (!match) {
      throw new Error("Cannot extract name from function: " + func);
    }
    name = match[1];
    func.name = name;
  } else {
    name = func.name;
  }
  if (!name || name == 'anonymous') {
    throw new Error("Anonymous function has no name: " + func);
  }
  return func.name;
}

var rotlog_ = new Array(10);
var rotlogPos_ = 0;
function log(msg) {
  if (LOG_ON) {
    print(msg);
  }
}

var logWindow = null;
function _showLogWindow() {
  if (LOG_ON) {
    logWindow = window.open();
    logWindow.document.write(
        '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">'
        + '\n<html>'
        + '\n<head>'
        + '\n<title>Log</title>'
        + '\n<script type=text/javascript><!--'
        + '\nfunction wrap(s) {'
        + '\n  s = String(s);'
        + '\n  if (s.length <= 80) { return s; }'
        + '\n  var lines = s.split(/\\r\\n?|\\n/g);'
        + '\n  for (var i = 0; i < lines.length; ++i) {'
        + '\n    lines[i] = wrapLine(lines[i]);'
        + '\n  }'
        + '\n  return lines.join("\\r\\n");'
        + '\n}'
        + '\n'
        + '\nfunction wrapLine(s) {'
        + '\n  if (s.length <= 80) { return s; }'
        + '\n  var parts = [];'
        + '\n  var i = 0;'
        + '\n  var pre = "\\r\\n" + s.match(/^\\s*/)[0];'
        + '\n  if (pre.length > 40) { pre = "\\r\\n"; }'
        + '\n  while (i + 80 < s.length) {'
        + '\n    parts.push(s.substring(i, i + 80));'
        + '\n    i += 80;'
        + '\n    if (i < s.length) { parts.push(pre); }'
        + '\n  }'
        + '\n  parts.push(s.substring(i, s.length));'
        + '\n  return parts.join("");'
        + '\n}'
        + '\n'
        + '\nfunction log(msg) {'
        + '\n  var logcont = document.getElementById("log");'
        + '\n  var e = document.createElement("PRE");'
        + '\n  e.appendChild(document.createTextNode(wrap(msg)));'
        + '\n  logcont.appendChild(e);'
        + '\n  setTimeout(function () {'
        + '\n    e.style.color = "#444";'
        + '\n    setTimeout(function () { e.style.color = "#888"; }, 10000);'
        + '\n  }, 1000);'
        + '\n}'
        + '\n// log stuff that was saved from before.'
        + '\n// Make sure that it shows up first'
        + '\nvar lastPrelogged = null;'
        + '\nfunction prelog(msg) {'
        + '\n  var logcont = document.getElementById("log");'
        + '\n  var e = document.createElement("PRE");'
        + '\n  e.appendChild(document.createTextNode(wrap(msg)));'
        + '\n'
        + '\n  e.style.color = "#888";'
        + '\n  if (!lastPrelogged || !lastPrelogged.nextSibling) {'
        + '\n    logcont.appendChild(e);'
        + '\n  } else {'
        + '\n    logcont.insertBefore(e, lastPrelogged.nextSibling);'
        + '\n  }'
        + '\n  lastPrelogged = e;'
        + '\n}'
        + '\n--></script>'
        + '\n</head>'
        + '\n'
        + '\n<body bgcolor=white>'
        + '\n<div id=log></div>'
        + '\n</body>'
        + '\n</html>'
        );
    logWindow.document.close();

    var tries = 10;
    function prelog() {
      if (logWindow.prelog) {
        var rl = rotlog_;
        var rlp = rotlogPos_;
        rotlog_ = new Array(10);
        rotlogPos_ = 0;
        var i = rlp;
        while (true) {
          if (rl[i]) { logWindow.prelog(rl[i]); }
          i = (i + 1) % rl.length;
          if (i === rlp) { break; }
        }
      } else if (--tries >= 0) {
        setTimeout(prelog, 250);
      }
    }
    prelog();
  }
}

/** give focus to an item with a specific id
  * @param idOrElement {string|Element}, the item to give focus to;
  *  if string, then use it as the id of the node; otherwise,
  *  use the element itself
  */
function SetFocus(idOrElement) {
  AssertTrue(idOrElement);
  var item =
     (typeof idOrElement === 'string') ? forid(idOrElement) : idOrElement;
  if (item) {
    try {
      item.focus();
      if (item.tagName == 'INPUT' && (item.type == 'text' ||
                                      item.type == 'password')) {
        SetCursorPos(window, item, 0);
      }
    } catch (e) {}
    // EXCEPTION = noop. Sometimes IE throws an exception if the item is hidden
  }
}

/**
 * list of pairs of [ varz variable name, function() ].  The function returns
 * the value of the monitored variable.
 */
var varZ = [];
function exportVarZ(name, handlerFunction) {
  if (LOG_ON) {
    AssertType(name, String);
    AssertType(handlerFunction, Function);
    varZ.push([ name, handlerFunction ]);
  }
}

/**
 * generates an html table describing the current state of the varzs.
 */
function _getVarZHtml() {
  if (LOG_ON) {
    var html = [ '<table>' ];
    for (var i = 0; i < varZ.length; ++i) {
      var oneVar = varZ[i];
      var result;
      try {
        result = HtmlEscape('' + oneVar[1]());
      } catch (e) {
        result = '<font color=red>' + HtmlEscape(e.toString()) + '</font>';
      }
      html.push('<tr><td>', HtmlEscape(oneVar[0]), '<td>', result);
    }
    html.push('</table>');
    return html.join('');
  }
}

/**
 * pops up a window which periodically refreshes the varz state.
 */
function _showVarZWindow() {
  if (LOG_ON) {
    var varZWindow = window.open();
    varZWindow.document.write(
        '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">'
        + '\n<html>'
        + '\n<head>'
        + '\n<title>VarZ</title>'
        + '\n<script type=text/javascript><!--'
        + '\nfunction fetch() {'
        + '\n  try {\n'
        + '\n    document.getElementById("varZ").innerHTML ='
        + '\n      window.opener._getVarZHtml();'
        + '\n    setTimeout(fetch, 2000);'
        // the opener window was closed
        + '\n  } catch (e) {'
        + '\n    document.getElementById("varZ").innerHTML = "closing";'
        + '\n    setTimeout(function () { window.close(); }, 2000);'
        + '\n  }'
        + '\n}'
        + '\n--></script>'
        + '\n</head>'
        + '\n'
        + '\n<body bgcolor=white onload="fetch()">'
        + '\n<div id=varZ></div>'
        + '\n</body>'
        + '\n</html>'
        );
    varZWindow.document.close();
  }
}
/** make sure the varz window displays the time it was last updated. */
exportVarZ('last updated', function() { return new Date; });

/**
 * Escape a string to be used in a regular expression.  (By putting a backslash
 * before all regex special characters.
 */
function escapeRegExpPart(str) {
  return str.replace(/([\^*+\-\$\\\{\}\(\)\[\]\#?\.])/g, '\\$1');
}
