#ifndef LUMINADB_TYPES_HPP
#define LUMINADB_TYPES_HPP

#include <cstdint>

namespace LuminaDB {

/**
 * Structure of a Slot within the page.
 * Indicates where a record begins and its length.
 */
struct Slot {
	uint16_t offset; // Position relative to the beginning of the page
	uint16_t size;	 // Size of serialized object
};

struct RecordID {
	uint32_t page_id;  // Data Page ID (Phase 2)
	uint16_t slot_num; // Index of the slot within that page

	// A null value to initialize
	static RecordID Invalid() { return {0xFFFFFFFF, 0xFFFF}; }
};

} // namespace LuminaDB

#endif