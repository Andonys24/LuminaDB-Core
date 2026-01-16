#ifndef LUMINADB_LRU_REPLACER_HPP
#define LUMINADB_LRU_REPLACER_HPP

#include <list>
#include <mutex>
#include <unordered_map>

namespace LuminaDB {
class LRUReplacer {
  private:
	std::mutex latch;
	std::list<uint32_t> lru_list;
	std::unordered_map<uint32_t, std::list<uint32_t>::iterator> lru_map;
	size_t max_pages;

  public:
	explicit LRUReplacer(size_t num_pages);
	~LRUReplacer();

	/**
	 * Choose the oldest frame to be evicted.
	 * Returns true if one is found, false if there is no one to be evicted.
	 */
	bool victim(uint32_t *frame_id);

	/**
	 * "Pin" a page. Removes the frame from the replacer because someone is using it.
	 */
	void pin(uint32_t frame_id);

	/**
	 * "Release" a page. Adds the frame to the replacer so it's a candidate to be removed.
	 */
	void unpin(uint32_t frame_id);

	size_t Size();
};

} // namespace LuminaDB

#endif