#include "luminadb/database/Database.hpp"
#include <iostream>

namespace LuminaDB {

Database::Database(const std::string &filename, uint32_t buffer_pool_size)
	: db_file(filename), next_data_page_id(1000) { // Start data pages at 1000 (B+ Tree uses 0-999)
	std::cout << "[Database] Initializing with file: " << filename << std::endl;

	// Step 1: Create DiskManager
	disk_manager = std::make_unique<DiskManager>(filename);

	// Step 2: Create BufferPoolManager
	buffer_pool_manager = std::make_unique<BufferPoolManager>(buffer_pool_size, disk_manager.get());

	// Step 3: Create B+ Tree index
	// Root ID = 0 means it will create a new root automatically
	index = std::make_unique<BPlusTree>(0, buffer_pool_manager.get());

	std::cout << "[Database] Initialized successfully" << std::endl;
	std::cout << "[Database] B+ Tree uses pages 0-999, Data uses pages 1000+" << std::endl;
}

Database::~Database() {
	std::cout << "[Database] Closing database..." << std::endl;
	// BufferPool destructor flushes all dirty pages
	buffer_pool_manager.reset();
	disk_manager.reset();
	std::cout << "[Database] Closed" << std::endl;
}

uint32_t Database::allocateDataPage() {
	uint32_t page_id = next_data_page_id++;
	std::cout << "[Database] Allocated data page: " << page_id << std::endl;
	return page_id;
}

RecordID Database::storeObject(const Storable &obj) {
	// Step 1: Get serialized size and allocate buffer
	size_t serialized_size = obj.getSerializedSize();
	std::vector<char> buffer(serialized_size);

	// Step 2: Serialize the object to buffer
	obj.serializeToBuffer(buffer.data());

	// Step 3: Allocate a new data page for this object
	uint32_t page_id = allocateDataPage();
	Page *page = buffer_pool_manager->newPage(page_id, obj.getType());

	if (page == nullptr) {
		throw std::runtime_error("Failed to allocate data page");
	}

	// Step 4: Insert serialized data using the slotted-page API
	if (!page->insertRecord(buffer.data(), static_cast<uint16_t>(serialized_size))) {
		buffer_pool_manager->unpinPage(page_id, false);
		throw std::runtime_error("Failed to insert record into page (size exceeds capacity)");
	}

	// Step 5: Build RecordID (slot_num stores the slot index)
	RecordID result{};
	result.page_id = page_id;
	result.slot_num = static_cast<uint16_t>(page->getHeader()->slot_count - 1);

	buffer_pool_manager->unpinPage(page_id, true);

	return result;
}

std::vector<char> Database::retrieveObjectBuffer(const RecordID &record_id) {
	// Step 1: Fetch the data page
	Page *page = buffer_pool_manager->fetchPage(record_id.page_id);

	if (page == nullptr) {
		throw std::runtime_error("Data page not found: " + std::to_string(record_id.page_id));
	}

	// Step 2: Read serialized data from the stored slot
	uint16_t data_size = 0;
	const char *page_data = page->getRecord(record_id.slot_num, data_size);

	if (page_data == nullptr) {
		buffer_pool_manager->unpinPage(record_id.page_id, false);
		throw std::runtime_error("Record slot not found in page: " + std::to_string(record_id.page_id));
	}

	// Step 3: Copy data to result buffer
	std::vector<char> result(page_data, page_data + data_size);

	buffer_pool_manager->unpinPage(record_id.page_id, false);

	return result;
}

} // namespace LuminaDB
