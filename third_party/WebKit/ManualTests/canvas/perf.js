// perf.js
(function(window) {

let TIME_WINDOW = 10;

let timers = [];
let memory = [];
const bar = document.createElement("div");
const barTime = document.createElement("div");
const barExtra = document.createElement("div");
let lastTime = window.performance.now();

function ntom(n) {
  if(n == 0) return '0B';
  const sizes = [ "B", "KB", "MB", "GB", "TB" ];
  const i = Math.floor(Math.log(n) / Math.log(1024));
  return parseInt(n / Math.pow(1024, i)) + sizes[i];
}

function ntos(n) {
  let unit = "s";
  if (n < 1) {
    unit = "ms";
    n *= 1000;
  }
  const p = Math.floor(n);
  let r = "" + Math.round((n-p)*100);
  while (r.length < 2) r = "0" + r;
  return p + "." + r + unit;
}

function ntoserr(avg, std) {
  let unit = "s";
  if (avg < 1) {
    unit = "ms";
    avg *= 1000;
    std *= 1000;
  }
  const p = Math.floor(avg);
  let r = "" + Math.round((avg-p)*100);
  while (r.length < 2) r = "0" + r;

  const p2 = Math.floor(std);
  let r2 = "" + Math.round((std-p2)*100);
  while (r2.length < 2) r2 = "0" + r2;

  return p + "." + r + "&plusmn;" + p2 + "." + r2 + unit;
}

var finished = false;

function perfNext() {
  const scripts = JSON.parse(localStorage.getItem("scripts")) || [];
  const next = scripts.shift();
  if (!next) return;
  localStorage.setItem("scripts", JSON.stringify(scripts));
  console.log(next);
  window.location.href = next;
  finished = true;
}

function done(average, stddev) {
  if (finished) return;
  let perf = JSON.parse(localStorage.getItem("perf")) || [];

  let name = window.location.pathname.split('/');
  name = name[name.length-1];
  perf = perf.filter(x => x[0] != name);
  perf.push([ name, average, stddev ]);

  localStorage.setItem("perf", JSON.stringify(perf));

  perfNext();
}

function measure() {
  const t = window.performance.now();
  const delta = (t - lastTime)/1000.0;
  lastTime = t;
  timers.push(delta);
  memory.push(window.performance.memory.usedJSHeapSize);

  const sum = timers.reduce((a, b) => a + b, 0);
  const average = sum / timers.length;
  let isDone = false;
  while(timers.length > Math.ceil(TIME_WINDOW/average)) {
    timers.shift();
    memory.shift();
    isDone = true;
  }

  let worst = timers[0];
  let stddev = 0.0;
  for (let i = 0; i < timers.length; ++i) {
    stddev += Math.pow(timers[i] - average, 2);
  }
  stddev = Math.sqrt(stddev/timers.length);

  barTime.innerHTML = `avg: ${ntoserr(average, stddev)}`;

  const summ = memory.reduce((a, b) => a + b, 0) / memory.length;
  barExtra.innerHTML = `JSheap:${ntom(summ)} [${ntos(sum)}]`;
  requestAnimationFrame(measure);

  if (isDone) {
    done(average, stddev);
  }
}

document.addEventListener("DOMContentLoaded", () => {
  bar.style.position = "fixed";
  bar.style.backgroundColor = "#444";
  bar.style.color = "#EEE";
  bar.style.bottom = "0";
  bar.style.width = "600px";
  bar.style.left = "50%";
  bar.style.marginLeft = "-302px";
  bar.style.fontFamily = "arial";
  bar.style.padding = "10px 4px";
  bar.style.textAlign = "center";

  barTime.style.width = "300px";
  barTime.style.display = "inline-block";

  barExtra.style.width = "300px";
  barExtra.style.display = "inline-block";

  bar.appendChild(barTime);
  bar.appendChild(barExtra);

  document.body.appendChild(bar);
  measure();
});

if (window.performance.memory.usedJSHeapSize == 10000000) {
  console.error("Start Chrome with --disable-gpu-vsync --enable-precise-memory-info for this test");
}

window.perfNext = perfNext;

})(window);
