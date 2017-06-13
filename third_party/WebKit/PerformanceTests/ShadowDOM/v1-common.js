function createDeepDiv(nest) {
  const x = document.createElement('div');
  if (nest > 0)
    x.appendChild(createDeepDiv(nest - 1));
  return x;
}

function createDeepComponent(nest) {
  // Creating a nested component where a node is re-distributed into a slot in a despendant shadow tree
  const x = document.createElement('div');
  x.appendChild(document.createElement('slot'));
  x.appendChild(document.createElement('p'));
  const sr = x.attachShadow({ mode: 'open' });
  if (nest == 0) {
    sr.appendChild(document.createElement('slot'));
  } else {
    sr.appendChild(createDeepComponent(nest - 1));
  }
  return x;
}

function createHostTree(hostChildren) {
  return createHostTreeWith({
    hostChildren,
    createChildFunction: () => document.createElement('div'),
  });
}

function createHostTreeWithDeepComponentChild(hostChildren) {
  return createHostTreeWith({
    hostChildren,
    createChildFunction: () => createDeepComponent(100),
  });
}

function createHostTreeWith({hostChildren, createChildFunction}) {
  const host = document.createElement('div');
  host.id = 'host';
  for (let i = 0; i < hostChildren; ++i) {
    const div = createChildFunction();
    host.appendChild(div);
  }

  const shadowRoot = host.attachShadow({ mode: 'open' });
  shadowRoot.appendChild(document.createElement('slot'));
  return host;
}

function runHostChildrenMutationThenGetDistribution(host, loop) {
  const slot = host.shadowRoot.querySelector('slot');

  const div = document.createElement('div');
  for (let i = 0; i < loop; ++i) {
    const firstChild = host.firstChild;
    firstChild.remove();
    host.appendChild(firstChild);
    slot.assignedNodes({ flatten: true });
  }
}

function runHostChildrenMutationThenLayout(host, loop) {
  const div = document.createElement('div');
  for (let i = 0; i < loop; ++i) {
    const firstChild = host.firstChild;
    firstChild.remove();
    host.appendChild(firstChild);
    PerfTestRunner.forceLayout();
  }
}
