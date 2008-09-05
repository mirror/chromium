// Copyright 2006-2008 Google Inc. All Rights Reserved.

package com.google.v8.benchmark;

import com.google.v8.table.Alignment;
import com.google.v8.table.AsciiTable;
import com.google.v8.table.AsciiTableColumn;
import com.google.v8.table.AsciiTableRow;
import com.google.v8.table.LineType;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.text.DateFormatSymbols;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.SortedMap;
import java.util.SortedSet;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Utility class for running a set of benchmarks.
 * 
 * @author plesner@google.com (Christian Plesner Hansen)
 */
public class BenchmarkRunner {

  public static void runBenchmarks(Main.IOptions commandLine, 
      List<Runtime> runtimes, IBenchmarkMonitor monitor) throws IOException {
    SortedMap<File, FileBenchmarkResult> results = new TreeMap<File, FileBenchmarkResult>();
    List<ScriptDescriptor> scripts = new ArrayList<ScriptDescriptor>();
    for (File script : commandLine.getBenchmarks()) {
      SortedMap<Runtime, RuntimeBenchmarkResults> fileResults = new TreeMap<Runtime, RuntimeBenchmarkResults>();
      SortedSet<String> cases = new TreeSet<String>();
      for (Runtime runtime : runtimes) {
        monitor.startBenchmark(runtime, script);
        String command = runtime.getCommand(script.getAbsoluteFile().toString());
        File directory = script.getParentFile().getAbsoluteFile();
        ProcessRunner runner = new ProcessRunner(command, directory);
        BenchmarkOutputListener listener = new BenchmarkOutputListener(monitor);
        runner.setStreams(listener);
        runner.start();
        try { runner.join(); }
        catch (InterruptedException ie) { }
        String output = listener.getOutput();
        String error = listener.getError();
        SortedMap<String, SingleBenchmarkResult> singleResults = processOutput(output, script.getName());
        cases.addAll(singleResults.keySet());
        monitor.benchmarkDone(singleResults.values(), output, error);
        fileResults.put(runtime, new RuntimeBenchmarkResults(runtime, singleResults));
      }
      scripts.add(new ScriptDescriptor(script, new ArrayList<String>(cases)));
      results.put(script, new FileBenchmarkResult(script, fileResults));
    }
    CollectedBenchmarkResult collected = new CollectedBenchmarkResult(results);
    BenchmarkDescriptor desc = new BenchmarkDescriptor(scripts, runtimes);
    if (commandLine.getHistory() != null)
      storeHistory(collected, commandLine.getRecordPath());
    Runtime compared = getCompared(commandLine.getCompared(), runtimes);
    System.out.println();
    printResults(collected, desc, compared, System.out);
    List<PrintStream> outs = new ArrayList<PrintStream>();
    outs.add(System.out);
    PrintStream fileOut = null;
    if (commandLine.getOutputFile() != null) {
      fileOut = new PrintStream(commandLine.getOutputFile());
      outs.add(fileOut);
    }
    if (fileOut != null)
      printResults(collected, desc, compared, fileOut);
    if (commandLine.getHistory() != null) {
      Runtime history = new Runtime(new File(commandLine.getHistory()));
      long days = commandLine.getHistorySize();
      printHistory(commandLine.getRecordPath(), history,
          (int) days, outs);
    }
    if (fileOut != null)
      fileOut.close();
  }

  @SuppressWarnings("nls")
  private static void storeHistory(CollectedBenchmarkResult collected,
      File recordPath) throws IOException {
    Calendar time = Calendar.getInstance();
    String name = getFileNameFor(time);
    File record = getHistoryFile(recordPath, name);
    if (record == null) return;
    PrintWriter out = new PrintWriter(new FileWriter(record, false));
    for (FileBenchmarkResult fileResult : collected.getResults()) {
      for (RuntimeBenchmarkResults runtimeResult : fileResult.getResults()) {
        for (SingleBenchmarkResult singleResult : runtimeResult.getResults()) {
          out.print(fileResult.getFile().getName());
          out.print(":" + runtimeResult.getRuntime().getFile().getName());
          out.print(":" + singleResult.getName());
          out.print(":" + singleResult.getValue());
          out.println();
        }
      }
    }
    out.close();
  }
  
  private static CollectedBenchmarkResult readHistory(File file) throws IOException {
    BufferedReader in = new BufferedReader(new FileReader(file));
    SortedMap<File, SortedMap<Runtime, SortedMap<String, Double>>> scriptMap = new TreeMap<File, SortedMap<Runtime, SortedMap<String, Double>>>();
    while (in.ready()) {
      String line = in.readLine();
      String[] entries = line.split(":"); //$NON-NLS-1$
      File script = new File(entries[0]);
      if (!scriptMap.containsKey(script))
        scriptMap.put(script, new TreeMap<Runtime, SortedMap<String, Double>>());
      SortedMap<Runtime, SortedMap<String, Double>> runtimeMap = scriptMap.get(script);
      Runtime runtime = new Runtime(new File(entries[1]));
      if (!runtimeMap.containsKey(runtime))
        runtimeMap.put(runtime, new TreeMap<String, Double>());
      SortedMap<String, Double> cahseMap = runtimeMap.get(runtime);
      String cahse = entries[2];
      double value = Double.parseDouble(entries[3]);
      cahseMap.put(cahse, value);
    }
    SortedMap<File, FileBenchmarkResult> allResults = new TreeMap<File, FileBenchmarkResult>();
    for (Map.Entry<File, SortedMap<Runtime, SortedMap<String, Double>>> scriptEntry : scriptMap.entrySet()) {
      File scriptFile = scriptEntry.getKey();
      SortedMap<Runtime, RuntimeBenchmarkResults> runtimeResults = new TreeMap<Runtime, RuntimeBenchmarkResults>();
      for (Map.Entry<Runtime, SortedMap<String, Double>> runtimeEntry : scriptEntry.getValue().entrySet()) {
        Runtime runtime = runtimeEntry.getKey();
        SortedMap<String, SingleBenchmarkResult> singleResults = new TreeMap<String, SingleBenchmarkResult>();
        for (Map.Entry<String, Double> singleEntry : runtimeEntry.getValue().entrySet()) {
          singleResults.put(singleEntry.getKey(), 
              new SingleBenchmarkResult(singleEntry.getKey(), singleEntry.getValue()));
        }
        runtimeResults.put(runtime, new RuntimeBenchmarkResults(runtime, singleResults));
      }
      allResults.put(scriptFile, new FileBenchmarkResult(scriptFile, runtimeResults));
    }
    in.close();
    return new CollectedBenchmarkResult(allResults);
  }

  private static String getFileNameFor(Calendar time) {
    int year = time.get(Calendar.YEAR);
    DateFormatSymbols dfs = new DateFormatSymbols();
    String month = dfs.getShortMonths()[time.get(Calendar.MONTH)];
    int day = time.get(Calendar.DATE);
    String name = year + "-" + month + "-" + day + ".history"; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
    return name;
  }
  
  private static final int ONE_DAY = 24 * 60 * 60 * 1000;
  private static void printHistory(File recordPath, Runtime history,
      int days, List<PrintStream> outs) throws IOException {
    List<String> times = new ArrayList<String>();
    List<CollectedBenchmarkResult> results = new ArrayList<CollectedBenchmarkResult>();
    for (int i = 0; i < days; i++) {
      Calendar time = Calendar.getInstance();
      time.setTimeInMillis(time.getTimeInMillis() - (ONE_DAY * (days - i - 1)));
      String fileName = getFileNameFor(time);
      File file = getHistoryFile(recordPath, fileName);
      if (file == null || !file.exists()) continue;
      DateFormatSymbols dfs = new DateFormatSymbols();
      String month = dfs.getShortMonths()[time.get(Calendar.MONTH)];
      int day = time.get(Calendar.DATE);
      times.add(month + " " + day); //$NON-NLS-1$
      results.add(readHistory(file));
    }
    for (PrintStream out : outs) {
      out.println();
      out.println();
    }
    printHistory(times, results, history, outs);
  }
  
  @SuppressWarnings("nls")
  private static void printHistory(List<String> times, 
      List<CollectedBenchmarkResult> results,
      Runtime history, List<PrintStream> outs) throws IOException {
    AsciiTable valueTable = new AsciiTable();
    AsciiTable relativeTable = new AsciiTable();
    NumberFormat valueFormat = NumberFormat.getNumberInstance();
    valueFormat.setMaximumFractionDigits(2);
    valueFormat.setMinimumFractionDigits(2);
    valueTable.setNumberFormat(valueFormat);
    NumberFormat relativeFormat = NumberFormat.getPercentInstance();
    relativeFormat.setMaximumFractionDigits(1);
    relativeFormat.setMinimumFractionDigits(1);
    relativeTable.setNumberFormat(relativeFormat);
    valueTable.addColumn(new AsciiTableColumn("name").setLabel("History"));
    relativeTable.addColumn(new AsciiTableColumn("name").setLabel("Relative"));
    boolean first = true;
    for (String time : times) {
      if (first) first = false;
      else relativeTable.addColumn(new AsciiTableColumn(time).setAlignment(Alignment.RIGHT));
      valueTable.addColumn(new AsciiTableColumn(time).setAlignment(Alignment.RIGHT));
    }
    SortedMap<File, SortedSet<String>> cases = new TreeMap<File, SortedSet<String>>();
    for (CollectedBenchmarkResult collected : results) {
      for (FileBenchmarkResult fileResults : collected.getResults()) {
        if (!cases.containsKey(fileResults.getFile()))
          cases.put(fileResults.getFile(), new TreeSet<String>());
        RuntimeBenchmarkResults runtimeResults = fileResults.getResult(history);
        if (runtimeResults == null) continue;
        Set<String> caseSet = cases.get(fileResults.getFile());
        for (SingleBenchmarkResult singleResult : runtimeResults.getResults())
          caseSet.add(singleResult.getName());
      }
    }
    for (Map.Entry<File, SortedSet<String>> file : cases.entrySet()) {
      AsciiTableRow valueRow = new AsciiTableRow();
      valueRow.setValue("name", file.getKey().getName());
      valueRow.setLineBefore(LineType.THIN);
      AsciiTableRow relativeRow = new AsciiTableRow();
      relativeRow.setValue("name", file.getKey().getName());
      relativeRow.setLineBefore(LineType.THIN);
      Double current = null;
      for (int i = 0; i < times.size(); i++) {
        Double lastValue = current;
        current = null;
        FileBenchmarkResult fileResults = results.get(i).getResult(file.getKey());
        if (fileResults == null) continue;
        RuntimeBenchmarkResults runtimeResults = fileResults.getResult(history);
        if (runtimeResults == null) continue;
        current = runtimeResults.getSum();
        valueRow.setValue(times.get(i), current);
        if (lastValue != null && current != 0.0)
          relativeRow.setValue(times.get(i), ((lastValue - current) / current));
      }
      valueTable.addRow(valueRow);
      relativeTable.addRow(relativeRow);
      if (file.getValue().size() == 1) continue;
      for (String cahse : file.getValue()) {
        AsciiTableRow cahseRow = new AsciiTableRow(cahse);
        AsciiTableRow cahseRelativeRow = new AsciiTableRow(cahse);
        current = null;
        for (int i = 0; i < times.size(); i++) {
          Double lastValue = current;
          current = null;
          FileBenchmarkResult fileResults = results.get(i).getResult(file.getKey());
          if (fileResults == null) continue;
          RuntimeBenchmarkResults runtimeResults = fileResults.getResult(history);
          if (runtimeResults == null) continue;
          SingleBenchmarkResult singleResult = runtimeResults.getResult(cahse);
          if (singleResult == null) continue;
          current = singleResult.getValue();
          cahseRow.setValue(times.get(i), singleResult.getValue());
          if (lastValue != null && current != 0.0)
            cahseRelativeRow.setValue(times.get(i), ((lastValue - current) / current));
        }
        valueRow.addRow(cahseRow);
        relativeRow.addRow(cahseRelativeRow);
      }
    }
    for (PrintStream out : outs) {
      valueTable.render(new PrintWriter(out));
      out.println();
      out.println();
      relativeTable.render(new PrintWriter(out));
    }
  }

  private static File getHistoryFile(File recordPath, String name) {
    return (recordPath == null)
      ? new File(name)
      : new File(recordPath, name);
  }

  private static void printResults(CollectedBenchmarkResult collected, BenchmarkDescriptor desc, Runtime compared, PrintStream out) throws IOException {
    desc.printResults(out, collected, null);
    if (compared != null) {
      out.println();
      out.println();
      desc.printResults(out, collected, compared);
    }
  }
  
  private static Runtime getCompared(String comparedName, List<Runtime> runtimes) {
    Runtime compared = null;
    if (comparedName != null) {
      for (Runtime runtime : runtimes) {
        if (runtime.getName().equals(comparedName))
          compared = runtime;
      }
    }
    return compared;
  }
  
  @SuppressWarnings("nls")
  private static SortedMap<String, SingleBenchmarkResult> processOutput(String str,
      String fileName) {
    SortedMap<String, SingleBenchmarkResult> result = new TreeMap<String, SingleBenchmarkResult>();
    String[] lines = str.split("[.\n]+"); //$NON-NLS-1$
    for (String rawLine : lines) {
      String line = rawLine.trim();
      RawBenchmarkOutput rawResult = extractResult(line);
      if (rawResult == null) continue;
      if (!rawResult.type.equals("Time") && !rawResult.type.equals("Score"))
        continue;
      double value = Double.parseDouble(rawResult.value);
      if (rawResult.unit != null) {
        Double factor = Units.getFactor(rawResult.unit);
        if (factor != null) value = value * factor;        
      }
      String name = fileName;
      if (rawResult.name != null) name = rawResult.name;
      result.put(name, new SingleBenchmarkResult(name, value));
    }
    return result;
  }
  
  private static final Pattern PATTERN = Pattern.compile("([A-Za-z]+)\\s*(?:\\(([A-Za-z0-9_\\-]+)\\))?\\s*:\\s*([0-9.]+)\\s*([a-zA-Z]+)?"); //$NON-NLS-1$
  public static RawBenchmarkOutput extractResult(String line) {
    Matcher matcher = PATTERN.matcher(line);
    if (!matcher.find()) return null;
    String type = matcher.group(1);
    String name = matcher.group(2);
    String value = matcher.group(3);
    String unit = matcher.group(4);
    return new RawBenchmarkOutput(type, name, value, unit);
  }
  
  public static class RawBenchmarkOutput {
    
    private final String type, name, value, unit;
    
    public RawBenchmarkOutput(String type, String name, String value,
        String unit) {
      this.type = type;
      this.name = name;
      this.value = value;
      this.unit = unit;
    }
    
    @SuppressWarnings("nls")
    public @Override String toString() {
      StringBuilder buf = new StringBuilder();
      buf.append(type);
      if (name != null) buf.append("/").append(name);
      buf.append("/").append(value);
      if (unit != null) buf.append("/").append(unit);
      return buf.toString();
    }
    
  }
  
}
