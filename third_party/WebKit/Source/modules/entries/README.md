# File and Directory Entries API

Draft spec: https://wicg.github.io/entries-api/

This represents the subset of the File System API (see modules/filesystem)
that was implemented by other browsers to support the "directory upload"
scenario (webkitdirectory attribute and directory drag/drop). Other browsers
based their implementation on Chrome's API but with a handful of differences
around interface names, error types, and so forth.

The implementation here relies on the same underlying file system services
but provides the consensus names for the types when used in the form-based
flows and provides clearer error names.

Modifying the existing modules/filesystem types proves challenging as they
are used by chrome.* APIs for apps/extensions, devtools, and the closure
compiler's externs, so a parallel front-end was added.

* DataTransferItemFileSystem - extension to DataTransferItem that provides
    the `webkitGetAsEntry` method.

* HTMLInputElementFileSystem - extension to HTMLInputElement that provides
    the `webkitEntries` property.

* FileSystem, FileSystemEntry, FileSystemFileEntry, FileSystemDirectoryEntry
    are the script-exposed interfaces representing file system elements.

* FileSystemDirectoryReader is a script-exposed interface allowing for
    iterating directories via callbacks.

* FileCallback, FileSystemEntryCallback, FileSystemEntriesCallback are
    callbacks implemented by script. (The spec predates Promises.)

* FileSystemEntryCallbacks implements internal handlers for calls to the
    Platform calls into the FileSystem service. Sorry about the naming.
