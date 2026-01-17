#include "luminadb/model/ModelFactory.hpp"
#include "luminadb/model/Course.hpp"
#include "luminadb/model/SensorData.hpp"
#include "luminadb/model/User.hpp"

namespace LuminaDB {

std::unique_ptr<Storable> ModelFactory::create(ModelType type) {
	switch (type) {
	case ModelType::SENSOR:
		return std::make_unique<SensorData>();
	case ModelType::USER:
		return std::make_unique<User>();
	case ModelType::COURSE:
		return std::make_unique<Course>();

	default:
		return nullptr;
	}
}

} // namespace LuminaDB