self.addEventListener('message', (e) => {
  const test1 = 'paymentManager' in self.registration;
  const test2 = 'userHint' in self.registration.paymentManager;
  const test3 = 'instruments' in self.registration.paymentManager;
  e.data.port.postMessage(test1 && test2 && test3);
});
