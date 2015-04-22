#include <algorithm>
#include <cmath>
#include <ctime>
#include <bitset>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include "global.h"
#include "searcher.h"
#include "util.h"

using namespace std;

int hit_number = 0;
int list_contain_id = 0;
int sub_fingerprint = 0;
long long list_len = 0;

bool comp(pair<KeyType, MusicInfo> a,
	pair<KeyType, MusicInfo> b) {
	return a.first != b.first ?
		a.first < b.first : a.second.id < b.second.id;
}

int Searcher::BuildIndex(string dirPath) {
	finger_database.clear();
	finger_database.resize(DATABASE_SIZE);
	time_t sort_start, sort_end;
	vector<string> allFiles = Util::LoadDir(FINGER_ROOTPATH, "txt");
	for (int i = 0; i < (signed)allFiles.size(); i++) {
		if (i % 1000 == 0)
			cout << "Index: " << i << endl;
		_BuildOneFileIndex(allFiles[i]);
	}

	sort_start = clock();
	sort(_index.begin(), _index.end(), comp);

	sort_end = clock();
	double sort_time = (double)(sort_end - sort_start) / CLOCKS_PER_SEC;
	cout << "Sort time: " << sort_time << endl;
	return 0;
}

int Searcher::_BuildOneFileIndex(const string filepath) {
	vector<KeyType> audio_file;
	Util::LoadOneFile(filepath, audio_file);
	string originFile = filepath.substr(filepath.find_last_of("\\") + 1, filepath.find_last_of("."));
	int finger_id = stoi(originFile);
#ifdef SUB_SAMPLING
	vector<ItemType> temp_v = Util::VectorIntToVectorBitset(audio_file);
	vector<ItemType> fingers_block;
	for (int i = 0; i < temp_v.size(); i++)	{
		if (i % M == 0)
			fingers_block.push_back(temp_v[i]);
	}
	finger_database[finger_id] = fingers_block;
#else
	finger_database[finger_id] = Util::VectorKeyToVectorBitset(audio_file);
#endif
	MusicInfo m(finger_id, 0);
	int i = 0;

#ifdef SUB_SAMPLING
	for (i = 0; i < (signed)audio_file.size(); i++) {
		if (i % M == 0)	{
			m.i_frame = i / M;
			_insert_one_item(audio_file[i], m);
		}
	}
#else
	for (i = 0; i < (signed)audio_file.size(); i++)	{
		m.i_frame = i;
		_InsertOneItem(audio_file[i], m);
	}
#endif
	return i;
}

int Searcher::_InsertOneItem(KeyType key, MusicInfo& m) {
	if (key == 0)
		return 0;
	_index.push_back(make_pair(key, m));
	return 0;
}

int Searcher::Search(int queryId, ItemType* finger_block, const int block_size) {
	std::map<double, int> result_map;
	//exact match
	//从查询指纹块的第一条指纹开始
	for (int i = 0; i < block_size; i++) {
		KeyType key = finger_block[i].to_ulong();
		if (key == 0)
			continue;
		int result = 0;
		result = _InnerSearch(queryId, key, finger_block, block_size, i, &result_map);
		if (result > 0)
			return result;
	}

#ifdef ONE_BIT_SEARCH
	for (int i = 0; i < block_size; i++) {
		for (int j = 0; j < ITEM_BITS; j++) {
			ItemType item = finger_block[i];
			item.flip(j);
			KeyType key = item.to_ulong();
			if (key == 0)
				continue;
			int result = _InnerSearch(queryId, key, finger_block, block_size, i, &result_map);
			if (result > 0)
				return result;
		}
	}
#endif

#ifdef TWO_BIT_SEARCH
	for (int i = 0; i < block_size; i++) {
		for (int j = 0; j < ITEM_BITS - 1; j++) {
			for (int k = j + 1; k < ITEM_BITS; k++) {
				ItemType item = finger_block[i];
				item.flip(j);
				item.flip(k);
				KeyType key = item.to_ulong();
				if (key == 0)
					continue;
				int result = _InnerSearch(queryId, key, finger_block, block_size, i, &result_map);
				if (result > 0)
					return result;

			}
		}
	}
#endif
	if (result_map.size() > 0) {
		//cerr << result_map.begin()->first << endl;
		return result_map.begin()->second;
	}
	else
		return -1;
}

int Searcher::_InnerSearch(int queryId, KeyType key, ItemType* finger_block,
	const int block_size, const int i, map<double, int>* result_map) {
	time_t search_start, search_end;
	search_start = clock();
	long long result = _BinarySearch(key);
	if (result == -1)
		return -1;
	long long start = result;
	long long end = result;
	do {
		start--;
	} while (start >= 0 && _index[start].first == key);
	start++;
	do {
		end++;
	} while (end < (signed)_index.size() && _index[end].first == key);
	end--;
	sub_fingerprint++;
	list_len += (end - start + 1);
	search_end = clock();
	//duration_search += (double)(search_end - search_start) / CLOCKS_PER_SEC;
	for (long long iter = start; iter <= end; iter++) {
		double diffbits = _CompareBitsets(_index[iter].second.id, finger_block,
			block_size, i, _index[iter].second.i_frame);
		//hit_number++;
		if (diffbits <= BIT_ERROR_RATE) {
			//cerr << diffbits << endl;
			return _index[iter].second.id;
			(*result_map)[diffbits] = _index[iter].second.id;
			if (diffbits <= MUST_RIGHT) {
				return _index[iter].second.id;
			}
		}
	}
	return -1;
}


/*
*i_frame_in_block: 在query指纹块中命中的index
*i_frame_in_file: 在file中命中的index
*/
double Searcher::_CompareBitsets(int id, ItemType* finger_block, const int block_size, \
	const int i_frame_in_block, int i_frame_in_file) {
	if (i_frame_in_file - i_frame_in_block < 0)
		return INT_MAX;//表示错误，返回1.0
	int diff_bits = 0;
	vector<ItemType>& full_audio_fingers = finger_database[id];

	if (i_frame_in_file + block_size - i_frame_in_block > full_audio_fingers.size())
		return INT_MAX;
	i_frame_in_file -= i_frame_in_block;

	//time_compare_start = clock();

	for (int i = 0; i < block_size; i++) {
		ItemType subfinger_xor = finger_block[i] ^ full_audio_fingers[i_frame_in_file];
		i_frame_in_file++;
		diff_bits += (int)subfinger_xor.count();
	}

	//time_compare_finish = clock();
	//duration_compare += (double)(time_compare_finish - time_compare_start)/CLOCKS_PER_SEC;
	return (double)diff_bits / (ITEM_BITS * QUERY_FINGER_NUM);
}

long long Searcher::_BinarySearch(KeyType key) {
	long long start = 0;
	long long end = _index.size() - 1;
	long long mid;
	while (end >= start) {
		mid = start + (end - start) / 2;
		if (key < _index[mid].first)
			end = mid - 1;
		else if (key > _index[mid].first)
			start = mid + 1;
		else
			return mid;
	}
	return -1;
}

int Searcher::LoadIndex(string filepath) {
	_index.clear();
	unsigned int index_size = 0;
	ifstream fin(filepath, ios::in | ifstream::binary);
	fin.read(reinterpret_cast<char *>(&index_size), sizeof(int));
	if (index_size == 0) {
		fin.close();
		return -1;
	}
	_index.resize(index_size);
	fin.read(reinterpret_cast<char *>(&_index[0]), _index.size() * sizeof(_index[0]));
	fin.close();
	return 0;
}

int Searcher::_LoadFingerFromOneFile(string filepath_prefix, unsigned int fileNum) {
	ifstream fin(filepath_prefix + to_string(fileNum), ios::in | ifstream::binary);
	int databaseSize = 0;
	fin.read(reinterpret_cast<char *>(&databaseSize), sizeof(databaseSize));
	finger_database.resize(databaseSize);
	unsigned int songID = 0;
	unsigned int fingerSize = 0;
	vector<KeyType> iv;
	while (true) {
		fin.read(reinterpret_cast<char *>(&songID), sizeof(songID));
		if (fin.eof())
			break;
		fin.read(reinterpret_cast<char *>(&fingerSize), sizeof(fingerSize));
		if (fingerSize != 0) {
			iv.resize(fingerSize);
			fin.read(reinterpret_cast<char *>(&iv[0]), fingerSize * sizeof(iv[0]));
			finger_database[songID] = Util::VectorKeyToVectorBitset(iv);
		}
	}
	fin.close();
	return 0;
}

int Searcher::LoadFingerDatabase(string filepath_prefix) {
	finger_database.clear();
	vector<thread> threads(OUTPUT_THREAD);
	for (int i = 0; i < OUTPUT_THREAD; i++) {
		threads[i] = thread(&Searcher::_LoadFingerFromOneFile, this, filepath_prefix, i);
	}

	for (int i = 0; i < OUTPUT_THREAD; i++)	{
		threads[i].join();
	}
	return 0;
}

int Searcher::_OutputFingerToOneFile(string filepath_prefix,
	unsigned int databaseSize, unsigned int fileNum) {
	ofstream fout(filepath_prefix + to_string(fileNum), ios::out | ofstream::binary);
	fout.write(reinterpret_cast<char *>(&databaseSize), sizeof(unsigned int));
	unsigned int fingerSize = 0;
	vector<KeyType> intVector;
	for (unsigned int i = 0; i < databaseSize; i++) {
		if (i % OUTPUT_THREAD == fileNum) {
			intVector = Util::VectorBitsetToVectorKey(finger_database[i]);
			fingerSize = (unsigned int)intVector.size();
			fout.write(reinterpret_cast<char *>(&i), sizeof(i));
			fout.write(reinterpret_cast<char *>(&fingerSize), sizeof(fingerSize));
			if (fingerSize != 0)
				fout.write(reinterpret_cast<char *>(&intVector[0]), fingerSize * sizeof(intVector[0]));
		}
	}
	fout.close();
	return 0;
}

int Searcher::OutputFingerToFile(string filepath_prefix) {
	unsigned int databaseSize = (unsigned int)finger_database.size();
	vector<thread> threads(OUTPUT_THREAD);

	for (int i = 0; i < OUTPUT_THREAD; i++)	{
		threads[i] = thread(&Searcher::_OutputFingerToOneFile, this, filepath_prefix, databaseSize, i);
	}

	for (int i = 0; i < OUTPUT_THREAD; i++)	{
		threads[i].join();
	}
	return 0;
}

int Searcher::OutputIndexToFile(string filepath) {
	ofstream fout(filepath, ios::out | ofstream::binary);
	size_t index_size = _index.size();
	fout.write(reinterpret_cast<char *>(&index_size), sizeof(int));
	if (index_size == 0) {
		fout.close();
		return -1;
	}
	fout.write(reinterpret_cast<char *>(&_index[0]), _index.size() * sizeof(_index[0]));
	fout.close();
	return 0;
}

int Searcher::Clear() {
	_index.clear();
	finger_database.clear();
	return 0;
}

void Searcher::DoStatistics() {
	/*
	ofstream fout;
	vector<int> bits(ITEM_BITS, 0);
	map<int, int> counter;
	int total_item = 0;
	for (IndexType::iterator iter = _index.begin(); iter != _index.end(); ++iter) {
		int length = iter->second.size();
		if (length < 5)
			counter[0]++;
		else if (length < 10)
			counter[1]++;
		else if (length < 20)
			counter[2]++;
		else if (length < 50)
			counter[3]++;
		else if (length < 100)
			counter[4]++;
		else
			counter[5]++;
		total_item ++;
	}
	
	for (const auto iter : counter)
		cout << iter.first << "\t" << (double)iter.second / total_item * 100 << endl;
	cout << "Statistics Done." << endl;
	getchar();
	*/
	
	int number = 1;
	int distinct_key = 1;
	vector<int> distribution(21, 0);
	for (int i = 1; i < _index.size(); i++) {
		if (_index[i].first != _index[i - 1].first) {
			if (number < 20)
				distribution[number]++;
			else
				distribution[20]++;
			distinct_key++;
			number = 1;
		}
		else {
			number++;
		}
	}
	cout << "Total keys: " << _index.size() << endl;
	cout << "Distinct keys: " << distinct_key << endl;
	for (int i = 1; i < distribution.size(); i++)
		cout << "Key numbers for value list length " << i << ": "
		<< (double)distribution[i] / distinct_key * 100 << "%" << endl;
	cout << "Average length: " << (double)_index.size() / distinct_key << endl;

	/*
	for (int i = 0; i < _index.size(); i++) {
		for (int j = 0; j < ITEM_BITS; j++) {
			if ((_index[i].first >> j) & 1)
				bits[j]++;
		}
	}
	
	
	vector<int> idx(ITEM_BITS);
	for (int i = 0; i < ITEM_BITS; i++)
		idx[i] = i;
	for (int i = 0; i < ITEM_BITS - 1; i++) {
		for (int j = i + 1; j < ITEM_BITS; j++) {
			if (bits[i] < bits[j]) {
				swap(bits[i], bits[j]);
				swap(idx[i], idx[j]);
			}
		}
	}
	
	for (int i = 0; i < ITEM_BITS; i++)
		fout << (double)bits[i] / _index.size() * 100 << endl;
		//cout << "1 in bit " << idx[i] << ": " << (double)bits[i] / _index.size() * 100 << "%" << endl;
	fout.close();
	*/
}