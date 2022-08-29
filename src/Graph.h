#ifndef Graph_H
#define Graph_H

#include <vector>
#include <cstdio>
#include <cstdlib>

class Graph
 {
 public:
 	Graph(int num_v):numVertices(num_v){}
 	~Graph();
 
 	/* data */
 	std::vector<int> adjacencyList; // all edges
    std::vector<int> edgesOffset; // offset to adjacencyList for every vertex
    std::vector<int> edgesSize; //number of edges for every vertex
    int numVertices;
    int numEdges;


    void generateGraph(std::vector<int> _adjacencyList, std::vector<int> _edgesOffset, std::vector<int> _edgesSize){
    	adjacencyList = _adjacencyList;
    	edgesOffset = _edgesOffset;
    	edgesSize = _edgesSize;
    	numEdges = _adjacencyList.size();
    }
    //void deleteVertice();

 }; 



#endif //BFS_CUDA_GRAPH_H
