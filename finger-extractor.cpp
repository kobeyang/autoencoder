#include <algorithm>
#include <bitset>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "finger-extractor.h"
#include "global.h"
#include "util.h"

using namespace std;
vector<int> FingerprintExtractor::_layers
= vector<int>{ 1024, 512, 256, 128, 64 };
vector<int> FingerprintExtractor::_selected_bits
= vector < int > {0, 1, 3, 5, 8, 9, 10, 12, 14, 17, 20, 23, 25, 26, 28,
30, 31, 34, 37, 39, 40, 41, 43, 45, 49, 51, 52, 54, 55, 56, 58, 62};
vector<vector<vector<double>>> FingerprintExtractor::_W;

const double E = 2.718281832452;

// 32 bands.
const double freq_bind[] =
{
	300.000, 318.323, 337.766, 358.396, 380.286,
	403.513, 428.158, 454.309, 482.057, 511.500,
	542.741, 575.890, 611.064, 648.387, 687.989,
	730.009, 774.597, 821.907, 872.107, 925.374,
	981.893, 1041.86, 1105.50, 1173.02, 1244.67,
	1320.69, 1401.35, 1486.94, 1577.76, 1674.13,
	1776.38, 1884.88, 2000.00
};

void FingerprintExtractor::CreateImage(const string& filepath) {
	this->_wavepath = filepath;
	_wp.OpenWaveFile(_wavepath.c_str());
	_wp.MakeTargetSamplesData();
	unsigned long all_time_data_size = 0;
	_wp.GetSamplesVector(_all_time_data, all_time_data_size);
	_wp.CloseWaveFile();
	_Energying(all_time_data_size);
	return;
}

void FingerprintExtractor::CalcFingerprint(const string& filepath) {
	this->CreateImage(filepath);
	vector<Sample> samples;
	this->GetSamples(&samples);
	for (int i = 0; i < samples.size(); i++)
		_fingers[i] = this->GenerateSubFingerprint(samples[i]);
}

// Fead Forward process.
string FingerprintExtractor::GenerateSubFingerprint(const Sample& sample) {
	vector<vector<double>> a;
	for (int i = 0; i < _layers.size(); i++)
		a.push_back(vector<double>(_layers[i] + 1, 1)); // 1 for bias term.
	vector<double> input;
	input.push_back(1.0); // bias term.
	double max_value = INT_MIN, min_value = INT_MAX;
	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < BINDS_NUM; j++) {
			input.push_back(sample.image[i][j]);
			max_value = max(max_value, sample.image[i][j]);
			min_value = min(min_value, sample.image[i][j]);
		}
	}
	// Regulation.
	max_value = max_value + 1; // Avoid max_value equals to min_value;
	for (int i = 1; i < input.size(); i++)
		a[0][i] = (input[i] - min_value) / (max_value - min_value);

	for (int l = 0; l < _layers.size() - 1; l++) {
		for (int i = 0; i < _layers[l+1]; i++) {
			double value = 0;
			for (int j = 0; j < _layers[l] + 1; j++) {
				//cout << i << "\t" << j << endl;
				value += a[l][j] * _W[l][i][j];
			}
			a[l + 1][i + 1] = 1.0 / (1 + pow(E, -value));
		}
	}
	string sub_finger;
	int end = (int)_layers.size() - 1;
	for (int idx : _selected_bits) {
		if (a[end][idx] > MEDIAN)
			sub_finger.push_back('1');
		else
			sub_finger.push_back('0');
	}
	return sub_finger;
}

void FingerprintExtractor::GetSamples(vector<Sample>* samples) {
	samples->clear();
	int pixel_jump = FRAME_LENGTH / 32;
	for (int i = 0; i < _frame_number; i++) {
		Sample sample;
		for (int j = 0; j < FRAME_LENGTH; j += pixel_jump ) {
			for (int k = 0; k < BINDS_NUM; k++) {
				double value = 0.0;
				for (int m = 0; m < pixel_jump; m++)
					value += _energy[i + j + m][k];
				sample.image[j / pixel_jump][k] = value;
			}
		}
		samples->push_back(sample);
	}
	return;
}

void FingerprintExtractor::ReadW(const string& filepath) {
	// Read weight W.
	for (int i = 0; i < _layers.size() - 1; i++) {
		_W.push_back(vector<vector<double>>(_layers[i + 1], vector<double>(_layers[i] + 1)));
	}
	for (int i = 0; i < _W.size(); i++) {
		string file = filepath + "W_" + to_string(i + 1) + ".txt";
		cout << file << endl;
		fstream fin(file, fstream::in);
		double weight;
		for (int j = 0; j < _W[i].size(); j++) {
			for (int k = 0; k < _W[i][0].size(); k++) {
				fin >> weight;
				_W[i][j][k] = weight;
			}
		}
		fin.close();
	}
	// Read max_value.
	/*
	string file = filepath + "max_value.txt";
	fstream fin(file, fstream::in);
	double tmp;
	for (int i = 0; i < 32 * BINDS_NUM; i++) {
		fin >> tmp;
		_max_value.push_back(tmp);
	}
	fin.close();

	// Read min_value.
	file = filepath + "min_value.txt";
	fin.open(file, fstream::in);
	for (int i = 0; i < 32 * BINDS_NUM; i++) {
		fin >> tmp;
		_min_value.push_back(tmp);
	}
	fin.close();
	*/
	cout << "Read W finished." << endl;
}

void FingerprintExtractor::GetQueryFinger(ItemType* new_finger, int& size) {
	size = _frame_number;
	for (int i = 0; i < _frame_number; i++) {
		ItemType item(_fingers[i]);
		new_finger[i] = item;
	}
	return;
}

int FingerprintExtractor::GetFrameNumber() {
	return _frame_number;
}

int FingerprintExtractor::GetFileId() {
	string originFile = _wavepath.substr(_wavepath.find_last_of("\\") + 1, _wavepath.size());
	return stoi(originFile.substr(0, originFile.find(".")));
}

void FingerprintExtractor::PrintFingerToFile(const string& fingerFile) {
	FILE *fp = fopen(fingerFile.c_str(), "w");
	string sub_finger;
	for (int i = 0; i < _frame_number; i++) {
		sub_finger = "";
		for (int j = 0; j < ITEM_BITS; j++)
			sub_finger.push_back(_fingers[i][j]);
		ItemType b(sub_finger);
		fprintf(fp, "%lu\n", b.to_ulong());
	}
	fclose(fp);
	return;
}

int FingerprintExtractor::_Energying(long all_time_data_size) {
	memset(_energy, 0, sizeof(double)* QUERY_FINGER_NUM * BINDS_NUM);
	_frame_number = 0;
	int start = 0;
	int jump_samples = (int)(sampleRate * TIME_INTERVAL); // 5000 means the sample rate.

	while (start + NumSamplesPerFrameM < all_time_data_size) {
		short time_data[1850];
		cpxv_t freq_data[2048];
		double bind_energy[BINDS_NUM];
		memset(bind_energy, 0, sizeof(double)* BINDS_NUM);
		for (int i = 0; i < NumSamplesPerFrameM; i++) {
			time_data[i] = _all_time_data[i + start];
		}

		//FFT_start = clock();
		DoFFT(time_data, freq_data);
		//FFT_end = clock();
		//duration_FFT += (double)(FFT_end - FFT_start) / CLOCKS_PER_SEC;
		double point_freq = 0;
		for (int j = 0; j < NumBinsInFftWinM; j++) {
			//FFT结果第n个点代表的频率值
			point_freq = (j + 1) * sampleRate / NumBinsInFftWinM;
			if (point_freq < 300 || point_freq > 2000) {
				continue;
			}
			else {
				int bind = _SelectBind(point_freq); // [0,32]
				bind_energy[bind] += (freq_data[j].re * freq_data[j].re + freq_data[j].im * freq_data[j].im);
			}
		}
		for (int i = 0; i < BINDS_NUM; i++)
			_energy[_frame_number][i] = log(bind_energy[i] + 1); // Get log here!

		//下一帧
		_frame_number++;
		start += jump_samples;
	}
	_frame_number -= FRAME_LENGTH;
	return _frame_number;
}

int FingerprintExtractor::_SelectBind(double point_freq) {
	int start = 0;
	int end = BINDS_NUM;
	while (start <= end) {
		int mid = start + (end - start) / 2;
		if (point_freq < freq_bind[mid])
			end = mid - 1;
		else if (point_freq > freq_bind[mid + 1])
			start = mid + 1;
		else
			return mid;
	}
	return -1;
}