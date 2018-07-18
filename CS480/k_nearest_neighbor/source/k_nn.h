#ifndef K_NN_H
#define K_NN_H

#ifndef GET_TSC_HPP
#define GET_TSC_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <math.h>
#include <float.h>
#include <set>
#include <functional>
#include <stdlib.h>
#include <chrono>

typedef struct buildtree_thread_data{
	std::vector<float*>* data_points= NULL;
	int depth = 0;
	int num_dimensions = 0;
	int start_index = 0;
	int end_index = 0;
	buildtree_thread_data() = default;
	buildtree_thread_data(int depth);
	void *parent;
	int left_right = -1;
} buildtree_thread_data;

class KNN{

	//typedef struct Node Node;

	typedef struct Node{
		float* point;
		Node *left = NULL;
		Node *right = NULL;
	} Node;
	public:
		Node* root = NULL;
		KNN(std::ifstream &tfile, std::ifstream &qfile, std::ofstream &ofile);
		void build_tree(void *bt_thread_args);
		void set_root(std::vector<int> new_root);
		Node *get_root();
		Node *get_node(std::vector<int> comparing_points);
		void printLevelOrder(Node* root);
		void printGivenLevel(Node* node, int level);
		Node* nearest_neighbor_search(float* me, Node* node, float &current_best_distance, int num_dimensions, int depth, int num_neighbors, std::set<KNN::Node*, std::function<bool(KNN::Node*, KNN::Node*)>> &queue);
		static float distance(const float* me, const KNN::Node* node, int num_dimensions);
};
inline unsigned long long
get_tsc(){
	unsigned int a, d;
	asm volatile("rdtsc" : "=a" (a), "=d" (d));
	return (unsigned long) a | (((unsigned long) d)<<32);;
}

#endif
#endif
