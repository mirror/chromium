// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

import java.io.File;
import java.util.Collection;

/**
 * A benchmark monitor is notified during the execution of a benchmark
 * and can display (or not) progress information.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public interface IBenchmarkMonitor {
  
  /**
   * A benchmark has been started.
   */
  public void startBenchmark(Runtime runtime, File script);
  
  /**
   * The process generated output.
   */
  public void printOut(String str);
  
  /**
   * The process generated output on standard error.  All error output
   * produced during execution will also be provided in a call to
   * benchmarkDone.
   */
  public void printErr(String str);
  
  /**
   * The benchmark is done.
   */
  public void benchmarkDone(Collection<SingleBenchmarkResult> results, String stdout,
      String stderr);
  
}
