#ifndef LUMINADB_BPLUSTREE_HPP
#define LUMINADB_BPLUSTREE_HPP

#include "BPlusTreePage.hpp"
#include "luminadb/buffer/BufferPoolManager.hpp"
#include "luminadb/common/types.hpp"

namespace LuminaDB {
class BPlusTree {
  private:
	// Attributes
	uint32_t root_page_id;
	BufferPoolManager *bpm;

	// --- AUXILIARY METHODS ---

	// Find the leaf page that should contain the key 'key'
	Page *findLeafPage(uint32_t key);

	// Propagate a split from child to parent (recursive insertion into ancestors)
	void insertIntoParent(uint32_t left_child_id, uint32_t key, uint32_t right_child_id);

	// Create a new root when the current root splits
	void createNewRoot(uint32_t left_child_id, uint32_t key, uint32_t right_child_id);

	// Create a new blank page for the tree
	// Page *createNewNode(IndexPageType type);

  public:
	BPlusTree(uint32_t root_id, BufferPoolManager *bpm);

	// Main function to search for data
	bool getValue(uint32_t key, RecordID &result);

	// Main function to insert
	void insert(uint32_t key, const RecordID &value);
};

} // namespace LuminaDB

#endif