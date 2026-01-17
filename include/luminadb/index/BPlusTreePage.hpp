#ifndef LUMINADB_BPLUSTREEPAGE_HPP
#define LUMINADB_BPLUSTREEPAGE_HPP

#include "luminadb/common/types.hpp"

namespace LuminaDB {
enum class IndexPageType { INTERNAL_NODE = 0, LEAF_NODE = 1 };

/**
 * Specific header for the pages that make up the B+ Tree.
 * It is located at the beginning of the 4096 bytes of the page.
 */
struct BPlusTreeHeader {
	IndexPageType page_type;
	uint32_t parent_page_id; // Parent page ID (0 if root)
	uint32_t current_size;	 // How many keys do you have today
	uint32_t max_size;		 // How many keys fit maximum
	uint32_t next_page_id;	 // For leaves only: pointer to right sibling
};

class BPlusTreePage {
  protected:
	char *data;

  public:
	BPlusTreePage(char *raw_data);

	// Easy header access using reinterpret_cast
	const BPlusTreeHeader *getHeader() const;
	BPlusTreeHeader *getHeader();

	// Basic utility methods
	bool isLeaf() const;

	uint32_t getSize() const;

	void setSize(uint32_t size);

	void init(IndexPageType type, uint32_t parent = 0, uint32_t max_keys = 0);
};

class BPlusTreeLeafPage : public BPlusTreePage {
  public:
	using BPlusTreePage::BPlusTreePage; // Use the base constructor

	// --- ACCESS METHODS ---

	// Returns the key at index 'index'
	uint32_t keyAt(int index) const;

	// Returns the RecordID at index 'index'
	RecordID valueAt(int index) const;

	// --- SEARCH METHODS ---

	// Find the first index where KeyAt(index) >= key (Binary Search)
	int lookup(uint32_t key) const;

	// --- DATA WRITE ---
	bool insert(uint32_t key, const RecordID &value);
};

class BPlusTreeInternalPage : public BPlusTreePage {
  public:
	using BPlusTreePage::BPlusTreePage;

	// Methods to manage keys and PageIDs (children)
	uint32_t keyAt(int index) const;
	void setKeyAt(int index, uint32_t key);

	uint32_t valueAt(int index) const; // "value" is a PageID
	void setValueAt(int index, uint32_t value);

	// Search which thread to go down based on the key
	uint32_t lookup(uint32_t key) const;
};

} // namespace LuminaDB

#endif