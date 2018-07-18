#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <bitset>
#include <vector>
#include <math.h>
#include <deque>
#include <limits.h>

using namespace std;

const int NUM_ADDRESS_BITS = 32;
const int CACHE_LINE_BYTE_SIZE = 32;
const int CACHE_LINE_BIT_SIZE = CACHE_LINE_BYTE_SIZE * 8;
const int BYTE_OFFSET_SIZE = 5;
const int KB = 1024;

string direct_mapped(int cache_size, vector<string>& PCS);
string set_associative(int cache_size, int associativity, vector<string>& PCS);
string fully_associative_lru(int cache_size, vector<string>& PCS);
string set_associative_write_miss(int cache_size, int associativity, vector<string>& PCS);
string set_associative_prefetch(int cache_size, int associativity, vector<string>& PCS);
string set_associative_prefetch_miss(int cache_size, int associativity, vector<string>& PCS);

int main(int argc, char** argv){
	ifstream infile;
	ofstream ofile;

	vector<string> PCS;

	string line;
	infile.open(argv[1]);
	ofile.open(argv[2]);

	while(getline(infile, line)){
		PCS.push_back(line);
	}

	infile.close();

	ofile << direct_mapped(1*KB, PCS) << " ";
	ofile << direct_mapped(4*KB, PCS) << " ";
	ofile << direct_mapped(16*KB, PCS) << " ";
	ofile << direct_mapped(32*KB, PCS) << endl;

	ofile << set_associative(16*KB, 2, PCS) << " ";
	ofile << set_associative(16*KB, 4, PCS) << " ";
	ofile << set_associative(16*KB, 8, PCS) << " ";
	ofile << set_associative(16*KB, 16, PCS) << endl;

	ofile << fully_associative_lru(16*KB, PCS) << endl;

	ofile << endl;

	ofile << set_associative_write_miss(16*KB, 2, PCS) << " ";
	ofile << set_associative_write_miss(16*KB, 4, PCS) << " ";
	ofile << set_associative_write_miss(16*KB, 8, PCS) << " ";
	ofile << set_associative_write_miss(16*KB, 16, PCS) << endl;

	ofile << set_associative_prefetch(16*KB, 2, PCS) << " ";
	ofile << set_associative_prefetch(16*KB, 4, PCS) << " ";
	ofile << set_associative_prefetch(16*KB, 8, PCS) << " ";
	ofile << set_associative_prefetch(16*KB, 16, PCS) << endl;

	ofile << set_associative_prefetch_miss(16*KB, 2, PCS) << " ";
	ofile << set_associative_prefetch_miss(16*KB, 4, PCS) << " ";
	ofile << set_associative_prefetch_miss(16*KB, 8, PCS) << " ";
	ofile << set_associative_prefetch_miss(16*KB, 16, PCS) << endl;

	ofile.close();
}

string direct_mapped(int cache_size, vector<string>& PCS){
	int hit = 0;
	int count = 0;
	int num_cache_lines = cache_size / CACHE_LINE_BYTE_SIZE;

	bitset<CACHE_LINE_BIT_SIZE> cache[num_cache_lines];

	for(int i = 0; i < num_cache_lines; i++){
		cache[i].reset();
	}

	vector<string>::iterator it = PCS.begin();
	string line;

	unsigned byte_mask = (1 << BYTE_OFFSET_SIZE) - 1;
	unsigned tag_mask = (1 << (BYTE_OFFSET_SIZE + (int)log2( (double) num_cache_lines)) ) - 1;

	while(it != PCS.end()){
		count++;
		stringstream ss;

		line = (*it).substr((*it).find(" ") + 1);
		//		cout << line << endl;

		unsigned n;
		unsigned byte_offset;
		unsigned index;
		unsigned tag;

		ss << hex << line;
		ss >> n;
		byte_offset = n & byte_mask;

		index = (n & tag_mask) - byte_offset;

		tag = n - (byte_offset + index);

		index = (index >> BYTE_OFFSET_SIZE) % num_cache_lines;
		tag = tag >> (BYTE_OFFSET_SIZE + (int)log2( (double) num_cache_lines));

		bitset<32> b(byte_offset);
		bitset<32> t(tag);
		bitset<32> i(index);

		//		cout << "byte offset: " << b << endl;
		//		cout << "tag: " << t << endl;
		//		cout << "index: " << i << endl;
		//		cout << index << endl;

		int tag_size = NUM_ADDRESS_BITS - (BYTE_OFFSET_SIZE + (int)log2( (double) num_cache_lines));
		int j;
		if(cache[index][CACHE_LINE_BIT_SIZE - 1] == 0){
			cache[index][CACHE_LINE_BIT_SIZE - 1] = 1;

			for(j = 0; j < tag_size; j++){
				cache[index][CACHE_LINE_BIT_SIZE - 1 - tag_size + j] = t[j];
			}
		}else{
			bool match = true;
			for(j = 0; j < tag_size; j++){
				if(cache[index][CACHE_LINE_BIT_SIZE - 1 - tag_size + j] != t[j]){
					match = false;
					break;
				}
			}

			if(match){
				hit++;
			}else{
				for(int k = 0; k < tag_size; k++){
					cache[index][CACHE_LINE_BIT_SIZE - 1 - tag_size + k] = t[k];
				}
			}
		}

		//		cout << "cache[index]: " << cache[index] << endl;
		it++;
	}

	stringstream ss;
	string result;
	ss << hit << ", ";
	ss >> result;
	ss.str(string());
	ss << count;
	result = result + ss.str() + ";";
	return result;
}

string set_associative(int cache_size, int associativity, vector<string>& PCS){

	int hit = 0;
	int count = 0;

	int num_lines = cache_size / CACHE_LINE_BYTE_SIZE;
	int num_sets = num_lines / associativity;
	int num_index_bits = (int) log2((double) num_sets);
	int num_tag_bits = NUM_ADDRESS_BITS - num_index_bits - BYTE_OFFSET_SIZE;

	bitset<CACHE_LINE_BIT_SIZE> cache[num_sets][associativity];
	vector< deque<int> > lru_history (num_sets, deque<int>(associativity, -1));

	for(int i = 0; i < num_sets; i++){
		for(int j = 0; j < associativity; j++){
			cache[i][j].reset();
		}
	}


	vector<string>::iterator it = PCS.begin();


	unsigned byte_offset_mask = (1 << BYTE_OFFSET_SIZE) - 1;
	unsigned byte_index_mask = (1 << (BYTE_OFFSET_SIZE + num_index_bits)) - 1;

	string line;
	while(it != PCS.end()){
		count++;
		//		cout << count << endl;

		line = (*it).substr((*it).find(" ")+1);

		unsigned address;
		unsigned unsigned_tag;
		unsigned index;
		unsigned byte_offset;

		stringstream ss;
		ss << hex << line;
		ss >> address;

		byte_offset = address & byte_offset_mask;
		unsigned_tag = (address - (address & byte_index_mask));
		index = address - byte_offset - unsigned_tag;
		unsigned_tag = unsigned_tag >> (BYTE_OFFSET_SIZE + num_index_bits);
		index = index >> (BYTE_OFFSET_SIZE);
		index = index % num_sets;

		bitset<32> b(byte_offset);
		bitset<32> tag(unsigned_tag);
		bitset<32> i(index);
		bitset<32> addr(address);

		bool match = false;
		//Look for a match
		for(int i = 0; i < associativity; i++){
			if(tag.to_string().substr(NUM_ADDRESS_BITS - num_tag_bits) == cache[index][i].to_string().substr(1, num_tag_bits)){
				match = true;
				break;
			}
		}

		if(match){
			hit++;

			int cache_line_index;
			for(cache_line_index = 0; cache_line_index < associativity; cache_line_index++){
				if(lru_history[index][cache_line_index] == (int) unsigned_tag) break;
			}

			lru_history[index].erase(lru_history[index].begin() + cache_line_index);
			lru_history[index].push_front(unsigned_tag);

		}else{
			bool empty = false;
			int empty_cache_line = -1;

			for(int i = 0; i < associativity; i++){
				if(lru_history[index][i] == -1){
					empty_cache_line = i;
					empty = true;
					break;
				}
			}

			if(empty){
				//empty cache line exists
				cache[index][empty_cache_line][CACHE_LINE_BIT_SIZE - 1] = 1;

				for(int i = 0; i < num_tag_bits; i++){
					cache[index][empty_cache_line][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag[i];
				}

			}else{
				//empty cache line does not exist

				//Find the cache_line to replace

				bitset<32> removing_tag(lru_history[index][associativity-1]);

				int lru_index;
				for(lru_index = 0; lru_index < associativity; lru_index++){

					if(removing_tag.to_string().substr(NUM_ADDRESS_BITS - num_tag_bits).compare(cache[index][lru_index].to_string().substr(1, num_tag_bits)) == 0){
						break;
					}
				}

				cache[index][lru_index][CACHE_LINE_BIT_SIZE - 1] = 1;

				for(int i = 0; i < num_tag_bits; i++){
					cache[index][lru_index][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag[i];
				}
			}

			lru_history[index].pop_back();
			lru_history[index].push_front(unsigned_tag);

		}

		it++;
	}

	stringstream ss;
	string result;
	ss << hit << ", ";
	ss >> result;
	ss.str(string());
	ss << count;
	result = result + ss.str() + ";";
	return result;
}

string fully_associative_lru(int cache_size, vector<string>& PCS){

	int hit = 0;
	int count = 0;

	int num_lines = cache_size/CACHE_LINE_BYTE_SIZE;
	int num_tag_bits = NUM_ADDRESS_BITS - BYTE_OFFSET_SIZE;
	int empty_cache_line = num_lines;

	bitset<CACHE_LINE_BIT_SIZE> cache[num_lines];
	int lru_history[num_lines];

	//	for(int i = 0; i < num_lines; i++){
	//		cache[i].reset();
	//	}

	vector<string>::iterator it = PCS.begin();

	unsigned byte_offset_mask = (1 << BYTE_OFFSET_SIZE) - 1;

	string line;

	while(it != PCS.end()){
		count++;
		/*
		if(count % 1000 == 0){
		cout << hit << endl;
		cout << count << endl;
	}
	*/
	line = *it;
	line = (line).substr(line.find(" ") + 1);

	unsigned address;
	unsigned unsigned_tag;
	unsigned byte_offset;

	stringstream ss;
	ss << hex << line;
	ss >> address;

	byte_offset = address & byte_offset_mask;
	unsigned_tag = address >> BYTE_OFFSET_SIZE;

	bitset<BYTE_OFFSET_SIZE> b(byte_offset);
	bitset<NUM_ADDRESS_BITS - BYTE_OFFSET_SIZE> tag(unsigned_tag);
	bitset<NUM_ADDRESS_BITS> addr(address);

	//		cout << endl;
	//		cout << "address: " << addr << endl;
	//		cout << "byte offset: " << b << endl;
	//		cout << "tag: " << tag << endl;


	bool match = false;
	int hit_index;
	for(hit_index = 0; hit_index < num_lines; hit_index++){
		if(tag.to_string().compare(cache[hit_index].to_string().substr(1, num_tag_bits)) == 0){
			match = true;
			break;
		}else if(cache[hit_index][CACHE_LINE_BIT_SIZE-1] == 0){
			break;
		}
	}

	if(match){
		hit++;
		lru_history[hit_index] = count;

	}else{
		bool empty = false;
		if(empty_cache_line >= 1){
			empty = true;
		}


		if(empty){
			cache[num_lines - empty_cache_line][CACHE_LINE_BIT_SIZE - 1] = 1;

			for(int i = 0; i < num_tag_bits; i++){
				cache[num_lines - empty_cache_line][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag[i];
			}

			lru_history[num_lines - empty_cache_line] = count;
			empty_cache_line--;

		}else{
			int lru = INT_MAX;
			int lru_index;
			for(int i = 0; i < num_lines; i++){
				if(lru_history[i] < lru){
					lru = lru_history[i];
					lru_index = i;
				}
			}

			cache[lru_index][CACHE_LINE_BIT_SIZE - 1] = 1;

			for(int j = 0; j < num_tag_bits; j++){
				cache[lru_index][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + j] = tag[j];
			}

			lru_history[lru_index] = count;

		}


	}
	it++;
}

stringstream ss;
string result;
ss << hit << ", ";
ss >> result;
ss.str(string());
ss << count;
result = result + ss.str() + ";";
return result;
}

string set_associative_write_miss(int cache_size, int associativity, vector<string>& PCS){

	int hit = 0;
	int count = 0;

	int num_lines = cache_size / CACHE_LINE_BYTE_SIZE;
	int num_sets = num_lines / associativity;
	int num_index_bits = (int) log2((double) num_sets);
	int num_tag_bits = NUM_ADDRESS_BITS - num_index_bits - BYTE_OFFSET_SIZE;

	bitset<CACHE_LINE_BIT_SIZE> cache[num_sets][associativity];
	vector< deque<int> > lru_history (num_sets, deque<int>(associativity, -1));

	for(int i = 0; i < num_sets; i++){
		for(int j = 0; j < associativity; j++){
			cache[i][j].reset();
		}
	}

	vector<string>::iterator it = PCS.begin();


	unsigned byte_offset_mask = (1 << BYTE_OFFSET_SIZE) - 1;
	unsigned byte_index_mask = (1 << (BYTE_OFFSET_SIZE + num_index_bits)) - 1;

	string line;
	string outcome;
	while(it != PCS.end()){
		count++;
		//		cout << count << endl;
		line = (*it).substr((*it).find(" ")+1);
		outcome = (*it).substr(0, (*it).find(" "));

		unsigned address;
		unsigned unsigned_tag;
		unsigned index;
		unsigned byte_offset;

		stringstream ss;
		ss << hex << line;
		ss >> address;

		byte_offset = address & byte_offset_mask;
		unsigned_tag = (address - (address & byte_index_mask));
		index = address - byte_offset - unsigned_tag;
		unsigned_tag = unsigned_tag >> (BYTE_OFFSET_SIZE + num_index_bits);
		index = index >> (BYTE_OFFSET_SIZE);
		index = index % num_sets;

		bitset<32> b(byte_offset);
		bitset<32> tag(unsigned_tag);
		bitset<32> i(index);
		bitset<32> addr(address);

		bool match = false;
		//Look for a match
		for(int i = 0; i < associativity; i++){
			if(tag.to_string().substr(NUM_ADDRESS_BITS - num_tag_bits) == cache[index][i].to_string().substr(1, num_tag_bits)){
				match = true;
				break;
			}
		}

		if(match){
			hit++;

			int cache_line_index;
			for(cache_line_index = 0; cache_line_index < associativity; cache_line_index++){
				if(lru_history[index][cache_line_index] == (int) unsigned_tag) break;
			}

			lru_history[index].erase(lru_history[index].begin() + cache_line_index);
			lru_history[index].push_front(unsigned_tag);

		}else{

			if(outcome != "S"){
				bool empty = false;
				int empty_cache_line = -1;

				for(int i = 0; i < associativity; i++){
					if(lru_history[index][i] == -1){
						empty_cache_line = i;
						empty = true;
						break;
					}
				}

				if(empty){

					//empty cache line exists
					cache[index][empty_cache_line][CACHE_LINE_BIT_SIZE - 1] = 1;

					for(int i = 0; i < num_tag_bits; i++){
						cache[index][empty_cache_line][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag[i];
					}

				}else{
					//empty cache line does not exist

					//Find the cache_line to replace

					bitset<32> removing_tag(lru_history[index][associativity-1]);

					int lru_index;
					for(lru_index = 0; lru_index < associativity; lru_index++){

						if(removing_tag.to_string().substr(NUM_ADDRESS_BITS - num_tag_bits).compare(cache[index][lru_index].to_string().substr(1, num_tag_bits)) == 0){
							break;
						}
					}

					cache[index][lru_index][CACHE_LINE_BIT_SIZE - 1] = 1;

					for(int i = 0; i < num_tag_bits; i++){
						cache[index][lru_index][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag[i];
					}
				}

				lru_history[index].pop_back();
				lru_history[index].push_front(unsigned_tag);
			}
		}

		//		break;
		it++;
	}

	stringstream ss;
	string result;
	ss << hit << ", ";
	ss >> result;
	ss.str(string());
	ss << count;
	result = result + ss.str() + ";";
	return result;
}

string set_associative_prefetch(int cache_size, int associativity, vector<string>& PCS){

	int hit = 0;
	int count = 0;

	int num_lines = cache_size / CACHE_LINE_BYTE_SIZE;
	int num_sets = num_lines / associativity;
	int num_index_bits = (int) log2((double) num_sets);
	int num_tag_bits = NUM_ADDRESS_BITS - num_index_bits - BYTE_OFFSET_SIZE;

	bitset<CACHE_LINE_BIT_SIZE> cache[num_sets][associativity];
	vector< deque<int> > lru_history (num_sets, deque<int>(associativity, -1));

	for(int i = 0; i < num_sets; i++){
		for(int j = 0; j < associativity; j++){
			cache[i][j].reset();
		}
	}


	vector<string>::iterator it = PCS.begin();


	unsigned byte_offset_mask = (1 << BYTE_OFFSET_SIZE) - 1;
	unsigned byte_index_mask = (1 << (BYTE_OFFSET_SIZE + num_index_bits)) - 1;

	string line;
	string next_line;
	while(it != PCS.end()){
		count++;
		//		cout << count << endl;

		line = (*it).substr((*it).find(" ")+1);

		unsigned address;
		unsigned unsigned_tag;
		unsigned index;
		unsigned byte_offset;

		stringstream ss;
		ss << hex << line;
		ss >> address;

		byte_offset = address & byte_offset_mask;
		unsigned_tag = (address - (address & byte_index_mask));
		index = address - byte_offset - unsigned_tag;
		unsigned_tag = unsigned_tag >> (BYTE_OFFSET_SIZE + num_index_bits);
		index = index >> (BYTE_OFFSET_SIZE);
		index = index % num_sets;

		bitset<32> b(byte_offset);
		bitset<32> tag(unsigned_tag);
		bitset<32> i(index);
		bitset<32> addr(address);

		bool match = false;
		//Look for a match
		for(int i = 0; i < associativity; i++){
			if(tag.to_string().substr(NUM_ADDRESS_BITS - num_tag_bits) == cache[index][i].to_string().substr(1, num_tag_bits)){
				match = true;
				break;
			}
		}

		if(match){
			hit++;

			int cache_line_index;
			for(cache_line_index = 0; cache_line_index < associativity; cache_line_index++){
				if(lru_history[index][cache_line_index] == (int) unsigned_tag) break;
			}

			lru_history[index].erase(lru_history[index].begin() + cache_line_index);
			lru_history[index].push_front(unsigned_tag);

		}else{
			bool empty = false;
			int empty_cache_line = -1;

			for(int i = 0; i < associativity; i++){
				if(lru_history[index][i] == -1){
					empty_cache_line = i;
					empty = true;
					break;
				}
			}

			if(empty){
				//empty cache line exists
				cache[index][empty_cache_line][CACHE_LINE_BIT_SIZE - 1] = 1;

				for(int i = 0; i < num_tag_bits; i++){
					cache[index][empty_cache_line][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag[i];
				}

			}else{
				//empty cache line does not exist

				//Find the cache_line to replace

				bitset<32> removing_tag(lru_history[index][associativity-1]);

				int lru_index;
				for(lru_index = 0; lru_index < associativity; lru_index++){

					if(removing_tag.to_string().substr(NUM_ADDRESS_BITS - num_tag_bits).compare(cache[index][lru_index].to_string().substr(1, num_tag_bits)) == 0){
						break;
					}
				}

				cache[index][lru_index][CACHE_LINE_BIT_SIZE - 1] = 1;

				for(int i = 0; i < num_tag_bits; i++){
					cache[index][lru_index][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag[i];
				}
			}

			lru_history[index].pop_back();
			lru_history[index].push_front(unsigned_tag);

		}

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------


		address = address + NUM_ADDRESS_BITS;
		byte_offset = address & byte_offset_mask;
		unsigned_tag = (address - (address & byte_index_mask));
		index = address - byte_offset - unsigned_tag;
		unsigned_tag = unsigned_tag >> (BYTE_OFFSET_SIZE + num_index_bits);
		index = index >> (BYTE_OFFSET_SIZE);
		index = index % num_sets;

		bitset<32> b2(byte_offset);
		bitset<32> tag2(unsigned_tag);
		bitset<32> i2(index);
		bitset<32> addr2(address);

		match = false;
		//Look for a match
		for(int i = 0; i < associativity; i++){
			if(tag2.to_string().substr(NUM_ADDRESS_BITS - num_tag_bits) == cache[index][i].to_string().substr(1, num_tag_bits)){
				match = true;
				break;
			}
		}

		if(match){
			int cache_line_index;

			for(cache_line_index = 0; cache_line_index < associativity; cache_line_index++){
				if(lru_history[index][cache_line_index] == (int) unsigned_tag) break;
			}

			lru_history[index].erase(lru_history[index].begin() + cache_line_index);
			lru_history[index].push_front(unsigned_tag);

		}else{
			bool empty = false;
			int empty_cache_line = -1;

			for(int i = 0; i < associativity; i++){
				if(lru_history[index][i] == -1){
					empty_cache_line = i;
					empty = true;
					break;
				}
			}

			if(empty){
				//empty cache line exists
				cache[index][empty_cache_line][CACHE_LINE_BIT_SIZE - 1] = 1;

				for(int i = 0; i < num_tag_bits; i++){
					cache[index][empty_cache_line][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag2[i];
				}

			}else{
				//empty cache line does not exist

				//Find the cache_line to replace

				bitset<32> removing_tag(lru_history[index][associativity-1]);

				int lru_index;
				for(lru_index = 0; lru_index < associativity; lru_index++){

					if(removing_tag.to_string().substr(NUM_ADDRESS_BITS - num_tag_bits).compare(cache[index][lru_index].to_string().substr(1, num_tag_bits)) == 0){
						break;
					}
				}

				cache[index][lru_index][CACHE_LINE_BIT_SIZE - 1] = 1;

				for(int i = 0; i < num_tag_bits; i++){
					cache[index][lru_index][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag2[i];
				}
			}

			lru_history[index].pop_back();
			lru_history[index].push_front(unsigned_tag);

		}

		it++;
	}

	stringstream ss;
	string result;
	ss << hit << ", ";
	ss >> result;
	ss.str(string());
	ss << count;
	result = result + ss.str() + ";";
	return result;
}

string set_associative_prefetch_miss(int cache_size, int associativity, vector<string>& PCS){
	int hit = 0;
	int count = 0;

	int num_lines = cache_size / CACHE_LINE_BYTE_SIZE;
	int num_sets = num_lines / associativity;
	int num_index_bits = (int) log2((double) num_sets);
	int num_tag_bits = NUM_ADDRESS_BITS - num_index_bits - BYTE_OFFSET_SIZE;

	bitset<CACHE_LINE_BIT_SIZE> cache[num_sets][associativity];
	vector< deque<int> > lru_history (num_sets, deque<int>(associativity, -1));

	for(int i = 0; i < num_sets; i++){
		for(int j = 0; j < associativity; j++){
			cache[i][j].reset();
		}
	}


	vector<string>::iterator it = PCS.begin();


	unsigned byte_offset_mask = (1 << BYTE_OFFSET_SIZE) - 1;
	unsigned byte_index_mask = (1 << (BYTE_OFFSET_SIZE + num_index_bits)) - 1;

	string line;
	string next_line;
	while(it != PCS.end()){
		count++;
		//		cout << count << endl;

		line = (*it).substr((*it).find(" ")+1);

		unsigned address;
		unsigned unsigned_tag;
		unsigned index;
		unsigned byte_offset;

		stringstream ss;
		ss << hex << line;
		ss >> address;

		byte_offset = address & byte_offset_mask;
		unsigned_tag = (address - (address & byte_index_mask));
		index = address - byte_offset - unsigned_tag;
		unsigned_tag = unsigned_tag >> (BYTE_OFFSET_SIZE + num_index_bits);
		index = index >> (BYTE_OFFSET_SIZE);
		index = index % num_sets;

		bitset<32> b(byte_offset);
		bitset<32> tag(unsigned_tag);
		bitset<32> i(index);
		bitset<32> addr(address);

		bool match = false;
		//Look for a match
		for(int i = 0; i < associativity; i++){
			if(tag.to_string().substr(NUM_ADDRESS_BITS - num_tag_bits) == cache[index][i].to_string().substr(1, num_tag_bits)){
				match = true;
				break;
			}
		}

		if(match){
			hit++;

			int cache_line_index;
			for(cache_line_index = 0; cache_line_index < associativity; cache_line_index++){
				if(lru_history[index][cache_line_index] == (int) unsigned_tag) break;
			}

			lru_history[index].erase(lru_history[index].begin() + cache_line_index);
			lru_history[index].push_front(unsigned_tag);

		}else{
			bool empty = false;
			int empty_cache_line = -1;

			for(int i = 0; i < associativity; i++){
				if(lru_history[index][i] == -1){
					empty_cache_line = i;
					empty = true;
					break;
				}
			}

			if(empty){
				//empty cache line exists
				cache[index][empty_cache_line][CACHE_LINE_BIT_SIZE - 1] = 1;

				for(int i = 0; i < num_tag_bits; i++){
					cache[index][empty_cache_line][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag[i];
				}

			}else{
				//empty cache line does not exist

				//Find the cache_line to replace

				bitset<32> removing_tag(lru_history[index][associativity-1]);

				int lru_index;
				for(lru_index = 0; lru_index < associativity; lru_index++){

					if(removing_tag.to_string().substr(NUM_ADDRESS_BITS - num_tag_bits).compare(cache[index][lru_index].to_string().substr(1, num_tag_bits)) == 0){
						break;
					}
				}

				cache[index][lru_index][CACHE_LINE_BIT_SIZE - 1] = 1;

				for(int i = 0; i < num_tag_bits; i++){
					cache[index][lru_index][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag[i];
				}
			}

			lru_history[index].pop_back();
			lru_history[index].push_front(unsigned_tag);

		}

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		if(!match){
			address = address + NUM_ADDRESS_BITS;
			byte_offset = address & byte_offset_mask;
			unsigned_tag = (address - (address & byte_index_mask));
			index = address - byte_offset - unsigned_tag;
			unsigned_tag = unsigned_tag >> (BYTE_OFFSET_SIZE + num_index_bits);
			index = index >> (BYTE_OFFSET_SIZE);
			index = index % num_sets;

			bitset<32> b2(byte_offset);
			bitset<32> tag2(unsigned_tag);
			bitset<32> i2(index);
			bitset<32> addr2(address);

			match = false;
			//Look for a match
			for(int i = 0; i < associativity; i++){
				if(tag2.to_string().substr(NUM_ADDRESS_BITS - num_tag_bits) == cache[index][i].to_string().substr(1, num_tag_bits)){
					match = true;
					break;
				}
			}

			if(match){
				int cache_line_index;

				for(cache_line_index = 0; cache_line_index < associativity; cache_line_index++){
					if(lru_history[index][cache_line_index] == (int) unsigned_tag) break;
				}

				lru_history[index].erase(lru_history[index].begin() + cache_line_index);
				lru_history[index].push_front(unsigned_tag);

			}else{
				bool empty = false;
				int empty_cache_line = -1;

				for(int i = 0; i < associativity; i++){
					if(lru_history[index][i] == -1){
						empty_cache_line = i;
						empty = true;
						break;
					}
				}

				if(empty){
					//empty cache line exists
					cache[index][empty_cache_line][CACHE_LINE_BIT_SIZE - 1] = 1;

					for(int i = 0; i < num_tag_bits; i++){
						cache[index][empty_cache_line][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag2[i];
					}

				}else{
					//empty cache line does not exist

					//Find the cache_line to replace

					bitset<32> removing_tag(lru_history[index][associativity-1]);

					int lru_index;
					for(lru_index = 0; lru_index < associativity; lru_index++){

						if(removing_tag.to_string().substr(NUM_ADDRESS_BITS - num_tag_bits).compare(cache[index][lru_index].to_string().substr(1, num_tag_bits)) == 0){
							break;
						}
					}

					cache[index][lru_index][CACHE_LINE_BIT_SIZE - 1] = 1;

					for(int i = 0; i < num_tag_bits; i++){
						cache[index][lru_index][CACHE_LINE_BIT_SIZE - 1 - num_tag_bits + i] = tag2[i];
					}
				}

				lru_history[index].pop_back();
				lru_history[index].push_front(unsigned_tag);

			}
		}

		it++;
	}

	stringstream ss;
	string result;
	ss << hit << ", ";
	ss >> result;
	ss.str(string());
	ss << count;
	result = result + ss.str() + ";";
	return result;
}
