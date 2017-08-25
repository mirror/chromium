var blobs = [];
var totalBytes = 0;
var errors = [];

function recordError(message) {
  console.log(message);
  errors.push(message);

  var error = document.createElement('div');
  error.text = message;
  document.body.appendChild(error);
}

function createAndRead(size) {
  var reader = new FileReader();
  var blob = new Blob([new Uint8Array(size)], {type: 'application/octet-string'});

  return new Promise(resolve => {
    reader.onloadend = function(e) {
      if (reader.error)
        recordError('Error reading blob: ' + reader.error);
      else if (reader.result.byteLength != size)
        recordError('Error reading blob: Blob size does not match.');
      resolve();
    };
    reader.readAsArrayBuffer(blob);
  });
}

function createBlob(size) {
  var blob = new Blob([new Uint8Array(size)], {type: 'application/octet-string'});
  blobs.push(blob);
  totalBytes += size;
}

function garbageCollect() {
  if (typeof(Blob.prototype.close) !== 'undefined')
    blobs.map(blob => { blob.close(); });

  blobs = [];
  totalBytes = 0;
}

function readBlobsSerially() {
  if (blobs.length == 0)
    return;
  var reader = new FileReader();
  var currentBlob = 0;
  var totalReadSize = 0;

  return new Promise(resolve => {
    reader.onloadend = function(e) {
      if (reader.error) {
        recordError('Error reading blob ' + currentBlob + ': ' + reader.error);
        resolve();
        return;
      }

      totalReadSize += reader.result.byteLength;
      currentBlob++;

      if (currentBlob < blobs.length)
        reader.readAsArrayBuffer(blobs[currentBlob]);
      else {
        if (totalReadSize != totalBytes)
          recordError('Error reading blob: Total blob sizes do not match, ' + totalReadSize + ' vs ' + totalBytes);
        resolve();
      }
    }
    reader.readAsArrayBuffer(blobs[currentBlob]);
  });
}

function readBlobsInParallel() {
  if (blobs.length == 0)
    return;

  var currentBlob = 0;
  var totalReadSize = 0;
  var numRead = 0;

  var promises = blobs.map(blob => new Promise(resolve => {
    var genReader = function(index) {
      var reader = new FileReader();
      reader.onloadend = function(e) {
        if (reader.error) {
          recordError('Error reading blob ' + index + ': ' + reader.error);
          resolve();
          return;
        }

        totalReadSize += Number(reader.result.byteLength);
        numRead++;

        if (numRead >= blobs.length && totalReadSize != totalBytes)
          recordError('Error reading blob: Total blob sizes do not match, ' + totalReadSize + ' vs ' + totalBytes);
        resolve();
      }
      return reader;
    }
    genReader(currentBlob).readAsArrayBuffer(blobs[currentBlob]);
  }));
  return Promise.all(promises);
}

async function blobCreateAndImmediatelyRead(numBlobs, size) {
  var start = performance.now();
  errors = [];

  logToDocumentBody('Creating and reading ' + numBlobs + ' blobs...');
  for (var i = 0; i < numBlobs; i++)
    await createAndRead(size);
  logToDocumentBody('Finished.');

  if (errors.length)
    window.PerfTestRunner.logFatalError('Errors on page: ' + errors.join(', '));
}

async function blobCreateAllThenReadSerially(numBlobs, size) {
  errors = [];

  logToDocumentBody('Creating ' + numBlobs + ' blobs...');
  for (var i = 0; i < numBlobs; i++)
    createBlob(size);
  logToDocumentBody('Finished creating.');

  logToDocumentBody('Reading ' + numBlobs + ' blobs serially...');
  await readBlobsSerially();
  logToDocumentBody('Finished reading.');

  garbageCollect();
  if (errors.length)
    window.PerfTestRunner.logFatalError('Errors on page: ' + errors.join(', '));
}

async function blobCreateAllThenReadInParallel(numBlobs, size) {
  errors = [];

  logToDocumentBody('Creating ' + numBlobs + ' blobs...');
  for (var i = 0; i < numBlobs; i++)
    createBlob(size);
  logToDocumentBody('Finished creating.');

  logToDocumentBody('Reading ' + numBlobs + ' blobs in parallel...');
  await readBlobsInParallel();
  logToDocumentBody('Finished reading.');

  garbageCollect();
  if (errors.length)
    window.PerfTestRunner.logFatalError('Errors on page: ' + errors.join(', '));
}
