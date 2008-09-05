// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

import com.google.v8.benchmark.console.ConsoleBenchmarkMonitor;

import org.quenta.lipwig.Accessor;
import org.quenta.lipwig.CommandLineParser;
import org.quenta.lipwig.ICommandLine;
import org.quenta.lipwig.Option;
import org.quenta.lipwig.OptionFilter;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.Writer;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.List;

/**
 * Entry-point to the profiler.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
@SuppressWarnings("nls")
public class Main {
  
  @SuppressWarnings("nls")
  private static final CommandLineParser COMMAND_LINE = new CommandLineParser(IOptions.class) {{
    this.addOption(new Option("--benchmark", "-b")
      .setFilter(OptionFilter.EXISTING_FILE, OptionFilter.COLLECT)
      .setUsage("Add the specified benchmark to the list of benchmarks to run"));
    this.addOption(new Option("--runtime",   "-r")
      .setFilter(OptionFilter.COLLECT)
      .setUsage("Add the specified runtime to the list of runtimes to benchmark"));
    this.addOption(new Option("--compare",   "-x")
      .setUsage("Compare all results with the results from the specified runtime"));
    this.addOption(new Option("--log",       "-l")
      .setFilter(OptionFilter.FILE)
      .setUsage("Write the raw output from the runtimes to the specified file"));
    this.addOption(new Option("--output",    "-o")
      .setFilter(OptionFilter.FILE)
      .setUsage("Write output to the specified file"));
    this.addOption(new Option("--history-size")
      .setFilter(OptionFilter.LONG)
      .setDefault(7)
      .setUsage("Specify the number of days include in the history"));
    this.addOption(new Option("--history",   "-h")
      .setUsage("Show the history of performance results for the specified vm"));
    this.addOption(new Option("--record-path")
      .setFilter(OptionFilter.EXISTING_DIRECTORY)
      .setUsage("Set the directory used for storing benchmark history"));
  }};
  
  static interface IOptions extends ICommandLine {
    public @Accessor("--benchmark")    List<File> getBenchmarks();
    public @Accessor("--runtime")      List<String> getRuntimes();
    public @Accessor("--record-path")  File getRecordPath();
    public @Accessor("--compare")      String getCompared();
    public @Accessor("--history")      String getHistory();
    public @Accessor("--log")          File getLogFile();
    public @Accessor("--output")       File getOutputFile();
    public @Accessor("--history-size") Long getHistorySize();
  }
  
  public static void main(String[] args) throws IOException {
    if (args.length == 0) {
      PrintWriter out = new PrintWriter(System.out);
      COMMAND_LINE.printUsage(out);
      out.flush();
      return;
    }
    IOptions commandLine = parseCommandLine(args);
    if (commandLine == null) return;
    List<Runtime> runtimes = getRuntimeFiles(commandLine.getRuntimes(),
        commandLine.getCompared());
    if (runtimes == null) return;
    Writer log = null;
    if (commandLine.getLogFile() != null)
      log = new FileWriter(commandLine.getLogFile());
    IBenchmarkMonitor monitor = new ConsoleBenchmarkMonitor(log);
    try {
      BenchmarkRunner.runBenchmarks(commandLine, runtimes, monitor);
    } finally {
      if (log != null)
        log.close();
    }
  }

  private static IOptions parseCommandLine(String[] args) {
    COMMAND_LINE.assertChecked();
    IOptions commandLine = (IOptions) COMMAND_LINE.parse(args);
    if (commandLine.reportErrors(new PrintWriter(System.out)))
        return null;
    return commandLine;
  }

  static List<Runtime> getRuntimeFiles(List<String> commands,
      String compared) {
    List<Runtime> runtimes = new ArrayList<Runtime>();
    for (String command : commands) {
      Runtime runtime = getRuntime(command);
      File file = runtime.getFile();
      assert file != null;
      if (!file.exists()) {
        System.err.println(MessageFormat.format(Messages.getString("Main.missingFileMessage"), file)); //$NON-NLS-1$
        return null;
      }
      runtimes.add(runtime);
    }
    compareCheck: if (compared != null) {
      for (Runtime runtime : runtimes) {
        if (runtime.getName().equals(compared))
          break compareCheck;
      }
      System.out.println(MessageFormat.format("There is no runtime {0}", compared));
      return null;
    }
    return runtimes;
  }

  public static Runtime getRuntime(String rawCommand) {
    String command = rawCommand.trim();
    if (command.length() == 0) return null;
    String[] parts = command.split("\\s+"); //$NON-NLS-1$
    parts[0] = new File(parts[0]).getAbsolutePath();
    StringBuilder builder = new StringBuilder();
    for (String part : parts) builder.append(part);
    return new Runtime(builder.toString(), new File(parts[0]));
  }
  
}
