#ifndef LUMINADB_USER_HPP
#define LUMINADB_USER_HPP

#include "Storable.hpp"
#include <cstdint>
#include <string>

namespace LuminaDB {

class User : public Storable {
  private:
	uint32_t id;
	std::string name;
	uint16_t age;

  public:
	User();
	User(uint32_t id, std::string name, uint16_t age);

	// Getters
	uint32_t getId() const;
	std::string getName() const;
	uint16_t getAge() const;

	// Setters
	void setId(uint32_t id);
	void setName(const std::string &name);
	void setAge(uint16_t age);

	ModelType getType() const override;

	size_t getSerializedSize() const override;

	void serializeToBuffer(char *dest) const override;

	void deserializeFromBuffer(const char *src) override;

	~User();
};

} // namespace LuminaDB

#endif