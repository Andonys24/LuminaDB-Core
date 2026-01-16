#include "luminadb/buffer/LRUReplacer.hpp"
#include <algorithm>

namespace LuminaDB {
LRUReplacer::LRUReplacer(size_t num_pages) : max_pages(num_pages) {}

LRUReplacer::~LRUReplacer() = default;

/**
 * VICTIM: The "sacrifice".
 * Extract the item at the top of the list (the oldest).
 */
bool LRUReplacer::victim(uint32_t *frame_id) {
	std::lock_guard<std::mutex> lock(latch);

	if (lru_list.empty())
		return false;

	// The candidate is first on the list (the one that hasn't been used the longest)
	uint32_t victim_id = lru_list.front();
	lru_list.pop_front();
	lru_map.erase(victim_id);

	*frame_id = victim_id;
	return true;
}

/**
 * PIN: Someone is using this page.
 * Must be removed from the Replacer to avoid becoming a victim.
 */
void LRUReplacer::pin(uint32_t frame_id) {
	std::lock_guard<std::mutex> lock(latch);

	if (lru_map.count(frame_id)) {
		lru_list.erase(lru_map[frame_id]);
		lru_map.erase(frame_id);
	}
}

/**
 * UNPIN: Someone has finished using the page.
 * Now it's a candidate for removal if we need space.
 * We'll put it at the end (the "youngest" position).
 */
void LRUReplacer::unpin(uint32_t frame_id) {
	std::lock_guard<std::mutex> lock(latch);

	if (lru_map.count(frame_id))
		return;

	if (lru_list.size() >= max_pages)
		return;

	lru_list.push_back(frame_id);

	// Save the iterator at the end of the list to delete it in O(1) later
	lru_map[frame_id] = std::prev(lru_list.end());
}

size_t LRUReplacer::Size() {
	std::lock_guard<std::mutex> lock(latch);
	return lru_list.size();
}

} // namespace LuminaDB