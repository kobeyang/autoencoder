#pragma once
#include <bitset>
#include <iostream>
#include <string>
#include <vector>
#include "deep-learning.h"
#include "fft.h"
#include "global.h"
#include "wave-processor.h"

extern const int NumBinsInFftWinM; // 2048
extern const int NumSamplesPerFrameM; //1850
const double frameInterval = 0.37;
const int sampleRate = 5000;

class FingerprintExtractor {
public:
	FingerprintExtractor() : _wp(5000){};
	void CreateImage(const std::string& filepath);
	void CalcFingerprint(const string& filepath);
	void GetSamples(vector<Sample>* samples);
	void GetQueryFinger(ItemType* new_finger, int& size);
	void PrintFingerToFile(const std::string& filepath);
	int GetFrameNumber();
	int GetFileId();
	static void ReadW(const string& filepath);
	string GenerateSubFingerprint(const Sample& sample);

private:
	std::string _wavepath;
	WaveProcessor _wp;
	int _SelectBind(double point_freq);
	int _Energying(long all_time_data_size);

	int _frame_number; // The total number of frames in the wave file.
	short _all_time_data[SamplesVectorSize];
	// energy[n,m] indicates the energy in nth frame and mth frequency band.
	double _energy[SUB_FINGER_NUM][BINDS_NUM];
	string _fingers[SUB_FINGER_NUM]; // final fingerprint
	static std::vector<std::vector<std::vector<double>>> _W;
	static std::vector<int> _layers;
	static std::vector<int> _selected_bits;
};