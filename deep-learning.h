#pragma once
#include <vector>
#include <string>
#include "global.h"

class Sample {
public:
	double image[32][BINDS_NUM];
};

class DeepLearning {
public:
	void GetDeepLearningTrainingSamples(const std::string& wave_path,
		const std::string& output_path);
	static void _GetDeepLearningTrainingSamplesSingleThread(
		const std::vector<std::string>& files);
	void GetDeepLearningTestingSamples(const std::string& wavepath,
		const std::string& output_path);
private:
	void _OutputDeepLearningSamples(const std::string& file_path);
	static std::vector<Sample> _samples;
};