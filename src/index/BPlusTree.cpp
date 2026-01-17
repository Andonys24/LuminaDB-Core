#include "luminadb/index/BPlusTree.hpp"
#include <iostream>

namespace LuminaDB {

BPlusTree::BPlusTree(uint32_t root_id, BufferPoolManager *bpm_param) : root_page_id(root_id), bpm(bpm_param) {

	// If root_id is 0, try to load existing root from disk page 0, or create new one
	if (root_page_id == 0) {
		// First try to fetch page 0 from disk (if it exists and has data)
		Page *existing_page = bpm->fetchPage(0);

		bool valid_root = false;
		if (existing_page != nullptr) {
			BPlusTreePage base(const_cast<char *>(existing_page->getRawData()));
			const BPlusTreeHeader *hdr = base.getHeader();

			// Validate header to ensure it's a real B+Tree page
			if (hdr->page_id == 0 &&
				(hdr->page_type == IndexPageType::LEAF_NODE || hdr->page_type == IndexPageType::INTERNAL_NODE) &&
				hdr->max_size > 0 && hdr->max_size <= 2000 && hdr->current_size <= hdr->max_size) {
				valid_root = true;
			}
		}

		if (valid_root) {
			root_page_id = 0;
			std::cout << "B+ Tree Root loaded from existing Page: 0" << std::endl;
			bpm->unpinPage(0, false);
		} else {
			if (existing_page != nullptr) {
				bpm->unpinPage(0, false);
			}
			// Page 0 invalid or missing - create new root
			uint32_t new_id;
			Page *page = bpm->newPage(new_id, ModelType::B_PLUS_TREE);

			if (page != nullptr) {
				root_page_id = new_id;

				char *raw_data = const_cast<char *>(page->getRawData());
				BPlusTreeLeafPage leaf(raw_data);
				leaf.init(IndexPageType::LEAF_NODE, 0, 250);
				leaf.getHeader()->page_id = root_page_id;

				bpm->unpinPage(root_page_id, true);
				std::cout << "B+ Tree Root created at Page: " << root_page_id << std::endl;
			}
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
		// Leaf is full, need to split
		// Get the page_id from the B+Tree header (not from Page object)
		uint32_t leaf_id = leaf.getHeader()->page_id;

		// Perform the split
		SplitResult split_result = leaf.split(key, value, bpm);

		// Mark leaf as dirty before unpinning (split modified it)
		bpm->unpinPage(leaf_id, true);

		// Propagate the split to the parent
		insertIntoParent(leaf_id, split_result.middle_key, split_result.new_page_id);
	} else {
		// Insert was successful, just mark as dirty and release
		// Get page_id from B+Tree header
		uint32_t leaf_id = leaf.getHeader()->page_id;
		bpm->unpinPage(leaf_id, true);
	}
}

// --- PROPAGATION METHODS ---

void BPlusTree::insertIntoParent(uint32_t left_child_id, uint32_t key, uint32_t right_child_id) {
	std::cout << "\n[insertIntoParent] Inserting key=" << key << " with right_child=" << right_child_id
			  << " from left_child=" << left_child_id << std::endl;

	// STEP 1: Fetch the left child to get its parent ID
	Page *left_child_page = bpm->fetchPage(left_child_id);
	BPlusTreePage left_child_base(const_cast<char *>(left_child_page->getRawData()));
	uint32_t parent_id = left_child_base.getHeader()->parent_page_id;

	bpm->unpinPage(left_child_id, false);

	// STEP 2: Special case - if left_child is the root, create a new root
	if (left_child_id == root_page_id) {
		std::cout << "[insertIntoParent] Left child is root, creating new root" << std::endl;
		createNewRoot(left_child_id, key, right_child_id);
		return;
	}

	// STEP 3: Fetch the parent page
	Page *parent_page = bpm->fetchPage(parent_id);
	BPlusTreeInternalPage parent(const_cast<char *>(parent_page->getRawData()));

	// STEP 4: Try to insert the key into the parent
	bool insert_success = parent.insertAfter(key, right_child_id);

	if (insert_success) {
		// Parent had space, just mark it as dirty and we're done
		std::cout << "[insertIntoParent] Key inserted into parent successfully" << std::endl;
		bpm->unpinPage(parent_id, true);
	} else {
		// Parent is full, need to split it recursively
		std::cout << "[insertIntoParent] Parent is full, need to split it recursively" << std::endl;

		// For now, we'll handle this in a future step (Phase 5.3.2)
		// TODO: Implement internal node split and recursive propagation
		std::cout << "[TODO] Internal node split not yet implemented" << std::endl;
		bpm->unpinPage(parent_id, false);
	}
}

void BPlusTree::createNewRoot(uint32_t left_child_id, uint32_t key, uint32_t right_child_id) {
	std::cout << "\n[createNewRoot] Creating new root with key=" << key << std::endl;

	// STEP 1: Create a new page for the new root (internal node)
	uint32_t new_root_id;
	Page *new_root_page = bpm->newPage(new_root_id, ModelType::B_PLUS_TREE);

	BPlusTreeInternalPage new_root(const_cast<char *>(new_root_page->getRawData()));
	new_root.init(IndexPageType::INTERNAL_NODE, 0, 250); // parent_id=0 (it's the root)

	// Store the page_id in the header
	new_root.getHeader()->page_id = new_root_id;

	// STEP 2: Set up the internal structure
	// The new root has 2 children and 1 key:
	// [left_child_id] key [right_child_id]

	new_root.setValueAt(0, left_child_id);	// Left child
	new_root.setKeyAt(0, key);				// Separator key
	new_root.setValueAt(1, right_child_id); // Right child
	new_root.setSize(1);					// 1 key = 2 children

	bpm->unpinPage(new_root_id, true);

	// STEP 3: Update the parent pointers of both children
	Page *left_child_page = bpm->fetchPage(left_child_id);
	BPlusTreePage left_child_base(const_cast<char *>(left_child_page->getRawData()));
	left_child_base.getHeader()->parent_page_id = new_root_id;
	bpm->unpinPage(left_child_id, true);

	Page *right_child_page = bpm->fetchPage(right_child_id);
	BPlusTreePage right_child_base(const_cast<char *>(right_child_page->getRawData()));
	right_child_base.getHeader()->parent_page_id = new_root_id;
	bpm->unpinPage(right_child_id, true);

	// STEP 4: Update the tree's root_page_id
	root_page_id = new_root_id;

	std::cout << "[createNewRoot] New root created at page " << new_root_id << std::endl;
}

// Debug
Page *BPlusTree::findLeafPage(uint32_t key) {
	uint32_t page_id = root_page_id;
	std::cout << "\n[findLeafPage] Starting from root=" << page_id << " searching for key=" << key << std::endl;

	while (true) {
		Page *page = bpm->fetchPage(page_id);
		BPlusTreePage base(const_cast<char *>(page->getRawData()));

		std::cout << "[findLeafPage] At page " << page_id << ", type=" << (base.isLeaf() ? "LEAF" : "INTERNAL")
				  << std::endl;

		if (base.isLeaf()) {
			return page;
		}

		BPlusTreeInternalPage internal(const_cast<char *>(page->getRawData()));
		uint32_t next_page = internal.lookup(key);

		bpm->unpinPage(page_id, false);
		page_id = next_page;
	}
}

} // namespace LuminaDB
