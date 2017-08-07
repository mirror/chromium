
# ApplicationContext

ApplicationContext stores application state and give access to global
objects (singletons). The state accessible from the ApplicationContext
is shared by all browsing session (either incognito or not).

# ChromeBrowserState

ChromeBrowserState stores the state tied to a browsing session. Roughly
it correspond to one user account (whether it is synchronised or not).
There is also a special ChromeBrowserState corresponding to the incognito
session.

For each ChromeBrowserState, there is a directory used to store the
corresponding state (preferences, session state, ...).

The ChromeBrowserState also owns most of the services that are not tied
to a specific tab (history, bookmarks, read list, ...).

# BrowserList

BrowserList is a container owning Browser instances. It is owned by the
ChromeBrowserState and each ChromeBrowserState has one associated
BrowserList.

# Browser

Browser represents a window containing multiple tabs (currently there
is only one Browser per BrowserList on iOS). It owns the Dispatcher
and has a back pointer to the owning ChromeBrowserState.

The Browser also owns a WebStateList.

# Dispatcher

Dispatcher allow decoupling implementation of the Chrome commands (e.g.
open a new tab, ...) from the UI invoking the command.

# WebStateList

WebStateList represents a list of WebStates. It maintains a notion of
an active WebState and opener-opened relationship between WebStates.

# WebState

WebState wraps a WKWebView and allows navigation. A WebState can have
many tab helper attached. A WebState in a WebStateList correspond to
an open tab and the corresponding tab helper can be assumed to be
created.

