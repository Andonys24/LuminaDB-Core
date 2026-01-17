#ifndef LUMINADB_BUFFER_POOL_MANAGER_HPP
#define LUMINADB_BUFFER_POOL_MANAGER_HPP

#include "LRUReplacer.hpp"
#include "luminadb/model/Storable.hpp"
#include "luminadb/storage/DiskManager.hpp"
#include "luminadb/storage/Page.hpp"
#include <list>
#include <mutex>
#include <unordered_map>

namespace LuminaDB {
class BufferPoolManager {
  private:
	size_t pool_size;								   // How many pages fit in RAM
	DiskManager *disk_manager;						   // To read/write the file
	LRUReplacer *replacer;							   // "referee" LRU
	Page *pages;									   // Physical arrangement of pages in RAM (frames)
	std::unordered_map<uint32_t, uint32_t> page_table; // page_id -> frame_id
	std::list<uint32_t> free_list;					   // Frames that have never been used
	std::mutex latch;								   // Thread safety

	bool *is_dirty;
	uint32_t *pin_count;
	uint32_t next_page_id;

  public:
	BufferPoolManager(size_t pool_size, DiskManager *disk_manager);
	~BufferPoolManager();

	// Brings a page into RAM. If it's already there, just increase the pin_count.
	Page *fetchPage(uint32_t page_id);

	// Indicates that you no longer use the page. isdirty = true if modified.
	bool unpinPage(uint32_t page_id, bool is_dirty_flag);

	// Creates a new page on the disk and loads it into RAM.
	Page *newPage(uint32_t &page_id, ModelType object_type);

	// Deletes a page from the disk and RAM.
	bool deletePage(uint32_t page_id);

	// Forces writing a page to the disk.
	bool flushPage(uint32_t page_id);
};

} // namespace LuminaDB

#endif