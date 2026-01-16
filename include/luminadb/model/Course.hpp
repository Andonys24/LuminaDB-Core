#ifndef LUMINADB_COURSE_HPP
#define LUMINADB_COURSE_HPP

#include "Storable.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace LuminaDB {
class Course : public Storable {
  private:
	uint32_t course_id;
	std::string title;
	std::vector<uint32_t> students_ids;

  public:
	Course();
	Course(uint32_t course_id, const std::string &title, const std::vector<uint32_t> &students_ids);

	// Getters
	uint32_t getCourseId() const;
	std::string getTitle() const;
	std::vector<uint32_t> getStudentsIds() const;

	// Setters
	void setCourseId(uint32_t id);
	void setTitle(const std::string &title);
	void setStudentsIds(const std::vector<uint32_t> &students);
	void addStudentId(uint32_t student_id);

	ModelType getType() const override;

	size_t getSerializedSize() const override;

	void serializeToBuffer(char *dest) const override;

	void deserializeFromBuffer(const char *src) override;
};
} // namespace LuminaDB

#endif