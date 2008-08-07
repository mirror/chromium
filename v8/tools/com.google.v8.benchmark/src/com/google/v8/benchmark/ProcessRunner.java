// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.concurrent.Semaphore;

/**
 * A process runner can be used to execute a command and provides
 * access to a process' output.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class ProcessRunner {
  
  private final String command;
  private final File directory;
  private Exception asyncError;
  private Process process;
  private boolean processDead = false, outputDead = false;
  private IOutputListener listener;
  private Thread processThread, outputThread;
  private final Semaphore doneSemaphore = new Semaphore(0);
  
  public ProcessRunner(String command, File directory) {
    this.command = command;
    this.directory = directory;
  }
  
  public Exception getAsyncError() {
    return this.asyncError;
  }
  
  public void setStreams(IOutputListener listener) {
    this.listener = listener;
  }
  
  @SuppressWarnings("nls")
  public void start() throws IOException {
    this.process = java.lang.Runtime.getRuntime().exec(
        this.command, null, this.directory);
    this.processThread = new Thread(new Runnable() {
      public void run() {
        runProcess();
      }
    }, "Process '" + command + "'");
    this.outputThread = new Thread(new Runnable() {
      public void run() {
        try {
          listenOutput();
        } catch (IOException ioe) {
          asyncError = ioe;
        } finally {
          doneSemaphore.release(1);
          outputDead = true;
        }
      }
    }, "Output listener for '" + command + "'");
    this.outputThread.start();
    this.processThread.start();
  }
  
  public void join() throws InterruptedException {
    doneSemaphore.acquire(2);
  }
  
  public void stop() {
    this.processThread.interrupt();
    this.outputThread.interrupt();
  }
  
  private void listenOutput() throws IOException {
    Reader in = new InputStreamReader(this.process.getInputStream());
    Reader err = new InputStreamReader(this.process.getErrorStream());
    char[] buf = new char[1024];
    while (!processDead) {
      int count = in.read(buf);
      while (count > 0) {
        listener.output(new String(buf, 0, count));
        count = in.read(buf);
      }
      count = err.read(buf);
      while (count > 0) {
        listener.error(new String(buf, 0, count));
        count = err.read();
      }
      if (processDead) return;
      try { Thread.sleep(100); }
      catch (InterruptedException ie) { return; }
    }
  }

  private void runProcess() {
    try {
      this.process.waitFor();
    } catch (InterruptedException ie) {
      this.asyncError = ie;
      this.process.destroy();
    } finally {
      this.processDead = true;
      this.doneSemaphore.release(1);
    }
  }
  
  /**
   * Has the execution been terminated (normally or abruptly)?
   */
  public boolean isDone() {
    return this.processDead && this.outputDead;
  }

}
