

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
// * value - dictionary, w/ nested dictionaries, strings, numbers, arrays (one of which has lots of items - numbers, strings, further arrays)
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
var populate_fake_docs_database = function(db) {

  var doc_id1 = "19OmSKoWTJrUPwoA3G2-ZHe6jPMw2voDpyStgHFXH7qI";
  var doc_id2 = "1HybJ594lZzyzPzZGCgf1JFwXA5rPx5Z0bO19glO-a5k";

  function random_alpha_num(length) {
    var chars = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-';
    var result = '';
    for (var i = length; i > 0; --i) result += chars[Math.floor(Math.random() * chars.length)];
    return result;
  }

  var other_doc_ids = [];
  for (let i = 0; i < 729; i++) {
    other_doc_ids.push(random_alpha_num(44));
  }


  var populate_user = function() {
    var Users_value = {"id":"ufa1a865c6eb48d97","fastTrack":"true","emailAddress":"dmurph@google.com","locale":"en","internal":"true","optInReasons":[1]}
    var users = db.createObjectStore("Users", { keyPath: "id" });
    users.put(Users_value);
  }

  var populate_document_locks = function() {
    var DocumentLocks_value = {"e":1497567754647,"dlKey":[doc_id1],"sId":"38033a00ec4b057f"}

    var documentLocks = db.createObjectStore("DocumentLocks", { keyPath: "dlKey"});
    documentLocks.put(DocumentLocks_value);
    DocumentLocks_value.dlKey = [doc_id2];
    documentLocks.put(DocumentLocks_value);

    for (let other_doc_id of other_doc_ids) {
      DocumentLocks_value.id = other_doc_id;
      documentLocks.put(DocumentLocks_value);
    }
  }

  var populate_documents = function() {
    var Documents1_value = {"id":doc_id1,"documentType":"kix","hpmdo":"","relevancyRank":120,"jobset":"prod","isFastTrack":"true","lastSyncedTimestamp":1497566293056,"lsst":1497566291952,"title":"Blob Storage Refactor","ind":"","lastModifiedServerTimestamp":1493420139817,"modelVersion":0,"featureVersion":0,"inc":"","acl":{"ufa1a865c6eb48d97":4},"docosKeyData":[null,null,0,"",null,"0",[null,1,null,1,1,1],null,["Daniel Murphy",null,"//lh5.googleusercontent.com/-7TvIUEovWGE/AAAAAAAAAAI/AAAAAAAABUs/WoZoPBOY_iw/s96-k-no/photo.jpg","100317903111359348790",1,null,0,"dmurph@google.com"],1,doc_id1,null,1,null,null,null,0,1,1,1,null,null,null,null,1,1,"/comments/u/100317903111359348790/c",0,null,1,null,null,1,null,null,null,1,1,1,1,0,null,0,["tf",60000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],[[5700016,5700019,5700028,5700032,5700042,5700057,5700058,5700085,5700100,5700103,5700105,5700114,5700127,5700130,5700136,5700146,5700162,5700180,5700187,5700189,5700213,5700248,5700250,5700258,5700266,5700286,5700333,5700350,5700362,5700386,5700394,5700410,5700446,5700470,5700505,5700527,5700547,5700551,5700567,5700588,5700591,5700600,5700611,5700634,5700638,5700650,5700658,5700668,5700672,5700684,5700784,5700788,5700808,5700838,5700892,5700900,5700908,5700933,5700937,5700962,5700978,5700982,5701022,5701054,5701062,5701067,5701075,5701143,5701167,5701176,5701192,5701204,5701224,5701244,5701248,5701264,5701284,5701288,5701302,5701323,5701358,5701373,5701401,5701442,5701540]],125],"ips":"","snapshotState":1,"lastServerSnapshotTimestamp":1497566292898,"rev":818,"rai":["34WZV82YtTmOtA"],"lss":"true","lsft":1497566292527,"lastWarmStartedTimestamp":1497566292882,"pendingQueueState":1,"startupHints":{"fontFamilies":"[]"}}
    var Documents2_value = {"id":doc_id2,"documentType":"kix","hpmdo":"","relevancyRank":121,"jobset":"prod","isFastTrack":"true","lastSyncedTimestamp":1497566108980,"lsst":1497566107286,"title":"Blob Storage Refactor Plan/Requirements (bit.ly/BlobPlan)","ind":"","lastModifiedServerTimestamp":1457989229859,"modelVersion":0,"featureVersion":0,"inc":"","acl":{"ufa1a865c6eb48d97":2},"docosKeyData":[null,null,0,"",null,"0",[null,1,null,1,1,1],null,["Daniel Murphy",null,"//lh5.googleusercontent.com/-7TvIUEovWGE/AAAAAAAAAAI/AAAAAAAABUs/WoZoPBOY_iw/s96-k-no/photo.jpg","100317903111359348790",1,null,0,"dmurph@google.com"],1,doc_id2,null,1,null,null,null,0,1,1,1,null,null,null,null,1,1,"/comments/u/100317903111359348790/c",0,null,1,null,null,1,null,null,null,1,1,1,1,0,null,0,["tf",60000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],[[5700016,5700019,5700028,5700032,5700042,5700057,5700058,5700085,5700100,5700103,5700105,5700114,5700127,5700130,5700136,5700146,5700162,5700180,5700187,5700189,5700213,5700248,5700250,5700258,5700266,5700286,5700333,5700350,5700362,5700386,5700394,5700410,5700446,5700470,5700505,5700527,5700547,5700551,5700567,5700588,5700591,5700600,5700611,5700634,5700638,5700650,5700658,5700668,5700672,5700684,5700784,5700788,5700808,5700838,5700892,5700900,5700908,5700933,5700937,5700962,5700978,5700982,5701022,5701054,5701062,5701067,5701075,5701143,5701167,5701176,5701192,5701204,5701224,5701244,5701248,5701264,5701284,5701288,5701302,5701323,5701358,5701373,5701401,5701442,5701540]],125],"startupHints":{"fontFamilies":"[\"Consolas\"]"},"ips":"","snapshotState":1,"lastServerSnapshotTimestamp":1497566108405,"rev":2142,"rai":["3WTgfg5N4LGHwA"],"lss":"true","lsft":1497566108980,"lastWarmStartedTimestamp":1497561959618,"pendingQueueState":1}

    // first put our 2 docs, then copy the rest.
    var documents = db.createObjectStore("Documents", { keyPath: "id"});
    documents.put(Documents1_value);
    documents.put(Documents2_value);

    for (let other_doc_id of other_doc_ids) {
      Documents2_value.id = other_doc_id;
      documents.put(Documents2_value);
    }
  }

  var populate_document_commands = function() {
    var commands = db.createObjectStore("DocumentCommands", { keyPath: "dcKey"});
    commands.put(t17_DocumentCommands_value);

    for (let i = 0; i < 50; i++) {
      t17_DocumentCommands_value.dcKey[0] = random_alpha_num(44);
      commands.put(t17_DocumentCommands_value);
    }
  }

  var populate_pending_queues = function() {
    var PendingQueues1_value = {"docId":doc_id1,"r":818,"ra":["34WZV82YtTmOtA"],"b":[],"t":"kix","u":false,"uc":false,"a":4}
    var PendingQueues2_value = {"docId":doc_id2,"r":2142,"ra":["3WTgfg5N4LGHwA"],"b":[],"t":"kix","u":false,"uc":false,"a":2}
    
    // first put our 2 docs, then copy the rest.
    var queues = db.createObjectStore("PendingQueues", { keyPath: "docId"});
    queues.put(PendingQueues1_value);
    queues.put(PendingQueues2_value);

    for (let other_doc_id of other_doc_ids) {
      PendingQueues2_value.id = other_doc_id;
      queues.put(PendingQueues2_value);
    }
  }

  var create_pending_queue_commands = function() {
    db.createObjectStore("PendingQueueCommands");
  }

  var populate_sync_objects = function () {
    var SyncObjects1_value = {"keyPath":["syncMap","preferences","docs-show_smart_todo","1"],"state":{"timestamp":1476131907463159},"data":{"value":false}}
    var SyncObjects2_value = {"keyPath":["syncMap","preferences","docs-voice_tips_shown"],"state":{"timestamp":1456437957716583},"data":{"value":true}}
    var SyncObjects3_value = {"keyPath":["syncMap","domainFonts","2"],"state":{"hashValue":"00000000"},"data":{"value":[]}}
    var SyncObjects4_value = {"keyPath":["syncMap","domainFonts","5"],"state":{"hashValue":"00000000"},"data":{"value":[]}}

    var sync_objects = db.createObjectStore("SyncObjects", { keyPath: "keyPath" });
    sync_objects.put(SyncObjects1_value);
    sync_objects.put(SyncObjects2_value);
    sync_objects.put(SyncObjects3_value);
    sync_objects.put(SyncObjects4_value);
    for (let i = 0; i < 51; ++i) {
      SyncObjects1_value.keyPath[2] = random_alpha_num(10);
      sync_objects.put(SyncObjects4_value);
    }
  }

  var populate_font_metadata = function() {
    var FontMetadata1_value = {"fontFamily":"Cambria","version":"v8","fontFaces":[{"fontFamily":"Cambria","subset":"LATIN","isMenuFont":false,"source":[{"format":"woff2","isSystemFont":true,"url":"Cambria"},{"format":"woff2","isSystemFont":false,"url":"filesystem:https://docs.google.com/persistent/docs/fonts/kTvgLiCA8aQWSqmAzkL5Ig.woff2"}],"style":"normal","weight":400},{"fontFamily":"Cambria","subset":"LATIN","isMenuFont":false,"source":[{"format":"woff2","isSystemFont":true,"url":"Cambria Italic"},{"format":"woff2","isSystemFont":true,"url":"Cambria-Italic"},{"format":"woff2","isSystemFont":false,"url":"filesystem:https://docs.google.com/persistent/docs/fonts/MD9mg8Cy2AOZQTj1RMSKultXRa8TVwTICgirnJhmVJw.woff2"}],"style":"italic","weight":400},{"fontFamily":"Cambria","subset":"LATIN","isMenuFont":false,"source":[{"format":"woff2","isSystemFont":true,"url":"Cambria Bold"},{"format":"woff2","isSystemFont":true,"url":"Cambria-Bold"},{"format":"woff2","isSystemFont":false,"url":"filesystem:https://docs.google.com/persistent/docs/fonts/wkshsMjunzKumD3zh1j69fk_vArhqVIZ0nv9q090hN8.woff2"}],"style":"normal","weight":700},{"fontFamily":"Cambria","subset":"LATIN","isMenuFont":false,"source":[{"format":"woff2","isSystemFont":true,"url":"Cambria Bold Italic"},{"format":"woff2","isSystemFont":true,"url":"Cambria-BoldItalic"},{"format":"woff2","isSystemFont":false,"url":"filesystem:https://docs.google.com/persistent/docs/fonts/v58W6z8cm42h92CJRCAnu-gdm0LZdjqr5-oayXSOefg.woff2"}],"style":"italic","weight":700},{"fontFamily":"Cambria--Menu","subset":"MENU","isMenuFont":true,"source":[{"format":"woff2","isSystemFont":false,"url":"filesystem:https://docs.google.com/persistent/docs/fonts/sbviuG-CrcABsuQ0sUqA2Q.woff2"}],"style":"normal","weight":400}]}
    var FontMetadata2_value = {"fontFamily":"Cambria|*","version":"v8","fontFaces":[{"fontFamily":"Cambria","subset":"*","isMenuFont":false,"source":[{"format":"woff2","isSystemFont":true,"url":"Cambria"},{"format":"woff2","isSystemFont":false,"url":"filesystem:https://docs.google.com/persistent/docs/fonts/133SsNFRLekIfsI-p7dGDA.woff2"}],"style":"normal","weight":400},{"fontFamily":"Cambria","subset":"*","isMenuFont":false,"source":[{"format":"woff2","isSystemFont":true,"url":"Cambria Italic"},{"format":"woff2","isSystemFont":true,"url":"Cambria-Italic"},{"format":"woff2","isSystemFont":false,"url":"filesystem:https://docs.google.com/persistent/docs/fonts/QsRvbygpx3GqovnubwJCpA.woff2"}],"style":"italic","weight":400},{"fontFamily":"Cambria","subset":"*","isMenuFont":false,"source":[{"format":"woff2","isSystemFont":true,"url":"Cambria Bold"},{"format":"woff2","isSystemFont":true,"url":"Cambria-Bold"},{"format":"woff2","isSystemFont":false,"url":"filesystem:https://docs.google.com/persistent/docs/fonts/7z3COyp-8Ysy-M17sN2-OPesZW2xOQ-xsNqO47m55DA.woff2"}],"style":"normal","weight":700},{"fontFamily":"Cambria","subset":"*","isMenuFont":false,"source":[{"format":"woff2","isSystemFont":true,"url":"Cambria Bold Italic"},{"format":"woff2","isSystemFont":true,"url":"Cambria-BoldItalic"},{"format":"woff2","isSystemFont":false,"url":"filesystem:https://docs.google.com/persistent/docs/fonts/v58W6z8cm42h92CJRCAnu_k_vArhqVIZ0nv9q090hN8.woff2"}],"style":"italic","weight":700},{"fontFamily":"Cambria--Menu","subset":"menu","isMenuFont":true,"source":[{"format":"woff2","isSystemFont":false,"url":"filesystem:https://docs.google.com/persistent/docs/fonts/sbviuG-CrcABsuQ0sUqA2Q.woff2"}],"style":"normal","weight":400}]}

    var fonts = db.createObjectStore("FontMetadata", { keyPath: "fontFamily" });
    fonts.put(FontMetadata1_value);
    fonts.put(FontMetadata2_value);

    for (let i = 0; i < 148; ++i) {
      FontMetadata1_value.fontFamily = random_alpha_num(10);
      fonts.put(FontMetadata1_value);
    }
  }

  var create_blob_metadata = function() {
    db.createObjectStore("BlobMetadata", { keyPath: ["d", "p"]});
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