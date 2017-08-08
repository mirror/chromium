// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Namespace for utility functions.
 */
var filelist = {};

/**
 * File table list.
 * @constructor
 * @struct
 * @extends {cr.ui.table.TableList}
 */
function FileTableList() {
  throw new Error('Designed to decorate elements');
}

/**
 * Decorates TableList as FileTableList.
 * @param {!cr.ui.table.TableList} self A tabel list element.
 */
FileTableList.decorate = function(self) {
  self.__proto__ = FileTableList.prototype;
}

FileTableList.prototype.__proto__ = cr.ui.table.TableList.prototype;

/**
 * @type {?function(number, number)}
 */
FileTableList.prototype.onMergeItems_ = null;

/**
 * @param {function(number, number)} onMergeItems callback called from
 *     |mergeItems| with the parameters |beginIndex| and |endIndex|.
 */
FileTableList.prototype.setOnMergeItems = function(onMergeItems) {
  assert(!this.onMergeItems_);
  this.onMergeItems_ = onMergeItems;
};

/** @override */
FileTableList.prototype.mergeItems = function(beginIndex, endIndex) {
  cr.ui.table.TableList.prototype.mergeItems.call(this, beginIndex, endIndex);

  // Make sure that list item's selected attribute is updated just after the
  // mergeItems operation is done. This prevents checkmarks on selected items
  // from being animated unintentionally by redraw.
  for (var i = beginIndex; i < endIndex; i++) {
    var item = this.getListItemByIndex(i);
    if (!item)
      continue;
    var isSelected = this.selectionModel.getIndexSelected(i);
    if (item.selected != isSelected)
      item.selected = isSelected;
  }

  if (this.onMergeItems_) {
    this.onMergeItems_(beginIndex, endIndex);
  }
};

/** @override */
FileTableList.prototype.createSelectionController = function(sm) {
  return new FileListSelectionController(assert(sm));
};

/**
 * Processes touch events and calls back upon tap, longpress and longtap.
 * @constructor
 */
function FileTapHandler() {
  /**
   * Whether the pointer is currently down and at the same place as the initial
   * position.
   * @type {boolean}
   * @private
   */
  this.tapStarted_ = false;

  /**
   * @type {boolean}
   */
  this.isLongTap_ = false;

  /**
   * @type {boolean}
   */
  this.hasLongPressProcessed_ = false;

  /**
   * @type {?number}
   */
  this.longTapDetectorTimeout_ = null;
}

/**
 * @type {number}
 * @const
 */
FileTapHandler.LONG_PRESS_THRESHOLD_MILLISECONDS = 500;

/**
 * @enum {string}
 * @const
 */
FileTapHandler.TapEvent = {
  TAP: 'tap',
  LONG_PRESS: 'longpress',
  LONG_TAP: 'longtap'
};

/**
 * Handles touch events.
 * The propagation of the |event| will be cancelled if the |callback| takes any
 * action, so as to avoid receiving mouse click events for the tapping and
 * processing them duplicatedly.
 * @param {!Event} event a touch event.
 * @param {number} index of the target item in the file list.
 * @param {function(!Event, number, !FileTapHandler.TapEvent)} callback called
 *     when a tap event is detected. Should return ture if it has taken any
 *     action, and false if it ignroes the event.
 */
FileTapHandler.prototype.handleTouchEvents = function(event, index, callback) {
  switch (event.type) {
    case 'touchstart':
      this.tapStarted_ = true;
      this.isLongTap_ = false;
      this.hasLongPressProcessed_ = false;
      this.longTapDetectorTimeout_ = setTimeout(function() {
        this.isLongTap_ = true;
        this.longTapDetectorTimeout_ = null;
        if (callback(event, index, FileTapHandler.TapEvent.LONG_PRESS)) {
          this.hasLongPressProcessed_ = true;
          // Prevent opening context menu.
          event.preventDefault();
        }
      }.bind(this), FileTapHandler.LONG_PRESS_THRESHOLD_MILLISECONDS);
      break;
    case 'touchmove':
      // If the pointer is slided, it is a drag. It is no longer a tap.
      this.tapStarted_ = false;
      break;
    case 'touchend':
      if (this.longTapDetectorTimeout_ != null) {
        clearTimeout(this.longTapDetectorTimeout_);
      }
      if (!this.tapStarted_)
        break;
      if (this.isLongTap_) {
        if (this.hasLongPressProcessed_ ||
            callback(event, index, FileTapHandler.TapEvent.LONG_TAP)) {
          event.preventDefault();
        }
      } else {
        if (callback(event, index, FileTapHandler.TapEvent.TAP)) {
          event.preventDefault();
        }
      }
      break;
  }
};

/**
 * Selection controller for the file table list.
 * @param {!cr.ui.ListSelectionModel} selectionModel The selection model to
 *     interact with.
 * @constructor
 * @extends {cr.ui.ListSelectionController}
 * @struct
 */
function FileListSelectionController(selectionModel) {
  cr.ui.ListSelectionController.call(this, selectionModel);

  /**
   * Whether to allow touch-specific interaction.
   * @type {boolean}
   */
  this.enableTouchMode_ = false;
  util.isTouchModeEnabled(function(enabled) {
    this.enableTouchMode_ = enabled;
  }.bind(this));

  /**
   * @type {!FileTapHandler}
   */
  this.tapHandler_ = new FileTapHandler();
}

FileListSelectionController.prototype = /** @struct */ {
  __proto__: cr.ui.ListSelectionController.prototype
};

/** @override */
FileListSelectionController.prototype.handlePointerDownUp = function(e, index) {
  filelist.handlePointerDownUp.call(this, e, index);
};

/** @override */
FileListSelectionController.prototype.handleTouchEvents = function(e, index) {
  if (!this.enableTouchMode_)
    return;
  this.tapHandler_.handleTouchEvents(e, index, filelist.handleTap.bind(this));
};

/** @override */
FileListSelectionController.prototype.handleKeyDown = function(e) {
  filelist.handleKeyDown.call(this, e);
};

/**
 * Common item decoration for table's and grid's items.
 * @param {cr.ui.ListItem} li List item.
 * @param {Entry} entry The entry.
 * @param {!MetadataModel} metadataModel Cache to
 *     retrieve metadada.
 */
filelist.decorateListItem = function(li, entry, metadataModel) {
  li.classList.add(entry.isDirectory ? 'directory' : 'file');
  // The metadata may not yet be ready. In that case, the list item will be
  // updated when the metadata is ready via updateListItemsMetadata. For files
  // not on an external backend, externalProps is not available.
  var externalProps = metadataModel.getCache(
      [entry], ['hosted', 'availableOffline', 'customIconUrl', 'shared'])[0];
  filelist.updateListItemExternalProps(li, externalProps);

  // Overriding the default role 'list' to 'listbox' for better
  // accessibility on ChromeOS.
  li.setAttribute('role', 'option');

  Object.defineProperty(li, 'selected', {
    /**
     * @this {cr.ui.ListItem}
     * @return {boolean} True if the list item is selected.
     */
    get: function() {
      return this.hasAttribute('selected');
    },

    /**
     * @this {cr.ui.ListItem}
     */
    set: function(v) {
      if (v)
        this.setAttribute('selected', '');
      else
        this.removeAttribute('selected');
    }
  });
};

/**
 * Render the type column of the detail table.
 * @param {!Document} doc Owner document.
 * @param {!Entry} entry The Entry object to render.
 * @param {string=} opt_mimeType Optional mime type for the file.
 * @return {!HTMLDivElement} Created element.
 */
filelist.renderFileTypeIcon = function(doc, entry, opt_mimeType) {
  var icon = /** @type {!HTMLDivElement} */ (doc.createElement('div'));
  icon.className = 'detail-icon';
  icon.setAttribute('file-type-icon', FileType.getIcon(entry, opt_mimeType));
  return icon;
};

/**
 * Render filename label for grid and list view.
 * @param {!Document} doc Owner document.
 * @param {!Entry} entry The Entry object to render.
 * @return {!HTMLDivElement} The label.
 */
filelist.renderFileNameLabel = function(doc, entry) {
  // Filename need to be in a '.filename-label' container for correct
  // work of inplace renaming.
  var box = /** @type {!HTMLDivElement} */ (doc.createElement('div'));
  box.className = 'filename-label';
  var fileName = doc.createElement('span');
  fileName.className = 'entry-name';
  fileName.textContent = entry.name;
  box.appendChild(fileName);

  return box;
};

/**
 * Updates grid item or table row for the externalProps.
 * @param {cr.ui.ListItem} li List item.
 * @param {Object} externalProps Metadata.
 */
filelist.updateListItemExternalProps = function(li, externalProps) {
  if (li.classList.contains('file')) {
    if (externalProps.availableOffline)
      li.classList.remove('dim-offline');
    else
      li.classList.add('dim-offline');
    // TODO(mtomasz): Consider adding some vidual indication for files which
    // are not cached on LTE. Currently we show them as normal files.
    // crbug.com/246611.
  }

  var iconDiv = li.querySelector('.detail-icon');
  if (!iconDiv)
    return;

  if (externalProps.customIconUrl)
    iconDiv.style.backgroundImage = 'url(' + externalProps.customIconUrl + ')';
  else
    iconDiv.style.backgroundImage = '';  // Back to the default image.

  if (li.classList.contains('directory'))
    iconDiv.classList.toggle('shared', !!externalProps.shared);
};

/**
 * Handles touchend/touchstart events on file list to change the selection
 * state.
 *
 * @param {!Event} e The browser mouse event.
 * @param {number} index The index that was under the mouse pointer, -1 if
 *     none.
 * @param {string} event_type
 * @return True if conducted any action. False when if did nothing special for
 *     tap.
 * @this {cr.ui.ListSelectionController}
 */
filelist.handleTap = function(e, index, event_type) {
  if (index == -1) {
    return false;
  }
  var sm = /** @type {!FileListSelectionModel|!FileListSingleSelectionModel} */
      (this.selectionModel);
  var isTap = event_type == FileTapHandler.TapEvent.TAP ||
      event_type == FileTapHandler.TapEvent.LONG_TAP;
  if (sm.multiple && sm.getCheckSelectMode() && isTap && !e.shiftKey) {
    // toggle item selection. Equivalent to mouse click on checkbox.
    // If a selected item is clicked when the selection mode
    // is not check-select, we should avoid toggling(unselectrg) the
    // item. It is done here by toggling the selection twice.
    sm.beginChange();
    sm.setIndexSelected(index, !sm.getIndexSelected(index));
    // Toggle the current one and make it anchor index.
    sm.leadIndex = index;
    sm.anchorIndex = index;
    sm.endChange();
    return true;
  } else if (sm.multiple) {
    if (event_type == 'longpress') {
      if (!sm.getIndexSelected(index)) {
        sm.beginChange();
        sm.setIndexSelected(index, true);
        sm.setCheckSelectMode(true);
        sm.endChange();
        return true;
      } else {
        // Do nothing, so as to avoid unselecting before drag.
      }
    } else if (event_type == 'longtap') {
      sm.beginChange();
      sm.setIndexSelected(index, false);
      sm.endChange();
      return true;
    }
  }
  return false;
};

/**
 * Unselects all items.
 * @this {cr.ui.ListSelectionController}
 */
filelist.unselectAll = function() {
  var sm = /** @type {!FileListSelectionModel|!FileListSingleSelectionModel} */
      (this.selectionModel);
  sm.beginChange();
  sm.leadIndex = sm.anchorIndex = -1;
  sm.unselectAll();
  sm.endChange();
};

/**
 * Selects range.
 * @param {number} index which was clicked
 * @this {cr.ui.ListSelectionController}
 */
filelist.rangeSelect = function(index) {};

/**
 * Handles mouseup/mousedown events on file list to change the selection state.
 *
 * Basically the content of this function is identical to
 * cr.ui.ListSelectionController's handlePointerDownUp(), but following
 * handlings are inserted to control the check-select mode.
 *
 * 1) When checkmark area is clicked, toggle item selection and enable the
 *    check-select mode.
 * 2) When non-checkmark area is clicked in check-select mode, disable the
 *    check-select mode.
 *
 * @param {!Event} e The browser mouse event.
 * @param {number} index The index that was under the mouse pointer, -1 if
 *     none.
 * @return True if conducted any action.
 * @this {cr.ui.ListSelectionController}
 */
filelist.handlePointerDownUp = function(e, index) {
  var actionTaken = false;
  var sm = /** @type {!FileListSelectionModel|!FileListSingleSelectionModel} */
           (this.selectionModel);
  var anchorIndex = sm.anchorIndex;
  var isDown = (e.type == 'mousedown');
  var isTap = false;
  var isLongPress = false;
  var isLongTap = false;

  var isTargetCheckmark = e.target.classList.contains('detail-checkmark') ||
                          e.target.classList.contains('checkmark');
  // If multiple selection is allowed and the checkmark is clicked without
  // modifiers(Ctrl/Shift), the click should toggle the item's selection.
  // (i.e. same behavior as Ctrl+Click)
  var isClickOnCheckmark = isTargetCheckmark && e.button == 0 && sm.multiple &&
      index != -1 && !e.shiftKey && !e.ctrlKey;

  var isTapToSelect = isTap && sm.getCheckSelectMode() ||
      isLongPress && !sm.getIndexSelected(index);
  if (isLongPress) {
    // Long press on a selected item may be the drag.
    // Do nothing until tuoch end.
    if (sm.getIndexSelected(index)) {
      return false;
    }
  }

  sm.beginChange();

  if (index == -1) {
    sm.leadIndex = sm.anchorIndex = -1;
    actionTaken = true;
    sm.unselectAll();
  } else {
    if (sm.multiple && (e.ctrlKey || isClickOnCheckmark || isTapToSelect) &&
        !e.shiftKey) {
      // Selection is handled at mouseUp.
      if (!isDown) {
        // 1) When checkmark area is clicked or the file row is tapped,
        //    toggle item selection and enable the check-select mode.
        if (isClickOnCheckmark) {
          // If a selected item's checkmark is clicked when the selection mode
          // is not check-select, we should avoid toggling(unselecting) the
          // item. It is done here by toggling the selection twice.
          if (!sm.getCheckSelectMode() && sm.getIndexSelected(index))
            sm.setIndexSelected(index, !sm.getIndexSelected(index));
          // Always enables check-select mode on clicks on checkmark.
          sm.setCheckSelectMode(true);
        }
        // Toggle the current one and make it anchor index.
        sm.setIndexSelected(index, !sm.getIndexSelected(index));
        sm.leadIndex = index;
        sm.anchorIndex = index;
        actionTaken = true;
      }
    } else if (
        sm.multiple && e.shiftKey && anchorIndex != -1 &&
        anchorIndex != index) {
      // When an item is either clicked or tapped while pressing shift key,
      // the range between the anchor item and the currently clicked one should
      // be selected. Shift is done in mousedown.
      if (isDown) {
        sm.unselectAll();
        sm.leadIndex = index;
        sm.selectRange(anchorIndex, index);
        actionTaken = true;
      }
    } else {
      // Right click for a context menu needs to not clear the selection.
      var isRightClick = e.button == 2;

      // If the index is selected this is handled in mouseup.
      var indexSelected = sm.getIndexSelected(index);
      if ((indexSelected && !isDown || !indexSelected && isDown) &&
          !(indexSelected && isRightClick)) {
        // 2) When non-checkmark area is clicked in check-select mode, disable
        //    the check-select mode.
        if (sm.getCheckSelectMode()) {
          // Unselect all items once to ensure that the check-select mode is
          // terminated.
          sm.endChange();
          sm.unselectAll();
          sm.beginChange();
        }
        actionTaken = true;
        sm.selectedIndex = index;
      }
    }
  }
  sm.endChange();
  if (actionTaken && (isTap || isLongPress || isLongTap)) {
    // If tap event is handled, should prevent default not to issue click event.
    e.preventDefault();
  }
  return actionTaken;
};

/**
 * Handles key events on file list to change the selection state.
 *
 * Basically the content of this function is identical to
 * cr.ui.ListSelectionController's handleKeyDown(), but following handlings is
 * inserted to control the check-select mode.
 *
 * 1) When pressing direction key results in a single selection, the
 *    check-select mode should be terminated.
 *
 * @param {Event} e The keydown event.
 * @this {cr.ui.ListSelectionController}
 */
filelist.handleKeyDown = function(e) {
  var SPACE_KEY_CODE = 32;
  var tagName = e.target.tagName;

  // If focus is in an input field of some kind, only handle navigation keys
  // that aren't likely to conflict with input interaction (e.g., text
  // editing, or changing the value of a checkbox or select).
  if (tagName == 'INPUT') {
    var inputType = e.target.type;
    // Just protect space (for toggling) for checkbox and radio.
    if (inputType == 'checkbox' || inputType == 'radio') {
      if (e.keyCode == SPACE_KEY_CODE)
        return;
    // Protect all but the most basic navigation commands in anything else.
    } else if (e.key != 'ArrowUp' && e.key != 'ArrowDown') {
      return;
    }
  }
  // Similarly, don't interfere with select element handling.
  if (tagName == 'SELECT')
    return;

  var sm = /** @type {!FileListSelectionModel|!FileListSingleSelectionModel} */
           (this.selectionModel);
  var newIndex = -1;
  var leadIndex = sm.leadIndex;
  var prevent = true;

  // Ctrl/Meta+A
  if (sm.multiple && e.keyCode == 65 &&
      (cr.isMac && e.metaKey || !cr.isMac && e.ctrlKey)) {
    sm.selectAll();
    e.preventDefault();
    return;
  }

  // Esc
  if (e.keyCode === 27 && !e.ctrlKey && !e.shiftKey) {
    sm.unselectAll();
    e.preventDefault();
    return;
  }

  // Space
  if (e.keyCode == SPACE_KEY_CODE) {
    if (leadIndex != -1) {
      var selected = sm.getIndexSelected(leadIndex);
      if (e.ctrlKey || !selected) {
        sm.setIndexSelected(leadIndex, !selected || !sm.multiple);
        return;
      }
    }
  }

  switch (e.key) {
    case 'Home':
      newIndex = this.getFirstIndex();
      break;
    case 'End':
      newIndex = this.getLastIndex();
      break;
    case 'ArrowUp':
      newIndex = leadIndex == -1 ?
          this.getLastIndex() : this.getIndexAbove(leadIndex);
      break;
    case 'ArrowDown':
      newIndex = leadIndex == -1 ?
          this.getFirstIndex() : this.getIndexBelow(leadIndex);
      break;
    case 'ArrowLeft':
    case 'MediaTrackPrevious':
      newIndex = leadIndex == -1 ?
          this.getLastIndex() : this.getIndexBefore(leadIndex);
      break;
    case 'ArrowRight':
    case 'MediaTrackNext':
      newIndex = leadIndex == -1 ?
          this.getFirstIndex() : this.getIndexAfter(leadIndex);
      break;
    default:
      prevent = false;
  }

  if (newIndex >= 0 && newIndex < sm.length) {
    sm.beginChange();

    sm.leadIndex = newIndex;
    if (e.shiftKey) {
      var anchorIndex = sm.anchorIndex;
      if (sm.multiple)
        sm.unselectAll();
      if (anchorIndex == -1) {
        sm.setIndexSelected(newIndex, true);
        sm.anchorIndex = newIndex;
      } else {
        sm.selectRange(anchorIndex, newIndex);
      }
    } else {
      // 1) When pressing direction key results in a single selection, the
      //    check-select mode should be terminated.
      sm.setCheckSelectMode(false);

      if (sm.multiple)
        sm.unselectAll();
      sm.setIndexSelected(newIndex, true);
      sm.anchorIndex = newIndex;
    }

    sm.endChange();

    if (prevent)
      e.preventDefault();
  }
};
