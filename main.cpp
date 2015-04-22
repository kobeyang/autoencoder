#include <ctime>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include "deep-learning.h"
#include "finger-extractor.h"
#include "searcher.h"
#include "util.h"

using namespace std;

int yes = 0;
int not_found = 0;
Searcher searcher;

extern double duration_get_energy;
extern int hit_number;
extern int list_contain_id;
extern long long list_len;
extern int sub_fingerprint;

void ExtractFingerprint(vector<string>& all_files) {
	FingerprintExtractor extractor;
	for (const string& f : all_files) {
		size_t pos = f.find_last_of("\\");
		string tmp = f.substr(pos + 1, f.size() - pos);
		string filename = tmp.substr(0, tmp.find("."));
		extractor.CalcFingerprint(f);
		extractor.PrintFingerToFile(FINGER_ROOTPATH + "\\" + filename + ".txt");
		cerr << "File: " << f << " done." << endl;
	}
}

void SearchOneFile(vector<string>& allQueryFiles) {
	FingerprintExtractor extractor;
	for (int i = 0; i < (signed)allQueryFiles.size(); i++) {
		extractor.CalcFingerprint(allQueryFiles[i]);
		ItemType finger_block[QUERY_FINGER_NUM];
		int size = 0;
		int queryId = extractor.GetFileId();
		//cout << queryId << endl;
		extractor.GetQueryFinger(finger_block, size);
		//extractor.PrintFingerToFile(QUERY_WAVE_PATH + "\\" + to_string(queryId) + ".txt");
		int result = searcher.Search(queryId, finger_block, size);
		if (result == -1) {
			//cout<<"file: "<<queryId<<" Not found"<<endl;
			not_found++;
		}
		else if (result == queryId) {
			yes++;
		}
		else {
			cout << queryId << "\t" << result << "\t" << endl;
		}
	}
}

int main() {
	
	time_t start, end;
	
	string w_path = "\\\\172.31.18.25\\yangguang\\deep-learning\\";
	FingerprintExtractor::ReadW(w_path);
	/*
	string path_root = "E:\\yangguang\\autoencoder\\data";
	string wave_path = path_root + "\\training\\part4";
	string output_path = path_root + "\\train_4.txt";
	string test_original_wave = path_root + "\\testing\\original";
	string test_degraded_wave = path_root + "\\testing\\32kbps";
	string test_original_file = path_root + "\\test_original.txt";
	string test_degraded_file = path_root + "\\test_degraded.txt";
	DeepLearning dp;
	//dp.GetDeepLearningTrainingSamples(wave_path, output_path);
	dp.GetDeepLearningTestingSamples(test_original_wave, test_original_file);
	dp.GetDeepLearningTestingSamples(test_degraded_wave, test_degraded_file);
	cout << "Get deep learning samples done!" << endl;
	getchar();
	*/
	/*
	vector<thread> threads;
	start = clock();
	vector<string> allFiles = Util::LoadDir(WAVE_ROOTPATH, "wav");
	vector<vector<string>> allQueryFiles(THREAD_NUM);
	for (int i = 0; i < allFiles.size(); i++)
	allQueryFiles[i%THREAD_NUM].push_back(allFiles[i]);
	for (int i = 0; i < THREAD_NUM; i++)
	threads.push_back(thread(ExtractFingerprint, allQueryFiles[i]));
	for (int i = 0; i < THREAD_NUM; i++)
	threads[i].join();

	cout << "Extract Done!" << endl;
	end = clock();
	cout << "Time: " << (end - start) / CLOCKS_PER_SEC << endl;
	getchar();
	*/
	//start = clock();
	searcher.BuildIndex(FINGER_ROOTPATH);
	//searcher.DoStatistics();
	//getchar();
	//searcher.OutputIndexToFile(INDEX_FILE_PATH);
	//searcher.OutputFingerToFile(WHOLE_FINGER_PATH);
	//searcher.LoadIndex(INDEX_FILE_PATH);
	//searcher.LoadFingerDatabase(WHOLE_FINGER_PATH);
	end = clock();
	//cout << "Load index: " << (double)(end - start) / CLOCKS_PER_SEC << endl;
	vector<string> path{"128kbps", "96kbps", "64kbps", "32kbps"};
	for(const string p : path) {
		vector<thread> threads;
		yes = 0;
		not_found = 0;
		start = clock();
		vector<vector<string>> query_files(THREAD_NUM);
		Util::LoadDirSpecific(query_files, QUERY_WAVE_PATH + p, "wav");
		for(int i = 0; i < THREAD_NUM; i++)
			threads.push_back(thread(SearchOneFile, query_files[i]));
		for(int i = 0; i < THREAD_NUM; i++)
			threads[i].join();
		end = clock();
		cout << p << endl;
		cout << "Yes: " << yes << endl;
		cout << "Not found: " << not_found << endl;
		cout << "Time: " << (double)(end - start) / CLOCKS_PER_SEC << endl;
	}

	getchar();
	return 0;
}
