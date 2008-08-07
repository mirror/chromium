// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

import java.io.File;
import java.util.Collection;
import java.util.SortedMap;
/**
 * The result of running all runtimes on a single file.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class FileBenchmarkResult {
  
  private final File file;
  private final SortedMap<Runtime, RuntimeBenchmarkResults> results;
  
  public FileBenchmarkResult(File file, SortedMap<Runtime, RuntimeBenchmarkResults> results) {
    this.file = file;
    this.results = results;
  }
  
  public RuntimeBenchmarkResults getResult(Runtime runtime) {
    return results.get(runtime);
  }
  
  public File getFile() {
    return file;
  }
  
  public Collection<RuntimeBenchmarkResults> getResults() {
    return this.results.values();
  }

}
