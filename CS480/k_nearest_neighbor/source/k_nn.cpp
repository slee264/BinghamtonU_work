#include "k_nn.h"
#include "threadpool.h"

using namespace std;

int main(int argc, char* argv[]){

	if(argc !=5 ){
		cerr << "Usage: ./k-nn <n_cores> <training_file> <query_file> <result_file>" <<
endl;			
		return 0;
	}

	int n_cores = *(int*)argv[1];
	char* tfile_name = argv[2];
	char* qfile_name = argv[3];
	char* ofile_name = argv[4];

/*	
	const int64_t f1 = 314;
	const int64_t f2 = 315;
	const int64_t f3 = 316;
	ofstream ofile("test.bin", ios::binary);
	ofile.write((char*) &f1, sizeof(double));
	ofile.write((char*) &f2, sizeof(double));
	ofile.write((char*) &f3, sizeof(double));
*/

	ifstream tfile(tfile_name, ios::in|ios::binary);
	ifstream qfile(qfile_name, ios::in|ios::binary);
	ofstream ofile(ofile_name, ios::out|ios::binary);
	KNN tree(tfile, qfile, ofile);
	tfile.close();
	qfile.close();
	ofile.close();

}


KNN::KNN(ifstream &tfile, ifstream &qfile, ofstream &ofile){
	chrono::steady_clock::time_point start_tree = chrono::steady_clock::now();
	size_t header_size = sizeof(int64_t);
	size_t point_size = sizeof(float);

	//1. Read training file
	char *training_file_type = new char[header_size];
	char *training_file_ID = new char[header_size];
        char *num_points = new char[header_size];
        char *num_dimensions = new char[header_size];  
	
	if(tfile.is_open()){
                
                //make sure we are at the start of the file
                tfile.seekg(0, ios::beg);
		tfile.read(training_file_type, header_size);
		tfile.read(training_file_ID, header_size);
		tfile.read(num_points, header_size);
		tfile.read(num_dimensions, header_size);

	}else{
		cout << "Training file not found" << endl;
		return;
	}

	int num_points_int = *(int64_t*) num_points;
	int num_dimensions_int = *(int64_t*) num_dimensions;
/*
	cout << "File Type: " << training_file_type << endl;
	cout << "Training File ID: " << *(int64_t*) training_file_ID << endl;
	cout << "Number of Points: "<< num_points_int << endl;
	cout << "Number of Dimensions: "<< num_dimensions_int << endl;
*/	
	vector<float*> data_points;	
	char* point;
	float* points;
	for(int i = 0; i < num_points_int; i++){
		point = new char[point_size];
		points = new float[num_dimensions_int];
		for(int j = 0; j < num_dimensions_int; j++){
			tfile.read(point, point_size);
			points[j] = *(float*)point;
		}
		data_points.push_back(points);
	}

	//2. Read basic info from query file
	char *query_file_type = new char[header_size];
	char *query_file_ID = new char[header_size];
	char *num_queries = new char[header_size];
	char *query_num_dimensions = new char[header_size];
	char *num_neighbors = new char[header_size];

	if(qfile.is_open()){
		qfile.seekg(0, ios::beg);
		qfile.read(query_file_type, header_size);
		qfile.read(query_file_ID, header_size);
		qfile.read(num_queries, header_size);
		qfile.read(query_num_dimensions, header_size);
		qfile.read(num_neighbors, header_size);
	}/*else{
		cout << "Query file not found" << endl;
		return;
	}
*/
	int num_queries_int = *(int64_t*) num_queries;
	int query_num_dimensions_int = *(int64_t*) query_num_dimensions;
	int num_neighbors_int = *(int64_t*) num_neighbors;


	//3. build the tree

//	future<int> answer = pool.push(test, (void*) &a);

//	KNN::Node* root = build_tree(&data_points, 0, num_dimensions_int, 0, data_points.size() - 1, pool);

	
//	pthread_t  *pool = new pthread_t[num_cores*2];
	
	if(query_num_dimensions_int != 0 && query_num_dimensions_int < num_dimensions_int) num_dimensions_int = query_num_dimensions_int;

	buildtree_thread_data* bt_thread_args = new buildtree_thread_data;
	bt_thread_args->data_points = &data_points;
	bt_thread_args->depth = 0;
	bt_thread_args->num_dimensions = num_dimensions_int;
	bt_thread_args->start_index = 0;
	bt_thread_args->end_index = data_points.size() - 1;
	bt_thread_args->parent = (void*)root;
	bt_thread_args->left_right = 2;

/*
	for(int j = 0; j < num_cores*2; ++j){
		pthread_create(&pool[j], NULL, &queue_worker, NULL);
	}
*/	
	build_tree((void*) bt_thread_args);
	chrono::steady_clock::time_point end_tree = chrono::steady_clock::now();
	chrono::milliseconds tree_ms = chrono::duration_cast<chrono::milliseconds>(end_tree - start_tree);
//	cout << (chrono::duration_cast<chrono::milliseconds>(end_tree-start_tree)).count() << endl;
	cout << "Sequential: time to build the tree (milliseconds): " << tree_ms.count() << endl;
//	add_job(NULL);
/*
	for(int j = 0; j < num_cores*2; ++j){
		pthread_join(pool[j],NULL);
	}
*/
//	printLevelOrder(root);

//	cout << (root->point)[0] << endl;
//	cout << (root->point)[1] << endl;

	//4. Write result file
	srand(time(NULL));
//	cout << rand() % 10000000 << endl;
	int result_file_ID = rand() % 10000000;

	if(ofile.is_open()){
		char result_file_type[] = "RESULT";
                //make sure we are at the start of the file
                ofile.seekp(0, ios::beg);
		ofile.write(result_file_type, header_size);
		ofile.write(training_file_ID, header_size);
		ofile.write(query_file_ID, header_size);
		ofile.write((char*)&result_file_ID, header_size);
		ofile.write(num_queries, header_size);
		ofile.write(num_dimensions, header_size);
		ofile.write(num_neighbors, header_size);

	}else{
		cout << "Result file not found" << endl;
		return;
	}
	//5. Define queue and start searcing
	chrono::steady_clock::time_point start_queries = chrono::steady_clock::now();
	float flt_max = FLT_MAX;
	float* me = new float[num_dimensions_int];
	for(int i = 0; i < num_queries_int; ++i){	

		for(int j = 0; j < num_dimensions_int; ++j){
			qfile.read((char*)&me[j], point_size);
		}


		set<KNN::Node*, function<bool(KNN::Node*, KNN::Node*)>> queue([num_dimensions_int, me](KNN::Node* a, KNN::Node* b){
				return distance(me, a, num_dimensions_int) < distance(me, b, num_dimensions_int);
		});

		nearest_neighbor_search(me, root, flt_max, num_dimensions_int, 0, num_neighbors_int, queue);

/*
	cout << "Queue size: " << queue.size() << endl;

	for(vector<float*>::iterator it = data_points.begin(); it != data_points.end(); ++it){
		
		float distance = 0;
		cout << "Point: " << endl;
		for(int i = 0; i < num_dimensions_int; i++){
			cout << (*it)[i] << endl;
			distance += pow((*it)[i] - me[i], 2.0);
		}
		distance = sqrt(distance);
		cout << "Distance: " << distance << endl << endl;

	}
*/
		for(set<KNN::Node*, function<bool(KNN::Node*, KNN::Node*)>>::iterator it = queue.begin(); it != queue.end() ; it++){
			for(int j = 0; j < num_dimensions_int; ++j){
				float value = ((*it)->point)[j];
				//cout << value << ", ";
				ofile.write((char*)&value, point_size);
			}
		}

	}

	chrono::steady_clock::time_point end_queries = chrono::steady_clock::now();

	chrono::milliseconds query_ms = chrono::duration_cast<chrono::milliseconds>(end_queries - start_queries);

	cout << "Sequential: total time to executue the queries(milliseconds): " << query_ms.count() << endl;


	delete[] point;
	delete[] points;
	delete[] training_file_type;
	delete[] training_file_ID;
	delete[] num_points;
	delete[] num_dimensions;

}

void KNN::build_tree(void *bt_thread_args){

	vector<float*>* data_points = ((buildtree_thread_data*)bt_thread_args) -> data_points;
	int depth = ((buildtree_thread_data*)bt_thread_args) -> depth;
	int num_dimensions = ((buildtree_thread_data*)bt_thread_args) -> num_dimensions;
	int start_index = ((buildtree_thread_data*)bt_thread_args) -> start_index;
	int end_index = ((buildtree_thread_data*)bt_thread_args) -> end_index;
	int left_right = ((buildtree_thread_data*)bt_thread_args) -> left_right;
	KNN::Node* parent = (KNN::Node*)(((buildtree_thread_data*)bt_thread_args) -> parent);
	if(start_index > end_index) return;	
	// 1. Compute the axis
	int axis = depth % num_dimensions;
	
	// 2. Sort (part of) the array on the axis
	sort((*data_points).begin()+start_index, (*data_points).begin() + end_index + 1, 
			[axis](float* i, float* j){
				return i[axis] < j[axis] ;
			}
	);

/*
	for(vector<float*>::iterator it =(*data_points).begin()+start_index; it != (*data_points).begin() + end_index + 1; ++it){
		cout << "Point: " << endl;
		for(int i = 0; i < num_dimensions; i++){
			cout << (*it)[i] << endl;
		}
		cout << endl;
	}
*/

	// 3. find median
	int median_index = start_index + ( (end_index - start_index) / 2);
	if( (end_index - start_index) % 2 != 0) median_index++;

	// 4. Create a node and write proper information to it
	KNN::Node* node = new KNN::Node;
	node-> point = new float[num_dimensions];
	for(int i = 0; i < num_dimensions; i++){
		(node->point)[i] = (*data_points)[median_index][i];	
	}

	if(left_right == 2){
		parent = node;
		root = node;
	}
/*	
	cout << "Start_index: " << start_index << endl;
	cout << "End_index: " << end_index << endl;
	cout << "Median_index: " << median_index << endl;
	cout << "Axis: " << axis << endl;
	cout << "("  << (node->point)[0] << ", " << (node->point)[1] << ", " << (node->point)[2] << ", " << ")" << endl << endl;
*/

	if(end_index > start_index){
		buildtree_thread_data left_args;
		left_args.data_points = data_points;
		left_args.depth = depth+1;
		left_args.num_dimensions = num_dimensions;
		left_args.start_index = start_index;
		left_args.end_index = median_index - 1;
		left_args.left_right = 0;
		left_args.parent = (void*)node;
	
//		cout << "left thread entered" << endl;
//		pool.push(build_tree ,(void*) &left_args);
//		Job* left_job = new Job();
		build_tree((void*) &left_args);
//		add_job(left_job, (void*)&left_args);
//		cout << "left thread exited" << endl;
//		node->left = build_tree(id, (void*) &left_args);
//		node->left = pool.push(build_tree, data_points, depth+1, num_dimensions, start_index, median_index -1, pool);

		if(median_index <= end_index){

			buildtree_thread_data right_args;
			right_args.data_points = data_points;
			right_args.depth = depth+1;
			right_args.num_dimensions = num_dimensions;
			right_args.start_index = median_index+1;
			right_args.end_index = end_index;
			right_args.left_right = 1;
			right_args.parent = (void*) node;
//			cout << "right thread entered" << endl;
//			pool.push(build_tree, (void*) &right_args);
//			Job* right_job = new Job();
//			add_job(right_job, (void*) &right_args);

			build_tree((void*) &right_args);
//			cout << "right thread exited" << endl;
//			node->right = right_future.get();	
			
//			node->right = build_tree(id, (void*) &right_args);
//			node->right = build_tree(data_points, depth+1, num_dimensions, median_index+1, end_index, pool);
		}
	}

	if(left_right != 2){
		if(left_right == 0){
			parent->left = node;
		}
		if(left_right == 1){
			parent->right = node;
		}
	}

}

void KNN::set_root(vector<int> new_root){
	

}

KNN::Node* KNN::get_root(void){
}

KNN::Node* KNN::get_node(vector<int> comparing_points){
            
}

void KNN::printLevelOrder(KNN::Node* root){
	for(int i = 0; i <= 4; i++){
		printGivenLevel(root, i);
	}
}

void KNN::printGivenLevel(KNN::Node* node, int level){
	if(node == NULL) return;
	if(level == 0){	
		cout << "(" << (node->point)[0] << ", " << (node->point)[1] << ", "<< (node->point)[2] << ")" << endl;
	}else if(level > 0){	
		printGivenLevel(node->left, level-1);
		printGivenLevel(node->right, level-1);
	}
}
/*
void *start_buildtree_thread(void *threadarg){
	
}
*/

KNN::Node* KNN::nearest_neighbor_search(float* me, KNN::Node* node, float &current_best_distance, int num_dimensions, int depth, int num_neighbors, set<KNN::Node*, function<bool(KNN::Node*, KNN::Node*)>> &queue){

	if(node == NULL){
		return NULL;
	}

	int axis = depth % num_dimensions;

	float dist = distance(me, node, num_dimensions);

	if(dist < current_best_distance) current_best_distance = dist;

	if(queue.size() < num_neighbors) queue.insert(node);

	if(queue.size() == num_neighbors && distance(me, *(--queue.end()), num_dimensions) > dist){
		queue.erase(--queue.end());
		queue.insert(node);
	}


	if(me[axis] <= (node->point)[axis]){
		nearest_neighbor_search(me, node->left, current_best_distance, num_dimensions, depth+1, num_neighbors, queue);

		if(distance(me, *(--queue.end()), num_dimensions) > current_best_distance){
			nearest_neighbor_search(me, node->right, current_best_distance, num_dimensions, depth+1, num_neighbors, queue);
		}
	}else{	
		nearest_neighbor_search(me, node->right, current_best_distance, num_dimensions, depth+1, num_neighbors, queue);

		if(distance(me, *(--queue.end()), num_dimensions) > current_best_distance){
			nearest_neighbor_search(me, node->left, current_best_distance, num_dimensions, depth+1, num_neighbors, queue);
		}
	}

}

float KNN::distance(const float* me, const KNN::Node *node, int num_dimensions){

	float distance = 0;

	for(int i = 0; i < num_dimensions; i++){
		distance += pow((node->point)[i] - me[i], 2.0);
	}

	distance = sqrt(distance);

	return distance;
}
