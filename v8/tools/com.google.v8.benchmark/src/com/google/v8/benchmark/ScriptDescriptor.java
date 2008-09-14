// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

import java.io.File;
import java.util.List;

/**
 * Describes the contents of a single test file.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class ScriptDescriptor {
  
  private final File file;
  private final List<String> cases;
  
  public ScriptDescriptor(File file, List<String> cases) {
    this.file = file;
    this.cases = cases;
  }

  public String getName() {
    return file.getName();
  }
  
  public List<String> getCases() {
    return cases;
  }

  public File getFile() {
    return this.file;
  }

}
