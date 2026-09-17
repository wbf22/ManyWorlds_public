#include <iostream>
#include <list>
#include <unordered_set>
#include <vector>

using namespace std;

struct Graph {
    int V; // Number of vertices
    vector<list<int>> adj; // Adjacency list


    Graph(int V) : V(V), adj(V) {}

    // Function to add an edge
    void addEdge(int v, int w) {
        adj[v].push_back(w); // Add w to v’s list.
    }

    // DFS traversal of the vertices reachable from v
    void DFSUtil(int v, unordered_set<int>& visited) {
        // Mark the current node as visited and print it
        visited.insert(v);
        cout << v << " ";

        // Recur for all the vertices adjacent to this vertex
        for (int neighbor : adj[v]) {
            if (visited.find(neighbor) == visited.end()) {
                DFSUtil(neighbor, visited);
            }
        }
    }

    // DFS traversal. It uses recursive DFSUtil()
    void DFS(int v) {
        unordered_set<int> visited; // Visited vertices
        DFSUtil(v, visited);
    }
};