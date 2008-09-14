// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

import java.io.File;

/**
 * Represents a runtime.  A runtime consists of a command to use for
 * execution and a file which is the runtime's executable.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class Runtime implements Comparable<Runtime> {
  
  private final String command;
  private final File file;
  
  public Runtime(String command, File file) {
    this.command = command;
    this.file = file;
  }
  
  public Runtime(File file) {
    this(null, file);
  }

  /**
   * Returns the raw command for executing this runtime.
   */
  public String getCommand() {
    return this.command;
  }
  
  public File getFile() {
    return this.file;
  }

  /**
   * Returns the command to invoke to run the specified script using
   * this runtime.
   */
  public String getCommand(String script) {
    if (command.indexOf('@') != -1) {
      return command.replaceAll("@", script); //$NON-NLS-1$
    } else {
      return command + " " + script; //$NON-NLS-1$
    }
  }

  public String getName() {
    return getFile().getName();
  }

  public int compareTo(Runtime o) {
    return file.compareTo(o.file);
  }
  
  public @Override int hashCode() {
    return file.hashCode();
  }
  
  public @Override boolean equals(Object obj) {
    if (this == obj) return true;
    if (!(obj instanceof Runtime)) return false;
    return this.file.equals(((Runtime) obj).file);
  }

}
