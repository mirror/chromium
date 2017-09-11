// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @const */ var SysInternals = {};

/** @const */
SysInternals.dataSeries = {};

/** @type {Array<LineChart.DataSeries>} */
SysInternals.dataSeries.cpus = null;

/**
 * @typedef {{
 *   memUsed: LineChart.DataSeries,
 *   swapUsed: LineChart.DataSeries,
 *   pswpin: LineChart.DataSeries,
 *   pswpout: LineChart.DataSeries
 * }}
 */
var MemoryDataSeriesSet;

/** @const {MemoryDataSeriesSet} */
SysInternals.dataSeries.memory = {
  memUsed: null,
  swapUsed: null,
  pswpin: null,
  pswpout: null,
};

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

/** @const {ZramDataSeriesSet} */
SysInternals.dataSeries.zram = {
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
SysInternals.generalInfo = {};

/** @type {GeneralCpuType} */
SysInternals.generalInfo.cpu = {
  core: '0',
  idle: '0',
  kernel: '0',
  usage: '0.00%',
  user: '0',
};

/** @type {GeneralMemoryType} */
SysInternals.generalInfo.memory = {
  swapTotal: '0.00 B',
  swapUsed: '0.00 B',
  total: '0.00 B',
  used: '0.00 B',
};

/** @type {GeneralZramType} */
SysInternals.generalInfo.zram = {
  compr: '0.00 B',
  comprRatio: '0.00%',
  orig: '0.00 B',
  total: '0.00 B',
};

/** @const */
SysInternals.counter = {};

/**
 * The page update period, in milliseconds.
 * @const {number}
 */
const UPDATE_PERIOD = 1000;

/** @const {Array<string>} */
const UNIT_NUMBER_PER_SECOND = ['/s', 'K/s', 'M/s'];

/** @const {number} */
const UNITBASE_NUMBER_PER_SECOND = 1000;

/** @const {Array<string>} */
const UNIT_MEMORY = ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];

/** @const {number} */
const UNITBASE_MEMORY = 1024;

/** @const {Array<string>} */
const CPU_COLOR_SET = [
  '#2fa2ff', '#ff93e2', '#a170d0', '#fe6c6c', '#2561a4', '#15b979', '#fda941',
  '#79dbcd'
];

/** @const {Array<string>} */
const MEMORY_COLOR_SET = ['#fa4e30', '#8d6668', '#73418c', '#41205e'];

/** @const {Array<string>} - Note: 4th and 5th colors use black menu text. */
const ZRAM_COLOR_SET = ['#9cabd4', '#4a4392', '#dcfaff', '#fff9c9', '#ffa3ab'];

/** @type {boolean} - Tag used by browser test. */
var DONT_SEND_UPDATE_REQUEST;

/** @const */
const UnitLabelAlign = LineChart.UnitLabelAlign;

/**
 * Initialize the whole page.
 */
function initialize() {
  initPage_();
  initDataSeries_();
  initChart_();
  sendUpdateRequest_();

  /* Initialize with the current url hash value. */
  onHashChange_();
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

/**
 * Open the navbar drawer menu.
 */
function openDrawer() {
  $('sys-internals-drawer').removeAttribute('hidden');
  /* Preventing CSS transition blocking by JS. */
  setTimeout(function() {
    $('sys-internals-drawer').className = '';
    $('sys-internals-drawer-menu').className = '';
  });
}

/**
 * Close the navbar drawer menu.
 */
function closeDrawer() {
  const /** Element */ drawer = $('sys-internals-drawer');
  drawer.className = 'hidden';
  $('sys-internals-drawer-menu').className = 'hidden';
  /* Wait for the drawer close. */
  setTimeout(drawer.setAttribute.bind(drawer), 200, 'hidden', '');
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

  const /** ZramDataSeriesSet */ zramDataSeries = SysInternals.dataSeries.zram;
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
 * Wait until next period, and then send the update request to backend to update
 * the system information.
 */
function sendUpdateRequest_() {
  if (DONT_SEND_UPDATE_REQUEST)
    return;

  const /** number */ now = Date.now();
  const /** number */ sleepTime = UPDATE_PERIOD - now % UPDATE_PERIOD;
  const /** number */ timeTick = now + sleepTime;
  setTimeout(function() {
    cr.sendWithPromise('getSysInfo').then(function(data) {
      handleUpdateData(data, timeTick);
    });
  }, sleepTime);
}

/**
 * Handle the new data which received from backend.
 * @param {SysInfoApiResult} data
 * @param {number} timeTick - The time tick for these data.
 */
function handleUpdateData(data, timeTick) {
  SysInternals.counterMax = data.const.counterMax;

  updateCpuData_(data.cpus, timeTick);
  updateMemoryData_(data.memory, timeTick);
  updateZramData_(data.zram, timeTick);

  if (location.hash == '') {
    updateInfoPage();
  } else {
    SysInternals.lineChart.updateEndTime(timeTick);
  }
  sendUpdateRequest_();
}

/**
 * Handle the new cpu data.
 * @param {Array<SysInfoApiCpuResult>} cpus
 * @param {number} timeTick
 */
function updateCpuData_(cpus, timeTick) {
  if (SysInternals.dataSeries.cpus == null) {
    initCpuDataSeries_(cpus);
  }
  const /** Array<LineChart.DataSeries> */ cpuDataSeries =
      SysInternals.dataSeries.cpus;
  if (cpus.length != cpuDataSeries.length) {
    console.warn('Cpu Data: Number of procesor changed.');
    return;
  }
  let allKernel = 0;
  let allUser = 0;
  let allIdle = 0;
  for (let i = 0; i < cpus.length; ++i) {
    /* Check if this cpu is offline */
    if (cpus[i].total == 0) {
      cpuDataSeries[i].addDataPoint(0, timeTick);
      continue;
    }
    const /** number */ user =
        getDiffAndUpdateCounter_('cpu' + i + 'user', cpus[i].user, timeTick);
    const /** number */ kernel = getDiffAndUpdateCounter_(
        'cpu' + i + 'kernel', cpus[i].kernel, timeTick);
    const /** number */ idle =
        getDiffAndUpdateCounter_('cpu' + i + 'idle', cpus[i].idle, timeTick);
    const /** number */ total =
        getDiffAndUpdateCounter_('cpu' + i + 'total', cpus[i].total, timeTick);
    /* total may be zero at first update. */
    const /** number */ percentage =
        total == 0 ? 0 : (user + kernel) / total * 100;
    cpuDataSeries[i].addDataPoint(percentage, timeTick);
    allKernel += kernel;
    allUser += user;
    allIdle += idle;
  }

  if (isInfoPage()) {
    const /** GeneralCpuType */ generalCpu = SysInternals.generalInfo.cpu;
    generalCpu.core = cpus.length.toString();
    const allTotal = allKernel + allUser + allIdle;
    const /** number */ usageNum =
        allTotal == 0 ? 0 : (allKernel + allUser) / allTotal;
    generalCpu.usage = toPercentageString(usageNum, 2);
    generalCpu.kernel = allKernel.toString();
    generalCpu.user = allUser.toString();
    generalCpu.idle = allIdle.toString();
  }
}

/**
 * Initialize the data series of the page. This function will be called when we
 * first time get the cpus data.
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
        new LineChart.DataSeries('CPU ' + i, CPU_COLOR_SET[colorIdx]);
  }
}

/**
 * Handle the new memory data.
 * @param {SysInfoApiMemoryResult} memory
 * @param {number} timeTick
 */
function updateMemoryData_(memory, timeTick) {
  const /** MemoryDataSeriesSet */ memDataSeries =
      SysInternals.dataSeries.memory;
  const /** number */ memUsed = memory.total - memory.available;
  memDataSeries.memUsed.addDataPoint(memUsed, timeTick);
  const /** number */ swapUsed = memory.swapTotal - memory.swapFree;
  memDataSeries.swapUsed.addDataPoint(swapUsed, timeTick);
  const /** number */ pswpin =
      getDiffAndUpdateCounter_('pswpin', memory.pswpin, timeTick);
  memDataSeries.pswpin.addDataPoint(pswpin, timeTick);
  const /** number */ pswpout =
      getDiffAndUpdateCounter_('pswpout', memory.pswpout, timeTick);
  memDataSeries.pswpout.addDataPoint(pswpout, timeTick);

  if (isInfoPage()) {
    const /** GeneralMemoryType */ generalMem = SysInternals.generalInfo.memory;
    generalMem.total =
        getValueWithUnit(memory.total, UNIT_MEMORY, UNITBASE_MEMORY);
    generalMem.used = getValueWithUnit(memUsed, UNIT_MEMORY, UNITBASE_MEMORY);
    generalMem.swapTotal =
        getValueWithUnit(memory.swapTotal, UNIT_MEMORY, UNITBASE_MEMORY);
    generalMem.swapUsed =
        getValueWithUnit(swapUsed, UNIT_MEMORY, UNITBASE_MEMORY);
  }
}

/**
 * Handle the new zram data.
 * @param {SysInfoApiZramResult} zram
 * @param {number} timeTick
 */
function updateZramData_(zram, timeTick) {
  const /** ZramDataSeriesSet */ zramDataSeries = SysInternals.dataSeries.zram;
  zramDataSeries.origDataSize.addDataPoint(zram.origDataSize, timeTick);
  zramDataSeries.comprDataSize.addDataPoint(zram.comprDataSize, timeTick);
  zramDataSeries.memUsedTotal.addDataPoint(zram.memUsedTotal, timeTick);
  const /** number */ numReads =
      getDiffAndUpdateCounter_('numReads', zram.numReads, timeTick);
  zramDataSeries.numReads.addDataPoint(numReads, timeTick);
  const /** number */ numWrites =
      getDiffAndUpdateCounter_('numWrites', zram.numWrites, timeTick);
  zramDataSeries.numWrites.addDataPoint(numWrites, timeTick);

  if (isInfoPage()) {
    const /** GeneralZramType */ generalZram = SysInternals.generalInfo.zram;
    generalZram.total =
        getValueWithUnit(zram.memUsedTotal, UNIT_MEMORY, UNITBASE_MEMORY);
    generalZram.orig =
        getValueWithUnit(zram.origDataSize, UNIT_MEMORY, UNITBASE_MEMORY);
    generalZram.compr =
        getValueWithUnit(zram.comprDataSize, UNIT_MEMORY, UNITBASE_MEMORY);
    /**
     * Note: This value may be NaN.
     * @type {number}
     */
    const comprRatioNum =
        (zram.origDataSize - zram.comprDataSize) / zram.origDataSize;
    generalZram.comprRatio = toPercentageString(comprRatioNum, 2);
  }
}

/**
 * @typedef {{value: number, timeTick: number}}
 */
var CounterType;

/**
 * Get the increments from the last value to the current value. Return the rate
 * of the increments, and store the current value.
 * @param {string} name - The key to identify the counter.
 * @param {number} newValue
 * @param {number} timeTick
 * @return {number}
 */
function getDiffAndUpdateCounter_(name, newValue, timeTick) {
  const /** CounterType */ counter = SysInternals.counter[name];
  if (counter == undefined) {
    /** @type {CounterType} */
    SysInternals.counter[name] = {value: newValue, timeTick: timeTick};
    return 0;
  }
  let /** number */ valueDelta = newValue - counter.value;

  /* If the increments of the counter is negative, it means that the counter is
   * circulating. Get the real increments with the |counterMax|. */
  if (valueDelta < 0) {
    valueDelta %= SysInternals.counterMax;
    valueDelta += SysInternals.counterMax;
  }
  /**
   * The time increments, in seconds.
   * @type {number}
   */
  const timeDelta = (timeTick - counter.timeTick) / 1000;
  SysInternals.counter[name].value = newValue;
  SysInternals.counter[name].timeTick = timeTick;
  const /** number */ diffPerSec =
      (timeDelta == 0) ? 0 : valueDelta / timeDelta;
  return diffPerSec;
}


/**
 * Return true if the current page is info page.
 * @return {boolean}
 */
function isInfoPage() {
  return location.hash == '';
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
  const suitableValue = result.value.toFixed(2);
  const unitStr = units[result.unitIdx];
  return `${suitableValue} ${unitStr}`;
}

/**
 * Updata the info page with the current data.
 */
function updateInfoPage() {
  const /** GeneralCpuType */ cpu = SysInternals.generalInfo.cpu;
  setTextById('sys-internals-infopage-num-of-cpu', cpu.core);
  setTextById('sys-internals-infopage-cpu-usage', cpu.usage);
  setTextById('sys-internals-infopage-cpu-kernel', cpu.kernel);
  setTextById('sys-internals-infopage-cpu-user', cpu.user);
  setTextById('sys-internals-infopage-cpu-idle', cpu.idle);

  const /** GeneralMemoryType */ memory = SysInternals.generalInfo.memory;
  setTextById('sys-internals-infopage-memory-total', memory.total);
  setTextById('sys-internals-infopage-memory-used', memory.used);
  setTextById('sys-internals-infopage-memory-swap-total', memory.swapTotal);
  setTextById('sys-internals-infopage-memory-swap-used', memory.swapUsed);

  const /** GeneralZramType */ zram = SysInternals.generalInfo.zram;
  setTextById('sys-internals-infopage-zram-total', zram.total);
  setTextById('sys-internals-infopage-zram-orig', zram.orig);
  setTextById('sys-internals-infopage-zram-compr', zram.compr);
  setTextById('sys-internals-infopage-zram-compr-ratio', zram.comprRatio);
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
 * Handle the url onhashchange event.
 */
function onHashChange_() {
  const /** string */ hash = location.hash;
  switch (hash) {
    case '':
      setupInfoPage();
      break;
    case '#CPU':
      setupCPUPage();
      break;
    case '#Memory':
      setupMemoryPage();
      break;
    case '#Zram':
      setupZramPage();
      break;
  }
  const /** Element */ title = $('sys-internals-drawer-title');
  const /** Element */ infoPage = $('sys-internals-infopage-root');
  if (hash == '') {
    title.innerText = 'Info';
    infoPage.removeAttribute('hidden');
  } else {
    title.innerText = hash.slice(1);
    infoPage.setAttribute('hidden', '');
  }
  closeDrawer();
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
  /* The cpu data series will initialize after we get the system cpu data. If
   * cpu data is undefined, we will set a timeout to wait for the data series
   * initialize and then call the function again. Because of this, we need to
   * check the page is still CPU page.*/
  if (location.hash != '#CPU')
    return;

  const cpuDataSeries /** Array<LineChart.DataSeries> */ =
      SysInternals.dataSeries.cpus;
  if (cpuDataSeries == null) {
    setTimeout(setupCPUPage, 1000);
    return;
  }

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
      UnitLabelAlign.LEFT, UNIT_NUMBER_PER_SECOND, UNITBASE_NUMBER_PER_SECOND);
  lineChart.setSubChart(UnitLabelAlign.RIGHT, UNIT_MEMORY, UNITBASE_MEMORY);
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
  const /** ZramDataSeriesSet */ zramDataSeries = SysInternals.dataSeries.zram;
  lineChart.setSubChart(
      UnitLabelAlign.LEFT, UNIT_NUMBER_PER_SECOND, UNITBASE_NUMBER_PER_SECOND);
  lineChart.setSubChart(UnitLabelAlign.RIGHT, UNIT_MEMORY, UNITBASE_MEMORY);
  lineChart.addDataSeries(UnitLabelAlign.RIGHT, zramDataSeries.origDataSize);
  lineChart.addDataSeries(UnitLabelAlign.RIGHT, zramDataSeries.comprDataSize);
  lineChart.addDataSeries(UnitLabelAlign.RIGHT, zramDataSeries.memUsedTotal);
  lineChart.addDataSeries(UnitLabelAlign.LEFT, zramDataSeries.numReads);
  lineChart.addDataSeries(UnitLabelAlign.LEFT, zramDataSeries.numWrites);
}

document.addEventListener('DOMContentLoaded', function() {
  initialize();
});
