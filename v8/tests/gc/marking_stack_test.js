/* Allocate a long list of nodes to trigger marking stack overflow in the
   mark-compact collector.
             root
            /    \
           N      N
          / \    
         N   N 
        / \   
       N   N 
      .     
     .     
    N  N
  Set the new space to 16K, this benchmark triggers marking stack overflow.
  */

function Node() {
  this.left = null;
  this.right = null;
}

root = new Node();
node = root;

NUM_LEVELS = 2500;
// Create a long list of tree, so that it can cause marking stack overflow
for (var i=0; i < NUM_LEVELS; ++i) {
  node.left = new Node();
  node.right = new Node();
  node = node.left;
}

print("Done");  
