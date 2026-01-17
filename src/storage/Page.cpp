#include "luminadb/storage/Page.hpp"
#include "luminadb/common/types.hpp"
#include <cstring>

namespace LuminaDB {

Page::Page() {}

void Page::init(uint32_t id, uint32_t type) {
	std::memset(data, 0, PAGE_SIZE);
	auto *header = getHeader();
	header->page_id = id;
	header->object_type = type;
	header->slot_count = 0;
	// The data space starts at the bottom of the page
	header->free_ptr = PAGE_SIZE;
}

PageHeader *Page::getHeader() { return reinterpret_cast<PageHeader *>(data); }

uint16_t Page::getFreeSpace() {
	auto *header = getHeader();
	uint16_t slots_end = sizeof(PageHeader) + (header->slot_count * sizeof(Slot));
	return header->free_ptr - slots_end;
}

bool Page::insertRecord(const char *record_data, uint16_t record_size) {
	if (record_size == 0)
		return false;

	if (getFreeSpace() < record_size + sizeof(Slot))
		return false;

	auto *header = getHeader();

	// Move the free space pointer backward
	header->free_ptr -= record_size;

	// Copy the object data to the calculated position
	std::memcpy(data + header->free_ptr, record_data, record_size);

	// Create the new slot after the last existing slot
	Slot *new_slot = reinterpret_cast<Slot *>(data + sizeof(PageHeader) + (header->slot_count * sizeof(Slot)));
	new_slot->offset = header->free_ptr;
	new_slot->size = record_size;

	header->slot_count++;
	return true;
}

const char *Page::getRecord(uint16_t slot_idx, uint16_t &out_size) {
	auto *header = getHeader();
	if (slot_idx >= header->slot_count) {
		return nullptr;
	}

	Slot *slots = reinterpret_cast<Slot *>(data + sizeof(PageHeader));
	out_size = slots[slot_idx].size;
	return data + slots[slot_idx].offset;
}

const char *Page::getRawData() const { return data; }
} // namespace LuminaDB