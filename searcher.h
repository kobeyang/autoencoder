#include <bitset>
#include <map>
#include <string>
#include <vector>
#include "global.h"

class Searcher {
public:
	Searcher(){};
	std::vector<std::vector<ItemType>> finger_database;
	int BuildIndex(std::string dirPath);
	int Search(int queryId, ItemType* finger_block, int size);
	int LoadIndex(std::string filepath);
	int LoadFingerDatabase(std::string filepath);
	int OutputIndexToFile(std::string filepath);
	int OutputFingerToFile(std::string filepath);
	int Clear();
	void DoStatistics();
private:
	IndexType _index;
	int _InsertOneItem(KeyType key, MusicInfo& m);
	int _BuildOneFileIndex(const std::string filepath);
	int _InnerSearch(int queryId, KeyType key, ItemType* finger_block,
		int block_size, int i, std::map<double, int>*);
	int _LoadFingerFromOneFile(std::string filepath_prefix, unsigned int fileNum);
	int _OutputFingerToOneFile(std::string filepath_prefix,
		unsigned int databaseSize, unsigned int fileNum);
	long long _BinarySearch(KeyType key);
	double _CompareBitsets(int id, ItemType* finger_block, int block_size,
		int i_frame_in_block, int i_frame_in_file);
};