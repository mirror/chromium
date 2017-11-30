# Offscreen, Invisible and Size

This document explains how Chrome interprets the guidelines to apply the labels
Offscreen and Invisible to nodes, and how size is calculated.

## Background

In general, screen reading tools may be interested in all nodes regardless of
whether they are presented to sighted users, but other Accessibility tools may
care what is visible to sighted users.

Specifically, tools like Select-to-Speak and Switch Access should not look at
nodes which are “offscreen”, “invisible”, or size=(0,0), as these are not
visible on the screen for mouse interactions. On the other hand, ChromeVox and
other screen readers may care about some of those nodes, which allow developers
to insert buttons visible only to users with a screen reader, or to navigate
below the fold.

## Offscreen
In Chrome, we define Offscreen as:

>Any object is offscreen if it is fully clipped or scrolled out of view by any
of its ancestors so that it is not rendered on the screen, and clicking in its
bounding box does not result in that object. Nodes with no area are not marked
as offscreen, but children of nodes with no area, which have overflow hidden,
are marked as offscreen.

As background, [MSDN](https://msdn.microsoft.com/en-us/library/dd373609(VS.85).aspx)
defines Offscreen as an object is not rendered, but not because it was
programatically hidden:

>The object is clipped or has scrolled out of view, but it is not
programmatically hidden. If the user makes the viewport larger, more of the
object will be visible on the computer screen.

In Chrome, we interpret this to mean that an object is fully clipped or
scrolled out of view by its parent or ancestors.

### Technical Implementation
Offscreen is calculated in[AXTree::RelativeToTreeBounds](https://cs.chromium.org/chromium/src/ui/accessibility/ax_tree.cc).
In this function, we walk up the accessibility tree adjusting a node's bounding
box to the frame of its ancestors. If the box is clipped because it lies
outside an ancestor's bounds, and that ancestor clips its children (i.e.
overflow:hidden, overflow:scroll, or it is a rootWebArea), offscreen is set to
true.

## Invisible
A node is marked Invisible in Chrome if it is hidden programatically. In some
cases, invisible nodes are simply excluded from the accessibility tree. Chrome defines invisible as:

>Invisible means that a node or its ancestor is explicitly invisible, like
display:none, visiblity:hidden, or similar.

This is the same as the definition from [MSDN](https://msdn.microsoft.com/en-us/library/dd373609(VS.85).aspx):

>The object is programmatically hidden.

## Size calculation
A node's location and size are calculated based on its intrinsic width, height
and location, and the sizes of its ancestors. We calculate size clipped by
ancestors by default, but can also expose an unclipped size through the
[automation API](https://developer.chrome.com/extensions/automation).

### Technical implementation
A node's location and size are calculated in[AXTree::RelativeToTreeBounds](https://cs.chromium.org/chromium/src/ui/accessibility/ax_tree.cc),
and so closely tied to the offscreen calculation. In this function, we walk up
the accessibility tree adjusting a node's bounding box to the frame of its
ancestors.

In general, this calculation is straight forward. But there are several edge
cases:

* If a node has no intrinsic size, its size will be taken from the union of
its children.

* If a node still has no size after that union, its bounds will be set to the
size of the nearest ancestor which has size. However, this node will be marked
`offscreen`, because it isn't visible to the user.

In addition, [AXTree::RelativeToTreeBounds](https://cs.chromium.org/chromium/src/ui/accessibility/ax_tree.cc)
is used to determine how location and size may be clipped by ancestors,
allowing bounding boxes to reflect the location of a node clipped to its
ancestors.

* If a node is fully clipped by its ancestors such that the intersection of its
bounds and an ancestor's bounds are size 0, it will be pushed to the nearest
edge of the ancestor with width 1.

Both clipped and unclipped location and size are exposed through the
[Chrome automation API](https://developer.chrome.com/extensions/automation).

* `location` is the global location of a node as clipped by its ancestors. If
a node is fully scrolled off a rootWebArea in X, for example, its location will
be the height of the rootWebArea, and its height will be 1. The Y position and width will be unchanged.

* `unclippedLocation` is the global location of a node ignoring any clipping
by ancestors. If a node is fully scrolled off a rootWebArea in X, for example,
its location will simply be larger than the height of the rootWebArea, and its
size will also be unchanged.
