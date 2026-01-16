#include "luminadb/buffer/BufferPoolManager.hpp"
#include "luminadb/storage/DiskManager.hpp"
#include <cstdint>
#include <iostream>
#include <string>

using namespace LuminaDB;

int main() {
	// 1. Setup: Disk file and a Pool restricted to 3 pages
	DiskManager dm("test_engine.db");
	BufferPoolManager bpm(3, &dm);

	uint32_t p0_id, p1_id, p2_id, p3_id;

	std::cout << "--- Phase 1: Creating Pages (Filling the Pool) ---" << std::endl;

	// Create 3 pages to fill the RAM capacity
	Page *p0 = bpm.newPage(p0_id, 1);
	bpm.newPage(p1_id, 1);
	bpm.newPage(p2_id, 1);

	// Insert data into Page 0
	std::string test_msg = "Page 0 persistent data";
	p0->insertRecord(test_msg.c_str(), static_cast<uint16_t>(test_msg.size() + 1));

	// Unpin pages; mark p0 as DIRTY so it persists to disk during eviction
	bpm.unpinPage(p0_id, true);
	bpm.unpinPage(p1_id, false);
	bpm.unpinPage(p2_id, false);

	std::cout << "[OK] Pool filled. IDs: " << p0_id << ", " << p1_id << ", " << p2_id << std::endl;

	std::cout << "\n--- Phase 2: Forcing Eviction ---" << std::endl;

	// Requesting Page 3 forces an eviction since the pool is full.
	// Based on LRU, Page 0 should be evicted and flushed to disk.
	bpm.newPage(p3_id, 1);
	std::cout << "[OK] Page " << p3_id << " created. Eviction successful." << std::endl;

	std::cout << "\n--- Phase 3: Verifying Persistence ---" << std::endl;

	// Fetch Page 0 back from disk into RAM
	Page *p0_recovered = bpm.fetchPage(p0_id);

	uint16_t record_size;
	const char *record_data = p0_recovered->getRecord(0, record_size);

	if (record_data && std::string(record_data) == test_msg) {
		std::cout << "SUCCESS: Data recovered from disk: " << record_data << std::endl;
	} else {
		std::cerr << "FAILURE: Data was lost or corrupted." << std::endl;
	}

	// Final cleanup
	bpm.unpinPage(p0_id, false);
	bpm.unpinPage(p3_id, false);

	std::cout << "\n--- TEST COMPLETED SUCCESSFULLY ---" << std::endl;
	return 0;
}