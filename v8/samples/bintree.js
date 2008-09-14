/* Create a balanced binary tree with a large number of nodes. */

var start = new Date();

node_id = 0;

// Build a binary tree, with N levels (2^N - 1 nodes)
function Node(parent) {
  this.left = null;
  this.right = null;
  this.parent = parent;
  if (parent == null) {
    this.level = 0;
  } else {
    this.level = parent.level + 1;
  }
  this.id = node_id;
  ++node_id;
}

root = new Node(null);
node = root;
LIMIT = 17;  // allocates 2^LIMIT - 1 nodes
EXPECTING = Math.pow(2, LIMIT) - 1;

while (node != null) {
  if (node.level == LIMIT - 1) {
    node = node.parent;
  } else if (node.left == null) {
    node.left = new Node(node);
    node = node.left;
  } else if (node.right == null) {
    node.right = new Node(node);
    node = node.right;
  } else {
    node = node.parent;
  }
}


if (node_id != EXPECTING) {
  print("expecting " + EXPECTING + ", got " + node_id);
}

// check the left-most leave and right-most leave
left_most = root;
while (left_most.left != null) {
  left_most = left_most.left;
}

if (left_most.id != LIMIT - 1) {
  print("expecting left-most id " + (LIMIT - 1) + ", but got "+left_most.id);
}

right_most = root;
while (right_most.right != null) {
  right_most = right_most.right;
}

if (right_most.id != node_id - 1) {
  print("expecting right-most id " + (node_id - 1) + ", but got "+right_most.id); 
} 

root = null;
node = null;
right_most = null;
left_most = null;
// gc();
print("Time: " + (new Date() - start) + " ms.");

