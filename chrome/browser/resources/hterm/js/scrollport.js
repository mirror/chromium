// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * A 'viewport' view of fixed-height rows with support for selection and
 * copy-to-clipboard.
 *
 * 'Viewport' in this case means that only the visible rows are in the DOM.
 * If the rowProvider has 100,000 rows, but the ScrollPort is only 25 rows
 * tall, then only 25 dom nodes are created.  The ScrollPort will ask the
 * RowProvider to create new visible rows on demand as they are scrolled in
 * to the visible area.
 *
 * This viewport is designed so that select and copy-to-clipboard still works,
 * even when all or part of the selection is scrolled off screen.
 *
 * Note that the X11 mouse clipboard does not work properly when all or part
 * of the selection is off screen.  It would be difficult to fix this without
 * adding significant overhead to pathologically large selection cases.
 *
 * The RowProvider should return rows rooted by the custom tag name 'x-row'.
 * This ensures that we can quickly assign the correct display height
 * to the rows with css.
 *
 * @param {RowProvider} rowProvider An object capable of providing rows as
 *     raw text or row nodes.
 * @param {integer} fontSize The css font-size, in pixels.
 * @param {integer} opt_lineHeight Optional css line-height in pixels.
 *    If omitted it will be computed based on the fontSize.
 */
hterm.ScrollPort = function(rowProvider, fontSize, opt_lineHeight) {
  PubSub.addBehavior(this);

  this.rowProvider_ = rowProvider;
  this.fontSize_ = fontSize;
  this.rowHeight_ = opt_lineHeight || fontSize + 2;

  this.selection_ = new hterm.ScrollPort.Selection(this);

  // A map of rowIndex => rowNode for each row that is drawn as part of a
  // pending redraw_() call.  Null if there is no pending redraw_ call.
  this.currentRowNodeCache_ = null;

  // A map of rowIndex => rowNode for each row that was drawn as part of the
  // previous redraw_() call.
  this.previousRowNodeCache_ = {};

  // The css rule that we use to control the height of a row.
  this.xrowCssRule_ = null;

  this.div_ = null;
  this.document_ = null;

  this.observers_ = {};

  this.DEBUG_ = false;
}

/**
 * Proxy for the native selection object which understands how to walk up the
 * DOM to find the containing row node and sort out which comes first.
 *
 * @param {hterm.ScrollPort} scrollPort The parent hterm.ScrollPort instance.
 */
hterm.ScrollPort.Selection = function(scrollPort) {
  this.scrollPort_ = scrollPort;
  this.selection_ = null;

  /**
   * The row containing the start of the selection.
   *
   * This may be partially or fully selected.  It may be the selection anchor
   * or the focus, but its rowIndex is guaranteed to be less-than-or-equal-to
   * that of the endRow.
   *
   * If only one row is selected then startRow == endRow.  If there is no
   * selection or the selection is collapsed then startRow == null.
   */
  this.startRow = null;

  /**
   * The row containing the end of the selection.
   *
   * This may be partially or fully selected.  It may be the selection anchor
   * or the focus, but its rowIndex is guaranteed to be greater-than-or-equal-to
   * that of the startRow.
   *
   * If only one row is selected then startRow == endRow.  If there is no
   * selection or the selection is collapsed then startRow == null.
   */
  this.endRow = null;

  /**
   * True if startRow != endRow.
   */
  this.isMultiline = null;

  /**
   * True if the selection is just a point rather than a range.
   */
  this.isCollapsed = null;
};

/**
 * Synchronize this object with the current DOM selection.
 *
 * This is a one-way synchronization, the DOM selection is copied to this
 * object, not the other way around.
 */
hterm.ScrollPort.Selection.prototype.sync = function() {
  var selection = this.scrollPort_.getDocument().getSelection();

  this.startRow = null;
  this.endRow = null;
  this.isMultiline = null;
  this.isCollapsed = !selection || selection.isCollapsed;

  if (this.isCollapsed)
    return;

  var anchorRow = selection.anchorNode;
  while (anchorRow && !('rowIndex' in anchorRow)) {
    anchorRow = anchorRow.parentNode;
  }

  if (!anchorRow) {
    console.log('Selection anchor is not rooted in a row node: ' +
                selection.anchorNode.nodeName);
    return;
  }

  var focusRow = selection.focusNode;
  while (focusRow && !('rowIndex' in focusRow)) {
    focusRow = focusRow.parentNode;
  }

  if (!focusRow) {
    console.log('Selection focus is not rooted in a row node: ' +
                selection.focusNode.nodeName);
    return;
  }

  if (anchorRow.rowIndex <= focusRow.rowIndex) {
    this.startRow = anchorRow;
    this.endRow = focusRow;
  } else {
    this.startRow = focusRow;
    this.endRow = anchorRow;
  }

  this.isMultiline = anchorRow.rowIndex != focusRow.rowIndex;
};

/**
 * Turn a div into this hterm.ScrollPort.
 */
hterm.ScrollPort.prototype.decorate = function(div) {
  this.div_ = div;

  this.iframe_ = div.ownerDocument.createElement('iframe');
  this.iframe_.style.cssText = (
      'border: 0;' +
      'height: 100%;' +
      'position: absolute;' +
      'width: 100%');

  div.appendChild(this.iframe_);

  this.iframe_.contentWindow.addEventListener('resize',
                                              this.onResize.bind(this));

  var doc = this.document_ = this.iframe_.contentDocument;
  doc.body.style.cssText = (
      'background-color: black;' +
      'color: white;' +
      'font-family: monospace;' +
      'margin: 0px;' +
      'padding: 0px;' +
      'white-space: pre;' +
      '-webkit-user-select: none;');

  var style = doc.createElement('style');
  style.textContent = 'x-row {}';
  doc.head.appendChild(style);

  this.xrowCssRule_ = doc.styleSheets[0].cssRules[0];
  this.xrowCssRule_.style.display = 'block';
  this.xrowCssRule_.style.height = this.rowHeight_ + 'px';

  this.screen_ = doc.createElement('x-screen');
  this.screen_.style.cssText = (
      'display: block;' +
      'height: 100%;' +
      'overflow-y: scroll; overflow-x: hidden;' +
      'width: 100%;');

  doc.body.appendChild(this.screen_);

  this.screen_.addEventListener('scroll', this.onScroll_.bind(this));
  this.screen_.addEventListener('mousewheel', this.onScrollWheel_.bind(this));
  this.screen_.addEventListener('copy', this.onCopy_.bind(this));

  // This is the main container for the fixed rows.
  this.rowNodes_ = doc.createElement('div');
  this.rowNodes_.style.cssText = (
      'display: block;' +
      'position: fixed;' +
      '-webkit-user-select: text;');
  this.screen_.appendChild(this.rowNodes_);

  // Two nodes to hold offscreen text during the copy event.
  this.topSelectBag_ = doc.createElement('x-select-bag');
  this.topSelectBag_.style.cssText = (
      'display: block;' +
      'overflow: hidden;' +
      'white-space: pre;');

  this.bottomSelectBag_ = this.topSelectBag_.cloneNode();

  // Nodes above the top fold and below the bottom fold are hidden.  They are
  // only used to hold rows that are part of the selection but are currently
  // scrolled off the top or bottom of the visible range.
  this.topFold_ = doc.createElement('x-fold');
  this.topFold_.style.cssText = 'display: block;';
  this.rowNodes_.appendChild(this.topFold_);

  this.bottomFold_ = this.topFold_.cloneNode();
  this.rowNodes_.appendChild(this.bottomFold_);

  // This hidden div accounts for the vertical space that would be consumed by
  // all the rows in the buffer if they were visible.  It's what causes the
  // scrollbar to appear on the 'x-screen', and it moves within the screen when
  // the scrollbar is moved.
  //
  // It is set 'visibility: hidden' to keep the browser from trying to include
  // it in the selection when a user 'drag selects' upwards (drag the mouse to
  // select and scroll at the same time).  Without this, the selection gets
  // out of whack.
  this.scrollArea_ = doc.createElement('div');
  this.scrollArea_.style.cssText = 'visibility: hidden';
  this.screen_.appendChild(this.scrollArea_);

  this.setRowMetrics(this.fontSize_, this.rowHeight_);
};

hterm.ScrollPort.prototype.getForegroundColor = function() {
  return this.document_.body.style.color;
};

hterm.ScrollPort.prototype.setForegroundColor = function(color) {
  this.document_.body.style.color = color;
};

hterm.ScrollPort.prototype.getBackgroundColor = function() {
  return this.document_.body.style.backgroundColor;
};

hterm.ScrollPort.prototype.setBackgroundColor = function(color) {
  this.document_.body.style.backgroundColor = color;
};

hterm.ScrollPort.prototype.getRowHeight = function() {
  return this.rowHeight_;
};

hterm.ScrollPort.prototype.getScreenWidth = function() {
  return this.screen_.clientWidth;
};

hterm.ScrollPort.prototype.getScreenHeight = function() {
  return this.screen_.clientHeight;
};

hterm.ScrollPort.prototype.getCharacterWidth = function() {
  var span = this.document_.createElement('span');
  span.textContent = '\xa0';  // &nbsp;
  this.rowNodes_.appendChild(span);
  var width = span.offsetWidth;
  this.rowNodes_.removeChild(span);
  return width;
};

/**
 * Return the document that holds the visible rows of this hterm.ScrollPort.
 */
hterm.ScrollPort.prototype.getDocument = function() {
  return this.document_;
};

/**
 * Clear out any cached rowNodes.
 */
hterm.ScrollPort.prototype.resetCache = function() {
  this.currentRowNodeCache_ = null;
  this.previousRowNodeCache_ = {};
};

/**
 * Change the current rowProvider.
 *
 * This will clear the row cache and cause a redraw.
 *
 * @param {Object} rowProvider An object capable of providing the rows
 *     in this hterm.ScrollPort.
 */
hterm.ScrollPort.prototype.setRowProvider = function(rowProvider) {
  this.resetCache();
  this.rowProvider_ = rowProvider;
  this.redraw_();
};

/**
 * Inform the ScrollPort that a given range of rows is invalid.
 *
 * The RowProvider should call this method if the underlying x-row instance
 * for a given rowIndex is no longer valid.
 *
 * Note that this is not necessary when only the *content* of the x-row has
 * changed.  It's only needed when getRowNode(N) would return a different
 * x-row than it used to.
 *
 * If rows in the sepecified range are visible, they will be redrawn.
 */
hterm.ScrollPort.prototype.invalidateRowRange = function(start, end) {
  this.resetCache();

  var node = this.rowNodes_.firstChild;
  while (node) {
    if ('rowIndex' in node &&
        node.rowIndex >= start && node.rowIndex <= end) {
      var nextSibling = node.nextSibling;
      this.rowNodes_.removeChild(node);

      var newNode = this.rowProvider_.getRowNode(node.rowIndex);

      this.rowNodes_.insertBefore(newNode, nextSibling);
      this.previousRowNodeCache_[node.rowIndex] = newNode;

      node = nextSibling;
    } else {
      node = node.nextSibling;
    }
  }
};

/**
 * Set the fontSize and lineHeight of this hterm.ScrollPort.
 *
 * @param {integer} fontSize The css font-size, in pixels.
 * @param {integer} opt_lineHeight Optional css line-height in pixels.
 *    If omitted it will be computed based on the fontSize.
 */
hterm.ScrollPort.prototype.setRowMetrics = function(fontSize, opt_lineHeight) {
  this.fontSize_ = fontSize;
  this.rowHeight_ = opt_lineHeight || fontSize + 2;

  this.screen_.style.fontSize = this.fontSize_ + 'px';
  this.screen_.style.lineHeight = this.rowHeight_ + 'px';
  this.xrowCssRule_.style.height = this.rowHeight_ + 'px';

  this.topSelectBag_.style.height = this.rowHeight_ + 'px';
  this.bottomSelectBag_.style.height = this.rowHeight_ + 'px';

  if (this.DEBUG_) {
    // When we're debugging we add padding to the body so that the offscreen
    // elements are visible.
    this.document_.body.style.paddingTop = 3 * this.rowHeight_ + 'px';
    this.document_.body.style.paddingBottom = 3 * this.rowHeight_ + 'px';
  }

  this.resize();
};

/**
 * Reset dimensions and visible row count to account for a change in the
 * dimensions of the 'x-screen'.
 */
hterm.ScrollPort.prototype.resize = function() {
  var screenWidth = this.screen_.clientWidth;
  var screenHeight = this.screen_.clientHeight;

  // We don't want to show a partial row because it would be distracting
  // in a terminal, so we floor any fractional row count.
  this.visibleRowCount = Math.floor(screenHeight / this.rowHeight_);

  // Then compute the height of our integral number of rows.
  var visibleRowsHeight = this.visibleRowCount * this.rowHeight_;

  // Then the difference between the screen height and total row height needs to
  // be made up for as top margin.  We need to record this value so it
  // can be used later to determine the topRowIndex.
  this.visibleRowTopMargin = screenHeight - visibleRowsHeight;
  this.topFold_.style.marginBottom = this.visibleRowTopMargin + 'px';

  // Set the dimensions of the visible rows container.
  this.rowNodes_.style.width = screenWidth + 'px';
  this.rowNodes_.style.height = visibleRowsHeight + 'px';
  this.rowNodes_.style.left = this.screen_.offsetLeft + 'px';

  var self = this;
  this.publish
    ('resize', { scrollPort: this },
     function() {
       var index = self.bottomFold_.previousSibling.rowIndex;
       self.scrollRowToBottom(index);
     });
};

hterm.ScrollPort.prototype.syncScrollHeight = function() {
  // Resize the scroll area to appear as though it contains every row.
  this.scrollArea_.style.height = (this.rowHeight_ *
                                   this.rowProvider_.getRowCount() +
                                   this.visibleRowTopMargin + 'px');
};

/**
 * Redraw the current hterm.ScrollPort based on the current scrollbar position.
 *
 * When redrawing, we are careful to make sure that the rows that start or end
 * the current selection are not touched in any way.  Doing so would disturb
 * the selection, and cleaning up after that would cause flashes at best and
 * incorrect selection at worst.  Instead, we modify the DOM around these nodes.
 * We even stash the selection start/end outside of the visible area if
 * they are not supposed to be visible in the hterm.ScrollPort.
 */
hterm.ScrollPort.prototype.redraw_ = function() {
  this.resetSelectBags_();
  this.selection_.sync();

  this.syncScrollHeight();

  this.currentRowNodeCache_ = {};

  var topRowIndex = this.getTopRowIndex();
  var bottomRowIndex = this.getBottomRowIndex(topRowIndex);

  this.drawTopFold_(topRowIndex);
  this.drawBottomFold_(bottomRowIndex);
  this.drawVisibleRows_(topRowIndex, bottomRowIndex);

  this.syncRowNodesTop_();

  this.previousRowNodeCache_ = this.currentRowNodeCache_;
  this.currentRowNodeCache_ = null;
};

/**
 * Ensure that the nodes above the top fold are as they should be.
 *
 * If the selection start and/or end nodes are above the visible range
 * of this hterm.ScrollPort then the dom will be adjusted so that they appear
 * before the top fold (the first x-fold element, aka this.topFold).
 *
 * If not, the top fold will be the first element.
 *
 * It is critical that this method does not move the selection nodes.  Doing
 * so would clear the current selection.  Instead, the rest of the DOM is
 * adjusted around them.
 */
hterm.ScrollPort.prototype.drawTopFold_ = function(topRowIndex) {
  if (!this.selection_.startRow ||
      this.selection_.startRow.rowIndex >= topRowIndex) {
    // Selection is entirely below the top fold, just make sure the fold is
    // the first child.
    if (this.rowNodes_.firstChild != this.topFold_)
      this.rowNodes_.insertBefore(this.topFold_, this.rowNodes_.firstChild);

    return;
  }

  if (!this.selection_.isMultiline ||
      this.selection_.endRow.rowIndex >= topRowIndex) {
    // Only the startRow is above the fold.
    if (this.selection_.startRow.nextSibling != this.topFold_)
      this.rowNodes_.insertBefore(this.topFold_,
                                  this.selection_.startRow.nextSibling);
  } else {
    // Both rows are above the fold.
    if (this.selection_.endRow.nextSibling != this.topFold_) {
      this.rowNodes_.insertBefore(this.topFold_,
                                  this.selection_.endRow.nextSibling);
    }

    // Trim any intermediate lines.
    while (this.selection_.startRow.nextSibling !=
           this.selection_.endRow) {
      this.rowNodes_.removeChild(this.selection_.startRow.nextSibling);
    }
  }

  while(this.rowNodes_.firstChild != this.selection_.startRow) {
    this.rowNodes_.removeChild(this.rowNodes_.firstChild);
  }
};

/**
 * Ensure that the nodes below the bottom fold are as they should be.
 *
 * If the selection start and/or end nodes are below the visible range
 * of this hterm.ScrollPort then the dom will be adjusted so that they appear
 * after the bottom fold (the second x-fold element, aka this.bottomFold).
 *
 * If not, the bottom fold will be the last element.
 *
 * It is critical that this method does not move the selection nodes.  Doing
 * so would clear the current selection.  Instead, the rest of the DOM is
 * adjusted around them.
 */
hterm.ScrollPort.prototype.drawBottomFold_ = function(bottomRowIndex) {
  if (!this.selection_.endRow ||
      this.selection_.endRow.rowIndex <= bottomRowIndex) {
    // Selection is entirely above the bottom fold, just make sure the fold is
    // the last child.
    if (this.rowNodes_.lastChild != this.bottomFold_)
      this.rowNodes_.appendChild(this.bottomFold_);

    return;
  }

  if (!this.selection_.isMultiline ||
      this.selection_.startRow.rowIndex <= bottomRowIndex) {
    // Only the endRow is below the fold.
    if (this.bottomFold_.nextSibling != this.selection_.endRow)
      this.rowNodes_.insertBefore(this.bottomFold_,
                                  this.selection_.endRow);
  } else {
    // Both rows are below the fold.
    if (this.bottomFold_.nextSibling != this.selection_.startRow) {
      this.rowNodes_.insertBefore(this.bottomFold_,
                                  this.selection_.startRow);
    }

    // Trim any intermediate lines.
    while (this.selection_.startRow.nextSibling !=
           this.selection_.endRow) {
      this.rowNodes_.removeChild(this.selection_.startRow.nextSibling);
    }
  }

  while(this.rowNodes_.lastChild != this.selection_.endRow) {
    this.rowNodes_.removeChild(this.rowNodes_.lastChild);
  }
};

/**
 * Ensure that the rows between the top and bottom folds are as they should be.
 *
 * This method assumes that drawTopFold_() and drawBottomFold_() have already
 * run, and that they have left any visible selection row (selection start
 * or selection end) between the folds.
 *
 * It recycles DOM nodes from the previous redraw where possible, but will ask
 * the rowSource to make new nodes if necessary.
 *
 * It is critical that this method does not move the selection nodes.  Doing
 * so would clear the current selection.  Instead, the rest of the DOM is
 * adjusted around them.
 */
hterm.ScrollPort.prototype.drawVisibleRows_ = function(
    topRowIndex, bottomRowIndex) {
  var self = this;

  // Keep removing nodes, starting with currentNode, until we encounter
  // targetNode.  Throws on failure.
  function removeUntilNode(currentNode, targetNode) {
    while (currentNode != targetNode) {
      if (!currentNode)
        throw 'Did not encounter target node';

      if (currentNode == self.bottomFold_)
        throw 'Encountered bottom fold before target node';

      var deadNode = currentNode;
      currentNode = currentNode.nextSibling;
      deadNode.parentNode.removeChild(deadNode);
    }
  }

  // Shorthand for things we're going to use a lot.
  var selectionStartRow = this.selection_.startRow;
  var selectionEndRow = this.selection_.endRow;
  var bottomFold = this.bottomFold_;

  // The node we're examining during the current iteration.
  var node = this.topFold_.nextSibling;

  var targetDrawCount = Math.min(this.visibleRowCount,
                                 this.rowProvider_.getRowCount());

  for (var drawCount = 0; drawCount < targetDrawCount; drawCount++) {
    var rowIndex = topRowIndex + drawCount;

    if (node == bottomFold) {
      // We've hit the bottom fold, we need to insert a new row.
      var newNode = this.fetchRowNode_(rowIndex);
      if (!newNode) {
        console.log("Couldn't fetch row index: " + rowIndex);
        break;
      }

      this.rowNodes_.insertBefore(newNode, node);
      continue;
    }

    if (node.rowIndex == rowIndex) {
      // This node is in the right place, move along.
      node = node.nextSibling;
      continue;
    }

    if (selectionStartRow && selectionStartRow.rowIndex == rowIndex) {
      // The selection start row is supposed to be here, remove nodes until
      // we find it.
      removeUntilNode(node, selectionStartRow);
      node = selectionStartRow.nextSibling;
      continue;
    }

    if (selectionEndRow && selectionEndRow.rowIndex == rowIndex) {
      // The selection end row is supposed to be here, remove nodes until
      // we find it.
      removeUntilNode(node, selectionEndRow);
      node = selectionEndRow.nextSibling;
      continue;
    }

    if (node == selectionStartRow || node == selectionEndRow) {
      // We encountered the start/end of the selection, but we don't want it
      // yet.  Insert a new row instead.
      var newNode = this.fetchRowNode_(rowIndex);
      if (!newNode) {
        console.log("Couldn't fetch row index: " + rowIndex);
        break;
      }

      this.rowNodes_.insertBefore(newNode, node);
      continue;
    }

    // There is nothing special about this node, but it's in our way.  Replace
    // it with the node that should be here.
    var newNode = this.fetchRowNode_(rowIndex);
    if (!newNode) {
      console.log("Couldn't fetch row index: " + rowIndex);
      break;
    }

    if (node == newNode) {
      node = node.nextSibling;
      continue;
    }

    this.rowNodes_.insertBefore(newNode, node);
    if (!newNode.nextSibling)
      debugger;
    this.rowNodes_.removeChild(node);
    node = newNode.nextSibling;
  }

  if (node != this.bottomFold_)
    removeUntilNode(node, bottomFold);
};

/**
 * Empty out both select bags and remove them from the document.
 *
 * These nodes hold the text between the start and end of the selection
 * when that text is otherwise off screen.  They are filled out in the
 * onCopy_ event.
 */
hterm.ScrollPort.prototype.resetSelectBags_ = function() {
  if (this.topSelectBag_.parentNode) {
    this.topSelectBag_.textContent = '';
    this.topSelectBag_.parentNode.removeChild(this.topSelectBag_);
  }

  if (this.bottomSelectBag_.parentNode) {
    this.bottomSelectBag_.textContent = '';
    this.bottomSelectBag_.parentNode.removeChild(this.bottomSelectBag_);
  }
};

/**
 * Set the top coordinate of the row nodes.
 *
 * The rowNodes_ are a fixed position DOM element.  When nodes are stashed
 * above the top fold, we need to adjust the top position of rowNodes_
 * so that the first node *after* the top fold is always the first visible
 * DOM node.
 */
hterm.ScrollPort.prototype.syncRowNodesTop_ = function() {
  var topMargin = 0;
  var node = this.topFold_.previousSibling;
  while (node) {
    topMargin += this.rowHeight_;
    node = node.previousSibling;
  }

  this.rowNodes_.style.top = this.screen_.offsetTop - topMargin + 'px';
};

/**
 * Place a row node in the cache of visible nodes.
 *
 * This method may only be used during a redraw_.
 */
hterm.ScrollPort.prototype.cacheRowNode_ = function(rowNode) {
  this.currentRowNodeCache_[rowNode.rowIndex] = rowNode;
};

/**
 * Fetch the row node for the given index.
 *
 * This will return a node from the cache if possible, or will request one
 * from the RowProvider if not.
 *
 * If a redraw_ is in progress the row will be added to the current cache.
 */
hterm.ScrollPort.prototype.fetchRowNode_ = function(rowIndex) {
  var node;

  if (this.previousRowNodeCache_ && rowIndex in this.previousRowNodeCache_) {
    node = this.previousRowNodeCache_[rowIndex];
  } else {
    node = this.rowProvider_.getRowNode(rowIndex);
  }

  if (this.currentRowNodeCache_)
    this.cacheRowNode_(node);

  return node;
};

/**
 * Select all rows in the viewport.
 */
hterm.ScrollPort.prototype.selectAll = function() {
  var firstRow;

  if (this.topFold_.nextSibling.rowIndex != 0) {
    while (this.topFold_.previousSibling) {
      this.rowNodes_.removeChild(this.topFold_.previousSibling);
    }

    firstRow = this.fetchRowNode_(0);
    this.rowNodes_.insertBefore(firstRow, this.topFold_);
    this.syncRowNodesTop_();
  } else {
    firstRow = this.topFold_.nextSibling;
  }

  var lastRowIndex = this.rowProvider_.getRowCount() - 1;
  var lastRow;

  if (this.bottomFold_.previousSibling.rowIndex != lastRowIndex) {
    while (this.bottomFold_.nextSibling) {
      this.rowNodes_.removeChild(this.bottomFold_.nextSibling);
    }

    lastRow = this.fetchRowNode_(lastRowIndex);
    this.rowNodes_.appendChild(lastRow);
  } else {
    lastRow = this.bottomFold_.previousSibling.rowIndex;
  }

  var selection = this.document_.getSelection();
  selection.collapse(firstRow, 0);
  selection.extend(lastRow, lastRow.childNodes.length);

  this.selection_.sync();
};

/**
 * Return the maximum scroll position in pixels.
 */
hterm.ScrollPort.prototype.getScrollMax_ = function(e) {
  return (this.scrollArea_.clientHeight + this.visibleRowTopMargin -
          this.screen_.clientHeight);
};

/**
 * Scroll the given rowIndex to the top of the hterm.ScrollPort.
 *
 * @param {integer} rowIndex Index of the target row.
 */
hterm.ScrollPort.prototype.scrollRowToTop = function(rowIndex) {
  this.syncScrollHeight();

  var scrollTop = rowIndex * this.rowHeight_ + this.visibleRowTopMargin;

  var scrollMax = this.getScrollMax_();
  if (scrollTop > scrollMax)
    scrollTop = scrollMax;

  this.screen_.scrollTop = scrollTop;
  this.redraw_();
};

/**
 * Scroll the given rowIndex to the bottom of the hterm.ScrollPort.
 *
 * @param {integer} rowIndex Index of the target row.
 */
hterm.ScrollPort.prototype.scrollRowToBottom = function(rowIndex) {
  this.syncScrollHeight();

  var scrollTop = rowIndex * this.rowHeight_ + this.visibleRowTopMargin;
  scrollTop -= (this.visibleRowCount - 1) * this.rowHeight_;

  if (scrollTop < 0)
    scrollTop = 0;

  this.screen_.scrollTop = scrollTop;
  this.redraw_();
};

/**
 * Return the row index of the first visible row.
 *
 * This is based on the scroll position.  If a redraw_ is in progress this
 * returns the row that *should* be at the top.
 */
hterm.ScrollPort.prototype.getTopRowIndex = function() {
  return Math.floor(this.screen_.scrollTop / this.rowHeight_);
};

/**
 * Return the row index of the last visible row.
 *
 * This is based on the scroll position.  If a redraw_ is in progress this
 * returns the row that *should* be at the bottom.
 */
hterm.ScrollPort.prototype.getBottomRowIndex = function(topRowIndex) {
  return topRowIndex + this.visibleRowCount - 1;
};

/**
 * Handler for scroll events.
 *
 * The onScroll event fires when scrollArea's scrollTop property changes.  This
 * may be due to the user manually move the scrollbar, or a programmatic change.
 */
hterm.ScrollPort.prototype.onScroll_ = function(e) {
  this.redraw_();
  this.publish('scroll', { scrollPort: this });
};

/**
 * Handler for scroll-wheel events.
 *
 * The onScrollWheel event fires when the user moves their scrollwheel over this
 * hterm.ScrollPort.  Because the frontmost element in the hterm.ScrollPort is
 * a fixed position DIV, the scroll wheel does nothing by default.  Instead, we
 * have to handle it manually.
 */
hterm.ScrollPort.prototype.onScrollWheel_ = function(e) {
  var top = this.screen_.scrollTop - e.wheelDeltaY;
  if (top < 0)
    top = 0;

  var scrollMax = this.getScrollMax_();
  if (top > scrollMax)
    top = scrollMax;

  // Moving scrollTop causes a scroll event, which triggers the redraw.
  this.screen_.scrollTop = top;
};

/**
 * Handler for resize events.
 *
 * The browser will resize us such that the top row stays at the top, but we
 * prefer to the bottom row to stay at the bottom.
 */
hterm.ScrollPort.prototype.onResize = function(e) {
  this.resize();
};

/**
 * Handler for copy-to-clipboard events.
 *
 * If some or all of the selected rows are off screen we may need to fill in
 * the rows between selection start and selection end.  This handler determines
 * if we're missing some of the selected text, and if so populates one or both
 * of the "select bags" with the missing text.
 */
hterm.ScrollPort.prototype.onCopy_ = function(e) {
  this.resetSelectBags_();
  this.selection_.sync();

  if (!this.selection_.startRow ||
      this.selection_.endRow.rowIndex - this.selection_.startRow.rowIndex < 2) {
    return;
  }

  var topRowIndex = this.getTopRowIndex();
  var bottomRowIndex = this.getBottomRowIndex(topRowIndex);

  if (this.selection_.startRow.rowIndex < topRowIndex) {
    // Start of selection is above the top fold.
    var endBackfillIndex;

    if (this.selection_.endRow.rowIndex < topRowIndex) {
      // Entire selection is above the top fold.
      endBackfillIndex = this.selection_.endRow.rowIndex;
    } else {
      // Selection extends below the top fold.
      endBackfillIndex = this.topFold_.nextSibling.rowIndex;
    }

    this.topSelectBag_.textContent = this.rowProvider_.getRowsText(
        this.selection_.startRow.rowIndex + 1, endBackfillIndex);
    this.rowNodes_.insertBefore(this.topSelectBag_,
                                this.selection_.startRow.nextSibling);
    this.syncRowNodesTop_();
  }

  if (this.selection_.endRow.rowIndex > bottomRowIndex) {
    // Selection ends below the bottom fold.
    var startBackfillIndex;

    if (this.selection_.startRow.rowIndex > bottomRowIndex) {
      // Entire selection is below the bottom fold.
      startBackfillIndex = this.selection_.startRow.rowIndex + 1;
    } else {
      // Selection starts above the bottom fold.
      startBackfillIndex = this.bottomFold_.previousSibling.rowIndex + 1;
    }

    this.bottomSelectBag_.textContent = this.rowProvider_.getRowsText(
        startBackfillIndex, this.selection_.endRow.rowIndex);
    this.rowNodes_.insertBefore(this.bottomSelectBag_, this.selection_.endRow);
  }
};
