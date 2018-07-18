#include "histogram.h"

using namespace std;

Histogram::Histogram(){

};

void Histogram::sequential_histogram(int dimension){

	default_random_engine engine;
	uniform_real_distribution<float> dist(-1.0, 1.0);

	float* values;
	int count = 0;
	float distance;
	cout << "Dimension: " << dimension << endl;

	int nthreads, tid;
	while(count < N){
		distance = 0;
		values = new float[dimension];
		for(int j = 0; j < dimension; j++){
			values[j] = dist(engine);
		}
			
		for(int j = 0; j < dimension; j++){
			distance += values[j] * values[j];
		}

		distance = sqrt(distance);

		if(distance <= 1){
			//points.push_back(values);
			intervals[(int)floor(distance*100)] += 1;
			count++;
			if(count % 10000 == 0) cout << count << endl;
		}

		delete[] values;
	}

	for(int i = 0; i < I; i++){
		cout << "Interval " << i * 0.01 << " - " << (i+1) * 0.01 << ": " << intervals[i] << endl;
	}
	
	cout << endl;
};

void Histogram::parallel_histogram(int dimension){

	default_random_engine engine;
	uniform_real_distribution<float> dist(-1.0, 1.0);

	float values[16];
	int count = 0;
	float distance;
	cout << "Dimension: " << dimension << endl;
	int nthreads, tid;
	
	#pragma omp parallel private(tid)
	{
		while(count < N){

			distance = 0;
			for(int j = 0; j < dimension; j++){
				values[j] = dist(engine);
			}
			#pragma omp critical
			{
			for(int j = 0; j < dimension; j++){
				distance += values[j] * values[j]; 
			}

			distance = sqrt(distance);
				if(distance <= 1){	
					//points.push_back(values);
					intervals[(int)floor(distance*100)] += 1;
					count++;
					if(count % 10000 == 0) cout << count << endl;
				}

			}
		
		}
	}

	for(int i = 0; i < I; i++){
		cout << "Interval " << i * 0.01 << " - " << (i+1) * 0.01 << ": " << intervals[i] << endl;
	}
	
	cout << endl;
};

int main(){

//	for(int i = 2; i < 17; i++){
//		Histogram hist = Histogram(i);
//	}

	auto parallel_start = chrono::system_clock::now();
	omp_set_num_threads(8);
	for(int i = 2; i < 17; i++){
		Histogram hist = Histogram();
		hist.parallel_histogram(i);
	}


	auto parallel_stop = chrono::system_clock::now();
	cout <<"Parallel time: " << (N/chrono::duration<double>(parallel_stop-parallel_start).count())/1'000'000 << " Mobps/s" << endl;
	auto sequential_start = chrono::system_clock::now();
	for(int i = 2; i < 17; i++){
		Histogram hist = Histogram();
		hist.sequential_histogram(i);
	}
	auto sequential_stop = chrono::system_clock::now();

	cout <<"Sequential time: " << (N/chrono::duration<double>(sequential_stop-sequential_start).count())/1'000'000 << " Mobps/s" << endl;


}

