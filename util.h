#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <bitset>
#include <io.h>
#include "global.h"

class Util {
private:
	static std::vector<std::string> allFiles;
	static std::vector<std::vector<ItemType>> finger_database;
	static int _LoadFingerFromOneFile(std::string filepath_prefix, unsigned int fileNum);
	static int _OutputFingerToOneFile(std::string filepath_prefix, unsigned int fileNum);
public:
	static int LoadOneFile(std::string filepath, std::vector<KeyType>& audio_fingers);
	static std::vector<std::string> LoadDir(std::string dirpath, std::string type);
	static void LoadDirSpecific(std::vector<std::vector<std::string>>& allQueryFiles, std::string dirpath, std::string type);
	static int LoadIndex(std::string index_filepath, IndexType& index);
	static int OutputIndex(std::string index_filepath, IndexType& index);
	static int LoadFingerDatabase(std::string filepath_prefix);
	static int OutputFingerToFile(std::string filepath_prefix);
	static int DeleteSomeIndex(std::string index_filepath, std::set<int> remove_list);
	static int DeleteSomeIndex(std::string index_filepath, std::string filepath);
	static int DeleteFingerDatabase(std::string database_filepath, std::set<int> remove_list);
	static int DeleteFingerDatabase(std::string database_filepath, std::string filepath);
	static int IncrementalBuildIndex(std::string index_filepath, std::string dir_path);
	static int IncrementalBuildFingerDatabase(std::string database_filepath, std::string dir_path);
	static std::vector<ItemType> VectorKeyToVectorBitset(std::vector<KeyType> v);
	static std::vector<KeyType> VectorBitsetToVectorKey(std::vector<ItemType> v);
	//以上函数均测试通过
	static int t_DeleteFingerDatabase(std::vector<std::vector<ItemType>>& finger_database, std::set<int> remove_list);
	static int t_DeleteSomeIndex(IndexType& index, std::set<int> remove_list);
	static int t_OutputFingerToFile(std::string filepath_prefix, std::vector<std::vector<ItemType>>& database);
};