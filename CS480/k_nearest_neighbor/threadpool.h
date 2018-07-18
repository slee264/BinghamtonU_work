#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <pthread.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


struct Job;
static int job_num;
int num_threads;
int num_workers = 0;
std::queue<Job*> worker_queue;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


typedef struct Job{
	public:
		Job() : m_i(++job_num) { 
			std::cout << "Created job " << m_i << std::endl;
		};
		int m_i;
		void* data;
		void execute(){
//			std::cout << "test: " << *((int*)data) << std::endl;
//			KNN::build_tree(data);
		};
} Job;

void add_job(Job* job, void* data){
	pthread_mutex_lock(&mutex);
	job->data = data;
	worker_queue.push(job);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
};

void add_job(Job* job){
	pthread_mutex_lock(&mutex);
	worker_queue.push(job);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}
void* queue_worker(void *data){
	Job* job = NULL;
	for(;;){
		pthread_mutex_lock(&mutex);
		while(worker_queue.empty()){
			pthread_cond_wait(&cond, &mutex);
		}
		job = worker_queue.front();
		worker_queue.pop();
		pthread_mutex_unlock(&mutex);
		if(job == NULL){
			add_job(NULL);
			break;
		}
			
		job->execute();
		delete job;
	}

		return NULL;
};


#endif
