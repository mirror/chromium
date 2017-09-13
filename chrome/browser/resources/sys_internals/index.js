// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @typedef {{
 *   memUsed: LineChart.DataSeries,
 *   swapUsed: LineChart.DataSeries,
 *   pswpin: LineChart.DataSeries,
 *   pswpout: LineChart.DataSeries
 * }}
 */
var MemoryDataSeriesSet;

/**
 * @typedef {{
 *   origDataSize: LineChart.DataSeries,
 *   comprDataSize: LineChart.DataSeries,
 *   memUsedTotal: LineChart.DataSeries,
 *   numReads: LineChart.DataSeries,
 *   numWrites: LineChart.DataSeries
 * }}
 */
var ZramDataSeriesSet;

/**
 * @typedef {{value: number, timeStamp: number}}
 */
var CounterType;

cr.define('SysInternals', function() {

  var dataSeries = {};

  /** @type {Array<LineChart.DataSeries>} */
  dataSeries.cpus = null;

  /** @const {MemoryDataSeriesSet} */
  dataSeries.memory = {
    memUsed: null,
    swapUsed: null,
    pswpin: null,
    pswpout: null,
  };

  /** @const {ZramDataSeriesSet} */
  dataSeries.zram = {
    origDataSize: null,
    comprDataSize: null,
    memUsedTotal: null,
    numReads: null,
    numWrites: null,
  };

  /** @type {LineChart.LineChart} */
  SysInternals.lineChart = null;

  /**
   * The maximum value of the counter of the system info data.
   * @type {number}
   */
  SysInternals.counterMax = 0;

  /** @const */
  let generalInfo = {};

  /** @type {GeneralCpuType} */
  generalInfo.cpu = {
    core: 0,
    idle: 0,
    kernel: 0,
    usage: 0,
    user: 0,
  };

  /** @type {GeneralMemoryType} */
  generalInfo.memory = {
    swapTotal: 0,
    swapUsed: 0,
    total: 0,
    used: 0,
  };

  /** @type {GeneralZramType} */
  generalInfo.zram = {
    compr: 0,
    comprRatio: NaN,
    orig: 0,
    total: 0,
  };

  /**
   * The page update period, in milliseconds.
   * @const {number}
   */
  const UPDATE_PERIOD = 1000;

  /** @const {Array<string>} */
  const UNITS_NUMBER_PER_SECOND = ['/s', 'K/s', 'M/s'];

  /** @const {number} */
  const UNITBASE_NUMBER_PER_SECOND = 1000;

  /** @const {Array<string>} */
  const UNITS_MEMORY = ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];

  /** @const {number} */
  const UNITBASE_MEMORY = 1024;

  /** @const {number} - The precision of the number on the info page. */
  const INFO_PAGE_PRECISION = 2;

  /** @const {Array<string>} */
  const CPU_COLOR_SET = [
    '#2fa2ff', '#ff93e2', '#a170d0', '#fe6c6c', '#2561a4', '#15b979', '#fda941',
    '#79dbcd'
  ];

  /** @const {Array<string>} */
  const MEMORY_COLOR_SET = ['#fa4e30', '#8d6668', '#73418c', '#41205e'];

  /** @const {Array<string>} - Note: 4th and 5th colors use black menu text. */
  const ZRAM_COLOR_SET =
      ['#9cabd4', '#4a4392', '#dcfaff', '#fff9c9', '#ffa3ab'];

  /** @enum {string} */
  const PAGE_HASH = {
    INFO: '',
    CPU: '#CPU',
    MEMORY: '#Memory',
    ZRAM: '#Zram',
  };

  /** @type {boolean} - Tag used by browser test. */
  var DONT_SEND_UPDATE_REQUEST;

  /** @const */
  const UnitLabelAlign = LineChart.UnitLabelAlign;

  /** @const - For test. */
  SysInternals.waitSysInternalsInitialized = new PromiseResolver();

  /**
   * Initialize the whole page.
   */
  function initialize() {
    initPage_();
    initDataSeries_();
    initChart_();
    setUpdateRequestInterval_();

    /* Initialize with the current url hash value. */
    onHashChange_();
    SysInternals.waitSysInternalsInitialized.resolve();
  }

  /**
   * Initialize the DOM of the page.
   */
  function initPage_() {
    $('sys-internals-nav-menu-btn').addEventListener('click', function(event) {
      event.preventDefault();
      openDrawer();
    });

    $('sys-internals-drawer').addEventListener('click', function(event) {
      closeDrawer();
    });

    window.addEventListener('hashchange', onHashChange_);
  }

  /** @type {PromiseResolver} - For test. */
  SysInternals.waitDrawerActionCompleted = null;

  /**
   * Open the navbar drawer menu.
   */
  function openDrawer() {
    $('sys-internals-drawer').removeAttribute('hidden');
    /* Preventing CSS transition blocking by JS. */
    setTimeout(function() {
      $('sys-internals-drawer').classList.remove('hidden');
      $('sys-internals-drawer-menu').classList.remove('hidden');
      if (SysInternals.waitDrawerActionCompleted) {
        SysInternals.waitDrawerActionCompleted.resolve();
      }
    });
  }

  /**
   * Close the navbar drawer menu.
   */
  function closeDrawer() {
    const /** Element */ drawer = $('sys-internals-drawer');
    drawer.classList.add('hidden');
    $('sys-internals-drawer-menu').classList.add('hidden');
    /* Wait for the drawer close. */
    setTimeout(function() {
      drawer.setAttribute('hidden', '');
      if (SysInternals.waitDrawerActionCompleted) {
        SysInternals.waitDrawerActionCompleted.resolve();
      }
    }, 200);
  }

  /**
   * Initialize the data series of the page.
   */
  function initDataSeries_() {
    const /** MemoryDataSeriesSet */ memDataSeries =
        SysInternals.dataSeries.memory;
    /** @const {LineChart.DataSeries} */
    memDataSeries.memUsed =
        new LineChart.DataSeries('Used Memory', MEMORY_COLOR_SET[0]);
    /** @const {LineChart.DataSeries} */
    memDataSeries.swapUsed =
        new LineChart.DataSeries('Used Swap', MEMORY_COLOR_SET[1]);
    /** @const {LineChart.DataSeries} */
    memDataSeries.pswpin =
        new LineChart.DataSeries('Pswpin', MEMORY_COLOR_SET[2]);
    /** @const {LineChart.DataSeries} */
    memDataSeries.pswpout =
        new LineChart.DataSeries('Pswpout', MEMORY_COLOR_SET[3]);

    const /** ZramDataSeriesSet */ zramDataSeries =
        dataSeries.zram;
    /** @const {LineChart.DataSeries} */
    zramDataSeries.origDataSize =
        new LineChart.DataSeries('Original Data Size', ZRAM_COLOR_SET[0]);
    /** @const {LineChart.DataSeries} */
    zramDataSeries.comprDataSize =
        new LineChart.DataSeries('Compress Data Size', ZRAM_COLOR_SET[1]);
    /** @const {LineChart.DataSeries} */
    zramDataSeries.memUsedTotal =
        new LineChart.DataSeries('Total Memory', ZRAM_COLOR_SET[2]);
    zramDataSeries.memUsedTotal.setMenuTextBlack(true);
    /** @const {LineChart.DataSeries} */
    zramDataSeries.numReads =
        new LineChart.DataSeries('Num Reads', ZRAM_COLOR_SET[3]);
    zramDataSeries.numReads.setMenuTextBlack(true);
    /** @const {LineChart.DataSeries} */
    zramDataSeries.numWrites =
        new LineChart.DataSeries('Num Writes', ZRAM_COLOR_SET[4]);
  }

  /**
   * Initialize the LineChart object.
   */
  function initChart_() {
    SysInternals.lineChart = new LineChart.LineChart();
    SysInternals.lineChart.attachRootDiv($('sys-internals-chart-root'));
  }

  /**
   * Wait until next period, and then send the update request to backend to
   * update the system information.
   */
  function setUpdateRequestInterval_() {
    if (DONT_SEND_UPDATE_REQUEST)
      return;

    setInterval(function() {
      cr.sendWithPromise('getSysInfo').then(function(data) {
        handleUpdateData(data, Date.now());
      });
    }, UPDATE_PERIOD);
  }

  /**
   * Handle the new data which received from backend.
   * @param {SysInfoApiResult} data
   * @param {number} timeStamp - The time tick for these data.
   */
  function handleUpdateData(data, timeStamp) {
    SysInternals.counterMax = data.const.counterMax;

    updateCpuData_(data.cpus, timeStamp);
    updateMemoryData_(data.memory, timeStamp);
    updateZramData_(data.zram, timeStamp);

    if (isInfoPage()) {
      updateInfoPage();
    } else {
      SysInternals.lineChart.updateEndTime(timeStamp);
    }
  }

  /**
   * Handle the new cpu data.
   * @param {Array<SysInfoApiCpuResult>} cpus
   * @param {number} timeStamp
   */
  function updateCpuData_(cpus, timeStamp) {
    if (SysInternals.dataSeries.cpus == null) {
      initCpuDataSeries_(cpus);
    }
    const /** Array<LineChart.DataSeries> */ cpuDataSeries =
        SysInternals.dataSeries.cpus;
    if (cpus.length != cpuDataSeries.length) {
      console.warn('Cpu Data: Number of processors changed.');
      return;
    }
    let allKernel = 0;
    let allUser = 0;
    let allIdle = 0;
    for (let i = 0; i < cpus.length; ++i) {
      /* Check if this cpu is offline */
      if (cpus[i].total == 0) {
        cpuDataSeries[i].addDataPoint(0, timeStamp);
        continue;
      }
      const /** number */ user =
          getDiffAndUpdateCounter_(`cpu_${i}_user`, cpus[i].user, timeStamp);
      const /** number */ kernel = getDiffAndUpdateCounter_(
          `cpu_${i}_kernel`, cpus[i].kernel, timeStamp);
      const /** number */ idle =
          getDiffAndUpdateCounter_(`cpu_${i}_idle`, cpus[i].idle, timeStamp);
      const /** number */ total =
          getDiffAndUpdateCounter_(`cpu_${i}_total`, cpus[i].total, timeStamp);
      /* Total may be zero at first update. */
      const /** number */ percentage =
          total == 0 ? 0 : (user + kernel) / total * 100;
      cpuDataSeries[i].addDataPoint(percentage, timeStamp);
      allKernel += kernel;
      allUser += user;
      allIdle += idle;
    }

    if (isInfoPage()) {
      const /** GeneralCpuType */ generalCpu = SysInternals.generalInfo.cpu;
      generalCpu.core = cpus.length;
      const allTotal = allKernel + allUser + allIdle;
      generalCpu.usage = allTotal == 0 ? 0 : (allKernel + allUser) / allTotal;
      generalCpu.kernel = allTotal == 0 ? 0 : allKernel / allTotal;
      generalCpu.user = allTotal == 0 ? 0 : allUser / allTotal;
      generalCpu.idle = allTotal == 0 ? 0 : allIdle / allTotal;
    }
  }

  /** @const {PromiseResolver} */
  SysInternals.waitCpuInitialized = new PromiseResolver();

  /**
   * Initialize the data series of the page. This function will be called for
   * the first time we get the cpus data.
   * @param {Array<SysInfoApiCpuResult>} cpus
   */
  function initCpuDataSeries_(cpus) {
    if (cpus.length == 0)
      return;
    const dataSeries = SysInternals.dataSeries;
    dataSeries.cpus = [];
    for (let i = 0; i < cpus.length; ++i) {
      const colorIdx = i % CPU_COLOR_SET.length;
      dataSeries.cpus[i] =
          new LineChart.DataSeries(`CPU ${i}`, CPU_COLOR_SET[colorIdx]);
    }
    SysInternals.waitCpuInitialized.resolve();
  }

  /**
   * Handle the new memory data.
   * @param {SysInfoApiMemoryResult} memory
   * @param {number} timeStamp
   */
  function updateMemoryData_(memory, timeStamp) {
    const /** MemoryDataSeriesSet */ memDataSeries =
        SysInternals.dataSeries.memory;
    const /** number */ memUsed = memory.total - memory.available;
    memDataSeries.memUsed.addDataPoint(memUsed, timeStamp);
    const /** number */ swapUsed = memory.swapTotal - memory.swapFree;
    memDataSeries.swapUsed.addDataPoint(swapUsed, timeStamp);
    const /** number */ pswpin =
        getDiffPerSecAndUpdateCounter_('pswpin', memory.pswpin, timeStamp);
    memDataSeries.pswpin.addDataPoint(pswpin, timeStamp);
    const /** number */ pswpout =
        getDiffPerSecAndUpdateCounter_('pswpout', memory.pswpout, timeStamp);
    memDataSeries.pswpout.addDataPoint(pswpout, timeStamp);

    if (isInfoPage()) {
      const /** GeneralMemoryType */ generalMem =
          SysInternals.generalInfo.memory;
      generalMem.total = memory.total;
      generalMem.used = memUsed;
      generalMem.swapTotal = memory.swapTotal;
      generalMem.swapUsed = swapUsed;
    }
  }

  /**
   * Handle the new zram data.
   * @param {SysInfoApiZramResult} zram
   * @param {number} timeStamp
   */
  function updateZramData_(zram, timeStamp) {
    const /** ZramDataSeriesSet */ zramDataSeries =
        SysInternals.dataSeries.zram;
    zramDataSeries.origDataSize.addDataPoint(zram.origDataSize, timeStamp);
    zramDataSeries.comprDataSize.addDataPoint(zram.comprDataSize, timeStamp);
    zramDataSeries.memUsedTotal.addDataPoint(zram.memUsedTotal, timeStamp);
    const /** number */ numReads =
        getDiffPerSecAndUpdateCounter_('numReads', zram.numReads, timeStamp);
    zramDataSeries.numReads.addDataPoint(numReads, timeStamp);
    const /** number */ numWrites =
        getDiffPerSecAndUpdateCounter_('numWrites', zram.numWrites, timeStamp);
    zramDataSeries.numWrites.addDataPoint(numWrites, timeStamp);

    if (isInfoPage()) {
      const /** GeneralZramType */ generalZram = SysInternals.generalInfo.zram;
      generalZram.total = zram.memUsedTotal;
      generalZram.orig = zram.origDataSize;
      generalZram.compr = zram.comprDataSize;
      /* Note: |comprRatio| may be NaN, this is ok because it can remind user
       * there is no comprRatio now. */
      generalZram.comprRatio =
          (zram.origDataSize - zram.comprDataSize) / zram.origDataSize;
    }
  }

  /** @const */
  SysInternals.counterDict_ = {};

  /**
   * Get the increments from the last value to the current value. Return the
   * increments, and store the current value.
   * @param {string} name - The key to identify the counter.
   * @param {number} newValue
   * @param {number} timeStamp
   * @return {number}
   */
  function getDiffAndUpdateCounter_(name, newValue, timeStamp) {
    if (SysInternals.counterDict_[name] == undefined) {
      /** @const {CounterType} */
      SysInternals.counterDict_[name] = {value: newValue, timeStamp: timeStamp};
      return 0;
    }
    const /** CounterType */ counter = SysInternals.counterDict_[name];
    let /** number */ valueDelta = newValue - counter.value;

    /* If the increments of the counter is negative, it means that the counter
     * is circulating. Get the real increments with the |counterMax|. The result
     * is guaranteed to be greater than zero because the maximum difference of
     * two counter never greater than |counterMax|. */
    if (valueDelta < 0) {
      valueDelta += SysInternals.counterMax;
    }
    counter.value = newValue;
    counter.timeStamp = timeStamp;
    return valueDelta;
  }

  /**
   * Return the average increments per second, and store the current value.
   * @param {string} name - The key to identify the counter.
   * @param {number} newValue
   * @param {number} timeStamp
   * @return {number}
   */
  function getDiffPerSecAndUpdateCounter_(name, newValue, timeStamp) {
    const /** number */ oldTimeStamp = SysInternals.counterDict_[name] ?
        SysInternals.counterDict_[name].timeStamp :
        -1;
    const /** number */ valueDelta =
        getDiffAndUpdateCounter_(name, newValue, timeStamp);

    /* If oldTimeStamp is -1, it means that this is the first value of the
     * counter. */
    if (oldTimeStamp == -1)
      return 0;

    /**
     * The time increments, in seconds.
     * @type {number}
     */
    const timeDelta = (timeStamp - oldTimeStamp) / 1000;
    const /** number */ deltaPerSec =
        (timeDelta == 0) ? 0 : valueDelta / timeDelta;
    return deltaPerSec;
  }


  /**
   * Return true if the current page is info page.
   * @return {boolean}
   */
  function isInfoPage() {
    return location.hash == '';
  }

  /**
   * Updata the info page with the current data.
   */
  function updateInfoPage() {
    const /** GeneralCpuType */ cpu = SysInternals.generalInfo.cpu;
    setTextById('sys-internals-infopage-num-of-cpu', cpu.core.toString());
    setTextById(
        'sys-internals-infopage-cpu-usage',
        toPercentageString(cpu.usage, INFO_PAGE_PRECISION));
    setTextById(
        'sys-internals-infopage-cpu-kernel',
        toPercentageString(cpu.kernel, INFO_PAGE_PRECISION));
    setTextById(
        'sys-internals-infopage-cpu-user',
        toPercentageString(cpu.user, INFO_PAGE_PRECISION));
    setTextById(
        'sys-internals-infopage-cpu-idle',
        toPercentageString(cpu.idle, INFO_PAGE_PRECISION));

    const /** GeneralMemoryType */ memory = SysInternals.generalInfo.memory;
    setTextById(
        'sys-internals-infopage-memory-total',
        getValueWithUnit(memory.total, UNITS_MEMORY, UNITBASE_MEMORY));
    setTextById(
        'sys-internals-infopage-memory-used',
        getValueWithUnit(memory.used, UNITS_MEMORY, UNITBASE_MEMORY));
    setTextById(
        'sys-internals-infopage-memory-swap-total',
        getValueWithUnit(memory.swapTotal, UNITS_MEMORY, UNITBASE_MEMORY));
    setTextById(
        'sys-internals-infopage-memory-swap-used',
        getValueWithUnit(memory.swapUsed, UNITS_MEMORY, UNITBASE_MEMORY));

    const /** GeneralZramType */ zram = SysInternals.generalInfo.zram;
    setTextById(
        'sys-internals-infopage-zram-total',
        getValueWithUnit(zram.total, UNITS_MEMORY, UNITBASE_MEMORY));
    setTextById(
        'sys-internals-infopage-zram-orig',
        getValueWithUnit(zram.orig, UNITS_MEMORY, UNITBASE_MEMORY));
    setTextById(
        'sys-internals-infopage-zram-compr',
        getValueWithUnit(zram.compr, UNITS_MEMORY, UNITBASE_MEMORY));
    setTextById(
        'sys-internals-infopage-zram-compr-ratio',
        toPercentageString(zram.comprRatio, INFO_PAGE_PRECISION));
  }

  /**
   * Set the element inner text by the element id.
   * @param {string} id
   * @param {string} text
   */
  function setTextById(id, text) {
    $(id).innerText = text;
  }

  /**
   * Transform the number to percentage string.
   * @param {number} number
   * @param {number} fixed - The percision of the number.
   * @return {string}
   */
  function toPercentageString(number, fixed) {
    const fixedNumber = (number * 100).toFixed(fixed);
    return fixedNumber + '%';
  }

  /**
   * Return the value with a suitable unit. See
   * |LineChart.getSuitableUint()|.
   * @param {number} value
   * @param {Array<string>} units
   * @param {number} unitBase
   * @return {string}
   */
  function getValueWithUnit(value, units, unitBase) {
    const result = LineChart.getSuitableUnit(value, units, unitBase);
    const suitableValue = result.value.toFixed(INFO_PAGE_PRECISION);
    const unitStr = units[result.unitIdx];
    return `${suitableValue} ${unitStr}`;
  }

  /** @type {PromiseResolver} - For test. */
  SysInternals.waitOnHashChangeCompleted = null;

  /**
   * Handle the url onhashchange event.
   */
  function onHashChange_() {
    const /** string */ hash = location.hash;
    switch (hash) {
      case PAGE_HASH.INFO:
        setupInfoPage();
        break;
      case PAGE_HASH.CPU:
        /* Wait for cpu dataseries initialized. */
        SysInternals.waitCpuInitialized.promise.then(setupCPUPage);
        break;
      case PAGE_HASH.MEMORY:
        setupMemoryPage();
        break;
      case PAGE_HASH.ZRAM:
        setupZramPage();
        break;
    }
    const /** Element */ title = $('sys-internals-drawer-title');
    const /** Element */ infoPage = $('sys-internals-infopage-root');
    if (isInfoPage()) {
      title.innerText = 'Info';
      infoPage.removeAttribute('hidden');
    } else {
      title.innerText = hash.slice(1);
      infoPage.setAttribute('hidden', '');
    }

    if (SysInternals.waitOnHashChangeCompleted) {
      SysInternals.waitOnHashChangeCompleted.resolve();
    }
  }

  /**
   * Set the current page to info page.
   */
  function setupInfoPage() {
    const /** LineChart.LineChart */ lineChart = SysInternals.lineChart;
    lineChart.clearAllSubChart();
    updateInfoPage();
  }

  /**
   * Set the current page to cpu page.
   */
  function setupCPUPage() {
    /* This function is async so we need to check the page is still CPU page. */
    if (location.hash != PAGE_HASH.CPU)
      return;

    const /** Array<LineChart.DataSeries> */ cpuDataSeries =
        SysInternals.dataSeries.cpus;
    const /** LineChart.LineChart */ lineChart = SysInternals.lineChart;
    const /** number */ UNITBASE_NO_CARRY = 1;
    const /** Array<string> */ UNIT_PURE_NUMBER = [''];
    lineChart.setSubChart(
        UnitLabelAlign.LEFT, UNIT_PURE_NUMBER, UNITBASE_NO_CARRY);
    const /** Array<string> */ UNIT_PERCENTAGE = ['%'];
    lineChart.setSubChart(
        UnitLabelAlign.RIGHT, UNIT_PERCENTAGE, UNITBASE_NO_CARRY);
    lineChart.setSubChartMaxValue(UnitLabelAlign.RIGHT, 100);
    for (let i = 0; i < cpuDataSeries.length; ++i) {
      lineChart.addDataSeries(UnitLabelAlign.RIGHT, cpuDataSeries[i]);
    }
  }

  /**
   * Set the current page to memory page.
   */
  function setupMemoryPage() {
    const /** LineChart.LineChart */ lineChart = SysInternals.lineChart;
    const /** MemoryDataSeriesSet */ memDataSeries =
        SysInternals.dataSeries.memory;
    lineChart.setSubChart(
        UnitLabelAlign.LEFT, UNITS_NUMBER_PER_SECOND,
        UNITBASE_NUMBER_PER_SECOND);
    lineChart.setSubChart(UnitLabelAlign.RIGHT, UNITS_MEMORY, UNITBASE_MEMORY);
    lineChart.addDataSeries(UnitLabelAlign.RIGHT, memDataSeries.memUsed);
    lineChart.addDataSeries(UnitLabelAlign.RIGHT, memDataSeries.swapUsed);
    lineChart.addDataSeries(UnitLabelAlign.LEFT, memDataSeries.pswpin);
    lineChart.addDataSeries(UnitLabelAlign.LEFT, memDataSeries.pswpout);
  }

  /**
   * Set the current page to zram page.
   */
  function setupZramPage() {
    const /** LineChart.LineChart */ lineChart = SysInternals.lineChart;
    const /** ZramDataSeriesSet */ zramDataSeries =
        SysInternals.dataSeries.zram;
    lineChart.setSubChart(
        UnitLabelAlign.LEFT, UNITS_NUMBER_PER_SECOND,
        UNITBASE_NUMBER_PER_SECOND);
    lineChart.setSubChart(UnitLabelAlign.RIGHT, UNITS_MEMORY, UNITBASE_MEMORY);
    lineChart.addDataSeries(UnitLabelAlign.RIGHT, zramDataSeries.origDataSize);
    lineChart.addDataSeries(UnitLabelAlign.RIGHT, zramDataSeries.comprDataSize);
    lineChart.addDataSeries(UnitLabelAlign.RIGHT, zramDataSeries.memUsedTotal);
    lineChart.addDataSeries(UnitLabelAlign.LEFT, zramDataSeries.numReads);
    lineChart.addDataSeries(UnitLabelAlign.LEFT, zramDataSeries.numWrites);
  }

  return {
    dataSeries: dataSeries,
    generalInfo: generalInfo,
    initialize: initialize,
  };
});

document.addEventListener('DOMContentLoaded', function() {
  SysInternals.initialize();
});
