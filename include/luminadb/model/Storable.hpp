#ifndef LUMINADB_STORABLE_HPP
#define LUMINADB_STORABLE_HPP

#include <cstdint>

namespace LuminaDB {

enum class ModelType : uint32_t {
	UNKNOWN = 0,
	SENSOR = 1,
	USER = 2,
	COURSE = 3,
};

class Storable {
  public:
	virtual ~Storable() = default;

	// Returns the model type (for the PageHeader object_type)
	virtual ModelType getType() const = 0;

	// number of bytes currently occupied
	virtual size_t getSerializedSize() const = 0;

	// Write to the buffer
	virtual void serializeToBuffer(char *dest) const = 0;

	// Read from buffer
	virtual void deserializeFromBuffer(const char *src) = 0;
};

} // namespace LuminaDB

#endif