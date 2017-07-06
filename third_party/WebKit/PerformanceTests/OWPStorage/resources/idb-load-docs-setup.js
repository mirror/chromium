// Our database schema:
// Users
// * key - string
// * value - dictionary w/ strings and an array
// * 1 entry
// DocumentLocks
// * key - array w/ one string item
// * value - dictionary w/ string, number, and array of string
// * 469 entries
// Documents
// * key - string
// * value - dictionary, w/ nested dictionaries, strings, numbers, arrays
//   (one of which has lots of items - numbers, strings, further arrays)
// * 730 entries
// PendingQueues
// * key - string
// * value - dictionary w/ empty array, strings, numbers, bools, undefineds
// * 730 entries
// DocumentCommands
// * key - array
// * value - everything! large.
// * 2000 entries - we only do 20 to make things not too long for startup.
// PendingQueueCommands
// * empty
// SyncObjects
// * key - array of strings
// * value - dictionary w/ dictionaries, keypath, and timestamp
// * 55 entries
// FontMetadata
// * key - string of font name
// * value - dictionary of arrays of dictionaries. strings, numbers, etc
let populate_fake_docs_database = function(db) {

  function random_alpha_num(length) {
    const chars = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-';
    let result = '';
    for (let i = length; i > 0; --i)
      result += chars[Math.floor(Math.random() * chars.length)];
    return result;
  }

  let other_doc_ids = [];
  for (let i = 0; i < 729; i++) {
    other_doc_ids.push(random_alpha_num(44));
  }

  let populate_user = function() {
    let users = db.createObjectStore("Users", {
      keyPath: "id"
    });
    users.put(Users_value);
  }

  let populate_document_locks = function() {
    let documentLocks = db.createObjectStore("DocumentLocks", {
      keyPath: "dlKey"
    });
    documentLocks.put(DocumentLocks_value);
    DocumentLocks_value.dlKey = [doc_id2];
    documentLocks.put(DocumentLocks_value);

    for (let other_doc_id of other_doc_ids) {
      DocumentLocks_value.id = other_doc_id;
      documentLocks.put(DocumentLocks_value);
    }
  }

  let populate_documents = function() {
    // first put our 2 docs, then copy the rest.
    let documents = db.createObjectStore("Documents", {
      keyPath: "id"
    });
    documents.put(Documents1_value);
    documents.put(Documents2_value);

    for (let other_doc_id of other_doc_ids) {
      Documents2_value.id = other_doc_id;
      documents.put(Documents2_value);
    }
  }

  let populate_document_commands = function() {
    let commands = db.createObjectStore("DocumentCommands", {
      keyPath: "dcKey"
    });
    commands.put(t17_DocumentCommands_value);

    for (let i = 0; i < 50; i++) {
      t17_DocumentCommands_value.dcKey[0] = random_alpha_num(44);
      commands.put(t17_DocumentCommands_value);
    }
  }

  let populate_pending_queues = function() {
    // first put our 2 docs, then copy the rest.
    let queues = db.createObjectStore("PendingQueues", {
      keyPath: "docId"
    });
    queues.put(PendingQueues1_value);
    queues.put(PendingQueues2_value);

    for (let other_doc_id of other_doc_ids) {
      PendingQueues2_value.id = other_doc_id;
      queues.put(PendingQueues2_value);
    }
  }

  let create_pending_queue_commands = function() {
    db.createObjectStore("PendingQueueCommands");
  }

  let populate_sync_objects = function() {
    let sync_objects = db.createObjectStore("SyncObjects", {
      keyPath: "keyPath"
    });
    sync_objects.put(SyncObjects1_value);
    sync_objects.put(SyncObjects2_value);
    sync_objects.put(SyncObjects3_value);
    sync_objects.put(SyncObjects4_value);
    for (let i = 0; i < 51; ++i) {
      SyncObjects1_value.keyPath[2] = random_alpha_num(10);
      sync_objects.put(SyncObjects4_value);
    }
  }

  let populate_font_metadata = function() {
    let fonts = db.createObjectStore("FontMetadata", {
      keyPath: "fontFamily"
    });
    fonts.put(FontMetadata1_value);
    fonts.put(FontMetadata2_value);

    for (let i = 0; i < 148; ++i) {
      FontMetadata1_value.fontFamily = random_alpha_num(10);
      fonts.put(FontMetadata1_value);
    }
  }

  let create_blob_metadata = function() {
    db.createObjectStore("BlobMetadata", {
      keyPath: ["d", "p"]
    });
  }

  populate_user();
  populate_document_locks();
  populate_documents();
  populate_document_commands();
  populate_pending_queues();
  create_pending_queue_commands();
  populate_sync_objects();
  populate_font_metadata();
  create_blob_metadata();
}
