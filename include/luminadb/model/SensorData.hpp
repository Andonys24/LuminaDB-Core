#ifndef LUMINADB_SENSORDATA_HPP
#define LUMINADB_SENSORDATA_HPP

#include "Storable.hpp"

namespace LuminaDB {

class SensorData : public Storable {
  private:
	uint32_t sensor_id;
	double value;
	uint64_t timestamp;

  public:
	SensorData();
	SensorData(uint32_t sensor_id, double value, uint64_t timestamp);

	// Getters
	uint32_t getSensorId() const;
	double getValue() const;
	uint64_t getTimestamp() const;

	// Setters
	void setSensorId(uint32_t id);
	void setValue(double val);
	void setTimestamp(uint64_t ts);

	ModelType getType() const override;

	size_t getSerializedSize() const override;

	void serializeToBuffer(char *dest) const override;

	void deserializeFromBuffer(const char *src) override;

	~SensorData();
};

} // namespace LuminaDB

#endif