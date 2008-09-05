// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

import java.util.HashMap;
import java.util.Map;

/**
 * Manages the various units understood by the system.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class Units {
  
  @SuppressWarnings("nls")
  private static final Map<String, Double> factors = new HashMap<String, Double>() {{
    this.put("s",  1000.0);
    this.put("ms", 1.0);
    this.put("us", 0.001);
  }};

  public static Double getFactor(String unit) {
    return factors.get(unit);
  }

}
