// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var g_slideshow_data = null;

/**
 * FileManager constructor.
 *
 * FileManager objects encapsulate the functionality of the file selector
 * dialogs, as well as the full screen file manager application (though the
 * latter is not yet implemented).
 *
 * @param {HTMLElement} dialogDom The DOM node containing the prototypical
 *     dialog UI.
 * @param {DOMFileSystem} filesystem The HTML5 filesystem object representing
 *     the root filesystem for the new FileManager.
 * @param {Object} params A map of parameter names to values controlling the
 *     appearance of the FileManager.  Names are:
 *     - type: A value from FileManager.DialogType defining what kind of
 *       dialog to present.  Defaults to FULL_PAGE.
 *     - title: The title for the dialog.  Defaults to a localized string based
 *       on the dialog type.
 *     - defaultPath: The default path for the dialog.  The default path should
 *       end with a trailing slash if it represents a directory.
 */
function FileManager(dialogDom, rootEntries, params) {
  console.log('Init FileManager: ' + dialogDom);

  this.dialogDom_ = dialogDom;
  this.rootEntries_ = rootEntries;
  this.filesystem_ = rootEntries[0].filesystem;
  this.params_ = params || {};

  this.listType_ = null;

  this.metadataCache_ = {};

  // True if we should filter out files that start with a dot.
  this.filterFiles_ = true;

  this.commands_ = {};

  this.document_ = dialogDom.ownerDocument;
  this.dialogType_ = this.params_.type || FileManager.DialogType.FULL_PAGE;

  this.defaultPath_ = this.params_.defaultPath || '/';

  // TODO(dgozman): This will be changed to LocaleInfo.
  this.locale_ = new v8Locale(navigator.language);

  // TODO(rginda): 6/22/11: Remove this test when createDateTimeFormat is
  // available in all chrome trunk builds.
  if ('createDateTimeFormat' in this.locale_) {
    this.shortDateFormatter_ =
      this.locale_.createDateTimeFormat({'dateType': 'medium'});
  } else {
    this.shortDateFormatter_ = {
      format: function(d) {
        return (d.getMonth() + 1) + '/' + d.getDate() + '/' + d.getFullYear();
      }
    };
  }

  // TODO(rginda): 6/22/11: Remove this test when createCollator is
  // available in all chrome trunk builds.
  if ('createCollator' in this.locale_) {
    this.collator_ = this.locale_.createCollator({
      'numeric': true, 'ignoreCase': true, 'ignoreAccents': true});
  } else {
    this.collator_ = {
      compare: function(a, b) {
        if (a > b) return 1;
        if (a < b) return -1;
        return 0;
      }
    };
  }

  // Optional list of file types.
  this.fileTypes_ = this.params_.typeList;

  var ary = this.defaultPath_.match(/^\/home\/[^\/]+\/user\/Downloads(\/.*)?$/);
  if (ary) {
    // Chrome will probably suggest the full path to Downloads, but
    // we're working with 'virtual paths', so we have to translate.
    this.defaultPath_ = '/Downloads' + (ary[1] || '');
  }

  this.showCheckboxes_ =
      (this.dialogType_ == FileManager.DialogType.FULL_PAGE ||
       this.dialogType_ == FileManager.DialogType.SELECT_OPEN_MULTI_FILE);

  // DirectoryEntry representing the current directory of the dialog.
  this.currentDirEntry_ = null;

  window.addEventListener('popstate', this.onPopState_.bind(this));
  this.addEventListener('directory-changed',
                        this.onDirectoryChanged_.bind(this));
  this.addEventListener('selection-summarized',
                        this.onSelectionSummarized_.bind(this));

  this.initCommands_();
  this.initDom_();
  this.initDialogType_();
  this.initDefaultDirectory_();

  this.summarizeSelection_();
  this.updatePreview_();

  chrome.fileBrowserPrivate.onDiskChanged.addListener(
      this.onDiskChanged_.bind(this));

  this.document_.querySelector('[tabindex="0"]').focus();

  // Pass all URLs to the metadata reader until we have a correct filter.
  this.metadataUrlFilter_ = /.*/;

  this.metadataReader_ = new Worker('js/metadata_dispatcher.js');
  this.metadataReader_.onmessage = this.onMetadataMessage_.bind(this);
  this.metadataReader_.postMessage({verb: 'init'});
}

FileManager.prototype = {
  __proto__: cr.EventTarget.prototype
};

// Anonymous "namespace".
(function() {

  // Private variables and helper functions.

  /**
   * Unicode codepoint for 'BLACK RIGHT-POINTING SMALL TRIANGLE'.
   */
  const RIGHT_TRIANGLE = '\u25b8';

  /**
   * The DirectoryEntry.fullPath value of the directory containing external
   * storage volumes.
   */
  const MEDIA_DIRECTORY = '/media';

  /**
   * Mnemonics for the second parameter of the changeDirectory method.
   */
  const CD_WITH_HISTORY = true;
  const CD_NO_HISTORY = false;

  /**
   * Translated strings.
   */
  var localStrings;

  /**
   * Map of icon types to regular expressions.
   *
   * The first regexp to match the file name determines the icon type
   * assigned to dom elements for a file.  Order of evaluation is not
   * defined, so don't depend on it.
   */
  const iconTypes = {
    'audio': /\.(mp3|m4a|oga|ogg|wav)$/i,
    'html': /\.(html?)$/i,
    'image': /\.(bmp|gif|jpe?g|ico|png|webp)$/i,
    'pdf' : /\.(pdf)$/i,
    'text': /\.(pod|rst|txt|log)$/i,
    'video': /\.(avi|mov|mp4|m4v|mpe?g4?|ogm|ogv|ogx|webm)$/i
  };

  const previewArt = {
    'audio': 'images/filetype_large_audio.png',
    'folder': 'images/filetype_large_folder.png',
    'unknown': 'images/filetype_large_generic.png',
    'video': 'images/filetype_large_video.png'
  };

  /**
   * Return a translated string.
   *
   * Wrapper function to make dealing with translated strings more concise.
   * Equivilant to localStrings.getString(id).
   *
   * @param {string} id The id of the string to return.
   * @return {string} The translated string.
   */
  function str(id) {
    return localStrings.getString(id);
  }

  /**
   * Return a translated string with arguments replaced.
   *
   * Wrapper function to make dealing with translated strings more concise.
   * Equivilant to localStrings.getStringF(id, ...).
   *
   * @param {string} id The id of the string to return.
   * @param {...string} The values to replace into the string.
   * @return {string} The translated string with replaced values.
   */
  function strf(id, var_args) {
    return localStrings.getStringF.apply(localStrings, arguments);
  }

  /**
   * Checks if |parent_path| is parent file path of |child_path|.
   *
   * @param {string} parent_path The parent path.
   * @param {string} child_path The child path.
   */
  function isParentPath(parent_path, child_path) {
    if (!parent_path || parent_path.length == 0 ||
        !child_path || child_path.length == 0)
      return false;

    if (parent_path[parent_path.length -1] != '/')
      parent_path += '/';

    if (child_path[child_path.length -1] != '/')
      child_path += '/';

    return child_path.indexOf(parent_path) == 0;
  }

  /**
   * Returns parent folder path of file path.
   *
   * @param {string} path The file path.
   */
  function getParentPath(path) {
    var parent = path.replace(/[\/]?[^\/]+[\/]?$/,'');
    if (parent.length == 0)
      parent = '/';
    return parent;
  }

  /**
   * Get the icon type for a given Entry.
   *
   * @param {Entry} entry An Entry subclass (FileEntry or DirectoryEntry).
   * @return {string} One of the keys from FileManager.iconTypes, or
   *     'unknown'.
   */
  function getIconType(entry) {
    if (entry.cachedIconType_)
      return entry.cachedIconType_;

    var rv = 'unknown';

    if (entry.isDirectory) {
      rv = 'folder';
    } else {
      for (var name in iconTypes) {
        var value = iconTypes[name];

        if (value instanceof RegExp) {
          if (value.test(entry.name))  {
            rv = name;
            break;
          }
        } else if (typeof value == 'function') {
          try {
            if (value(entry)) {
              rv = name;
              break;
            }
          } catch (ex) {
            console.error('Caught exception while evaluating iconType: ' +
                          name, ex);
          }
        } else {
          console.log('Unexpected value in iconTypes[' + name + ']: ' + value);
        }
      }
    }

    entry.cachedIconType_ = rv;
    return rv;
  }

  /**
   * Call an asynchronous method on dirEntry, batching multiple callers.
   *
   * This batches multiple callers into a single invocation, calling all
   * interested parties back when the async call completes.
   *
   * The Entry method to be invoked should take two callbacks as parameters
   * (one for success and one for failure), and it should invoke those
   * callbacks with a single parameter representing the result of the call.
   * Example methods are Entry.getMetadata() and FileEntry.file().
   *
   * Warning: Because this method caches the first result, subsequent changes
   * to the entry will not be visible to callers.
   *
   * Error results are never cached.
   *
   * @param {DirectoryEntry} dirEntry The DirectoryEntry to apply the method
   *     to.
   * @param {string} methodName The name of the method to dispatch.
   * @param {function(*)} successCallback The function to invoke if the method
   *     succeeds.  The result of the method will be the one parameter to this
   *     callback.
   * @param {function(*)} opt_errorCallback The function to invoke if the
   *     method fails.  The result of the method will be the one parameter to
   *     this callback.  If not provided, the default errorCallback will throw
   *     an exception.
   */
  function batchAsyncCall(entry, methodName, successCallback,
                          opt_errorCallback) {
    var resultCache = methodName + '_resultCache_';

    if (entry[resultCache]) {
      // The result cache for this method already exists.  Just invoke the
      // successCallback with the result of the previuos call.
      // Callback via a setTimeout so the sync/async semantics don't change
      // based on whether or not the value is cached.
      setTimeout(function() { successCallback(entry[resultCache]) }, 0);
      return;
    }

    if (!opt_errorCallback) {
      opt_errorCallback = util.ferr('Error calling ' + methodName + ' for: ' +
                                    entry.fullPath);
    }

    var observerList = methodName + '_observers_';

    if (entry[observerList]) {
      // The observer list already exists, indicating we have a pending call
      // out to this method.  Add this caller to the list of observers and
      // bail out.
      entry[observerList].push([successCallback, opt_errorCallback]);
      return;
    }

    entry[observerList] = [[successCallback, opt_errorCallback]];

    function onComplete(success, result) {
      if (success)
        entry[resultCache] = result;

      for (var i = 0; i < entry[observerList].length; i++) {
        entry[observerList][i][success ? 0 : 1](result);
      }

      delete entry[observerList];
    };

    entry[methodName](function(rv) { onComplete(true, rv) },
                      function(rv) { onComplete(false, rv) });
  }

  /**
   * Get the size of a file, caching the result.
   *
   * When this method completes, the fileEntry object will get a
   * 'cachedSize_' property (if it doesn't already have one) containing the
   * size of the file in bytes.
   *
   * @param {Entry} entry An HTML5 Entry object.
   * @param {function(Entry)} successCallback The function to invoke once the
   *     file size is known.
   */
  function cacheEntrySize(entry, successCallback) {
    if (entry.isDirectory) {
      // No size for a directory, -1 ensures it's sorted before 0 length files.
      entry.cachedSize_ = -1;
    }

    if ('cachedSize_' in entry) {
      if (successCallback) {
        // Callback via a setTimeout so the sync/async semantics don't change
        // based on whether or not the value is cached.
        setTimeout(function() { successCallback(entry) }, 0);
      }
      return;
    }

    batchAsyncCall(entry, 'file', function(file) {
      entry.cachedSize_ = file.size;
      if (successCallback)
        successCallback(entry);
    });
  }

  /**
   * Get the mtime of a file, caching the result.
   *
   * When this method completes, the fileEntry object will get a
   * 'cachedMtime_' property (if it doesn't already have one) containing the
   * last modified time of the file as a Date object.
   *
   * @param {Entry} entry An HTML5 Entry object.
   * @param {function(Entry)} successCallback The function to invoke once the
   *     mtime is known.
   */
  function cacheEntryDate(entry, successCallback) {
    if ('cachedMtime_' in entry) {
      if (successCallback) {
        // Callback via a setTimeout so the sync/async semantics don't change
        // based on whether or not the value is cached.
        setTimeout(function() { successCallback(entry) }, 0);
      }
      return;
    }

    if (entry.isFile) {
      batchAsyncCall(entry, 'file', function(file) {
        entry.cachedMtime_ = file.lastModifiedDate;
        if (successCallback)
          successCallback(entry);
      });
    } else {
      batchAsyncCall(entry, 'getMetadata', function(metadata) {
        entry.cachedMtime_ = metadata.modificationTime;
        if (successCallback)
          successCallback(entry);
      });
    }
  }

  /**
   * Get the icon type of a file, caching the result.
   *
   * When this method completes, the fileEntry object will get a
   * 'cachedIconType_' property (if it doesn't already have one) containing the
   * icon type of the file as a string.
   *
   * The successCallback is always invoked synchronously, since this does not
   * actually require an async call.  You should not depend on this, as it may
   * change if we were to start reading magic numbers (for example).
   *
   * @param {Entry} entry An HTML5 Entry object.
   * @param {function(Entry)} successCallback The function to invoke once the
   *     icon type is known.
   */
  function cacheEntryIconType(entry, successCallback) {
    getIconType(entry);
    if (successCallback)
      setTimeout(function() { successCallback(entry) }, 0);
  }

  // Public statics.

  /**
   * List of dialog types.
   *
   * Keep this in sync with FileManagerDialog::GetDialogTypeAsString, except
   * FULL_PAGE which is specific to this code.
   *
   * @enum {string}
   */
  FileManager.DialogType = {
    SELECT_FOLDER: 'folder',
    SELECT_SAVEAS_FILE: 'saveas-file',
    SELECT_OPEN_FILE: 'open-file',
    SELECT_OPEN_MULTI_FILE: 'open-multi-file',
    FULL_PAGE: 'full-page'
  };

  FileManager.ListType = {
    DETAIL: 'detail',
    THUMBNAIL: 'thumb'
  };

  /**
   * Load translated strings.
   */
  FileManager.initStrings = function(callback) {
    chrome.fileBrowserPrivate.getStrings(function(strings) {
      localStrings = new LocalStrings(strings);
      if (callback)
        callback();
    });
  };

  // Instance methods.

  /**
   * One-time initialization of commands.
   */
  FileManager.prototype.initCommands_ = function() {
    var commands = this.dialogDom_.querySelectorAll('command');
    for (var i = 0; i < commands.length; i++) {
      var command = commands[i];
      cr.ui.Command.decorate(command);
      this.commands_[command.id] = command;
    }

    this.fileContextMenu_ = this.dialogDom_.querySelector('.file-context-menu');
    cr.ui.Menu.decorate(this.fileContextMenu_);

    this.document_.addEventListener(
        'canExecute', this.onRenameCanExecute_.bind(this));
    this.document_.addEventListener(
        'canExecute', this.onDeleteCanExecute_.bind(this));

    this.document_.addEventListener('command', this.onCommand_.bind(this));
  }

  /**
   * One-time initialization of various DOM nodes.
   */
  FileManager.prototype.initDom_ = function() {
    // Cache nodes we'll be manipulating.
    this.previewImage_ = this.dialogDom_.querySelector('.preview-img');
    this.previewFilename_ = this.dialogDom_.querySelector('.preview-filename');
    this.previewSummary_ = this.dialogDom_.querySelector('.preview-summary');
    this.previewMetadata_ = this.dialogDom_.querySelector('.preview-metadata');
    this.filenameInput_ = this.dialogDom_.querySelector('.filename-input');
    this.taskButtons_ = this.dialogDom_.querySelector('.task-buttons');
    this.okButton_ = this.dialogDom_.querySelector('.ok');
    this.cancelButton_ = this.dialogDom_.querySelector('.cancel');
    this.newFolderButton_ = this.dialogDom_.querySelector('.new-folder');

    this.renameInput_ = this.document_.createElement('input');
    this.renameInput_.className = 'rename';

    this.renameInput_.addEventListener(
        'keydown', this.onRenameInputKeyDown_.bind(this));
    this.renameInput_.addEventListener(
        'blur', this.onRenameInputBlur_.bind(this));

    this.filenameInput_.addEventListener(
        'keyup', this.onFilenameInputKeyUp_.bind(this));
    this.filenameInput_.addEventListener(
        'focus', this.onFilenameInputFocus_.bind(this));

    var listContainer = this.dialogDom_.querySelector('.list-container');
    listContainer.addEventListener('keydown', this.onListKeyDown_.bind(this));
    this.okButton_.addEventListener('click', this.onOk_.bind(this));
    this.cancelButton_.addEventListener('click', this.onCancel_.bind(this));

    this.dialogDom_.querySelector('button.new-folder').addEventListener(
        'click', this.onNewFolderButtonClick_.bind(this));

    this.dialogDom_.querySelector('button.detail-view').addEventListener(
        'click', this.onDetailViewButtonClick_.bind(this));
    this.dialogDom_.querySelector('button.thumbnail-view').addEventListener(
        'click', this.onThumbnailViewButtonClick_.bind(this));

    this.dialogDom_.ownerDocument.defaultView.addEventListener(
        'resize', this.onResize_.bind(this));

    var ary = this.dialogDom_.querySelectorAll('[visibleif]');
    for (var i = 0; i < ary.length; i++) {
      var expr = ary[i].getAttribute('visibleif');
      if (!eval(expr))
        ary[i].style.display = 'none';
    }

    // Populate the static localized strings.
    i18nTemplate.process(this.document_, localStrings.templateData);

    // Always sharing the data model between the detail/thumb views confuses
    // them.  Instead we maintain this bogus data model, and hook it up to the
    // view that is not in use.
    this.emptyDataModel_ = new cr.ui.ArrayDataModel([]);

    this.dataModel_ = new cr.ui.ArrayDataModel([]);
    var collator = this.collator_;
    this.dataModel_.setCompareFunction('name', function(a, b) {
      return collator.compare(a.name, b.name);
    });
    this.dataModel_.sort('name');
    this.dataModel_.prepareSort = this.prepareSort_.bind(this);

    if (this.dialogType_ == FileManager.DialogType.SELECT_OPEN_FILE ||
        this.dialogType_ == FileManager.DialogType.SELECT_OPEN_FOLDER ||
        this.dialogType_ == FileManager.DialogType.SELECT_SAVEAS_FILE) {
      this.selectionModelClass_ = cr.ui.ListSingleSelectionModel;
    } else {
      this.selectionModelClass_ = cr.ui.ListSelectionModel;
    }

    this.initTable_();
    this.initGrid_();

    this.setListType(FileManager.ListType.DETAIL);

    this.onResize_();
    this.dialogDom_.style.opacity = '1';
  };

  /**
   * "Save a file" dialog is supposed to have a combo box with available
   * file types. Selecting an item filters files by extension and specifies how
   * file should be saved.
   * @return {intener} Index of selected type from this.fileTypes_ + 1. 0
   *                   means value is not specified.
   */
  FileManager.prototype.getSelectedFilterIndex_= function(fileName) {
    // TODO(serya): Implement the combo box
    // For now try to guess choice by file extension.
    if (!this.fileTypes_ || this.fileTypes_.length == 0) return 0;

    var extension = /\.[^\.]+$/.exec(fileName);
    extension = extension ? extension[0].substring(1).toLowerCase() : "";
    var result = 0;  // Use first type by default.
    for (var i = 0; i < this.fileTypes_.length; i++) {
      if (this.fileTypes_[i].extensions.indexOf(extension)) {
        result = i;
      }
    }
    return result + 1;  // 1-based index.
  };

  /**
   * Force the canExecute events to be dispatched.
   */
  FileManager.prototype.updateCommands_ = function() {
    this.commands_['rename'].canExecuteChange();
    this.commands_['delete'].canExecuteChange();
  };

  /**
   * Invoked to decide whether the "rename" command can be executed.
   */
  FileManager.prototype.onRenameCanExecute_ = function(event) {
    event.canExecute =
        (// Initialized to the point where we have a current directory
         this.currentDirEntry_ &&
         // Rename not in progress.
         !this.renameInput_.currentEntry &&
         // Not in root directory.
         this.currentDirEntry_.fullPath != '/' &&
         // Not in media directory.
         this.currentDirEntry_.fullPath != MEDIA_DIRECTORY &&
         // Only one file selected.
         this.selection.totalCount == 1);
  };

  /**
   * Invoked to decide whether the "delete" command can be executed.
   */
  FileManager.prototype.onDeleteCanExecute_ = function(event) {
    event.canExecute =
        (// Initialized to the point where we have a current directory
         this.currentDirEntry_ &&
         // Rename not in progress.
         !this.renameInput_.currentEntry &&
         // Not in root directory.
         this.currentDirEntry_.fullPath != '/' &&
         // Not in media directory.
         this.currentDirEntry_.fullPath != MEDIA_DIRECTORY);
  };

  FileManager.prototype.setListType = function(type) {
    if (type && type == this.listType_)
      return;

    if (type == FileManager.ListType.DETAIL) {
      var selectedIndexes = this.grid_.selectionModel.selectedIndexes;
      this.table_.dataModel = this.dataModel_;
      this.table_.style.display = '';
      this.grid_.style.display = 'none';
      this.grid_.dataModel = this.emptyDataModel_;
      this.currentList_ = this.table_;
      this.dialogDom_.querySelector('button.detail-view').disabled = true;
      this.dialogDom_.querySelector('button.thumbnail-view').disabled = false;
      this.table_.selectionModel.selectedIndexes = selectedIndexes;
    } else if (type == FileManager.ListType.THUMBNAIL) {
      var selectedIndexes = this.table_.selectionModel.selectedIndexes;
      this.grid_.dataModel = this.dataModel_;
      this.grid_.style.display = '';
      this.table_.style.display = 'none';
      this.table_.dataModel = this.emptyDataModel_;
      this.currentList_ = this.grid_;
      this.dialogDom_.querySelector('button.thumbnail-view').disabled = true;
      this.dialogDom_.querySelector('button.detail-view').disabled = false;
      this.grid_.selectionModel.selectedIndexes = selectedIndexes;
    } else {
      throw new Error('Unknown list type: ' + type);
    }

    this.listType_ = type;
    this.onResize_();
  };

  /**
   * Initialize the file thumbnail grid.
   */
  FileManager.prototype.initGrid_ = function() {
    this.grid_ = this.dialogDom_.querySelector('.thumbnail-grid');
    cr.ui.Grid.decorate(this.grid_);

    var self = this;
    this.grid_.itemConstructor = function(entry) {
      return self.renderThumbnail_(entry);
    };

    this.grid_.selectionModel = new this.selectionModelClass_();

    this.grid_.addEventListener(
        'dblclick', this.onDetailDoubleClick_.bind(this));
    this.grid_.selectionModel.addEventListener(
        'change', this.onSelectionChanged_.bind(this));

    cr.ui.contextMenuHandler.addContextMenuProperty(this.grid_);
    this.grid_.contextMenu = this.fileContextMenu_;
    this.grid_.addEventListener('mousedown',
                                this.onGridMouseDown_.bind(this));
  };

  /**
   * Initialize the file list table.
   */
  FileManager.prototype.initTable_ = function() {
    var checkWidth = this.showCheckboxes_ ? 5 : 0;

    var columns = [
        new cr.ui.table.TableColumn('cachedIconType_', '',
                                    5.4 + checkWidth),
        new cr.ui.table.TableColumn('name', str('NAME_COLUMN_LABEL'),
                                    64 - checkWidth),
        new cr.ui.table.TableColumn('cachedSize_',
                                    str('SIZE_COLUMN_LABEL'), 15.5),
        new cr.ui.table.TableColumn('cachedMtime_',
                                    str('DATE_COLUMN_LABEL'), 21)
    ];

    columns[0].renderFunction = this.renderIconType_.bind(this);
    columns[1].renderFunction = this.renderName_.bind(this);
    columns[2].renderFunction = this.renderSize_.bind(this);
    columns[3].renderFunction = this.renderDate_.bind(this);

    this.table_ = this.dialogDom_.querySelector('.detail-table');
    cr.ui.Table.decorate(this.table_);

    this.table_.selectionModel = new this.selectionModelClass_();
    this.table_.columnModel = new cr.ui.table.TableColumnModel(columns);

    this.table_.addEventListener(
        'dblclick', this.onDetailDoubleClick_.bind(this));
    this.table_.selectionModel.addEventListener(
        'change', this.onSelectionChanged_.bind(this));

    cr.ui.contextMenuHandler.addContextMenuProperty(this.table_);
    this.table_.contextMenu = this.fileContextMenu_;

    this.table_.addEventListener('mousedown',
                                 this.onTableMouseDown_.bind(this));
  };

  /**
   * Respond to a command being executed.
   */
  FileManager.prototype.onCommand_ = function(event) {
    switch (event.command.id) {
      case 'rename':
        var leadIndex = this.currentList_.selectionModel.leadIndex;
        var li = this.currentList_.getListItemByIndex(leadIndex);
        var label = li.querySelector('.filename-label');
        if (!label) {
          console.warn('Unable to find label for rename of index: ' +
                       leadIndex);
          return;
        }

        this.initiateRename_(label);
        break;

      case 'delete':
        this.deleteEntries(this.selection.entries);
        break;
    }
  };

  /**
   * Respond to the back button.
   */
  FileManager.prototype.onPopState_ = function(event) {
    this.changeDirectory(event.state, CD_NO_HISTORY);
  };

  /**
   * Resize details and thumb views to fit the new window size.
   */
  FileManager.prototype.onResize_ = function() {
    this.table_.style.height = this.grid_.style.height =
      this.grid_.parentNode.clientHeight + 'px';
    this.table_.style.width = this.grid_.style.width =
      this.grid_.parentNode.clientWidth + 'px';

    this.table_.list_.style.width = this.table_.parentNode.clientWidth + 'px';
    this.table_.list_.style.height = (this.table_.clientHeight - 1 -
                                      this.table_.header_.clientHeight) + 'px';

    if (this.listType_ == FileManager.ListType.THUMBNAIL) {
      var self = this;
      setTimeout(function() {
          self.grid_.columns = 0;
          self.grid_.redraw();
      }, 0);
    } else {
      this.currentList_.redraw();
    }
  };

  /**
   * Resolve a path to either a DirectoryEntry or a FileEntry, regardless of
   * whether the path is a directory or file.
   *
   * @param path {string} The path to be resolved.
   * @param resultCallback{function(Entry)} Called back when a path is
   *     successfully resolved.  Entry will be either a DirectoryEntry or
   *     a FileEntry.
   * @param errorCallback{function(FileError)} Called back if an unexpected
   *     error occurs while resolving the path.
   */
  FileManager.prototype.resolvePath = function(
      path, resultCallback, errorCallback) {
    if (path == '' || path == '/') {
      resultCallback(this.filesystem_.root);
      return;
    }

    var root = this.filesystem_.root;
    root.getFile(
        path, {create: false},
        resultCallback,
        function (err) {
          if (err.code == FileError.TYPE_MISMATCH_ERR) {
            // Bah.  It's a directory, ask again.
            root.getDirectory(
                path, {create: false},
                resultCallback,
                errorCallback);
            } else  {
              errorCallback(err);
            }
        });
  };

  FileManager.prototype.initDefaultDirectory_ = function() {
    // Split the dirname from the basename.
    var ary = this.defaultPath_.match(/^(.*?)(?:\/([^\/]+))?$/);
    if (!ary) {
      console.warn('Unable to split default path: ' + path);
      self.changeDirectory('/', CD_NO_HISTORY);
      return;
    }

    var baseName = ary[1];
    var leafName = ary[2];

    var self = this;

    function onBaseFound(baseDirEntry) {
      if (!leafName) {
        // Default path is just a directory, cd to it and we're done.
        self.changeDirectoryEntry(baseDirEntry, CD_NO_HISTORY);
        return;
      }

      function onLeafFound(leafEntry) {
        if (leafEntry.isDirectory) {
          self.changeDirectoryEntry(leafEntry, CD_NO_HISTORY);
          return;
        }

        // Leaf is an existing file, cd to its parent directory and select it.
        self.changeDirectoryEntry(baseDirEntry, CD_NO_HISTORY, leafEntry.name);
      }

      function onLeafError(err) {
        // Set filename first so OK button will update in changeDirectoryEntry.
        self.filenameInput_.value = leafName;
        if (err = FileError.NOT_FOUND_ERR) {
          // Leaf does not exist, it's just a suggested file name.
          self.changeDirectoryEntry(baseDirEntry, CD_NO_HISTORY);
        } else {
          console.log('Unexpected error resolving default leaf: ' + err);
          self.changeDirectoryEntry('/', CD_NO_HISTORY);
        }
      }

      self.resolvePath(self.defaultPath_, onLeafFound, onLeafError);
    }

    function onBaseError(err) {
      // Set filename first so OK button will update in changeDirectory.
      self.filenameInput_.value = leafName;
      console.log('Unexpected error resolving default base "' +
                  baseName + '": ' + err);
      self.changeDirectory('/', CD_NO_HISTORY);
    }

    this.filesystem_.root.getDirectory(
        baseName, {create: false}, onBaseFound, onBaseError);
  };

  /**
   * Tweak the UI to become a particular kind of dialog, as determined by the
   * dialog type parameter passed to the constructor.
   */
  FileManager.prototype.initDialogType_ = function() {
    var defaultTitle;
    var okLabel = str('OPEN_LABEL');

    switch (this.dialogType_) {
      case FileManager.DialogType.SELECT_FOLDER:
        defaultTitle = str('SELECT_FOLDER_TITLE');
        break;

      case FileManager.DialogType.SELECT_OPEN_FILE:
        defaultTitle = str('SELECT_OPEN_FILE_TITLE');
        break;

      case FileManager.DialogType.SELECT_OPEN_MULTI_FILE:
        defaultTitle = str('SELECT_OPEN_MULTI_FILE_TITLE');
        break;

      case FileManager.DialogType.SELECT_SAVEAS_FILE:
        defaultTitle = str('SELECT_SAVEAS_FILE_TITLE');
        okLabel = str('SAVE_LABEL');
        break;

      case FileManager.DialogType.FULL_PAGE:
        break;

      default:
        throw new Error('Unknown dialog type: ' + this.dialogType_);
    }

    this.okButton_.textContent = okLabel;

    dialogTitle = this.params_.title || defaultTitle;
    this.dialogDom_.querySelector('.dialog-title').textContent = dialogTitle;
  };

  /**
   * Cache necessary data before a sort happens.
   *
   * This is called by the table code before a sort happens, so that we can
   * go fetch data for the sort field that we may not have yet.
   */
  FileManager.prototype.prepareSort_ = function(field, callback) {
    var cacheFunction;

    if (field == 'cachedMtime_') {
      cacheFunction = cacheEntryDate;
    } else if (field == 'cachedSize_') {
      cacheFunction = cacheEntrySize;
    } else if (field == 'cachedIconType_') {
      cacheFunction = cacheEntryIconType;
    } else {
      callback();
      return;
    }

    function checkCount() {
      if (uncachedCount == 0) {
        // Callback via a setTimeout so the sync/async semantics don't change
        // based on whether or not the value is cached.
        setTimeout(callback, 0);
      }
    }

    var dataModel = this.dataModel_;
    var uncachedCount = dataModel.length;

    for (var i = uncachedCount - 1; i >= 0 ; i--) {
      var entry = dataModel.item(i);
      if (field in entry) {
        uncachedCount--;
      } else {
        cacheFunction(entry, function() {
          uncachedCount--;
          checkCount();
        });
      }
    }

    checkCount();
  }

  /**
   * Render (and wire up) a checkbox to be used in either a detail or a
   * thumbnail list item.
   */
  FileManager.prototype.renderCheckbox_ = function(entry) {
    var input = this.document_.createElement('input');
    input.setAttribute('type', 'checkbox');
    input.setAttribute('tabindex', -1);
    input.className = 'file-checkbox';
    input.addEventListener('mousedown',
                           this.onCheckboxMouseDownUp_.bind(this));
    input.addEventListener('mouseup',
                           this.onCheckboxMouseDownUp_.bind(this));
    input.addEventListener('click',
                           this.onCheckboxClick_.bind(this));

    if (this.selection && this.selection.entries.indexOf(entry) != -1) {
      // Our DOM nodes get discarded as soon as we're scrolled out of view,
      // so we have to make sure the check state is correct when we're brought
      // back to life.
      input.checked = true;
    }

    return input;
  }

  FileManager.prototype.renderThumbnail_ = function(entry) {
    var li = this.document_.createElement('li');
    li.className = 'thumbnail-item';

    if (this.showCheckboxes_)
      li.appendChild(this.renderCheckbox_(entry));

    var div = this.document_.createElement('div');
    div.className = 'img-container';
    li.appendChild(div);

    var img = this.document_.createElement('img');
    this.getThumbnailURL(entry, function(type, url) { img.src = url });
    div.appendChild(img);

    div = this.document_.createElement('div');
    div.className = 'filename-label';
    var labelText = entry.name;
    if (this.currentDirEntry_.name == '')
      labelText = this.getLabelForRootPath_(labelText);

    div.textContent = labelText;
    div.entry = entry;

    li.appendChild(div);

    cr.defineProperty(li, 'lead', cr.PropertyKind.BOOL_ATTR);
    cr.defineProperty(li, 'selected', cr.PropertyKind.BOOL_ATTR);
    return li;
  }

  /**
   * Render the type column of the detail table.
   *
   * Invoked by cr.ui.Table when a file needs to be rendered.
   *
   * @param {Entry} entry The Entry object to render.
   * @param {string} columnId The id of the column to be rendered.
   * @param {cr.ui.Table} table The table doing the rendering.
   */
  FileManager.prototype.renderIconType_ = function(entry, columnId, table) {
    var div = this.document_.createElement('div');
    div.className = 'detail-icon-container';

    if (this.showCheckboxes_)
      div.appendChild(this.renderCheckbox_(entry));

    var icon = this.document_.createElement('div');
    icon.className = 'detail-icon';
    entry.cachedIconType_ = getIconType(entry);
    icon.setAttribute('iconType', entry.cachedIconType_);
    div.appendChild(icon);

    return div;
  };

  FileManager.prototype.getLabelForRootPath_ = function(path) {
    // This hack lets us localize the top level directories.
    if (path == 'Downloads')
      return str('DOWNLOADS_DIRECTORY_LABEL');

    if (path == 'media')
      return str('MEDIA_DIRECTORY_LABEL');

    return path || str('ROOT_DIRECTORY_LABEL');
  };

  /**
   * Render the Name column of the detail table.
   *
   * Invoked by cr.ui.Table when a file needs to be rendered.
   *
   * @param {Entry} entry The Entry object to render.
   * @param {string} columnId The id of the column to be rendered.
   * @param {cr.ui.Table} table The table doing the rendering.
   */
  FileManager.prototype.renderName_ = function(entry, columnId, table) {
    var label = this.document_.createElement('div');
    label.entry = entry;
    label.className = 'detail-name filename-label';
    if (this.currentDirEntry_.name == '') {
      label.textContent = this.getLabelForRootPath_(entry.name);
    } else {
      label.textContent = entry.name;
    }

    return label;
  };

  /**
   * Render the Size column of the detail table.
   *
   * @param {Entry} entry The Entry object to render.
   * @param {string} columnId The id of the column to be rendered.
   * @param {cr.ui.Table} table The table doing the rendering.
   */
  FileManager.prototype.renderSize_ = function(entry, columnId, table) {
    var div = this.document_.createElement('div');
    div.className = 'detail-size';

    div.textContent = '...';
    cacheEntrySize(entry, function(entry) {
      if (entry.cachedSize_ == -1) {
        div.textContent = '';
      } else {
        div.textContent = util.bytesToSi(entry.cachedSize_);
      }
    });

    return div;
  };

  /**
   * Render the Date column of the detail table.
   *
   * @param {Entry} entry The Entry object to render.
   * @param {string} columnId The id of the column to be rendered.
   * @param {cr.ui.Table} table The table doing the rendering.
   */
  FileManager.prototype.renderDate_ = function(entry, columnId, table) {
    var div = this.document_.createElement('div');
    div.className = 'detail-date';

    div.textContent = '...';

    var self = this;
    cacheEntryDate(entry, function(entry) {
      if (self.currentDirEntry_.fullPath == MEDIA_DIRECTORY &&
          entry.cachedMtime_.getTime() == 0) {
        // Mount points for FAT volumes have this time associated with them.
        // We'd rather display nothing than this bogus date.
        div.textContent = '---';
      } else {
        div.textContent = self.shortDateFormatter_.format(entry.cachedMtime_);
      }
    });

    return div;
  };

  /**
   * Compute summary information about the current selection.
   *
   * This method dispatches the 'selection-summarized' event when it completes.
   * Depending on how many of the selected files already have known sizes, the
   * dispatch may happen immediately, or after a number of async calls complete.
   */
  FileManager.prototype.summarizeSelection_ = function() {
    var selection = this.selection = {
      entries: [],
      urls: [],
      leadEntry: null,
      totalCount: 0,
      fileCount: 0,
      directoryCount: 0,
      bytes: 0,
      iconType: null,
      indexes: this.currentList_.selectionModel.selectedIndexes
    };

    this.previewSummary_.textContent = str('COMPUTING_SELECTION');
    this.taskButtons_.innerHTML = '';

    if (!selection.indexes.length) {
      cr.dispatchSimpleEvent(this, 'selection-summarized');
      return;
    }

    var fileCount = 0;
    var byteCount = 0;
    var pendingFiles = [];

    for (var i = 0; i < selection.indexes.length; i++) {
      var entry = this.dataModel_.item(selection.indexes[i]);

      selection.entries.push(entry);
      selection.urls.push(entry.toURL());

      if (selection.iconType == null) {
        selection.iconType = getIconType(entry);
      } else if (selection.iconType != 'unknown') {
        var iconType = getIconType(entry);
        if (selection.iconType != iconType)
          selection.iconType = 'unknown';
      }

      selection.totalCount++;

      if (entry.isFile) {
        selection.fileCount += 1;
        if (!('cachedSize_' in entry)) {
          // Any file that hasn't been rendered may be missing its cachedSize_
          // property.  For example, visit a large file list, and press ctrl-a
          // to select all.  In this case, we need to asynchronously get the
          // sizes for these files before telling the world the selection has
          // been summarized.  See the 'computeNextFile' logic below.
          pendingFiles.push(entry);
          continue;
        } else {
          selection.bytes += entry.cachedSize_;
        }
      } else {
        selection.directoryCount += 1;
      }
    }

    var leadIndex = this.currentList_.selectionModel.leadIndex;
    if (leadIndex > -1) {
      selection.leadEntry = this.dataModel_.item(leadIndex);
    } else {
      selection.leadEntry = selection.entries[0];
    }

    var self = this;

    function cacheNextFile(fileEntry) {
      if (fileEntry) {
        // We're careful to modify the 'selection', rather than 'self.selection'
        // here, just in case the selection has changed since this summarization
        // began.
        selection.bytes += fileEntry.cachedSize_;
      }

      if (pendingFiles.length) {
        cacheEntrySize(pendingFiles.pop(), cacheNextFile);
      } else {
        self.dispatchEvent(new cr.Event('selection-summarized'));
      }
    };

    if (this.dialogType_ == FileManager.DialogType.FULL_PAGE) {
      chrome.fileBrowserPrivate.getFileTasks(selection.urls,
                                             this.onTasksFound_.bind(this));
    }

    cacheNextFile();
  };

  FileManager.prototype.onMetadataResult_ = function(fileURL, metadata) {
    var observers = this.metadataCache_[fileURL];
    if (!observers || !(observers instanceof Array)) {
      console.error('Missing or invalid metadata observers: ' + fileURL + ': ' +
                    observers);
      return;
    }

    console.log('metadata result:', metadata);
    for (var i = 0; i < observers.length; i++) {
      observers[i](metadata);
    }

    this.metadataCache_[fileURL] = metadata;
  };

  FileManager.prototype.onMetadataError_ = function(fileURL, step, error) {
    console.warn('metadata: ' + fileURL + ': ' + step + ': ' + error);
    this.onMetadataResult_(fileURL, {});
  };

  FileManager.prototype.onMetadataFilter_ = function(regexp) {
    this.metadataUrlFilter_ = regexp;
  };

  FileManager.prototype.onMetadataLog_ = function(arglist) {
    console.log.apply(console, ['metadata:'].concat(arglist));
  };

  /**
   * Dispatch a message from a metadata reader to the appropriate onMetadata*
   * method.
   */
  FileManager.prototype.onMetadataMessage_ = function(event) {
    var data = event.data;

    var methodName = 'onMetadata' + data.verb.substr(0, 1).toUpperCase() +
        data.verb.substr(1) + '_';

    if (!(methodName in this)) {
      console.log('Unknown message from metadata reader: ' + data.verb, data);
      return;
    }

    this[methodName].apply(this, data.arguments);
  };

  FileManager.prototype.onTasksFound_ = function(tasksList) {
    this.taskButtons_.innerHTML = '';
    for (var i = 0; i < tasksList.length; i++) {
      var task = tasksList[i];

      // Tweak images, titles of internal tasks.
      var task_parts = task.taskId.split('|');
      if (task_parts[0] == this.getExtensionId_()) {
        if (task_parts[1] == 'preview') {
          // TODO(serya): This hack needed until task.iconUrl get working
          //              (see GetFileTasksFileBrowserFunction::RunImpl).
          task.iconUrl =
              chrome.extension.getURL('images/icon_preview_16x16.png');
          task.title = str('PREVIEW_IMAGE');
        } else if (task_parts[1] == 'play') {
          task.iconUrl =
              chrome.extension.getURL('images/icon_play_16x16.png');
          task.title = str('PLAY_MEDIA').replace("&", "");
        } else if (task_parts[1] == 'enqueue') {
          task.iconUrl =
              chrome.extension.getURL('images/icon_add_to_queue_16x16.png');
          task.title = str('ENQUEUE');
        }
      }

      var button = this.document_.createElement('button');
      button.addEventListener('click', this.onTaskButtonClicked_.bind(this));
      button.className = 'task-button';
      button.task = task;

      var img = this.document_.createElement('img');
      img.src = task.iconUrl;

      button.appendChild(img);
      button.appendChild(this.document_.createTextNode(task.title));

      this.taskButtons_.appendChild(button);
    }
  };

  FileManager.prototype.getExtensionId_ = function() {
    return chrome.extension.getURL('').split('/')[2];
  };

  FileManager.prototype.onTaskButtonClicked_ = function(event) {
    // Check internal tasks first.
    var task_parts = event.srcElement.task.taskId.split('|');
    if (task_parts[0] == this.getExtensionId_()) {
      if (task_parts[1] == 'preview') {
        g_slideshow_data = this.selection.urls;
        chrome.tabs.create({url: "slideshow.html"});
      } else if (task_parts[1] == 'play') {
        chrome.fileBrowserPrivate.viewFiles(this.selection.urls,
            event.srcElement.task.taskId);
      } else if (task_parts[1] == 'enqueue') {
        chrome.fileBrowserPrivate.viewFiles(this.selection.urls,
            event.srcElement.task.taskId);
      }
      return;
    }

    chrome.fileBrowserPrivate.executeTask(event.srcElement.task.taskId,
                                          this.selection.urls);
  }

  /**
   * Update the breadcrumb display to reflect the current directory.
   */
  FileManager.prototype.updateBreadcrumbs_ = function() {
    var bc = this.dialogDom_.querySelector('.breadcrumbs');
    bc.innerHTML = '';

    var fullPath = this.currentDirEntry_.fullPath.replace(/\/$/, '');
    var pathNames = fullPath.split('/');
    var path = '';

    for (var i = 0; i < pathNames.length; i++) {
      var pathName = pathNames[i];
      path += pathName + '/';

      var div = this.document_.createElement('div');
      div.className = 'breadcrumb-path';
      if (i <= 1) {
        // i == 0: root directory itself, i == 1: the files it contains.
        div.textContent = this.getLabelForRootPath_(pathName);
      } else {
        div.textContent = pathName;
      }

      div.path = path;
      div.addEventListener('click', this.onBreadcrumbClick_.bind(this));

      bc.appendChild(div);

      if (i == pathNames.length - 1) {
        div.classList.add('breadcrumb-last');
      } else {
        var spacer = this.document_.createElement('div');
        spacer.className = 'breadcrumb-spacer';
        spacer.textContent = RIGHT_TRIANGLE;
        bc.appendChild(spacer);
      }
    }
  };

  /**
   * Update the preview panel to display a given entry.
   *
   * The selection summary line is handled by the onSelectionSummarized handler
   * rather than this function, because summarization may not complete quickly.
   */
  FileManager.prototype.updatePreview_ = function() {
    // Clear the preview image first, in case the thumbnail takes long to load.
    this.previewImage_.src = '';
    // The transparent-background class is used to display the checkerboard
    // background for image thumbnails.  We don't want to display it for
    // non-thumbnail preview images.
    this.previewImage_.classList.remove('transparent-background');
    // The multiple-selected class indicates that more than one entry is
    // selcted.
    this.previewImage_.classList.remove('multiple-selected');

    if (!this.selection.totalCount) {
      this.previewFilename_.textContent = '';
      return;
    }

    var previewName = this.selection.leadEntry.name;
    if (this.currentDirEntry_.name == '')
      previewName = this.getLabelForRootPath_(previewName);

    this.previewFilename_.textContent = previewName;

    var iconType = getIconType(this.selection.leadEntry);
    if (iconType == 'image') {
      if (fileManager.selection.totalCount > 1)
        this.previewImage_.classList.add('multiple-selected');
    }

    var self = this;
    var leadEntry = this.selection.leadEntry;

    if (this.selection.totalCount == this.selection.fileCount == 1) {
      this.cacheMetadata_(leadEntry, function (metadata) {
          if (leadEntry != self.selection.leadEntry) {
            // Selection has changed since we asked for the metadata, nevermind.
            return;
          }

          self.setPreviewMetadata(metadata);
        });
    } else {
      self.setPreviewMetadata(null);
    }

    this.getThumbnailURL(leadEntry, function(iconType, url) {
      if (self.selection.leadEntry != leadEntry) {
        // Selection has changed since we asked, nevermind.
        return;
      }

      if (url) {
        self.previewImage_.src = url;
        if (iconType == 'image')
          self.previewImage_.classList.add('transparent-background');
      } else {
        self.previewImage_.src = previewArt['unknown'];
      }
    });
  };

  FileManager.prototype.cacheMetadata_ = function(entry, callback) {
    var url = entry.toURL();
    var cacheValue = this.metadataCache_[url];

    if (!cacheValue) {
      // This is the first time anyone's asked, go get it.
      if (entry.name.match(this.metadataUrlFilter_)) {
        this.metadataCache_[url] = [callback];
        this.metadataReader_.postMessage(
            {verb: 'request', arguments: [entry.toURL()]});
        return;
      }
      // Cannot extract metadata for this file, return an empty map.
      setTimeout(function() { callback({}) });
      return;
    }

    if (cacheValue instanceof Array) {
      // Something is already pending, add to the list of observers.
      cacheValue.push(callback);
      return;
    }

    if (cacheValue instanceof Object) {
      // We already know the answer, let the caller know in a fresh call stack.
      setTimeout(function() { callback(cacheValue) });
      return;
    }

    console.error('Unexpected metadata cache value:' + cacheValue);
  };

  FileManager.prototype.setPreviewMetadata = function(metadata) {
    this.previewMetadata_.textContent = '';
    if (!(metadata && metadata.ifd))
      return;

    var self = this;

    function addProperty(id, v) {
      var dom = self.document_.createElement('div');
      dom.className = 'metadata-item';
      var label = self.document_.createElement('div');
      label.className = 'metadata-label';
      label.textContent = str(id) || id;
      dom.appendChild(label);
      var value = self.document_.createElement('div');
      value.className = 'metadata-value';
      value.textContent = v;
      dom.appendChild(value);

      self.previewMetadata_.appendChild(dom);
    }

    // TODO(rginda): Split this function into metadata specific routines when
    // we add new metadata types.
    // TODO(rginda): Add const names for these numerics.
    var exifDir = metadata.ifd.exif;
    if (0xa002 in exifDir && 0xa003 in exifDir) {
      addProperty('DIMENSIONS_LABEL',
                  strf('DIMENSIONS_FORMAT', exifDir[0xa002].value,
                       exifDir[0xa003].value));
    }
  };

  FileManager.prototype.getThumbnailURL = function(entry, callback) {
    if (!entry)
      return;

    var iconType = getIconType(entry);

    this.cacheMetadata_(entry, function (metadata) {
      var url = metadata.thumbnailURL;
      if (!url) {
        if (iconType == 'image') {
          url = entry.toURL();
        } else {
          url = previewArt[iconType] || previewArt['unknown'];
        }
      }

      callback(iconType, url);
    });
  };

  FileManager.prototype.selectEntry = function(name) {
    for (var i = 0; i < this.dataModel_.length; i++) {
      if (this.dataModel_.item(i).name == name) {
        this.currentList_.selectionModel.selectedIndex = i;
        this.currentList_.scrollIndexIntoView(i);
        this.currentList_.focus();
        return;
      }
    }
  };

  /**
   * Change the current directory to the directory represented by a
   * DirectoryEntry.
   *
   * Dispatches the 'directory-changed' event when the directory is successfully
   * changed.
   *
   * @param {string} path The absolute path to the new directory.
   * @param {bool} opt_saveHistory Save this in the history stack (defaults
   *     to true).
   * @param {string} opt_selectedEntry The name of the file to select after
   *     changing directories.
   */
  FileManager.prototype.changeDirectoryEntry = function(dirEntry,
                                                        opt_saveHistory,
                                                        opt_selectedEntry) {
    if (this.currentDirEntry_ &&
        this.currentDirEntry_.fullPath == dirEntry.fullPath) {
      // Directory didn't actually change.
      if (opt_selectedEntry)
        this.selectEntry(opt_selectedEntry);
      return;
    }

    if (typeof opt_saveHistory == 'undefined') {
      opt_saveHistory = true;
    } else {
      opt_saveHistory = !!opt_saveHistory;
    }

    var e = new cr.Event('directory-changed');
    e.previousDirEntry = this.currentDirEntry_;
    e.newDirEntry = dirEntry;
    e.saveHistory = opt_saveHistory;
    e.selectedEntry = opt_selectedEntry;
    this.currentDirEntry_ = dirEntry;
    this.dispatchEvent(e);
  }

  /**
   * Change the current directory to the directory represented by a string
   * path.
   *
   * Dispatches the 'directory-changed' event when the directory is successfully
   * changed.
   *
   * @param {string} path The absolute path to the new directory.
   * @param {bool} opt_saveHistory Save this in the history stack (defaults
   *     to true).
   * @param {string} opt_selectedEntry The name of the file to select after
   *     changing directories.
   */
  FileManager.prototype.changeDirectory = function(path,
                                                   opt_saveHistory,
                                                   opt_selectedEntry) {
    if (path == '/')
      return this.changeDirectoryEntry(this.filesystem_.root);

    var self = this;

    this.filesystem_.root.getDirectory(
        path, {create: false},
        function(dirEntry) {
          self.changeDirectoryEntry(
              dirEntry, opt_saveHistory, opt_selectedEntry);
        },
        function(err) {
          console.error('Error changing directory to: ' + path + ', ' + err);
          if (!self.currentDirEntry_) {
            // If we've never successfully changed to a directory, force them
            // to the root.
            self.changeDirectory('/', false);
          }
        });
  };

  FileManager.prototype.deleteEntries = function(entries) {
    if (!window.confirm(str('CONFIRM_DELETE')))
      return;

    var count = entries.length;

    var self = this;
    function onDelete() {
      if (--count == 0)
         self.rescanDirectory_();
    }

    for (var i = 0; i < entries.length; i++) {
      var entry = entries[i];
      if (entry.isFile) {
        entry.remove(
            onDelete,
            util.flog('Error deleting file: ' + entry.fullPath, onDelete));
      } else {
        entry.removeRecursively(
            onDelete,
            util.flog('Error deleting folder: ' + entry.fullPath, onDelete));
      }
    }
  };

  /**
   * Update the selection summary UI when the selection summarization completes.
   */
  FileManager.prototype.onSelectionSummarized_ = function() {
    var selection = this.selection;
    var bytes = util.bytesToSi(selection.bytes);
    var text = '';
    if (selection.totalCount == 0) {
      text = str('NOTHING_SELECTED');
    } else if (selection.fileCount == 1 && selection.directoryCount == 0) {
      text = strf('ONE_FILE_SELECTED', bytes);
    } else if (selection.fileCount == 0 && selection.directoryCount == 1) {
      text = str('ONE_DIRECTORY_SELECTED');
    } else if (selection.directoryCount == 0) {
      text = strf('MANY_FILES_SELECTED', selection.fileCount, bytes);
    } else if (selection.fileCount == 0) {
      text = strf('MANY_DIRECTORIES_SELECTED', selection.directoryCount);
    } else {
      text = strf('MANY_ENTRIES_SELECTED', selection.totalCount, bytes);
    }
    this.previewSummary_.textContent = text;
  };

  /**
   * Handle a click event on a breadcrumb element.
   *
   * @param {Event} event The click event.
   */
  FileManager.prototype.onBreadcrumbClick_ = function(event) {
    this.changeDirectory(event.srcElement.path);
  };

  FileManager.prototype.onCheckboxMouseDownUp_ = function(event) {
    // If exactly one file is selected and its checkbox is *not* clicked,
    // then this should be treated as a "normal" click (ie. the previous
    // selection should be cleared).
    if (this.selection.totalCount == 1 && this.selection.entries[0].isFile) {
      var selectedIndex = this.selection.indexes[0];
      var listItem = this.currentList_.getListItemByIndex(selectedIndex);
      var checkbox = listItem.querySelector('input[type="checkbox"]');
      if (!checkbox.checked)
        return;
    }

    // Otherwise, treat clicking on a checkbox the same as a ctrl-click.
    // The default properties of event.ctrlKey make it read-only, but
    // don't prevent deletion, so we delete first, then set it true.
    delete event.ctrlKey;
    event.ctrlKey = true;
  };

  FileManager.prototype.onCheckboxClick_ = function(event) {
    if (event.shiftKey) {
      // Something about the timing of shift-clicks causes the checkbox
      // to get selected and then very quickly unselected.  It appears that
      // we forcibly select the checkbox as part of onSelectionChanged, and
      // then the default action of this click event fires and toggles the
      // checkbox back off.
      //
      // Since we're going to force checkboxes into the correct state for any
      // multi-selection, we can prevent this shift click from toggling the
      // checkbox and avoid the trouble.
      event.preventDefault();
    }
  };

  /**
   * Update the UI when the selection model changes.
   *
   * @param {cr.Event} event The change event.
   */
  FileManager.prototype.onSelectionChanged_ = function(event) {
    this.summarizeSelection_();
    this.updatePreview_();

    if (this.dialogType_ == FileManager.DialogType.SELECT_SAVEAS_FILE) {
      // If this is a save-as dialog, copy the selected file into the filename
      // input text box.
      if (this.selection.leadEntry && this.selection.leadEntry.isFile)
        this.filenameInput_.value = this.selection.leadEntry.name;
    }

    this.updateOkButton_();

    var self = this;
    setTimeout(function() { self.onSelectionChangeComplete_(event) }, 0);
  };

  /**
   * Handle selection change related tasks that won't run properly during
   * the actual selection change event.
   */
  FileManager.prototype.onSelectionChangeComplete_ = function(event) {
    // Inform tests it's OK to click buttons now.
    chrome.test.sendMessage('selection-change-complete');

    if (!this.showCheckboxes_)
      return;

    for (var i = 0; i < event.changes.length; i++) {
      // Turn off any checkboxes for items that are no longer selected.
      var selectedIndex = event.changes[i].index;
      var listItem = this.currentList_.getListItemByIndex(selectedIndex);
      if (!listItem) {
        // When changing directories, we get notified about list items
        // that are no longer there.
        continue;
      }

      if (!event.changes[i].selected) {
        var checkbox = listItem.querySelector('input[type="checkbox"]');
        checkbox.checked = false;
      }
    }

    if (this.selection.fileCount > 1) {
      // If more than one file is selected, make sure all checkboxes are lit
      // up.
      for (var i = 0; i < this.selection.entries.length; i++) {
        if (!this.selection.entries[i].isFile)
          continue;

        var selectedIndex = this.selection.indexes[i];
        var listItem = this.currentList_.getListItemByIndex(selectedIndex);
        if (listItem)
          listItem.querySelector('input[type="checkbox"]').checked = true;
      }
    }
  };

  FileManager.prototype.updateOkButton_ = function(event) {
    var selectable;

    if (this.dialogType_ == FileManager.DialogType.SELECT_FOLDER) {
      selectable = this.selection.directoryCount == 1 &&
          this.selection.fileCount == 0;
    } else if (this.dialogType_ == FileManager.DialogType.SELECT_OPEN_FILE) {
      selectable = (this.selection.directoryCount == 0 &&
                    this.selection.fileCount == 1);
    } else if (this.dialogType_ ==
               FileManager.DialogType.SELECT_OPEN_MULTI_FILE) {
      selectable = (this.selection.directoryCount == 0 &&
                    this.selection.fileCount >= 1);
    } else if (this.dialogType_ == FileManager.DialogType.SELECT_SAVEAS_FILE) {
      if (this.currentDirEntry_.fullPath == '/' ||
          this.currentDirEntry_.fullPath == MEDIA_DIRECTORY) {
        // Nothing can be saved in to the root or media/ directories.
        selectable = false;
      } else {
        selectable = !!this.filenameInput_.value;
      }
    } else if (this.dialogType_ == FileManager.DialogType.FULL_PAGE) {
      // No "select" buttons on the full page UI.
      selectable = true;
    } else {
      throw new Error('Unknown dialog type');
    }

    this.okButton_.disabled = !selectable;
    return selectable;
  };

  /**
   * Handle a double-click event on an entry in the detail list.
   *
   * @param {Event} event The click event.
   */
  FileManager.prototype.onDetailDoubleClick_ = function(event) {
    if (this.renameInput_.currentEntry) {
      // Don't pay attention to double clicks during a rename.
      return;
    }

    var entry = this.selection.leadEntry;
    if (!entry) {
      console.log('Invalid selection');
      return;
    }

    if (entry.isDirectory)
      return this.changeDirectory(entry.fullPath);

    if (!this.okButton_.disabled)
      this.onOk_();

  };

  /**
   * Update the UI when the current directory changes.
   *
   * @param {cr.Event} event The directory-changed event.
   */
  FileManager.prototype.onDirectoryChanged_ = function(event) {
    if (event.saveHistory) {
      history.pushState(this.currentDirEntry_.fullPath,
                        this.currentDirEntry_.fullPath,
                        location.href);
    }

    this.updateOkButton_();
    // New folder should never be enabled in the root or media/ directories.
    this.newFolderButton_.disabled =
        (this.currentDirEntry_.fullPath == '/' ||
         this.currentDirEntry_.fullPath == MEDIA_DIRECTORY);

    this.document_.title = this.currentDirEntry_.fullPath;

    var self = this;
    this.rescanDirectory_(function() {
        if (event.selectedEntry)
          self.selectEntry(event.selectedEntry);
        // For tests that open the dialog to empty directories, everything
        // is loaded at this point.
        chrome.test.sendMessage('directory-change-complete');
      });
  };

  /**
   * Update the UI when a disk is mounted or unmounted.
   *
   * @param {string} path The path that has been mounted or unmounted.
   */
  FileManager.prototype.onDiskChanged_ = function(event) {
    if (event.eventType == 'added') {
      this.changeDirectory(event.volumeInfo.mountPath);
    } else if (event.eventType == 'removed') {
      if (this.currentDirEntry_ &&
          isParentPath(event.volumeInfo.mountPath,
                       this.currentDirEntry_.fullPath)) {
        this.changeDirectory(getParentPath(event.volumeInfo.mountPath));
      }
    }
  };

  /**
   * Rescan the current directory, refreshing the list.
   *
   * @param {function()} opt_callback Optional function to invoke when the
   *     rescan is complete.
   */
  FileManager.prototype.rescanDirectory_ = function(opt_callback) {
    var self = this;
    var reader;

    function onReadSome(entries) {
      if (entries.length == 0) {
        if (opt_callback)
          opt_callback();
        return;
      }

      // Splice takes the to-be-spliced-in array as individual parameters,
      // rather than as an array, so we need to perform some acrobatics...
      var spliceArgs = [].slice.call(entries);

      // Hide files that start with a dot ('.').
      // TODO(rginda): User should be able to override this.  Support for other
      // commonly hidden patterns might be nice too.
      if (self.filterFiles_) {
        spliceArgs = spliceArgs.filter(function(e) {
            return e.name.substr(0, 1) != '.';
          });
      }

      spliceArgs.unshift(0, 0);  // index, deleteCount
      self.dataModel_.splice.apply(self.dataModel_, spliceArgs);

      // Keep reading until entries.length is 0.
      reader.readEntries(onReadSome);
    };

    // Updated when a user clicks on the label of a file, used to detect
    // when a click is eligible to trigger a rename.  Can be null, or
    // an object with 'path' and 'date' properties.
    this.lastLabelClick_ = null;

    // Clear the table first.
    this.dataModel_.splice(0, this.dataModel_.length);

    this.updateBreadcrumbs_();

    if (this.currentDirEntry_.fullPath != '/') {
      // If not the root directory, just read the contents.
      reader = this.currentDirEntry_.createReader();
      reader.readEntries(onReadSome);
      return;
    }

    // Otherwise, use the provided list of root subdirectories, since the
    // real local filesystem root directory (the one we use outside the
    // harness) can't be enumerated yet.
    var spliceArgs = [].slice.call(this.rootEntries_);
    spliceArgs.unshift(0, 0);  // index, deleteCount
    self.dataModel_.splice.apply(self.dataModel_, spliceArgs);

    if (opt_callback)
      opt_callback();
  };

  FileManager.prototype.findListItem_ = function(event) {
    var node = event.srcElement;
    while (node) {
      if (node.tagName == 'LI')
        break;
      node = node.parentNode;
    }

    return node;
  };

  FileManager.prototype.onGridMouseDown_ = function(event) {
    this.updateCommands_();

    if (this.allowRenameClick_(event, event.srcElement.parentNode)) {
      event.preventDefault();
      this.initiateRename_(event.srcElement);
    }

    if (event.button != 1)
      return;

    var li = this.findListItem_(event);
    if (!li)
      return;
  };

  FileManager.prototype.onTableMouseDown_ = function(event) {
    this.updateCommands_();

    if (this.allowRenameClick_(event,
                               event.srcElement.parentNode.parentNode)) {
      event.preventDefault();
      this.initiateRename_(event.srcElement);
    }

    if (event.button != 1)
      return;

    var li = this.findListItem_(event);
    if (!li) {
      console.log('li not found', event);
      return;
    }
  };

  /**
   * Determine whether or not a click should initiate a rename.
   *
   * Renames can happen on mouse click if the user clicks on a label twice,
   * at least a half second apart.
   */
  FileManager.prototype.allowRenameClick_ = function(event, row) {
    if (this.dialogType_ != FileManager.DialogType.FULL_PAGE ||
        this.currentDirEntry_.name == '') {
      // Renaming only enabled for full-page mode, outside of the root
      // directory.
      return false;
    }

    // Rename already in progress.
    if (this.renameInput_.currentEntry)
      return false;

    // Didn't click on the label.
    if (!event.srcElement.classList.contains('filename-label'))
      return false;

    // Wrong button or using a keyboard modifier.
    if (event.button != 0 || event.shiftKey || event.metaKey || event.altKey) {
      this.lastLabelClick_ = null;
      return false;
    }

    var now = new Date();
    var path = event.srcElement.entry.fullPath;

    if (this.lastLabelClick_ && this.lastLabelClick_.path == path) {
      var delay = now - this.lastLabelClick_.date;
      if (delay > 500 && delay < 2000)
        return true;
    }

    this.lastLabelClick_ = {path: path, date: now};
    return false;
  };

  FileManager.prototype.initiateRename_= function(label) {
    var input = this.renameInput_;

    input.value = label.textContent;
    input.style.top = label.offsetTop + 'px';
    input.style.left = label.offsetLeft + 'px';
    input.style.width = label.clientWidth + 'px';
    label.parentNode.appendChild(input);
    input.focus();
    var selectionEnd = input.value.lastIndexOf('.');
    if (selectionEnd == -1) {
      input.select();
    } else {
      input.selectionStart = 0;
      input.selectionEnd = selectionEnd;
    }

    // This has to be set late in the process so we don't handle spurious
    // blur events.
    input.currentEntry = label.entry;
  };

  FileManager.prototype.onRenameInputKeyDown_ = function(event) {
    if (!this.renameInput_.currentEntry)
      return;

    switch (event.keyCode) {
      case 27:  // Escape
        this.cancelRename_();
        event.preventDefault();
        break;

      case 13:  // Enter
        this.commitRename_();
        event.preventDefault();
        break;
    }
  };

  FileManager.prototype.onRenameInputBlur_ = function(event) {
    if (this.renameInput_.currentEntry)
      this.cancelRename_();
  };

  FileManager.prototype.commitRename_ = function() {
    var entry = this.renameInput_.currentEntry;
    var newName = this.renameInput_.value;
    if (!this.validateFileName_(newName, entry.isDirectory))
      return;

    this.renameInput_.currentEntry = null;
    this.lastLabelClick_ = null;

    if (this.renameInput_.parentNode)
      this.renameInput_.parentNode.removeChild(this.renameInput_);

    var self = this;
    function onSuccess() {
      self.rescanDirectory_(function() { self.selectEntry(newName) });
    }

    function onError(err) {
      window.alert(strf('ERROR_RENAMING', entry.name,
                        util.getFileErrorMnemonic(err.code)));
    }

    function resolveCallback(victim) {
      if (victim instanceof FileError) {
        entry.moveTo(self.currentDirEntry_, newName, onSuccess, onError);
      } else {
        var message = victim.isFile ?
            'FILE_ALREADY_EXISTS':
            'DIRECTORY_ALREADY_EXISTS';
        window.alert(strf(message, newName));
      }
    }

    this.resolvePath(this.currentDirEntry_.fullPath + '/' + newName,
        resolveCallback, resolveCallback);
  };

  FileManager.prototype.cancelRename_ = function(event) {
    this.renameInput_.currentEntry = null;
    this.lastLabelClick_ = null;

    if (this.renameInput_.parentNode)
      this.renameInput_.parentNode.removeChild(this.renameInput_);
  };

  FileManager.prototype.onFilenameInputKeyUp_ = function(event) {
    var enabled = this.updateOkButton_();
    if (enabled && event.keyCode == 13 /* Enter */)
      this.onOk_();
  };

  FileManager.prototype.onFilenameInputFocus_ = function(event) {
    var input = this.filenameInput_;

    // On focus we want to select everything but the extension, but
    // Chrome will select-all after the focus event completes.  We
    // schedule a timeout to alter the focus after that happens.
    setTimeout(function() {
        var selectionEnd = input.value.lastIndexOf('.');
        if (selectionEnd == -1) {
          input.select();
        } else {
          input.selectionStart = 0;
          input.selectionEnd = selectionEnd;
        }
    }, 0);
  };

  FileManager.prototype.onNewFolderButtonClick_ = function(event) {
    var name = '';

    while (1) {
      name = window.prompt(str('NEW_FOLDER_PROMPT'), name);
      if (name == '') {
        window.alert(str('ERROR_NEW_FOLDER_EMPTY_NAME'));
        continue;
      } else if (!name) {
        return;
      }

      if (this.validateFileName_(name, true)) break;
    }

    var self = this;

    function onSuccess(dirEntry) {
      self.rescanDirectory_(function() { self.selectEntry(name) });
    }

    function onError(err) {
      window.alert(strf('ERROR_CREATING_FOLDER', name,
                        util.getFileErrorMnemonic(err.code)));
    }

    this.currentDirEntry_.getDirectory(name, {create: true, exclusive: true},
                                       onSuccess, onError);
  };

  FileManager.prototype.onDetailViewButtonClick_ = function(event) {
    this.setListType(FileManager.ListType.DETAIL);
  };

  FileManager.prototype.onThumbnailViewButtonClick_ = function(event) {
    this.setListType(FileManager.ListType.THUMBNAIL);
  };

  /**
   * KeyDown event handler for the div.list-container element.
   */
  FileManager.prototype.onListKeyDown_ = function(event) {
    if (event.srcElement.tagName == 'INPUT') {
      // Ignore keydown handler in the rename input box.
      return;
    }

    switch (event.keyCode) {
      case 8:  // Backspace => Up one directory.
        event.preventDefault();
        var path = this.currentDirEntry_.fullPath;
        if (path && path != '/') {
          var path = path.replace(/\/[^\/]+$/, '');
          this.changeDirectory(path || '/');
        }
        break;

      case 13:  // Enter => Change directory or complete dialog.
        if (this.selection.totalCount == 1 &&
            this.selection.leadEntry.isDirectory &&
            this.dialogType_ != FileManager.SELECT_FOLDER) {
          event.preventDefault();
          this.changeDirectory(this.selection.leadEntry.fullPath);
        } else if (!this.okButton_.disabled) {
          event.preventDefault();
          this.onOk_();
        }
        break;

      case 27:  // Escape => Cancel dialog.
        if (this.dialogType_ != FileManager.DialogType.FULL_PAGE) {
          event.preventDefault();
          this.onCancel_();
        }
        break;

      case 32:  // Ctrl-Space => New Folder.
        if (this.newFolderButton_.style.display != 'none' && event.ctrlKey) {
          event.preventDefault();
          this.onNewFolderButtonClick_();
        }
        break;

      case 190:  // Ctrl-. => Toggle filter files.
        if (event.ctrlKey) {
          this.filterFiles_ = !this.filterFiles_;
          this.rescanDirectory_();
        }
        break;

      case 46:  // Delete.
        if (this.dialogType_ == FileManager.DialogType.FULL_PAGE &&
            this.selection.totalCount > 0) {
          event.preventDefault();
          this.deleteEntries(this.selection.entries);
        }
        break;
    }
  };

  /**
   * Handle a click of the cancel button.  Closes the window.
   *
   * @param {Event} event The click event.
   */
  FileManager.prototype.onCancel_ = function(event) {
    // Closes the window and does not return.
    chrome.fileBrowserPrivate.cancelDialog();
  };

  /**
   * Handle a click of the ok button.
   *
   * The ok button has different UI labels depending on the type of dialog, but
   * in code it's always referred to as 'ok'.
   *
   * @param {Event} event The click event.
   */
  FileManager.prototype.onOk_ = function(event) {
    var currentDirUrl = this.currentDirEntry_.toURL();

    if (currentDirUrl.charAt(currentDirUrl.length - 1) != '/')
      currentDirUrl += '/';

    if (this.dialogType_ == FileManager.DialogType.SELECT_SAVEAS_FILE) {
      // Save-as doesn't require a valid selection from the list, since
      // we're going to take the filename from the text input.
      var filename = this.filenameInput_.value;
      if (!filename)
        throw new Error('Missing filename!');
      if (!this.validateFileName_(filename, false))
        return;

      var self = this;
      function resolveCallback(victim) {
        if (!(victim instanceof FileError)) {
          if (victim.isDirectory) {
            // Do not allow to overwrite directory.
            window.alert(strf('DIRECTORY_ALREADY_EXISTS', filename));
            return;
          }
          if (!window.confirm(strf('CONFIRM_OVERWRITE_FILE', filename)))
            return;
        }

        // Closes the window and does not return.
        chrome.fileBrowserPrivate.selectFile(
            currentDirUrl + encodeURIComponent(filename),
            self.getSelectedFilterIndex_(filename));
      }

      this.resolvePath(this.currentDirEntry_.fullPath + '/' + filename,
          resolveCallback, resolveCallback);
      return;
    }

    var ary = [];
    var selectedIndexes = this.currentList_.selectionModel.selectedIndexes;

    // All other dialog types require at least one selected list item.
    // The logic to control whether or not the ok button is enabled should
    // prevent us from ever getting here, but we sanity check to be sure.
    if (!selectedIndexes.length)
      throw new Error('Nothing selected!');

    for (var i = 0; i < selectedIndexes.length; i++) {
      var entry = this.dataModel_.item(selectedIndexes[i]);
      if (!entry) {
        console.log('Error locating selected file at index: ' + i);
        continue;
      }

      ary.push(currentDirUrl + encodeURIComponent(entry.name));
    }

    // Multi-file selection has no other restrictions.
    if (this.dialogType_ == FileManager.DialogType.SELECT_OPEN_MULTI_FILE) {
      // Closes the window and does not return.
      chrome.fileBrowserPrivate.selectFiles(ary);
      return;
    }

    // In full screen mode, open all files for vieweing.
    if (this.dialogType_ == FileManager.DialogType.FULL_PAGE) {
      chrome.fileBrowserPrivate.viewFiles(ary, "default");
      // Window stays open.
      return;
    }

    // Everything else must have exactly one.
    if (ary.length > 1)
      throw new Error('Too many files selected!');

    if (this.dialogType_ == FileManager.DialogType.SELECT_FOLDER) {
      if (!this.selection.leadEntry.isDirectory)
        throw new Error('Selected entry is not a folder!');
    } else if (this.dialogType_ == FileManager.DialogType.SELECT_OPEN_FILE) {
      if (!this.selection.leadEntry.isFile)
        throw new Error('Selected entry is not a file!');
    }

    // Closes the window and does not return.
    chrome.fileBrowserPrivate.selectFile(
        ary[0], this.getSelectedFilterIndex_(ary[0]));
  };

  /**
   * Verifies the user entered name for file or folder to be created or
   * renamed to. Name restrictions must correspond to File API restrictions
   * (see DOMFilePath::isValidPath). Curernt WebKit implementation is
   * out of date (spec is
   * http://dev.w3.org/2009/dap/file-system/file-dir-sys.html, 8.3) and going to
   * be fixed. Shows message box if the name is invalid.
   *
   * @param {name} name New file or folder name.
   * @param {boolean} isFolder If true error message will be adjusted for
   *                           folders.
   * @return {boolean} True if name is vaild.
   */
  FileManager.prototype.validateFileName_ = function(name, isFolder) {
    if (name.length == 0) {
      return false;
    }
    var testResult = /[\/\\\<\>\:\?\*\"\|]/.exec(name);
    if (testResult) {
      var msgId = isFolder ? 'ERROR_INVALID_FOLDER_CHARACTER' :
                             'ERROR_INVALID_FILE_CHARACTER';
      window.alert(strf(msgId, testResult[0]));
      return false;
    }
    if (/^\s*$/i.test(name)) {
      window.alert(str('ERROR_WHITESPACE_NAME'));
      return false;
    }
    if (/^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$/i.test(name)) {
      window.alert(str('ERROR_RESERVED_NAME'));
      return false;
    }
    if (this.filterFiles_ && name[0] == '.') {
      window.alert(str('ERROR_HIDDEN_NAME'));
      return false;
    }
    return true;
  };
})();
