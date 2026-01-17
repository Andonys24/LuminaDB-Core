#include "luminadb/index/BPlusTreePage.hpp"
#include <cstring>

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

} // namespace LuminaDB