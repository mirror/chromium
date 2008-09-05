// This is scaled-down version of the Richards benchmark. Today, this
// is by far the most complicated JavaScript code our VM can execute
// and for that reason, I have turned it into a test run by the
// JavaScript testing framework (kasperl 11/08/06).


// ----------------------------------------------------------------------------


/**
 * JavaScript implementation of the OO Richards benchmark.
 *
 * The richards benchmark simulates the task dispatcher of an
 * operating system.
 **/

function run_richards() {
  var count = 100;
  
  var scheduler = new Scheduler();
  scheduler.add_idle_task(ID_IDLE, 0, null, count);
  
  var wkq = new Packet(null, ID_WORKER, KIND_WORK);
  wkq = new Packet(wkq,  ID_WORKER, KIND_WORK);
  scheduler.add_worker_task(ID_WORKER, 1000, wkq);
  
  wkq = new Packet(null, ID_DEVICE_A, KIND_DEVICE);
  wkq = new Packet(wkq,  ID_DEVICE_A, KIND_DEVICE);
  wkq = new Packet(wkq,  ID_DEVICE_A, KIND_DEVICE);
  scheduler.add_handler_task(ID_HANDLER_A, 2000, wkq);
  
  wkq = new Packet(null, ID_DEVICE_B, KIND_DEVICE);
  wkq = new Packet(wkq,  ID_DEVICE_B, KIND_DEVICE);
  wkq = new Packet(wkq,  ID_DEVICE_B, KIND_DEVICE);
  scheduler.add_handler_task(ID_HANDLER_B, 3000, wkq);
  
  scheduler.add_device_task(ID_DEVICE_A, 4000, null);

  scheduler.add_device_task(ID_DEVICE_B, 5000, null);

  print("Running Richards benchmark...");
  var start = new Date();
  scheduler.schedule();
  var end = new Date();
  var elapsed = end - start;
  
  print("Queue count : " + scheduler.queue_count);
  print("Hold count  : " + scheduler.hold_count);
}

/* --- *
 * S c h e d u l e r
 * --- */
  
function Scheduler() {
  this.queue_count = 0;
  this.hold_count = 0;
  this.tasks = new Array(NUMBER_OF_IDS);
  this.list = null;
  this.current_tcb = null;
  this.current_id = null;
}

var ID_IDLE       = 0;
var ID_WORKER     = 1;
var ID_HANDLER_A  = 2;
var ID_HANDLER_B  = 3;
var ID_DEVICE_A   = 4;
var ID_DEVICE_B   = 5;
var NUMBER_OF_IDS = 6;

var KIND_DEVICE   = 0;
var KIND_WORK     = 1;

Scheduler.prototype.add_idle_task = function (id, pri, queue, count) {
  this.add_running_task(id, pri, queue, new IdleTask(this, 1, count));
}

Scheduler.prototype.add_worker_task = function (id, pri, wkq) {
  this.add_task(id, pri, wkq, new WorkerTask(this, ID_HANDLER_A, 0));
}

Scheduler.prototype.add_handler_task = function (id, pri, wkq) {
  this.add_task(id, pri, wkq, new HandlerTask(this));
}

Scheduler.prototype.add_device_task = function (id, pri, wkq) {
  this.add_task(id, pri, wkq, new DeviceTask(this))
}

Scheduler.prototype.add_running_task = function (id, pri, wkq, task) {
  this.add_task(id, pri, wkq, task);
  this.current_tcb.set_running();
}

Scheduler.prototype.add_task = function (id, pri, wkq, task) {
  this.current_tcb = new TaskControlBlock(this.list, id, pri, wkq, task);
  this.list = this.current_tcb;
  this.tasks[id] = this.current_tcb;
}

Scheduler.prototype.schedule = function () {
  this.current_tcb = this.list;
  while (this.current_tcb != null) {
    if (this.current_tcb.is_held_or_suspended()) {
      this.current_tcb = this.current_tcb.link;
    } else {
      this.current_id = this.current_tcb.id;
      this.current_tcb = this.current_tcb.run();
    }
  }
}

Scheduler.prototype.release = function (id) {
  var t = this.tasks[id];
  if (t == null) return t;
  t.not_held();
  if (t.pri > this.current_tcb.pri) return t;
  else return this.current_tcb;
}

Scheduler.prototype.hold_current = function () {
  this.hold_count++;
  this.current_tcb.held();
  return this.current_tcb.link;
}

Scheduler.prototype.suspend_current = function () {
  this.current_tcb.suspended();
  return this.current_tcb;
}

Scheduler.prototype.queue = function (packet) {
  var t = this.tasks[packet.id];
  if (t == null) return t;
  this.queue_count++;
  packet.link = null;
  packet.id = this.current_id;
  return t.check_priority_add(this.current_tcb, packet);
}

/* --- *
 * T a s k   C o n t r o l   B l o c k
 * --- */
 
function TaskControlBlock(link, id, pri, wkq, task) {
  this.link = link;
  this.id = id;
  this.pri = pri;
  this.wkq = wkq;
  this.task = task;
  if (wkq == null) this.state = STATE_SUSPENDED;
  else this.state = STATE_SUSPENDED_RUNNABLE;
}

var STATE_RUNNING            = 0;
var STATE_RUNNABLE           = 1;
var STATE_SUSPENDED          = 2;
var STATE_HELD               = 4;
var STATE_SUSPENDED_RUNNABLE = STATE_SUSPENDED | STATE_RUNNABLE;
var STATE_NOT_HELD           = ~STATE_HELD;

TaskControlBlock.prototype.set_running = function () {
  this.state = STATE_RUNNING;
}

TaskControlBlock.prototype.not_held = function () {
  this.state = this.state & STATE_NOT_HELD;
}

TaskControlBlock.prototype.held = function () {
  this.state = this.state | STATE_HELD;
}

TaskControlBlock.prototype.is_held_or_suspended = function () {
  return (this.state & STATE_HELD) != 0
      || (this.state == STATE_SUSPENDED);
}

TaskControlBlock.prototype.suspended = function () {
  this.state = this.state | STATE_SUSPENDED;
}

TaskControlBlock.prototype.run = function () {
  var packet;
  if (this.state == STATE_SUSPENDED_RUNNABLE) {
    packet = this.wkq;
    this.wkq = packet.link;
    if (this.wkq == null) this.state = STATE_RUNNING;
    else this.state = STATE_RUNNABLE;
  } else {
    packet = null;
  }
  return this.task.run(packet);
}

TaskControlBlock.prototype.check_priority_add = function (task, packet) {
  if (this.wkq == null) {
    this.wkq = packet;
    this.state = this.state | STATE_RUNNABLE;
    if (this.pri > task.pri) return this;
  } else {
    this.wkq = packet.add_to(this.wkq);
  }
  return task;
}

TaskControlBlock.prototype.toString = function () {
  return "tcb { " + this.task + "@" + this.state + " }";
}

/* --- *
 * I d l e   T a s k 
 * --- */
 
function IdleTask(scheduler, v1, v2) {
  this.scheduler = scheduler;
  this.v1 = v1;
  this.v2 = v2;
}

IdleTask.prototype.run = function (packet) {
  print("[Idle]");
  this.v2--;
  if (this.v2 == 0) return this.scheduler.hold_current();
  if ((this.v1 & 1) == 0) {
    this.v1 = this.v1 >> 1;
    return this.scheduler.release(ID_DEVICE_A);
  } else {
    this.v1 = (this.v1 >> 1) ^ 0xD008;
    return this.scheduler.release(ID_DEVICE_B);
  }
}

IdleTask.prototype.toString = function () {
  return "IdleTask"
}

/* --- *
 * W o r k e r   T a s k
 * --- */
 
function WorkerTask(scheduler, v1, v2) {
  this.scheduler = scheduler;
  this.v1 = v1;
  this.v2 = v2;
}

WorkerTask.prototype.run = function (packet) {
  print("[Worker]");
  if (packet == null) {
    return this.scheduler.suspend_current();
  } else {
    if (this.v1 == ID_HANDLER_A) this.v1 = ID_HANDLER_B;
    else this.v1 = ID_HANDLER_A;
    packet.id = this.v1;
    packet.a1 = 0;
    for (var i = 0; i < DATA_SIZE; i++) {
      this.v2++;
      if (this.v2 > 26) this.v2 = 1;
      packet.a2[i] = this.v2;
    }
    return this.scheduler.queue(packet);
  }
}

WorkerTask.prototype.toString = function () {
  return "WorkerTask";
}

/* --- *
 * D e v i c e   T a s k
 * --- */
 
function DeviceTask(scheduler) {
  this.scheduler = scheduler;
}

DeviceTask.prototype.run = function (packet) {
  print("[Device]");
  if (packet == null) {
    if (this.v1 == null) return this.scheduler.suspend_current();
    var v = this.v1;
    this.v1 = null;
    return this.scheduler.queue(v);
  } else {
    this.v1 = packet;
    return this.scheduler.hold_current();
  }
}

DeviceTask.prototype.toString = function () {
  return "DeviceTask";
}

/* --- *
 * H a n d l e r   T a s k
 * --- */
 
function HandlerTask(scheduler) {
  this.scheduler = scheduler;
  this.v1 = null;
  this.v2 = null;
}

HandlerTask.prototype.toString = function () {
  return "HandlerTask";
}

HandlerTask.prototype.run = function (packet) {
  print("[Handler]");
  if (packet != null) {
    if (packet.kind == KIND_WORK) this.v1 = packet.add_to(this.v1);
    else this.v2 = packet.add_to(this.v2);
  }
  if (this.v1 != null) {
    var count = this.v1.a1;
    var v;
    if (count < DATA_SIZE) {
      if (this.v2 != null) {
        v = this.v2;
        this.v2 = this.v2.link;
        v.a1 = this.v1.a2[count];
        this.v1.a1 = count + 1;
        return this.scheduler.queue(v);
      }
    } else {
      v = this.v1;
      this.v1 = this.v1.link;
      return this.scheduler.queue(v);
    }
  }
  return this.scheduler.suspend_current();
}

/* --- *
 * P a c k e t
 * --- */
 
var DATA_SIZE = 4;

function Packet(link, id, kind) {
  this.link = link;
  this.id = id;
  this.kind = kind;
  this.a1 = 0;
  this.a2 = new Array(DATA_SIZE);
}

Packet.prototype.add_to = function (queue) {
  this.link = null;
  if (queue == null) return this;
  var next = queue;
  while ((peek = next.link) != null)
    next = peek;
  next.link = this;
  return queue;
}

Packet.prototype.toString = function () {
  return "Packet";
}


/* --- *
 * R u n   t h e   b e n c h m a r k
 * --- */
run_richards();


