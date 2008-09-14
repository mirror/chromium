/* Allocate million garbage objects. */

start = new Date();

function Node() {
  this.left = null;
  this.right = null;
}

// NUM_NODES = 1000000;
NUM_NODES = 100000;

for (var i=0; i < NUM_NODES; ++i) {
  new Node();
}

end = new Date();

// print("Time: " + (end - start) + " ms");  
print("Done");
