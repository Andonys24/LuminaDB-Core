#include "luminadb/model/Course.hpp"

namespace LuminaDB {

Course::Course() : course_id(0), title(""), students_ids() {}

Course::Course(uint32_t course_id, const std::string &title, const std::vector<uint32_t> &students_ids)
	: course_id(course_id), title(title), students_ids(students_ids) {}

uint32_t Course::getCourseId() const { return course_id; }

std::string Course::getTitle() const { return title; }

std::vector<uint32_t> Course::getStudentsIds() const { return students_ids; }

void Course::setCourseId(uint32_t id) { course_id = id; }

void Course::setTitle(const std::string &title) { this->title = title; }

void Course::setStudentsIds(const std::vector<uint32_t> &students) { students_ids = students; }

void Course::addStudentId(uint32_t student_id) { students_ids.push_back(student_id); }

ModelType Course::getType() const { return ModelType::COURSE; }

size_t Course::getSerializedSize() const {
	return sizeof(course_id) + sizeof(uint16_t) + title.length() + sizeof(uint16_t) +
		   (students_ids.size() * sizeof(uint32_t));
}

void Course::serializeToBuffer(char *dest) const {
	size_t offset = 0;

	// ID
	std::memcpy(dest + offset, &course_id, sizeof(course_id));
	offset += sizeof(course_id);

	// Title
	uint16_t title_len = static_cast<uint16_t>(title.length());
	std::memcpy(dest + offset, &title_len, sizeof(title_len));
	offset += sizeof(title_len);
	std::memcpy(dest + offset, title.c_str(), title_len);
	offset += title_len;

	// Students
	uint16_t student_count = static_cast<uint16_t>(students_ids.size());
	std::memcpy(dest + offset, &student_count, sizeof(student_count));
	offset += sizeof(student_count);

	// Copy the entire block of IDs in one fell swoop
	if (student_count > 0)
		std::memcpy(dest + offset, students_ids.data(), student_count * sizeof(uint32_t));
}

void Course::deserializeFromBuffer(const char *src) {
	size_t offset = 0;

	// ID
	std::memcpy(&course_id, src + offset, sizeof(course_id));
	offset += sizeof(course_id);

	// Title
	uint16_t title_len;
	std::memcpy(&title_len, src + offset, sizeof(title_len));
	offset += sizeof(title_len);
	title.assign(src + offset, title_len);
	offset += title_len;

	// Students
	uint16_t student_count;
	std::memcpy(&student_count, src + offset, sizeof(student_count));
	offset += sizeof(student_count);

	students_ids.resize(student_count);
	if (student_count > 0)
		std::memcpy(students_ids.data(), src + offset, student_count * sizeof(uint32_t));
}

} // namespace LuminaDB
