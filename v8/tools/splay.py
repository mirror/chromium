# Splay tree code from.
#
# http://code.google.com/p/pysplay/
#
# Distributed under an MIT license:
# -----------------------------------------------------------------------------
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# -----------------------------------------------------------------------------
#
# The code has been modified for our needs.
#

class Node:
    def __init__(self, key, value):
        self.key = key
        self.value = value
        self.left = self.right = None

    def equals(self, node):
        return self.key == node.key

class SplayTree:
    def __init__(self):
        self.root = None
        self.header = Node(None, None) # For Splay()

    def Insert(self, key, value):
        if (self.root == None):
            self.root = Node(key, value)
            return

        self.Splay(key)
        if self.root.key == key:
            # If the key is already there in the tree, don't do anything.
            return

        n = Node(key, value)
        if key < self.root.key:
            n.left = self.root.left
            n.right = self.root
            self.root.left = None
        else:
            n.right = self.root.right
            n.left = self.root
            self.root.right = None
        self.root = n

    def Remove(self, key):
        self.Splay(key)
        if key != self.root.key:
            raise 'KeyNotFound'
        tmp = self.root
        # Now delete the root.
        if self.root.left == None:
            self.root = self.root.right
        else:
            x = self.root.right
            self.root = self.root.left
            self.Splay(key)
            self.root.right = x
        # Return the removed element.
        return tmp

    def FindMin(self):
        if self.root == None:
            return None
        x = self.root
        while x.left != None:
            x = x.left
        self.Splay(x.key)
        return x

    def FindMax(self):
        if self.root == None:
            return None
        x = self.root
        while (x.right != None):
            x = x.right
        self.Splay(x.key)
        return x

    def Find(self, key):
        if self.root == None:
            return None
        self.Splay(key)
        if self.root.key != key:
            return None
        return self.root

    def FindGreatestsLessThan(self, key):
        if self.IsEmpty():
            return None
        # Splay on the key.  This will make the last node on the
        # search path looking for key the new root of the splay tree.
        # Therefore, the greatest element less than the key is either
        # the root or the max value of the left subtree.
        self.Splay(key)
        # Key found or root less than key.  In that case
        # the root is what we are after.
        if key == self.root.key or key > self.root.key:
            return self.root
        # Root key is greater than key.  Return the max of the left
        # subtree.
        x = self.root.left
        if x == None:
            return None
        while (x.right != None):
            x = x.right
        self.Splay(x.key)
        return x

    def ExportValueList(self):
        result = []
        nodes_to_visit = [self.root]
        while len(nodes_to_visit) > 0:
            node = nodes_to_visit.pop()
            if node == None:
                continue
            result.append(node.value)
            nodes_to_visit.append(node.left)
            nodes_to_visit.append(node.right)
        return result

    def IsEmpty(self):
        return self.root == None

    def Splay(self, key):
        l = r = self.header
        t = self.root
        self.header.left = self.header.right = None
        while True:
            if key < t.key:
                if t.left == None:
                    break
                if key < t.left.key:
                    y = t.left
                    t.left = y.right
                    y.right = t
                    t = y
                    if t.left == None:
                        break
                r.left = t
                r = t
                t = t.left
            elif key > t.key:
                if t.right == None:
                    break
                if key > t.right.key:
                    y = t.right
                    t.right = y.left
                    y.left = t
                    t = y
                    if t.right == None:
                        break
                l.right = t
                l = t
                t = t.right
            else:
                break
        l.right = t.left
        r.left = t.right
        t.left = self.header.right
        t.right = self.header.left
        self.root = t
