#include "luminadb/buffer/BufferPoolManager.hpp"
#include "luminadb/common/types.hpp"
#include "luminadb/index/BPlusTree.hpp"
#include "luminadb/storage/DiskManager.hpp"
#include <iostream>

using namespace LuminaDB;

int main() {
	std::cout << "\n========== B+ TREE SPLIT TEST ==========\n";

	DiskManager dm("lumina_split_test.db");
	BufferPoolManager bpm(10, &dm);

	// Create a new tree (root will be auto-created as empty leaf)
	BPlusTree tree(0, &bpm);

	std::cout << "\n--- Phase 1: Insert keys to trigger leaf split ---\n";

	// Insert 260 keys (max_size=250, so we'll force splits)
	// We insert: 10, 20, 30, ... 2600
	int num_inserts = 260;
	for (int i = 1; i <= num_inserts; i++) {
		uint32_t key = i * 10;
		RecordID value = {1, static_cast<uint16_t>(i)};

		tree.insert(key, value);

		// Print every 50 inserts to see progress
		if (i % 50 == 0) {
			std::cout << "Inserted " << i << " keys (up to key=" << key << ")\n";
		}
	}

	std::cout << "\nSuccessfully inserted " << num_inserts << " keys\n";

	std::cout << "\n--- Phase 2: Search for keys to verify tree structure ---\n";

	// Test search for various keys
	int tests_passed = 0;
	int tests_failed = 0;

	// Test keys that should exist
	int test_keys[] = {10, 100, 500, 1000, 1500, 2000, 2500, 2600};

	for (int test_key : test_keys) {
		RecordID result;
		if (tree.getValue(test_key, result)) {
			int expected_slot = test_key / 10;
			std::cout << "Found key=" << test_key << " at slot=" << result.slot_num << " (expected around slot "
					  << expected_slot << ")\n";
			tests_passed++;
		} else {
			std::cout << "FAILED: Could not find key=" << test_key << "\n";
			tests_failed++;
		}
	}

	// Test keys that should NOT exist
	int non_existent_keys[] = {5, 15, 505, 1005, 2605};

	for (int test_key : non_existent_keys) {
		RecordID result;
		if (tree.getValue(test_key, result)) {
			std::cout << "FAILED: Found non-existent key=" << test_key << "\n";
			tests_failed++;
		} else {
			std::cout << "Correctly did not find non-existent key=" << test_key << "\n";
			tests_passed++;
		}
	}

	std::cout << "\n========== TEST SUMMARY ==========\n";
	std::cout << "Tests passed: " << tests_passed << "\n";
	std::cout << "Tests failed: " << tests_failed << "\n";

	if (tests_failed == 0) {
		std::cout << "\nALL TESTS PASSED!\n";
	} else {
		std::cout << "\nSOME TESTS FAILED\n";
	}

	return 0;
}