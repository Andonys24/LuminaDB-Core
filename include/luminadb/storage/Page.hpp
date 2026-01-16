#ifndef LUMINADB_PAGE_HPP
#define LUMINADB_PAGE_HPP

#include <cstdint>
#include <cstring>

namespace LuminaDB {
// Industry standard page size (4KB)
inline constexpr size_t PAGE_SIZE = 4096;

/**
 * Structure of a Slot within the page.
 * Indicates where a record begins and its length.
 */
struct Slot {
	uint16_t offset; // Position relative to the beginning of the page
	uint16_t size;	 // Size of serialized object
};

/**
 * Page header (Metadata)
 */
struct PageHeader {
	uint32_t page_id; // Unique page ID
	uint32_t object_type;
	uint16_t slot_count; // How many objects are there currently
	uint16_t free_ptr;	 // Pointer to the beginning of the free space (from the end)
};

/**
 * Page Class: A PAGE_SIZE-byte memory block with a slotted structure
 */
class Page {
  private:
	char data[PAGE_SIZE];

  public:
	//   Constructor
	Page();
	void init(uint32_t id, uint32_t type);

	PageHeader *getHeader();

	// Returns how much space is left between the slots directory and the data
	uint16_t getFreeSpace();

	/**
	 * Inserts a serialized object into the page.
	 * Returns true if successful, false if there is no space.
	 */
	bool insertRecord(const char *record_data, uint16_t record_size);

	// Get the bytes of a specific object by its slot index
	const char *getRecord(uint16_t slot_idx, uint16_t &out_size);
	const char *getRawData() const;
};

} // namespace LuminaDB

#endif