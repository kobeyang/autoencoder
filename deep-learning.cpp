#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "deep-learning.h"
#include "finger-extractor.h"
#include "util.h"

using namespace std;
vector<Sample> DeepLearning::_samples;

void DeepLearning::GetDeepLearningTrainingSamples(
	const string& wave_path, const string& output_path) {
	_samples.clear();
	vector<string> all_files = Util::LoadDir(wave_path, "wav");

	vector<thread> threads;
	vector<vector<string>> parts(THREAD_NUM);
	for (int i = 0; i < all_files.size(); i++)
		parts[i%THREAD_NUM].push_back(all_files[i]);
	for (int i = 0; i < THREAD_NUM; i++)
		threads.push_back(thread(_GetDeepLearningTrainingSamplesSingleThread, parts[i]));
	for (int i = 0; i < THREAD_NUM; i++)
		threads[i].join();
	_OutputDeepLearningSamples(output_path);
}

void DeepLearning::_GetDeepLearningTrainingSamplesSingleThread(const vector<string>& files) {
	for (const auto file : files) {
		cout << file << endl;
		vector<Sample> samples;

		FingerprintExtractor extractor;
		extractor.CreateImage(file);
		extractor.GetSamples(&samples);
		for (size_t i = 0; i < samples.size(); i += 50)
			_samples.push_back(samples[i]);
	}
}

void DeepLearning::_OutputDeepLearningSamples(const string& file_path) {
	fstream fout;
	fout.open(file_path, fstream::out);
	int batch_size = 100;
	int output_samples = (int)_samples.size() / batch_size * batch_size;
	stringstream ss;
	for (int i = 0; i < output_samples; i++) {
		for (int j = 0; j < 32; j++) {
			for (int k = 0; k < BINDS_NUM; k++) {
				ss << setprecision(2) << std::fixed << _samples[i].image[j][k] << "\t";
			}
		}
		ss << endl;
	}
	fout << ss.str();
	ss.str("");
	fout.close();
}

void DeepLearning::GetDeepLearningTestingSamples(const string& wavepath, const string& output_path) {
	_samples.clear();
	vector<string> files = Util::LoadDir(wavepath, "wav");
	for (const auto file : files) {
		cout << file << endl;
		vector<Sample> samples;

		FingerprintExtractor extractor;
		extractor.CreateImage(file);
		extractor.GetSamples(&samples);
		for (size_t i = 0; i < samples.size(); i += 50) {
			_samples.push_back(samples[i]);
		}
	}
	_OutputDeepLearningSamples(output_path);
}