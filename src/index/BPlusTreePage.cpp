#include "luminadb/index/BPlusTreePage.hpp"
#include "luminadb/buffer/BufferPoolManager.hpp"
#include <cstring>
#include <iostream>
#include <vector>

namespace LuminaDB {

// --- BPlusTreePage ---

BPlusTreePage::BPlusTreePage(char *raw_data) : data(raw_data) {}

const BPlusTreeHeader *BPlusTreePage::getHeader() const { return reinterpret_cast<const BPlusTreeHeader *>(data); }

BPlusTreeHeader *BPlusTreePage::getHeader() { return reinterpret_cast<BPlusTreeHeader *>(data); }

bool BPlusTreePage::isLeaf() const {
	return reinterpret_cast<const BPlusTreeHeader *>(data)->page_type == IndexPageType::LEAF_NODE;
}

uint32_t BPlusTreePage::getSize() const { return reinterpret_cast<const BPlusTreeHeader *>(data)->current_size; }

void BPlusTreePage::setSize(uint32_t size) { getHeader()->current_size = size; }

void BPlusTreePage::init(IndexPageType type, uint32_t parent, uint32_t max_keys) {
	BPlusTreeHeader *header = getHeader();
	header->page_type = type;
	header->parent_page_id = parent;
	header->current_size = 0;
	header->max_size = max_keys;
	header->next_page_id = 0;
}

// --- BPlusTreeLeafPage ---

// --- ACCESS METHODS ---

// Returns the key at index 'index'
uint32_t BPlusTreeLeafPage::keyAt(int index) const {
	// Offset: Header + (index * key size)
	const char *offset = data + sizeof(BPlusTreeHeader) + (index * sizeof(uint32_t));
	return *reinterpret_cast<const uint32_t *>(offset);
}

// Returns the RecordID at index 'index'
RecordID BPlusTreeLeafPage::valueAt(int index) const {
	// The value array starts after the ENTIRE brace array
	uint32_t header_size = sizeof(BPlusTreeHeader);
	uint32_t keys_array_size = getHeader()->max_size * sizeof(uint32_t);

	const char *offset = data + header_size + keys_array_size + (index * sizeof(RecordID));
	return *reinterpret_cast<const RecordID *>(offset);
}

// --- SEARCH METHODS ---

// Find the first index where KeyAt(index) >= key (Binary Search)
int BPlusTreeLeafPage::lookup(uint32_t key) const {
	int low = 0, high = getSize() - 1;
	int index = getSize(); // By default, at the end

	while (low <= high) {
		int mid = low + (high - low) / 2;
		if (keyAt(mid) >= key) {
			index = mid;
			high = mid - 1;
		} else {
			low = mid + 1;
		}
	}
	return index;
}

// --- DATA WRITE ---
bool BPlusTreeLeafPage::insert(uint32_t key, const RecordID &value) {
	uint32_t size = getSize();

	// Find the position where it should go (Binary Search)
	int index = lookup(key);

	// 2. If the key already exists, it is not inserted
	if (index < (int)size && keyAt(index) == key) {
		return false;
	}

	// Check if there is space
	if (size >= getHeader()->max_size) {
		return false;
	}

	// --- MOTION ARITHMETIC ---

	// A. Move the KEYS to the right
	// Origin: The start of the key array + (index * 4 bytes)
	char *key_ptr = data + sizeof(BPlusTreeHeader) + (index * sizeof(uint32_t));
	// Destination: One key position ahead
	char *key_dest = key_ptr + sizeof(uint32_t);
	// Amount: All keys to the right of the index
	size_t keys_to_move = (size - index) * sizeof(uint32_t);

	if (size > (uint32_t)index) {
		std::memmove(key_dest, key_ptr, keys_to_move);
	}

	// B. Move the VALUES (RecordIDs) to the right
	// The array of values ​​starts after the ENTIRE keyspace
	uint32_t header_size = sizeof(BPlusTreeHeader);
	uint32_t keys_area_size = getHeader()->max_size * sizeof(uint32_t);

	char *val_ptr = data + header_size + keys_area_size + (index * sizeof(RecordID));
	char *val_dest = val_ptr + sizeof(RecordID);
	size_t vals_to_move = (size - index) * sizeof(RecordID);

	if (size > (uint32_t)index) {
		std::memmove(val_dest, val_ptr, vals_to_move);
	}

	// --- DATA WRITE ---

	// Insert the new key
	*reinterpret_cast<uint32_t *>(key_ptr) = key;
	*reinterpret_cast<RecordID *>(val_ptr) = value;

	setSize(size + 1);

	return true;
}

// --- UTILITY ---

uint32_t BPlusTreeLeafPage::getNextPageId() const { return getHeader()->next_page_id; }

void BPlusTreeLeafPage::setNextPageId(uint32_t next_id) { getHeader()->next_page_id = next_id; }

// --- SPLIT OPERATION ---

SplitResult BPlusTreeLeafPage::split(uint32_t key, const RecordID &value, BufferPoolManager *bpm) {
	uint32_t old_size = getSize();
	uint32_t total = old_size + 1; // Total entries after inserting new key

	// STEP 1: Create temporary vectors to hold ALL entries (existing + new)
	std::vector<uint32_t> temp_keys(total);
	std::vector<RecordID> temp_vals(total);

	// STEP 2: Merge existing entries + new entry into temp arrays (sorted order)
	uint32_t insert_pos = 0;
	bool inserted = false;

	for (uint32_t i = 0; i < old_size; i++) {
		// If we haven't inserted yet and current key is greater, insert here
		if (!inserted && key < keyAt(i)) {
			temp_keys[insert_pos] = key;
			temp_vals[insert_pos] = value;
			insert_pos++;
			inserted = true;
		}
		// Copy existing entry
		temp_keys[insert_pos] = keyAt(i);
		temp_vals[insert_pos] = valueAt(i);
		insert_pos++;
	}

	// If key is larger than all existing keys, insert at end
	if (!inserted) {
		temp_keys[insert_pos] = key;
		temp_vals[insert_pos] = value;
	}

	// STEP 3: Calculate split point (middle)
	uint32_t mid = total / 2; // For total=5: mid=2 → [0,1] | [2,3,4]

	// STEP 4: Create new sibling page
	uint32_t new_page_id;
	Page *new_page = bpm->newPage(new_page_id, ModelType::B_PLUS_TREE);
	BPlusTreeLeafPage sibling(const_cast<char *>(new_page->getRawData()));

	// Initialize sibling with same parent and max_size
	sibling.init(IndexPageType::LEAF_NODE, getHeader()->parent_page_id, getHeader()->max_size);

	// Store the page_id in the header
	sibling.getHeader()->page_id = new_page_id;

	// STEP 5: Redistribute entries
	// Original page keeps [0...mid-1]
	// Sibling gets [mid...total-1]

	// Clear current page and re-populate with first half
	setSize(0);
	for (uint32_t i = 0; i < mid; i++) {
		insert(temp_keys[i], temp_vals[i]);
	}

	// Populate sibling with second half
	for (uint32_t i = mid; i < total; i++) {
		sibling.insert(temp_keys[i], temp_vals[i]);
	}

	// STEP 6: Update linked list pointers (maintain leaf chain)
	// Before: [this] -> next_old
	// After:  [this] -> [sibling] -> next_old
	sibling.setNextPageId(this->getNextPageId());
	this->setNextPageId(new_page_id);

	// STEP 7: Mark new page as dirty and unpin
	bpm->unpinPage(new_page_id, true);

	// STEP 8: Return the first key of sibling (to promote to parent)
	uint32_t middle_key = sibling.keyAt(0);

	std::cout << "[SPLIT] Leaf split complete. Original has " << getSize() << " keys, Sibling (page " << new_page_id
			  << ") has " << sibling.getSize() << " keys. Promoting key=" << middle_key << std::endl;

	return {middle_key, new_page_id};
}

// --- BPlusTreeInternalPage ---

uint32_t BPlusTreeInternalPage::keyAt(int index) const {
	const char *offset = data + sizeof(BPlusTreeHeader) + (index * sizeof(uint32_t));
	return *reinterpret_cast<const uint32_t *>(offset);
}

void BPlusTreeInternalPage::setKeyAt(int index, uint32_t key) {
	char *offset = data + sizeof(BPlusTreeHeader) + (index * sizeof(uint32_t));
	*reinterpret_cast<uint32_t *>(offset) = key;
}

uint32_t BPlusTreeInternalPage::valueAt(int index) const {
	uint32_t header_size = sizeof(BPlusTreeHeader);
	// Child array starts after maximum brace space
	uint32_t keys_area_size = getHeader()->max_size * sizeof(uint32_t);
	const char *offset = data + header_size + keys_area_size + (index * sizeof(uint32_t));
	return *reinterpret_cast<const uint32_t *>(offset);
}

void BPlusTreeInternalPage::setValueAt(int index, uint32_t value) {
	uint32_t header_size = sizeof(BPlusTreeHeader);
	uint32_t keys_area_size = getHeader()->max_size * sizeof(uint32_t);
	char *offset = data + header_size + keys_area_size + (index * sizeof(uint32_t));
	*reinterpret_cast<uint32_t *>(offset) = value;
}

uint32_t BPlusTreeInternalPage::lookup(uint32_t key) const {
	uint32_t size = getSize();

	std::cout << "[Internal] Looking for key=" << key << " in node with size=" << size << std::endl;

	for (uint32_t i = 0; i < size; ++i) {
		uint32_t current_key = keyAt(i);
		std::cout << "  Checking keyAt(" << i << ")=" << current_key;

		if (key < current_key) {
			uint32_t child = valueAt(i);
			std::cout << " -> Going to child valueAt(" << i << ")=" << child << std::endl;
			return child;
		}
		std::cout << " (skip)" << std::endl;
	}

	uint32_t rightmost = valueAt(size);
	std::cout << "  -> Going to rightmost child valueAt(" << size << ")=" << rightmost << std::endl;
	return rightmost;
}

// --- INSERT INTO INTERNAL NODE ---

bool BPlusTreeInternalPage::insertAfter(uint32_t key, uint32_t right_child) {
	uint32_t size = getSize();

	// STEP 1: Check if there is space
	// Internal nodes store: [child_0] key_0 [child_1] key_1 ... [child_n]
	// So we need space for 1 more key and 1 more child pointer
	if (size >= getHeader()->max_size) {
		return false; // No space, need split (handled elsewhere)
	}

	// STEP 2: Find the correct position to insert (maintain key order)
	// We search for the position where the new key should go
	int insert_idx = 0;
	while (insert_idx < (int)size && keyAt(insert_idx) < key) {
		insert_idx++;
	}

	// STEP 3: Move all keys from insert_idx onwards to the right
	// We need to make space for the new key

	// Calculate source and destination for key movement
	uint32_t header_size = sizeof(BPlusTreeHeader);
	uint32_t keys_area_size = getHeader()->max_size * sizeof(uint32_t);

	// Starting position of the key to move (at insert_idx)
	char *key_src = data + header_size + (insert_idx * sizeof(uint32_t));
	// Where it should go (one position to the right)
	char *key_dest = key_src + sizeof(uint32_t);
	// How many bytes to move (all keys from insert_idx to end)
	size_t keys_to_move = (size - insert_idx) * sizeof(uint32_t);

	// Move keys to the right
	if (size > (uint32_t)insert_idx) {
		std::memmove(key_dest, key_src, keys_to_move);
	}

	// STEP 4: Move all child pointers from insert_idx+1 onwards to the right
	// Child pointers are stored after the entire key space

	// Starting position of the child to move (at insert_idx + 1)
	char *child_src = data + header_size + keys_area_size + ((insert_idx + 1) * sizeof(uint32_t));
	// Where it should go (one position to the right)
	char *child_dest = child_src + sizeof(uint32_t);
	// How many bytes to move (all children from insert_idx+1 to end)
	size_t children_to_move = (size - insert_idx) * sizeof(uint32_t);

	// Move children pointers to the right
	if (size > (uint32_t)insert_idx) {
		std::memmove(child_dest, child_src, children_to_move);
	}

	// STEP 5: Insert the new key and right child
	// The key goes at insert_idx
	setKeyAt(insert_idx, key);
	// The right_child becomes the child at insert_idx + 1
	setValueAt(insert_idx + 1, right_child);

	// STEP 6: Update size
	setSize(size + 1);

	std::cout << "[INSERT_AFTER] Inserted key=" << key << " with right_child=" << right_child
			  << " at index=" << insert_idx << ". Node now has " << getSize() << " keys." << std::endl;

	return true;
}

}