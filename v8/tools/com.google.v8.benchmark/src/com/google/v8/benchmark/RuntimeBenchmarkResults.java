// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

import java.util.Collection;
import java.util.Set;
import java.util.SortedMap;

/**
 * The results of running a file on single runtime.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class RuntimeBenchmarkResults {
  
  private final Runtime runtime;
  private final SortedMap<String, SingleBenchmarkResult> results;
  
  public RuntimeBenchmarkResults(Runtime runtime,
      SortedMap<String, SingleBenchmarkResult> results) {
    this.runtime = runtime;
    this.results = results;
  }
  
  public Collection<SingleBenchmarkResult> getResults() {
    return results.values();
  }
  
  public Runtime getRuntime() {
    return this.runtime;
  }
  
  public SingleBenchmarkResult getResult(String cahse) {
    return results.get(cahse);
  }

  public double getMean(RuntimeBenchmarkResults compared) {
    double product = 1;
    int count = 0;
    for (SingleBenchmarkResult single : results.values()) {
      double value = single.getValue();
      SingleBenchmarkResult compareResult = compared.getResult(single.getName());
      if (compareResult != null) {
        double comparedValue = compareResult.getValue();
        if (!Double.isNaN(value)) {
          product *= (single.getValue() / comparedValue);
          count++;
        }
      }
    }
    return Math.pow(product, 1.0 / count);
  }
  
  public Set<String> getCases() {
    return this.results.keySet();
  }

  public double getSum() {
    double result = 0;
    for (SingleBenchmarkResult single : results.values()) {
      double value = single.getValue();
      if (!Double.isNaN(value))
        result += value;
    }
    return result;
  }
  
}
