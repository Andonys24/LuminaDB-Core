#include "luminadb/buffer/BufferPoolManager.hpp"
#include "luminadb/index/BPlusTree.hpp"
#include "luminadb/storage/DiskManager.hpp"
#include <iostream>

using namespace LuminaDB;

int main() {
	DiskManager dm("lumina_index.db");
	// Small pool to ensure no strange behavior
	BufferPoolManager bpm(10, &dm);

	std::cout << "--- Phase 5: B+ Tree Basic Integration Test ---" << std::endl;

	// Initialize the tree.
	// root_id = 0 indicates that the tree should create its first page.
	BPlusTree tree(0, &bpm);

	// Insert test data
	// We insert out of order to test that the page sorts them internally
	std::cout << "[Step 1] Inserting keys: 30, 10, 20..." << std::endl;

	// RecordID(PageID, SlotID) -> We simulate addresses from Phase 2
	tree.insert(30, {1, 5});
	tree.insert(10, {1, 2});
	tree.insert(20, {1, 4});

	// Try to recover the data
	std::cout << "[Step 2] Retrieving keys..." << std::endl;

	uint32_t keys_to_test[] = {10, 20, 30, 40}; // 40 does not exist
	for (uint32_t k : keys_to_test) {
		RecordID result;
		if (tree.getValue(k, result)) {
			std::cout << "[FOUND] Key " << k << " points to RecordID(" << result.page_id << ", " << result.slot_num
					  << ")" << std::endl;
		} else {
			std::cout << "[NOT FOUND] Key " << k << " does not exist in the tree." << std::endl;
		}
	}

	bpm.flushPage(0);

	std::cout << "\n--- TEST COMPLETED ---" << std::endl;
	return 0;
}