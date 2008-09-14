// copied from google3/java/com/google/caribou/antlers/fin/jsdata

//------------------------------------------------------------------------
// Debugging and error reporting.
//------------------------------------------------------------------------
// Debug(): Write out debug message.
// Warn(): Writes out a warning message.
// DumpError(): Writes out a error message
// DumpException(): Writes out a exception error

var DB_mode = false;  // whether debug mode is active

// Log levels
var DEBUG = 0;
var SEVERE = 1;

// Number of request errors
var DB_request_errors = 0;

// Debug
function Debug(str) {
  DB_WriteDebugMsg(str, DEBUG);
}

// Report a js error
function DumpError(str) {
  try {
    throw str;
  } catch (e) {
    DumpException(e);
  }
}

// Dump exception info
function DumpException(e, opt_msg) {
  var title = "Javascript exception: " + (opt_msg ? opt_msg : "") + " " + e;
  if (is_ie) {
    // For IE, we need to make the titles more descriptive to better
    //   separate out the different exceptions in the aggregator
    title += ' ' + e.name + ': ' + e.message + ' (' + e.number + ')';
  }
  var error = "";
  if (typeof e == "string") {
    error = e + "\n";
  } else {
    for (var i in e) {
      try {
        error += i + ": " + e[i] + "\n";
      } catch (ex) {
        // Ignored, some properties of the error object are not accessible.
      }
    }
  }
  error += DB_GetStackTrace(DumpException.caller);

  DB_WriteDebugMsg(title + '\n' + error, SEVERE);
  DB_SendJSReport(title, error);
}

// This delimiter is used by the CaribouServer to separate the title of the
// exception from the details. It should be equivalent to that stored in FinUI
var DB_EXCEPTION_TITLE_SEPARATOR = "\n===\n";

// This send js error report back to the server. We throttle the
// sending rate to at most once per minute.
var DB_last_report_time = 0;
var DB_unsent_reports = 0;
var REPORT_INTERVAL = 60 * 1000; // = 1 minute
function DB_SendJSReport(title, msg) {
}

///------------------------------------------------------------------------
// Stack Trace
//------------------------------------------------------------------------
// Get function name given a function object
var function_name_re_ = /function (\w+)/;
function DB_GetFunctionName(fn) {
  var m = function_name_re_.exec(String(fn));
  if (m) {
    return m[1];
  }
  return "";
}

// Dumps out the stack.
function DB_GetStackTrace(fn) {
  try {
    if (is_nav) {
      // For mozilla, an Error object contains the stacktrace.
      return Error().stack;
    }

    // Traverse the call stack
    if (!fn)
      return "";
    var x = "\- " + DB_GetFunctionName(fn) + "(";
    for (var i = 0; i < fn.arguments.length; i++) {
      if (i > 0) x += ", ";
      var arg = String(fn.arguments[i]);
      if (arg.length > 40) {
        arg = arg.substr(0, 40) + "...";
      }
      x += arg;
    }
    x += ")\n";
    x += DB_GetStackTrace(fn.caller);
    return x;
  } catch (ex) {
    return /* SAFE */ "[Cannot get stack trace]: " + ex + "\n";
  }
}

//------------------------------------------------------------------------
// Timer
//------------------------------------------------------------------------
var DB_events = [];
var DB_starttime;
var DB_js_start = [];
var DB_js_time = [];
var DB_total_js_time = []

function DB_ClearTimings() {
  DB_events = [];
  DB_js_start = [];
  DB_js_time = [];
}

function DB_ClearTotalTimings() {
  DB_total_js_time = [];
}

function DB_Event(desc) {
  var now = Now();
  var n = DB_events.length;
  if (n == 0) {
    DB_starttime = now;
    DB_WriteDebugHtml("<hr>");
  }
  Debug(desc);
  DB_events[n] = [desc, now];
}

// Measures time spent in js functions.
function DB_StartJS(name) {
  DB_js_start[name] = Now();
}

function DB_EndJS(name) {
  var start = DB_js_start[name];
  if (start) {
    var elapsed = Now() - start;
    DB_js_time[name] = (DB_js_time[name] ? DB_js_time[name] : 0) + elapsed;
    DB_total_js_time[name] = (DB_total_js_time[name] ? DB_total_js_time[name]
                              : 0) + elapsed;
    DB_js_start[name] = 0;
  }
}

function DB_GetEventTime(name) {
  for (var i = 0; i < DB_events.length; i++) {
    if (DB_events[i][0] == name)
      return DB_events[i][1];
  }
  return -1;
}

// Report timings and add timing info at the bottom of the window
function DB_ShowTimingFooter(win) {
  var html = DB_ReportTimings(win);
  var div = CreateDIV(win, "_timingfooter");
  div.innerHTML = html;
}

// Gets a summary of the timings
function DB_ReportTimings(win) {
  // To update the js timings.
  for (var i in DB_js_time) {
    DB_EndJS(i);
  }

  var html = "Timing info:";
  var total = 0;
  for (var i = 0; i < DB_events.length; i++) {
    if (i > 0) {
      var elapsed = DB_events[i][1] - DB_events[i - 1][1];
      html += elapsed;
      total += elapsed;
    }
    html += " [" + DB_events[i][0] + "] ";
  }

  // Total time spent in JS
  var js_time = DB_js_time["JS"] ? DB_js_time["JS"] : 0;

  // Total time spent in document.write
  var docwrite_time = DB_js_time["Doc.write"] ? DB_js_time["Doc.write"] : 0;
  js_time -= docwrite_time;

  html += "[Total JS: " + js_time + "] " +
          "[Total Doc.write: " + docwrite_time + "]" +
          " Total: " + total + "ms.<br>Generated: " +
          (new Date()).toLocaleString();

  // Loading time
  var load_time = DB_GetEventTime("Start") - DB_GetEventTime("Loading");

  // Record timing info
  GetWindowData(win).time_info = load_time + "/" + js_time + "/" +
                                 docwrite_time + "/" + total;

  DB_ClearTimings();
  return html;
}

function DB_GetTotalTimings() {
  var str = "";
  for (var i in DB_total_js_time) {
    str += "[Total " + i + ": " + DB_total_js_time[i] + "ms] ";
  }
  DB_ClearTotalTimings();
  return str;
}

// This is called when a page is loaded. We export the stats in a cookie.
var GMAIL_STAT_COOKIE = "GMAIL_STAT";
function DB_ExportStats(win, servertime, id, view, time_info) {
  // Set GMAIL_STAT cookie to:
  // id/view/server time/load time/js time/document write time/total time
  var cookie = id + "/" + view + "/" + servertime + "/" + time_info + "/" +
               DB_request_errors;

  Debug("Setting cookie: " + cookie);
  document.cookie = GMAIL_STAT_COOKIE + "=" + cookie + ";path=/";

  // Reset request errors
  DB_request_errors = 0;
}

// Keeps track of request errors
function DB_IncRequestErrors(view) {
  DB_request_errors++;
}

//------------------------------------------------------------------------
// Debug window
//------------------------------------------------------------------------
var DB_win = null;
var DB_winopening = false;

// Open debug window
function DB_OpenDebugWindow() {
  if ((DB_win == null || DB_win.closed) &&
      !DB_winopening) {
    try {
      DB_winopening = true;
      DB_win = window.open(
          "","debug",
          (/* SAFE */ "width=700,height=500") +
          (/* SAFE */ ",toolbar=no,resizable=yes,scrollbars=yes,") +
          (/* SAFE */ "left=16,top=16,screenx=16,screeny=16"));
      DB_win.blur();
      //DB_win.document.close();
      DB_win.document.open();
      DB_winopening = false;
      /* SAFE */
      var html = "<font color=#ff0000><b>To turn off this debugging window," +
        "hit 'D' inside the main caribou window, " +
        "then close this window.</b></font><br>";
      DB_WriteDebugHtml(html);
    } catch (ex) {
    }
  }
}

// Write out to debug window
function DB_WriteDebugMsg(str, level) {
  if (!DB_mode) {
    if (typeof log != 'undefined') { log(HtmlEscape(str)); }
    return;
  }
  try {
    var t = Now() - DB_starttime;
    var html = "[" + t + "] " +
               HtmlEscape(str).replace(/\n/g, "<br>") + "<br>";
    if (level == SEVERE) {
      html = "<font color=#ff0000><b>Error: " + html + "</b></font>";
      DB_win.focus();
    }
    DB_WriteDebugHtml(html);
  } catch (ex) {
  }
}

// Write out to debug window
function DB_WriteDebugHtml(html) {
  if (!DB_mode) { return; }
  try {
    DB_OpenDebugWindow();
    DB_win.document.write(html);
    DB_win.scrollTo(0, 1000000);
  } catch (ex) {
  }
}

//------------------------------------------------------------------------
// Debugging tools
//------------------------------------------------------------------------
// Show page html source in a popup window.
function DB_ShowHtmlSource(win) {
  var w = win.open("","");
  w.document.open();
  w.document.write("<pre>" + HtmlEscape(win.document.documentElement.innerHTML)
                   + "\n</pre>");
  w.document.close();
}
