onconnect = e => {
  if (self.count === undefined)
    self.count = 0;
  self.count++;
  e.ports[0].postMessage(self.count);
};
