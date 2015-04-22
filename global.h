#pragma once
#include <bitset>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

//#define ONE_BIT_SEARCH
//#define TWO_BIT_SEARCH

class MusicInfo {
public:
	int id;
	int i_frame;
	MusicInfo(int ID, int FID) :id(ID), i_frame(FID){};
	MusicInfo(){};
};

const int ITEM_BITS = 32;
typedef unsigned int KeyType;
typedef std::bitset<ITEM_BITS> ItemType;
typedef std::vector<std::pair<KeyType, MusicInfo>> IndexType;

const int BINDS_NUM = 32;
const int FRAME_LENGTH = 64;
const int DATABASE_SIZE = 174000;
const double TIME_INTERVAL = 0.0116;
const double MEDIAN = 0.4498;
const double BIT_ERROR_RATE = 0.1;
const double MUST_RIGHT = 0.1;
const int THREAD_NUM = 10;
const int OUTPUT_THREAD = 10;
const int SUB_FINGER_NUM = 380000; // 380000, there are 186056 subfingerprints in 90408 with 23.2
// Hop 11.6ms, 64 frames: 781 for 10 seconds.
const int QUERY_FINGER_NUM = 783;

const std::string WAVE_ROOTPATH = "Z:\\200000_s48_24000hz_wav\\1";
const std::string FINGER_ROOTPATH = "E:\\yangguang\\autoencoder\\data\\fingers";
const std::string QUERY_WAVE_PATH = "E:\\yangguang\\data\\query\\not_exist\\";
const std::string INDEX_FILE_PATH = "E:\\yangguang\\autoencoder\\data\\index";
const std::string WHOLE_FINGER_PATH = "E:\\yangguang\\cvaf\\data\\index\\20k.finger";