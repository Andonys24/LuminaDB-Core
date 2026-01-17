#include "luminadb/index/BPlusTree.hpp"
#include <iostream>

namespace LuminaDB {

BPlusTree::BPlusTree(uint32_t root_id, BufferPoolManager *bpm_param) : root_page_id(root_id), bpm(bpm_param) {

	// If there is no root, the first page is created (which will be a Sheet)
	if (root_page_id == 0) {
		uint32_t new_id;

		// Request a new page from the BufferPool with the type Index
		Page *page = bpm->newPage(new_id, ModelType::B_PLUS_TREE);

		if (page != nullptr) {
			root_page_id = new_id;

			// Use const_cast to initialize it as Sheet
			char *raw_data = const_cast<char *>(page->getRawData());
			BPlusTreeLeafPage leaf(raw_data);

			// Initialize: Leaf Type, no parent (0), and capacity (of: 250 keys)
			leaf.init(IndexPageType::LEAF_NODE, 0, 250);

			// Release marking as dirty so that the Disk Manager saves it
			bpm->unpinPage(root_page_id, true);

			// Debug
			std::cout << "B+ Tree Root created at Page: " << root_page_id << std::endl;
		}
	}
}

bool BPlusTree::getValue(uint32_t key, RecordID &result) {
	Page *page = findLeafPage(key);

	if (page == nullptr)
		return false;

	// Interpret the page as a sheet
	BPlusTreeLeafPage leaf(const_cast<char *>(page->getRawData()));

	int index = leaf.lookup(key);

	bool found = false;
	if (index < static_cast<int>(leaf.getSize()) && leaf.keyAt(index) == key) {
		result = leaf.valueAt(index);
		found = true;
	}

	// Release the page (it is not dirty because it was only read)
	bpm->unpinPage(page->getPageId(), false);

	return found;
}

void BPlusTree::insert(uint32_t key, const RecordID &value) {
	Page *page = findLeafPage(key);
	if (page == nullptr)
		return;

	BPlusTreeLeafPage leaf(const_cast<char *>(page->getRawData()));

	bool success = leaf.insert(key, value);

	if (!success) {
		// For now, if it fails it is because it is full or duplicated.
	}

	// Release and mark as dirty if the insert was successful
	bpm->unpinPage(page->getPageId(), true);
}

// Debug
Page *BPlusTree::findLeafPage(uint32_t) { return bpm->fetchPage(root_page_id); }

} // namespace LuminaDB
