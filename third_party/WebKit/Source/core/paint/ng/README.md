# LayoutNG Paint #

This directory contains the paint system to work with
the Blink's new layout engine [LayoutNG].

This README can be viewed in formatted form
[here](https://chromium.googlesource.com/chromium/src/+/master/third_party/WebKit/Source/core/paint/ng/README.md).

## NGPaintFragment ##

LayoutNG produces a tree of [NGPhysicalFragment].

One of its goal is to share a sub-tree of NGPhysicalFragment across frames,
or even within a frame where possible.
This goal enforces a few characteristics:

* It must be immutable.
* It must be relative within the sub-tree, ideally only to its parent.

A [NGPaintFragment] owns (by `scoped_refptr`) a NGPhysicalFragment,
with additional characteristics:

* It can have mutable fields, such as `VisualRect()`.
* It can use its own coordinate system.
* Separate instances can be created when NGPhysicalFragment is shared.
* It can have its own tree structure, differently from NGPhysicalFragment tree.

### The tree structure ###

In short, one can think that the NGPaintFragment tree structure is
exactly the same as the NGPhysicalFragment tree structure.

In the early phase, LayoutNG copies the layout output to the LayoutObject tree
and uses the existing paint system, except for boxes with inline children.
Currently, NGPaintFragment is generated only for this case.

If `LayoutBlockFlow::PaintFragment()` exists,
it's a root of an NGPaintFragment tree.
It also means, in the current phase, it's a box that contains inline children.

Note that not all boxes with inline children can be laid out by LayoutNG today,
so the reverse is not true.

### Splitting the NGPaintFragment tree ###

When a NGPaintFragment tree is split,
one NGPhysicalFragment may appear twice;
once as a leaf of the parent tree, anotehr as the root of the child tree.
The leaf NGPaintFragment doesn't have children,
although its NGPhysicalFragment has children.
They share a NGPhysicalFragment, but are different NGPaintFragment instances.

Currently, each NGPaintFragment tree is one inline formatting context that
nested inline formatting context, such as inline blocks, floats,
or out of flow (e.g., absolute) positioned objects generate such structures.

Such leaves can also be determined by `NGPhysicalFragment::IsBlockLayoutRoot()`.

Note: this is still under experiments and may be reconsidered.

[LayoutNG]: ../../layout/ng/README.md
[NGPaintFragment]: ng_paint_fragment.h
[NGPhysicalFragment]: ../../layout/ng/ng_physical_fragment.h
