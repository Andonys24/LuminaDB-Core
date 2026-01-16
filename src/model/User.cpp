#include "luminadb/model/User.hpp"

namespace LuminaDB {

User::User() : id(0), name(""), age(0) {}

User::User(uint32_t id, std::string name, uint16_t age) : id(id), name(name), age(age) {}

uint32_t User::getId() const { return id; }

std::string User::getName() const { return name; }

uint16_t User::getAge() const { return age; }

void User::setId(uint32_t id) { this->id = id; }

void User::setName(const std::string &name) { this->name = name; }

void User::setAge(uint16_t age) { this->age = age; }

ModelType User::getType() const { return ModelType::USER; }

size_t User::getSerializedSize() const { return sizeof(id) + sizeof(age) + sizeof(uint16_t) + name.length(); }

void User::serializeToBuffer(char *dest) const {
	size_t offset = 0;

	// ID
	std::memcpy(dest + offset, &id, sizeof(id));
	offset += sizeof(id);

	// Age
	std::memcpy(dest + offset, &age, sizeof(age));
	offset += sizeof(age);

	// Name length
	uint16_t name_len = static_cast<uint16_t>(name.length());
	std::memcpy(dest + offset, &name_len, sizeof(name_len));
	offset += sizeof(name_len);

	// Pure name
	std::memcpy(dest + offset, name.c_str(), name_len);
}

void User::deserializeFromBuffer(const char *src) {
	size_t offset = 0;

	std::memcpy(&id, src + offset, sizeof(id));
	offset += sizeof(id);

	std::memcpy(&age, src + offset, sizeof(age));
	offset += sizeof(age);

	uint16_t name_len;
	std::memcpy(&name_len, src + offset, sizeof(name_len));
	offset += sizeof(name_len);

	// Rebuild the string from the buffer
	name.assign(src + offset, name_len);
}

} // namespace LuminaDB
