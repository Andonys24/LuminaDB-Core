#include "luminadb/model/SensorData.hpp"
#include <cstring>

namespace LuminaDB {

SensorData::SensorData() : sensor_id(0), value(0.0), timestamp(0) {}

SensorData::SensorData(uint32_t sensor_id, double value, uint64_t timestamp)
	: sensor_id(sensor_id), value(value), timestamp(timestamp) {}

uint32_t SensorData::getSensorId() const { return sensor_id; }

double SensorData::getValue() const { return value; }

uint64_t SensorData::getTimestamp() const { return timestamp; }

void SensorData::setSensorId(uint32_t id) { sensor_id = id; }

void SensorData::setValue(double val) { value = val; }

void SensorData::setTimestamp(uint64_t ts) { timestamp = ts; }

ModelType SensorData::getType() const { return ModelType::SENSOR; }

size_t SensorData::getSerializedSize() const { return sizeof(sensor_id) + sizeof(value) + sizeof(timestamp); }

void SensorData::serializeToBuffer(char *dest) const {
	size_t offset = 0;

	std::memcpy(dest + offset, &sensor_id, sizeof(sensor_id));
	offset += sizeof(sensor_id);

	std::memcpy(dest + offset, &value, sizeof(value));
	offset += sizeof(value);

	std::memcpy(dest + offset, &timestamp, sizeof(timestamp));
}

void SensorData::deserializeFromBuffer(const char *src) {
	size_t offset = 0;

	std::memcpy(&sensor_id, src + offset, sizeof(sensor_id));
	offset += sizeof(sensor_id);

	std::memcpy(&value, src + offset, sizeof(value));
	offset += sizeof(value);

	std::memcpy(&timestamp, src + offset, sizeof(timestamp));
}

} // namespace LuminaDB
