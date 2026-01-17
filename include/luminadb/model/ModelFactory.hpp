#ifndef LUMINADB_MODEL_FACTORY_HPP
#define LUMINADB_MODEL_FACTORY_HPP

#include "Course.hpp"
#include "SensorData.hpp"
#include "Storable.hpp"
#include "User.hpp"
#include <memory>

namespace LuminaDB {

class ModelFactory {
  public:
	// NOT USED - create() replaced by deserialize<T>()
	// Template deserialization is more type-safe and efficient.
	static std::unique_ptr<Storable> create(ModelType);

	// Template method to deserialize from buffer
	// Usage: User user = ModelFactory::deserialize<User>(buffer);
	template <typename T> static T deserialize(const char *buffer) {
		T obj;
		obj.deserializeFromBuffer(buffer);
		return obj;
	}

	// Template specialization for pointer return
	// Usage: std::unique_ptr<User> user = ModelFactory::deserializePtr<User>(buffer);
	template <typename T> static std::unique_ptr<T> deserializePtr(const char *buffer) {
		auto obj = std::make_unique<T>();
		obj->deserializeFromBuffer(buffer);
		return obj;
	}
};

} // namespace LuminaDB

#endif