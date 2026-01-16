#include "luminadb/storage/DiskManager.hpp"
#include "luminadb/storage/Page.hpp"

using namespace LuminaDB;

DiskManager::DiskManager(const std::string &db_file) : file_name(db_file) {
	// Start for reading, writing, and binary.
	// If it doesn't exist, create it.
	db_io.open(db_file, std::ios::binary | std::ios::in | std::ios::out);

	if (!db_io.is_open()) {
		// If it failed to open (perhaps the file does not exist), it is created from scratch.
		db_io.open(db_file, std::ios::binary | std::ios::trunc | std::ios::out);
		db_io.close();
		// Reopen in read/write mode
		db_io.open(db_file, std::ios::binary | std::ios::in | std::ios::out);
	}
}

void DiskManager::writePage(uint32_t page_id, const char *page_data) {
	size_t offset = page_id * PAGE_SIZE;
	db_io.seekp(offset);
	db_io.write(page_data, PAGE_SIZE);
	db_io.flush(); // Ensures that the bytes reach the physical disk
}

void DiskManager::readPage(uint32_t page_id, char *buffer) {
	size_t offset = page_id * PAGE_SIZE;
	db_io.seekg(offset);
	db_io.read(buffer, PAGE_SIZE);
}

DiskManager::~DiskManager() {
	if (db_io.is_open()) {
		db_io.close();
	}
}