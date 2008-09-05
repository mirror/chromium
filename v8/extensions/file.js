(function () {

  var global = this;

  global.io = { };
  
  global.io.File = File;
  function File(path) {
    if (path) this.path = String(path);
    else this.path = '.';
  }
  
  File.prototype.toString = function () {
    return 'File(' + this.path + ')';
  }
  
  native function FileExists(name);
  File.exists = FileExists;
  File.prototype.exists = function () {
    return File.exists(this.path);
  }
  
  native function FileRead(name);
  File.read = FileRead;
  File.prototype.read = function () {
    return File.read(this.path);
  }
  
  native function FileIsDirectory(name);
  File.isDirectory = FileIsDirectory;
  File.prototype.isDirectory = function () {
    return File.isDirectory(this.path);
  }
  
  native function FileList(name);
  File.list = function (name) {
    var result = FileList(name);
    for (var i = 0; i < result.length; i++)
      result[i] = new File(result[i]);
    return result;
  }
  File.prototype.list = function () {
    return File.list(this.path);
  }
  
  native function FileSeparator();
  var separator;
  File.__defineGetter__('separator', function () {
    if (!separator) separator = FileSeparator();
    return separator;
  });

})();
