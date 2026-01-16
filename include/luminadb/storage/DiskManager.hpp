#ifndef LUMINADB_DISKMANAGER_HPP
#define LUMINADB_DISKMANAGER_HPP

#include <fstream>
#include <string>

namespace LuminaDB {
class DiskManager {
  private:
	std::fstream db_io;
	std::string file_name;

  public:
	DiskManager(const std::string &db_file);
	void writePage(uint32_t page_id, const char *page_data);
	void readPage(uint32_t page_id, char *buffer);
	~DiskManager();
};
} // namespace LuminaDB

#endif