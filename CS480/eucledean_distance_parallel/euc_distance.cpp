#include <iostream>
#include <cmath>
#include <random>
#include <chrono>
#include <immintrin.h>

using namespace std;

const int N = 16 * 1'000'000;

int main(){
	
	alignas(32) static float t1[N];
	alignas(32) static float t2[N];
	alignas(32) static float x1[N];
	alignas(32) static float x2[N];
	alignas(32) static float y1[N];
	alignas(32) static float y2[N]; 
	alignas(32) static float z1[N];
	alignas(32) static float z2[N];
	alignas(32) static float dists[N];
	alignas(32) static float vector_dists[N];
	default_random_engine engine;
	uniform_real_distribution<float> dist(-1.0, 1.0);
	
	for(int i = 0; i < N; i++){
		t1[i] = dist(engine);
		t2[i] = dist(engine);
		x1[i] = dist(engine);
		x2[i] = dist(engine);
		y1[i] = dist(engine);
		y2[i] = dist(engine);
		z1[i] = dist(engine);
		z2[i] = dist(engine);
	}

	//THE WARM UP
	for(int i = 0; i < N; i++){
		dists[i] = sqrt(pow(t1[i]-t2[i], 2.0) + pow(x1[i]-x2[i], 2.0) + pow(y1[i]-y2[i], 2.0) + pow(z1[i]-z2[i], 2.0));
	}
	
	auto start = chrono::system_clock::now();
	for(int i = 0; i < N; i++){
		dists[i] = sqrt(pow(t1[i]-t2[i], 2.0) + pow(x1[i]-x2[i], 2.0) + pow(y1[i]-y2[i], 2.0) + pow(z1[i]-z2[i], 2.0));
	}
	auto stop = chrono::system_clock::now();

	cout << "Sequential Calculation Time: " << N/chrono::duration<double>(stop - start).count()/1'000'000 << " Mops/s"<< endl;

	//THE WARM UP
	for(int i = 0; i < N/8; i++){
		__m256 ymm_t1 = _mm256_load_ps(t1 + 8*i);
		__m256 ymm_t2 = _mm256_load_ps(t2 + 8*i);
		__m256 ymm_x1 = _mm256_load_ps(x1 + 8*i);
		__m256 ymm_x2 = _mm256_load_ps(x2 + 8*i);
		__m256 ymm_y1 = _mm256_load_ps(y1 + 8*i);
		__m256 ymm_y2 = _mm256_load_ps(y2 + 8*i);
		__m256 ymm_z1 = _mm256_load_ps(z1 + 8*i);
		__m256 ymm_z2 = _mm256_load_ps(z2 + 8*i);

		__m256 ymm_t = _mm256_sub_ps(ymm_t1, ymm_t2);
		__m256 ymm_x = _mm256_sub_ps(ymm_x1, ymm_x2);
		__m256 ymm_y = _mm256_sub_ps(ymm_y1, ymm_y2);
		__m256 ymm_z = _mm256_sub_ps(ymm_z1, ymm_z2);

		__m256 ymm_d = _mm256_sqrt_ps(
				_mm256_mul_ps(ymm_t, ymm_t) + 
				_mm256_mul_ps(ymm_x, ymm_x) + 
				_mm256_mul_ps(ymm_y, ymm_y) + 
				_mm256_mul_ps(ymm_z, ymm_z)
				);

		_mm256_store_ps(vector_dists + 8*i, ymm_d);
	}

	auto start_vector = chrono::system_clock::now();
	for(int i = 0; i < N/8; i++){

		__m256 ymm_t1 = _mm256_load_ps(t1 + 8*i);
		__m256 ymm_t2 = _mm256_load_ps(t2 + 8*i);
		__m256 ymm_x1 = _mm256_load_ps(x1 + 8*i);
		__m256 ymm_x2 = _mm256_load_ps(x2 + 8*i);
		__m256 ymm_y1 = _mm256_load_ps(y1 + 8*i);
		__m256 ymm_y2 = _mm256_load_ps(y2 + 8*i);
		__m256 ymm_z1 = _mm256_load_ps(z1 + 8*i);
		__m256 ymm_z2 = _mm256_load_ps(z2 + 8*i);

		__m256 ymm_t = _mm256_sub_ps(ymm_t1, ymm_t2);
		__m256 ymm_x = _mm256_sub_ps(ymm_x1, ymm_x2);
		__m256 ymm_y = _mm256_sub_ps(ymm_y1, ymm_y2);
		__m256 ymm_z = _mm256_sub_ps(ymm_z1, ymm_z2);

		__m256 ymm_d = _mm256_sqrt_ps(
				_mm256_mul_ps(ymm_t, ymm_t) + 
				_mm256_mul_ps(ymm_x, ymm_x) + 
				_mm256_mul_ps(ymm_y, ymm_y) + 
				_mm256_mul_ps(ymm_z, ymm_z)
				);
		_mm256_store_ps(vector_dists + 8*i, ymm_d);
	}

	auto stop_vector = chrono::system_clock::now();

	cout << "Vector Calculation Time: " << N/chrono::duration<double>(stop_vector - start_vector).count()/1'000'000 << " Mops/s" << endl;

	for(int i = 0; i < N; i++){
		if(dists[i] - vector_dists[i] > 0.000001){
			cout << "FALSE: " << i << endl;
			return 0;
		}
	}

}


