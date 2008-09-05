// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

import java.io.File;
import java.util.Collection;
import java.util.SortedMap;

/**
 * The collected result of running a whole benchmark.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class CollectedBenchmarkResult {
  
  private final SortedMap<File, FileBenchmarkResult> results;
  
  public CollectedBenchmarkResult(SortedMap<File, FileBenchmarkResult> results) {
    this.results = results;
  }
  
  public FileBenchmarkResult getResult(File file) {
    return results.get(file);
  }
  
  public Collection<FileBenchmarkResult> getResults() {
    return this.results.values();
  }
  
  public double getMean(Runtime runtime, Runtime compared) {
    double product = 1;
    int count = 0;
    for (FileBenchmarkResult fileResults : this.results.values()) {
      RuntimeBenchmarkResults runtimeResults = fileResults.getResult(runtime);
      RuntimeBenchmarkResults comparedResults = fileResults.getResult(compared);
      double runtimeMean = runtimeResults.getMean(comparedResults);
      if (!Double.isNaN(runtimeMean)) {
        product *= runtimeMean;
        count++;
      }
    }
    return Math.pow(product, 1.0 / count);
  }

}
