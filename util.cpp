#include <algorithm>
#include <bitset>
#include <fcntl.h>
#include <fstream>
#include <io.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "global.h"
#include "util.h"

using namespace std;

std::vector<std::string> Util::allFiles;
std::vector<std::vector<ItemType>> Util::finger_database;

int Util::LoadOneFile(string filepath, vector<KeyType>& audio_fingers) {
	FILE* fp = fopen(filepath.c_str(), "r");
	if (fp == NULL)	{
		std::cout << "no such file: " << filepath << std::endl;
		return -1;
	}
	audio_fingers.clear();
	char line[ITEM_BITS + 1];
	KeyType key = 0;
	while (true) {
		fgets(line, ITEM_BITS + 1, fp);
		// need to implement
		if (feof(fp))
			break;
		string s(line);
		s = s.substr(0, (signed)s.size() - 1);
		audio_fingers.push_back(stoul(s));
	}
	fclose(fp);
	return 0;
}

std::vector<std::string> Util::LoadDir(std::string dirpath, std::string type) {
	allFiles.clear();
	std::string temppath = dirpath + "\\*." + type;
	struct _finddata_t fileinfo;
	intptr_t handle = _findfirst(temppath.c_str(), &fileinfo);
	if (-1 == handle) {
		std::cout << "can not find dir or file!" << std::endl;
	}
	allFiles.push_back(dirpath + "\\" + string(fileinfo.name));
	while (!_findnext(handle, &fileinfo)) {
		allFiles.push_back(dirpath + "\\" + string(fileinfo.name));
	}
	_findclose(handle);
	return allFiles;
}

void Util::LoadDirSpecific(std::vector<std::vector<std::string>>& allQueryFiles,
	std::string dirpath, std::string type) {
	std::string temppath = dirpath + "\\*." + type;
	struct _finddata_t fileinfo;
	intptr_t handle = _findfirst(temppath.c_str(), &fileinfo);
	if (-1 == handle) {
		std::cout << "can not find dir or file!" << std::endl;
	}
	//std::string path(dirpath + "\\" + fileinfo.name);
	string filename = string(fileinfo.name);
	filename = filename.substr(0, filename.find("."));
	int i_filename = stoi(filename);
	int m = i_filename % THREAD_NUM;
	allQueryFiles[m].push_back(dirpath + "\\" + string(fileinfo.name));
	while (!_findnext(handle, &fileinfo)) {
		string filename = string(fileinfo.name);
		filename = filename.substr(0, filename.find("."));
		i_filename = stoi(filename);
		m = i_filename % THREAD_NUM;
		allQueryFiles[m].push_back(dirpath + "\\" + string(fileinfo.name));
	}
	_findclose(handle);
	return;
}

int Util::_LoadFingerFromOneFile(string filepath_prefix, unsigned int fileNum) {
	ifstream fin(filepath_prefix + to_string(fileNum), ios::in | ifstream::binary);
	int databaseSize = 0;
	fin.read(reinterpret_cast<char *>(&databaseSize), sizeof(databaseSize));
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

int Util::LoadFingerDatabase(string filepath_prefix) {
	finger_database.resize(DATABASE_SIZE);
	vector<thread> threads(OUTPUT_THREAD);
	for (int i = 0; i < OUTPUT_THREAD; i++) {
		threads[i] = thread(&Util::_LoadFingerFromOneFile, filepath_prefix, i);
	}

	for (int i = 0; i < OUTPUT_THREAD; i++) {
		threads[i].join();
	}
	return 0;
}

int Util::_OutputFingerToOneFile(string filepath_prefix, unsigned int fileNum) {
	ofstream fout(filepath_prefix + to_string(fileNum), ios::out | ofstream::binary);
	unsigned int databaseSize = (unsigned int)finger_database.size();
	fout.write(reinterpret_cast<char *>(&databaseSize), sizeof(unsigned int));
	unsigned int fingerSize = 0;
	vector<KeyType> intVector;
	for (unsigned int i = 0; i < databaseSize; i++)	{
		if (i % OUTPUT_THREAD == fileNum && finger_database[i].size() != 0)	{
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

int Util::OutputFingerToFile(string filepath_prefix) {
	unsigned int databaseSize = (unsigned int)finger_database.size();
	vector<thread> threads(OUTPUT_THREAD);

	for (int i = 0; i < OUTPUT_THREAD; i++)	{
		threads[i] = thread(&Util::_OutputFingerToOneFile, filepath_prefix, i);
	}

	for (int i = 0; i < OUTPUT_THREAD; i++)	{
		threads[i].join();
	}
	return 0;
}

int Util::LoadIndex(string filepath, IndexType& index) {
	index.clear();
	int index_size = 0;
	ifstream fin(filepath, ios::in | ifstream::binary);
	fin.read(reinterpret_cast<char *>(&index_size), sizeof(int));
	if (index_size == 0) {
		fin.close();
		return -1;
	}
	//index.resize(index_size);
	fin.read(reinterpret_cast<char *>(&index[0]), index.size() * sizeof(index[0]));
	fin.close();
	return 0;
}

int Util::OutputIndex(string filepath, IndexType& index) {
	unsigned int index_size = (unsigned int)index.size();
	ofstream fout(filepath, ios::out | ofstream::binary);
	fout.write(reinterpret_cast<char *>(&index_size), sizeof(unsigned int));
	if (index_size == 0) {
		fout.close();
		return -1;
	}
	fout.write(reinterpret_cast<char *>(&index[0]), index.size() * sizeof(index[0]));
	fout.close();
	return 0;
}

vector<ItemType> Util::VectorKeyToVectorBitset(vector<KeyType> v) {
	vector<ItemType> bv;
	for (int i = 0; i < v.size(); i++) {
		ItemType b(v[i]);
		bv.push_back(b);
	}
	return bv;
}

vector<KeyType> Util::VectorBitsetToVectorKey(vector<ItemType> v) {
	vector<KeyType> iv;
	for (int i = 0; i < v.size(); i++) {
		KeyType key = v[i].to_ulong();
		iv.push_back(key);
	}
	return iv;
}