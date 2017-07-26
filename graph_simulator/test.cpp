#include <vector>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

// Adjacent list.
// Graph g;
// g.size() == the number of vetices, i.e. module scripts.
// g[i]: the ordered list of module scripts imported from module script |i|.
typedef vector<vector<int> > Graph;

// Represents the order of single module load, i.e. the order of arrival of
// complete network responses of module scripts.
// Ordered list of (module script loaded) and
// (the number of module scripts whose network request is initiated).
typedef vector<pair<int,int>> Order;

Order CreateRandomTraversalOrder(const Graph& graph, const set<int>& error) {
  set<int> visited;
  set<int> visible;
  Order order;
  visible.insert(0);
  while (!visible.empty()) {
    vector<int> q;
    for (int v : visible) {
      if (!visited.count(v))
        q.push_back(v);
    }
    if (q.empty())
      break;

    const int i = q[rand() % q.size()];
    order.push_back(make_pair(i, visible.size()));
    visited.insert(i);

    if (error.count(i))
      continue;

    for (int j : graph[i]) {
      visible.insert(j);
    }
  }
  return order;
}

enum Result {
  kFoundError,
  kFoundInvisible,
  kNone
};

// Do Depth-First-Search and find the "first" error to be reported.
// If found, returns kFoundError (we are sure that there is an error and the
//   error to be reported is determined).
// If a module script not yet loaded is traversed before any error is found,
//   then returns kFoundInvisible (we are sure that we need to wait for the
//   module script to be loaded before we can determine the error to be
//   reported).
// If there are no error or loading module scripts, then returns kNone --
//   we are sure that the module script is successfully loaded.
Result DFS(const Graph& graph, const set<int>& error, const set<int>& visible, set<int>& visited, int pos) {
  if (!visible.count(pos))
    return kFoundInvisible;

  if (visited.count(pos))
    return kNone;

  if (error.count(pos))
    return kFoundError;

  visited.insert(pos);

  for (int j : graph[pos]) {
    Result r = DFS(graph, error, visible, visited, j);
    switch (r) {
      case kFoundError:
      case kFoundInvisible:
        return r;
      case kNone:
        break;
    }
  }
  return kNone;
}

// The following functions returns
// (the number of single module scripts loaded before termination) and
// (the number of module script requests initiated before termination).

// Strategy 1:
// Terminate if all submodules are successfully loaded, or found an error
// and we are sure the error is to be reported, i.e. the "first" error in
// the DFS order, regardless of module scripts not yet loaded.
// This preserves the determinism of the choice of error reported.
pair<int, int> EarliestTerminationDeterministic(const Graph& graph, const set<int>& error, const Order& order) {
  set<int> visible;
  for (auto ij : order) {
    visible.insert(ij.first);
    set<int> visited;
    if (DFS(graph, error, visible, visited, 0) != kFoundInvisible) {
      return make_pair(visible.size(), ij.second);
    }
  }
}

// Strategy 2:
// Terminate if an error module script is found.
// At this time, we might be not sure whether the found error is to be
// reported (because a new error can be found in the future and the new
// error can precede in the DFS order), but anyway terminate.
// This doesn't preserve the determinism of the choice of error reported,
// but terminates at the earliest possible time.
pair<int, int> EarliestTerminationNonDeterministic(const Graph& g, const set<int>& error, const Order& order) {
  set<int> visible;
  for (auto ij : order) {
    visible.insert(ij.first);
    if (error.count(ij.first) || ij == order[order.size()-1])
      return make_pair(visible.size(), ij.second);
  }
}

// Creates a random graph that consists of n vertices.
Graph CreateGraph(int n, int average_edges_per_vertex) {
  Graph out(n);
  vector<vector<bool> > adj(n, vector<bool>(n, false));
  int n_edges = rand() % (n * average_edges_per_vertex + 1);
  for (int t = 0; t < n_edges; ++t) {
    int i, j;
    do {
      i = rand() % n;
      j = rand() % n;
    } while(adj[i][j]);
    adj[i][j] = true;
    out[i].push_back(j);
  }
  return out;
}

bool CreateRandomError(const Graph& g, int n, set<int>& error) {
  error.clear();
  for (int i = 0; i < n; ++i) {
    Order o = CreateRandomTraversalOrder(g, error);
    if (o.empty())
      return false;
    error.insert(o[rand() % o.size()].first);
  }
  return true;
}

void ShowGraph(const Graph& graph, const set<int>& error) {
  printf("===========\n");
  for (int a = 0; a < (int)graph.size(); ++a) {
    if (error.count(a)) {
      printf("%d: error\n", a);
      continue;
    }
    for (int b : graph[a]) {
      printf("%d -> %d\n", a, b);
    }
  }
  printf("===========\n");
}

int main() {
  const bool debug = false;
  const int n_iter = 10000;

  // Note: all vertices / all error vertices are not necessarily reachable
  // from the root module script.
  const int n_vertices = 100;
  const int n_errors = 3;
  const int n_edges_per_vertex = 5;

  srand(time(NULL));

  vector<int> a(6);
  for (int t = 1; t <= n_iter; ++t) {
    set<int> error;
    Graph g;
    do {
      g = CreateGraph(n_vertices, n_edges_per_vertex);
    } while(!CreateRandomError(g, n_errors, error));
    Order order = CreateRandomTraversalOrder(g, error);

    pair<int, int> score_nondeterministic = EarliestTerminationNonDeterministic(g, error, order);
    pair<int, int> score_deterministic = EarliestTerminationDeterministic(g, error, order);

    // Strategy 3: Do not early-terminate. Wait for all module loads.
    pair<int, int> score_full = make_pair(order.size(), order[order.size()-1].second);

    if (debug) {
      ShowGraph(g, error);
      for (auto it : order) {
        printf("[%d/%d]", it.first, it.second);
      }
      printf("\n");
      printf ("%d / %d\n", score_nondeterministic.first, score_nondeterministic.second);
      printf ("%d / %d\n", score_deterministic.first, score_deterministic.second);
      printf ("%d / %d\n", score_full.first, score_full.second);
      break;
    }

    a[0] += score_nondeterministic.first;
    a[1] += score_nondeterministic.second;
    a[2] += score_deterministic.first;
    a[3] += score_deterministic.second;
    a[4] += score_full.first;
    a[5] += score_full.second;
    if (t  == n_iter) {
      printf("--------------------------------------------------------------------\n");
      printf("Max number of module scripts: %d (%d reachable on average)\n", n_vertices, a[4] / t);
      printf("Average number of imports per module script: %d\n", n_edges_per_vertex);
      printf("Number of error module scripts: %d\n", n_errors);
      printf("Strategy 1 (earliest possible, no determinism)   : %d loaded / %d started\n", a[0] / t, a[1] / t);
      printf("Strategy 2 (earliest when preserving determinism): %d loaded / %d started\n", a[2] / t, a[3] / t);
      printf("Strategy 3 (no early termination)                : %d loaded / %d started\n", a[4] / t, a[5] / t);
    }
  }

/*
  set<int> visible;
*/
}
