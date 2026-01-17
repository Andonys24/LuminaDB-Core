#include "luminadb/buffer/BufferPoolManager.hpp"

#include <iostream>

namespace LuminaDB {
BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager)
	: pool_size(pool_size), disk_manager(disk_manager) {

	// Recover the previous state
	next_page_id = disk_manager->getExistingPageCount();

	// Reset the memory block for the pages
	pages = new Page[pool_size];
	replacer = new LRUReplacer(pool_size);

	is_dirty = new bool[pool_size];
	pin_count = new uint32_t[pool_size];

	// Initially, all frames are empty.
	for (size_t i = 0; i < pool_size; ++i) {
		is_dirty[i] = false;
		pin_count[i] = 0;
		free_list.push_back(static_cast<uint32_t>(i));
	}

	// Debug
	if (next_page_id > 0) {
		std::cout << "[BPM] Resuming from page ID: " << next_page_id << std::endl;
	}
}

Page *BufferPoolManager::fetchPage(uint32_t page_id) {
	std::lock_guard<std::mutex> lock(latch);

	// CASE A: Is the page already in RAM?
	if (page_table.find(page_id) != page_table.end()) {
		uint32_t frame_id = page_table[page_id];
		pin_count[frame_id]++;
		replacer->pin(frame_id); // Remove from the victims list
		return &pages[frame_id];
	}

	// CASE B: The page is not in RAM. An empty frame is needed.
	uint32_t frame_id;
	if (!free_list.empty()) {
		frame_id = free_list.front();
		free_list.pop_front();
	} else if (replacer->victim(&frame_id)) {
		if (is_dirty[frame_id]) {
			disk_manager->writePage(pages[frame_id].getHeader()->page_id, pages[frame_id].getRawData());
			is_dirty[frame_id] = false;
		}
		page_table.erase(pages[frame_id].getHeader()->page_id);
	} else {
		return nullptr;
	}

	// Bring the page from the disk to the chosen frame
	char *frame_ptr = const_cast<char *>(pages[frame_id].getRawData());
	disk_manager->readPage(page_id, frame_ptr);

	// Update the table and notify the replacer
	page_table[page_id] = frame_id;
	pin_count[frame_id] = 1;	// First user
	is_dirty[frame_id] = false; // It comes clean from the disc
	replacer->pin(frame_id);

	return &pages[frame_id];
}

bool BufferPoolManager::unpinPage(uint32_t page_id, bool is_dirty_flag) {

	// Is the page in RAM?
	if (page_table.find(page_id) == page_table.end()) {
		return false;
	}

	uint32_t frame_id = page_table[page_id];

	// If the user modified it, the frame is marked as dirty.
	if (is_dirty_flag) {
		is_dirty[frame_id] = true;
	}

	// If nobody is using it (pins_count is already 0), there's nothing to do.
	if (pin_count[frame_id] <= 0) {
		return false;
	}

	// Decrease the count.
	// If it reaches 0, the Replacer is notified that it CAN now be a victim.
	pin_count[frame_id]--;

	if (pin_count[frame_id] == 0) {
		replacer->unpin(frame_id);
	}

	return true;
}

Page *BufferPoolManager::newPage(uint32_t &page_id, ModelType object_type) {
	std::lock_guard<std::mutex> lock(latch);
	uint32_t frame_id;

	// A. Search for an available frame (Same as in fetchPage)
	if (!free_list.empty()) {
		frame_id = free_list.front();
		free_list.pop_front();
	} else if (replacer->victim(&frame_id)) {
		Page *victim_page = &pages[frame_id];

		// IF THE VICTIM WAS DIRTY, IT WAS RECORDED
		if (is_dirty[frame_id]) {
			disk_manager->writePage(victim_page->getHeader()->page_id, victim_page->getRawData());
			is_dirty[frame_id] = false;
		}
		page_table.erase(victim_page->getHeader()->page_id);
	} else {
		return nullptr; // There is no space
	}

	// B. Generate a new ID and "format" the page
	page_id = next_page_id++;

	if (object_type == ModelType::B_PLUS_TREE) {
		char *raw_data = const_cast<char *>(pages[frame_id].getRawData());
		std::memset(raw_data, 0, PAGE_SIZE);

		auto *header = reinterpret_cast<PageHeader *>(const_cast<char *>(pages[frame_id].getRawData()));
		header->page_id = page_id;
		header->object_type = static_cast<uint32_t>(object_type);
	} else {
		pages[frame_id].init(page_id, object_type);
	}

	pages[frame_id].init(page_id, object_type);

	// C. Update Manager metadata
	page_table[page_id] = frame_id;
	pin_count[frame_id] = 1; // It is marked as used immediately.
	is_dirty[frame_id] = false;

	return &pages[frame_id];
}

bool BufferPoolManager::flushPage(uint32_t page_id) {
	std::lock_guard<std::mutex> lock(latch);

	// If the page is not in RAM, there is nothing to "flash".
	if (page_table.find(page_id) == page_table.end())
		return false;

	uint32_t frame_id = page_table[page_id];

	// Disk Manager is used to write
	disk_manager->writePage(page_id, pages[frame_id].getRawData());

	// Important: It's no longer "dirty", RAM and Disk are now the same
	is_dirty[frame_id] = false;
	return true;
}

bool BufferPoolManager::deletePage(uint32_t page_id) {
	std::lock_guard<std::mutex> lock(latch);

	// Is it in RAM?
	if (page_table.find(page_id) != page_table.end()) {
		uint32_t frame_id = page_table[page_id];

		// Does anyone use it? If so, it can't be deleted.
		if (pin_count[frame_id] > 0) {
			return false;
		}

		// Clean metadata in RAM
		page_table.erase(page_id);
		replacer->pin(frame_id); // It is removed from the replacer because it no longer exists

		is_dirty[frame_id] = false;
		pin_count[frame_id] = 0;

		// The frame is available again for anyone
		free_list.push_back(frame_id);
	}

	// In a real database, you would call disk manager->allocate Page(page_id)
	return true;
}

BufferPoolManager::~BufferPoolManager() {

	for (size_t i = 0; i < pool_size; ++i) {
		if (is_dirty[i]) {
			uint32_t p_id = pages[i].getHeader()->page_id;
			disk_manager->writePage(p_id, pages[i].getRawData());
		}
	}

	delete[] pages;
	delete[] is_dirty;
	delete[] pin_count;
	delete replacer;
}

} // namespace LuminaDB