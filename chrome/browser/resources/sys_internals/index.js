// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// <include src="line_chart/include.js">

cr.define('SysInternals', function() {
  'use strict';

  const UNIT_NUMBER_PER_SECOND = ['/s', 'K/s', 'M/s'];
  const UNITSCALE_NUMBER_PER_SECOND = 1000;
  const UNIT_MEMORY = ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];
  const UNITSCALE_MEMORY = 1024;

  function initialize() {
    console.info('sys-internals init.');

    initPage_();
    initDataSeries_();
    initChart_();

    sendUpdateRequest_();
    onHashChange_();
  }

  function initPage_() {
    const menuBtn = $('sys-internals-nav-menu-btn');
    menuBtn.onclick = function(event) {
      event.preventDefault();
      SysInternals.openDrawer();
    };

    const drawer = $('sys-internals-drawer');
    drawer.onclick = function(event) {
      SysInternals.clossDrawer();
    };

    window.onhashchange = onHashChange_;
  }

  function initDataSeries_() {
    const dataSeries = SysInternals.dataSeries;
    dataSeries.memUsed = new LineChart.DataSeries('Used Memory', '#fa4e30');
    dataSeries.swapUsed = new LineChart.DataSeries('Used Swap', '#8d6668');
    dataSeries.pswpin = new LineChart.DataSeries('Pswpin', '#73418c');
    dataSeries.pswpout = new LineChart.DataSeries('Pswpout', '#41205e');

    dataSeries.origDataSize =
        new LineChart.DataSeries('Original Data Size', '#9cabd4');
    dataSeries.comprDataSize =
        new LineChart.DataSeries('Compress Data Size', '#4a4392');
    dataSeries.memUsedTotal =
        new LineChart.DataSeries('Total Memory', '#dcfaff');
    dataSeries.memUsedTotal.setMenuTextBlack(true);
    dataSeries.numReads = new LineChart.DataSeries('Num Reads', '#fff9c9');
    dataSeries.numReads.setMenuTextBlack(true);
    dataSeries.numWrites = new LineChart.DataSeries('Num Writes', '#ffa3ab');
  }

  function initCpuDataSeries_(cpus) {
    if (cpus.length == 0)
      return;
    const dataSeries = SysInternals.dataSeries;
    dataSeries.cpus = [];
    const CPU_COLOR_SET = [
      '#393984', '#2a2b79', '#113270', '#a170d0', '#473f85', '#2fa2ff',
      '#bc9dfb', '#ff93e2'
    ];
    for (let i = 0; i < cpus.length; ++i) {
      const colorIdx = i % CPU_COLOR_SET.length;
      dataSeries.cpus[i] =
          new LineChart.DataSeries('CPU ' + (i + 1), CPU_COLOR_SET[colorIdx]);
    }
  }

  function initChart_() {
    SysInternals.lineChart =
        new LineChart.LineChart($('sys-internals-chart-root'));
  }

  function onHashChange_() {
    console.log('hash change');
    const hash = location.hash;
    if (hash == '#CPU') {
      setupCPUPage();
    } else if (hash == '#Memory') {
      setupMemoryPage();
    } else if (hash == '#Zram') {
      setupZramPage();
    } else if (hash == '') {
      setupInfoPage();
    }
    const title = $('sys-internals-drawer-title');
    const infoPage = $('sys-internals-infopage-root');
    if (hash == '') {
      title.innerText = 'Info';
      infoPage.removeAttribute('hidden');
    } else {
      title.innerText = hash.slice(1);
      infoPage.setAttribute('hidden', '');
    }
    clossDrawer();
  }

  function setTextById(id, text) {
    $(id).innerText = text;
  }

  function updateInfoPage() {
    const info = SysInternals.generalInfo;
    setTextById('sys-internals-infopage-num-of-cpu', info.cpu.core);
    setTextById('sys-internals-infopage-cpu-usage', info.cpu.usage);
    setTextById('sys-internals-infopage-cpu-kernel', info.cpu.kernel);
    setTextById('sys-internals-infopage-cpu-user', info.cpu.user);
    setTextById('sys-internals-infopage-cpu-idle', info.cpu.idle);

    setTextById('sys-internals-infopage-memory-total', info.memory.total);
    setTextById('sys-internals-infopage-memory-used', info.memory.used);
    setTextById(
        'sys-internals-infopage-memory-swap-total', info.memory.swapTotal);
    setTextById(
        'sys-internals-infopage-memory-swap-used', info.memory.swapUsed);

    setTextById('sys-internals-infopage-zram-total', info.zram.total);
    setTextById('sys-internals-infopage-zram-orig', info.zram.orig);
    setTextById('sys-internals-infopage-zram-compr', info.zram.compr);
    setTextById(
        'sys-internals-infopage-zram-compr-ratio', info.zram.comprRatio);
  }

  function setupInfoPage() {
    const lineChart = SysInternals.lineChart;
    lineChart.clearAllSubChart();
    updateInfoPage();
  }

  function setupCPUPage() {
    const lineChart = SysInternals.lineChart;
    const dataSeries = SysInternals.dataSeries;
    if (dataSeries.cpus == undefined) {
      setTimeout(setupCPUPage, 1000);
      return;
    }
    lineChart.setSubChart(LineChart.LabelAlign.LEFT, [''], 1);
    lineChart.setSubChart(LineChart.LabelAlign.RIGHT, ['%'], 1);
    for (let i = 0; i < dataSeries.cpus.length; ++i) {
      lineChart.addDataSeries(LineChart.LabelAlign.RIGHT, dataSeries.cpus[i]);
    }
  }

  function setupMemoryPage() {
    const lineChart = SysInternals.lineChart;
    const dataSeries = SysInternals.dataSeries;
    lineChart.setSubChart(
        LineChart.LabelAlign.LEFT, UNIT_NUMBER_PER_SECOND,
        UNITSCALE_NUMBER_PER_SECOND);
    lineChart.setSubChart(
        LineChart.LabelAlign.RIGHT, UNIT_MEMORY, UNITSCALE_MEMORY);
    lineChart.addDataSeries(LineChart.LabelAlign.RIGHT, dataSeries.memUsed);
    lineChart.addDataSeries(LineChart.LabelAlign.RIGHT, dataSeries.swapUsed);
    lineChart.addDataSeries(LineChart.LabelAlign.LEFT, dataSeries.pswpin);
    lineChart.addDataSeries(LineChart.LabelAlign.LEFT, dataSeries.pswpout);
  }

  function setupZramPage() {
    const lineChart = SysInternals.lineChart;
    const dataSeries = SysInternals.dataSeries;
    lineChart.setSubChart(
        LineChart.LabelAlign.LEFT, UNIT_NUMBER_PER_SECOND,
        UNITSCALE_NUMBER_PER_SECOND);
    lineChart.setSubChart(
        LineChart.LabelAlign.RIGHT, UNIT_MEMORY, UNITSCALE_MEMORY);
    lineChart.addDataSeries(
        LineChart.LabelAlign.RIGHT, dataSeries.origDataSize);
    lineChart.addDataSeries(
        LineChart.LabelAlign.RIGHT, dataSeries.comprDataSize);
    lineChart.addDataSeries(
        LineChart.LabelAlign.RIGHT, dataSeries.memUsedTotal);
    lineChart.addDataSeries(LineChart.LabelAlign.LEFT, dataSeries.numReads);
    lineChart.addDataSeries(LineChart.LabelAlign.LEFT, dataSeries.numWrites);
  }

  function openDrawer() {
    const drawer = $('sys-internals-drawer');
    drawer.removeAttribute('hidden');
    setTimeout(function() {
      const drawer = $('sys-internals-drawer');
      const menu = $('sys-internals-drawer-menu');
      drawer.className = '';
      menu.className = '';
    });
  }

  function clossDrawer() {
    const drawer = $('sys-internals-drawer');
    const menu = $('sys-internals-drawer-menu');
    drawer.className = 'hidden';
    menu.className = 'hidden';
    setTimeout(drawer.setAttribute.bind(drawer), 200, 'hidden', '');
  }

  function addFakeData(timeTick) {
    SysInternals.handleUpdateData(
        {
          memUsed: Math.floor(Math.random() * 1000000),
          swapUsed: Math.floor(Math.random() * 100000),
          swapReads: Math.floor(Math.random() * 10000),
          swapWrites: Math.floor(Math.random() * 100),
        },
        timeTick);
  }

  function sendUpdateRequest_() {
    const period = 1000;  // tmp value
    const now = Date.now();
    const sleepTime = period - now % period;
    const timeTick = now + sleepTime;
    setTimeout(function() {
      cr.sendWithPromise('getSysInfo').then(function(data) {
        handleUpdateData(data, timeTick);
      });
    }, sleepTime);
  }

  function getDiffAndUpdateCounter_(name, newValue, timeTick) {
    const counter = SysInternals.counter[name];
    if (counter == undefined) {
      SysInternals.counter[name] = {value: newValue, timeTick: timeTick};
      return 0;
    }
    let valueDelta = newValue - counter.value;
    if (valueDelta < 0) {
      valueDelta %= SysInternals.counter.max;
      valueDelta += SysInternals.counter.max;
    }
    const timeDelta = (timeTick - counter.timeTick) / 1000;
    SysInternals.counter[name].value = newValue;
    SysInternals.counter[name].timeTick = timeTick;
    let diffPerSec = 0;
    if (timeDelta > 0) {
      diffPerSec = valueDelta / (timeDelta);
    }
    return diffPerSec;
  }

  function handleUpdateData(data, timeTick) {
    // console.log(data);
    const dataSeries = SysInternals.dataSeries;
    SysInternals.counter.max = data.const.counterMax;

    // TODO: Disable page when not available
    updateCpuData_(data.cpus, timeTick);
    updateMemoryData_(data.memory, timeTick);
    updateZramData_(data.zram, timeTick);

    if (location.hash == '') {
      updateInfoPage();
    } else {
      SysInternals.lineChart.updateEndTimeToNow();
      SysInternals.lineChart.update();
    }
    sendUpdateRequest_();
  }

  function toPercentageString(number, fixed) {
    const fixedNumber = (number * 100).toFixed(fixed);
    return fixedNumber + '%';
  }

  function isInfoPage() {
    return location.hash == '';
  }

  function updateCpuData_(cpus, timeTick) {
    if (SysInternals.dataSeries.cpus == undefined) {
      initCpuDataSeries_(cpus);
    }
    const cpuDataSeries = SysInternals.dataSeries.cpus;
    if (cpus.length != cpuDataSeries.length) {
      console.warn('Cpu Data: Number of procesor changed.');
      return;
    }
    let allKernel = 0;
    let allUser = 0;
    let allIdle = 0;
    for (let i = 0; i < cpus.length; ++i) {
      const user =
          getDiffAndUpdateCounter_('cpu' + i + 'user', cpus[i].user, timeTick);
      const kernel = getDiffAndUpdateCounter_(
          'cpu' + i + 'kernel', cpus[i].kernel, timeTick);
      const idle =
          getDiffAndUpdateCounter_('cpu' + i + 'idle', cpus[i].idle, timeTick);
      const total = getDiffAndUpdateCounter_(
          'cpu' + i + 'total', cpus[i].total, timeTick);
      let percentage = 0;
      if (total != 0) {
        percentage = (user + kernel) / total * 100;
      }
      cpuDataSeries[i].addDataPoint(percentage, timeTick);
      allKernel += kernel;
      allUser += user;
      allIdle += idle;
    }

    if (isInfoPage()) {
      const generalCpu = SysInternals.generalInfo.cpu;
      generalCpu.core = cpus.length;
      const usageNum = (allKernel + allUser) / (allKernel + allUser + allIdle);
      generalCpu.usage = toPercentageString(usageNum, 2);
      generalCpu.kernel = allKernel;
      generalCpu.user = allUser;
      generalCpu.idle = allIdle;
    }
  }

  function getValueWithUnit(value, units, unitScale) {
    let unitIdx;
    [unitIdx, value] = LineChart.Label.getSuitalbeUnit(value, units, unitScale);
    return value.toFixed(2) + ' ' + units[unitIdx];
  }

  function updateMemoryData_(memory, timeTick) {
    const dataSeries = SysInternals.dataSeries;
    const memUsed = memory.total - memory.available;
    dataSeries.memUsed.addDataPoint(memUsed, timeTick);
    const swapUsed = memory.swapTotal - memory.swapFree;
    dataSeries.swapUsed.addDataPoint(swapUsed, timeTick);
    const pswpin = getDiffAndUpdateCounter_('pswpin', memory.pswpin, timeTick);
    dataSeries.pswpin.addDataPoint(pswpin, timeTick);
    const pswpout =
        getDiffAndUpdateCounter_('pswpout', memory.pswpout, timeTick);
    dataSeries.pswpout.addDataPoint(pswpout, timeTick);

    if (isInfoPage()) {
      const generalMem = SysInternals.generalInfo.memory;
      generalMem.total =
          getValueWithUnit(memory.total, UNIT_MEMORY, UNITSCALE_MEMORY);
      generalMem.used =
          getValueWithUnit(memUsed, UNIT_MEMORY, UNITSCALE_MEMORY);
      generalMem.swapTotal =
          getValueWithUnit(memory.swapTotal, UNIT_MEMORY, UNITSCALE_MEMORY);
      generalMem.swapUsed =
          getValueWithUnit(swapUsed, UNIT_MEMORY, UNITSCALE_MEMORY);
    }
  }

  function updateZramData_(zram, timeTick) {
    const dataSeries = SysInternals.dataSeries;
    dataSeries.origDataSize.addDataPoint(zram.origDataSize, timeTick);
    dataSeries.comprDataSize.addDataPoint(zram.comprDataSize, timeTick);
    dataSeries.memUsedTotal.addDataPoint(zram.memUsedTotal, timeTick);
    const numReads =
        getDiffAndUpdateCounter_('numReads', zram.numReads, timeTick);
    dataSeries.numReads.addDataPoint(numReads, timeTick);
    const numWrites =
        getDiffAndUpdateCounter_('numWrites', zram.numWrites, timeTick);
    dataSeries.numWrites.addDataPoint(numWrites, timeTick);

    if (isInfoPage()) {
      const generalZram = SysInternals.generalInfo.zram;
      generalZram.total =
          getValueWithUnit(zram.memUsedTotal, UNIT_MEMORY, UNITSCALE_MEMORY);
      generalZram.orig =
          getValueWithUnit(zram.origDataSize, UNIT_MEMORY, UNITSCALE_MEMORY);
      generalZram.compr =
          getValueWithUnit(zram.comprDataSize, UNIT_MEMORY, UNITSCALE_MEMORY);
      const comprRatioNum =
          (zram.origDataSize - zram.comprDataSize) / zram.origDataSize;
      generalZram.comprRatio = toPercentageString(comprRatioNum, 2);
    }
  }

  return {
    initialize: initialize,
    // sendUpdateRequest: sendUpdateRequest,
    handleUpdateData: handleUpdateData,
    // updateMemoryData: updateMemoryData,
    // getDiffAndUpdateCounter: getDiffAndUpdateCounter,
    setupCPUPage: setupCPUPage,
    setupMemoryPage: setupMemoryPage,
    setupZramPage: setupZramPage,
    openDrawer: openDrawer,
    clossDrawer: clossDrawer,
    dataSeries: {},
    counter: {},
    lineChart: null,
    generalInfo: {
      cpu: {core: 0, usage: '0%', kernel: 0, user: 0, idle: 0},
      memory: {total: 0, used: 0, swapTotal: 0, swapUsed: 0},
      zram: {total: 0, orig: 0, compr: 0, comprRatio: '0%'}
    }
  };
});

document.addEventListener('DOMContentLoaded', function() {
  SysInternals.initialize();
});