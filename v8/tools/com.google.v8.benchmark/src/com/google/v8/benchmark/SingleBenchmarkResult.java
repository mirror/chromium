// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

import java.text.NumberFormat;

/**
 * The result of running a benchmark.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class SingleBenchmarkResult {
  
  private final String name;
  private final double value;

  public SingleBenchmarkResult(String name, double value) {
    this.name = name;
    this.value = value;
  }
  
  public String getName() {
    return this.name;
  }
  
  public @Override String toString() {
    return name + ": " + value; //$NON-NLS-1$
  }
  
  public double getValue() {
    return this.value;
  }

  public static String toString(double value) {
    NumberFormat format = NumberFormat.getNumberInstance();
    format.setMaximumIntegerDigits(5);
    format.setMaximumFractionDigits(2);
    format.setMinimumFractionDigits(2);
    return format.format(value);
  }
  
}
