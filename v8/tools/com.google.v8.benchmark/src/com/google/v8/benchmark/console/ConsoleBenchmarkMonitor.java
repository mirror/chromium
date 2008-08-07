// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark.console;

import com.google.v8.benchmark.IBenchmarkMonitor;
import com.google.v8.benchmark.Runtime;
import com.google.v8.benchmark.SingleBenchmarkResult;

import java.io.File;
import java.io.IOException;
import java.io.Writer;
import java.text.MessageFormat;
import java.util.Collection;

/**
 * A benchmark monitor that displays output on the console.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class ConsoleBenchmarkMonitor implements IBenchmarkMonitor {

  private static final int DOT_WIDTH = 50;
  
  private final Writer log;
  int column = 0;
  
  public ConsoleBenchmarkMonitor(Writer log) {
    this.log = log;
  }
  
  public void startBenchmark(Runtime runtime, File script) {
    printDot();
    logln(MessageFormat.format(Messages.getString("ConsoleBenchmarkMonitor.runningHeader"), script.getName(), runtime.getName())); //$NON-NLS-1$
  }

  private void printDot() {
    if (column >= DOT_WIDTH) {
      System.out.println();
      column = 0;
    }
    System.out.print("."); //$NON-NLS-1$
    column++;
  }
  
  public void printErr(String str) {
    log(str);
  }

  private void log(String str) {
    if (this.log != null) {
      try {
        this.log.write(str);
        this.log.flush();
      } catch (IOException ioe) {
        throw new RuntimeException(ioe);
      }
    }
  }
  
  private void logln(String str) {
    log(str + "\n"); //$NON-NLS-1$
  }

  public void printOut(String str) {
    log(str);
  }
  
  public void benchmarkDone(Collection<SingleBenchmarkResult> results, String stdout, String stderr) {
    
  }

}
