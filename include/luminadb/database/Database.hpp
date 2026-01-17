#ifndef LUMINADB_DATABASE_HPP
#define LUMINADB_DATABASE_HPP

#include "luminadb/buffer/BufferPoolManager.hpp"
#include "luminadb/index/BPlusTree.hpp"
#include "luminadb/model/ModelFactory.hpp"
#include "luminadb/storage/DiskManager.hpp"
#include <memory>
#include <stdexcept>
#include <string>

namespace LuminaDB {

/**
 * High-level database abstraction.
 * Provides CRUD operations on typed objects using B+ Tree indexing.
 *
 * Usage:
 *   Database db("mydb.db");
 *   User user(1, "Alice", 25);
 *   db.insert<User>(1, user);
 *   User found = db.find<User>(1);
 */
class Database {
  private:
	std::unique_ptr<DiskManager> disk_manager;
	std::unique_ptr<BufferPoolManager> buffer_pool_manager;
	std::unique_ptr<BPlusTree> index;
	std::string db_file;
	uint32_t next_data_page_id; // For allocating new data pages

	// Helper: Convert object to RecordID (find where to store it)
	RecordID storeObject(const Storable &obj);

	// Helper: Retrieve object from page using RecordID
	std::vector<char> retrieveObjectBuffer(const RecordID &record_id);

	// Helper: Allocate a new data page
	uint32_t allocateDataPage();

  public:
	// Constructor: Opens or creates database
	explicit Database(const std::string &filename, uint32_t buffer_pool_size = 10);

	// Destructor: Flushes all pages to disk
	~Database();

	/**
	 * Insert a typed object with a key.
	 * Returns true if inserted, false if key already exists.
	 */
	template <typename T> bool insert(uint32_t key, const T &obj) {
		static_assert(std::is_base_of<Storable, T>::value, "T must inherit from Storable");

		try {
			// Step 1: Store the object in a page
			RecordID record_id = storeObject(obj);

			// Step 2: Insert into B+ Tree index
			index->insert(key, record_id);

			return true;
		} catch (const std::exception &) {
			// If insert fails, the object is still on disk but not indexed
			// This is a transaction consistency issue (would need WAL to fix)
			return false;
		}
	}

	/**
	 * Find a typed object by key.
	 * Returns the object if found, throws exception if not found.
	 */
	template <typename T> T find(uint32_t key) {
		static_assert(std::is_base_of<Storable, T>::value, "T must inherit from Storable");

		// Step 1: Search B+ Tree for the key
		RecordID record_id;
		if (!index->getValue(key, record_id)) {
			throw std::runtime_error("Key not found: " + std::to_string(key));
		}

		// Step 2: Retrieve raw bytes from page
		std::vector<char> buffer = retrieveObjectBuffer(record_id);

		// Step 3: Deserialize using ModelFactory
		return ModelFactory::deserialize<T>(buffer.data());
	}

	/**
	 * Check if key exists.
	 */
	bool exists(uint32_t key) {
		RecordID dummy;
		return index->getValue(key, dummy);
	}

	/**
	 * Remove an object by key.
	 * Note: This doesn't reclaim space (no vacuum in this version).
	 */
	bool remove([[maybe_unused]] uint32_t key) {
		// TODO: Implement when B+ Tree delete is available
		// For now, just return false
		return false;
	}

	/**
	 * Get database filename.
	 */
	std::string getFilename() const { return db_file; }
};

} // namespace LuminaDB

#endif
