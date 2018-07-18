#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <iostream>
#include <random>
#include <cmath>
#include <vector>
#include <chrono>
#include <omp.h>

static const int N =10*10'000;
static const int I = 100;

class Histogram{

	//std::vector<float*> points;
	int intervals[I] = {0};

	public:
		Histogram();
		void sequential_histogram(int dimension);
		void parallel_histogram(int dimension);
};

#endif
