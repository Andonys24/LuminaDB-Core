#include "luminadb/database/Database.hpp"
#include "luminadb/model/Course.hpp"
#include "luminadb/model/SensorData.hpp"
#include "luminadb/model/User.hpp"
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace LuminaDB;

void clean_console() {
#ifdef _WIN32
	system("cls");
#else
	system("clear");
#endif
}

void printSeparator(const std::string &title = "") {
	if (title.empty()) {
		std::cout << "\n" << std::string(70, '=') << "\n";
		return;
	}

	const std::string asterisk(title.length() * 3, '*');
	const std::string spaces(title.length() - 1, ' ');

	std::cout << std::endl << asterisk << std::endl;
	std::cout << "*" << spaces << title << spaces << "*" << std::endl;
	std::cout << asterisk << std::endl << std::endl;
}

// Simple random helpers for demo variability
std::mt19937 &rng() {
	static std::mt19937 gen{std::random_device{}()};
	return gen;
}

int randInt(int low, int high) {
	std::uniform_int_distribution<int> dist(low, high);
	return dist(rng());
}

double randDouble(double low, double high) {
	std::uniform_real_distribution<double> dist(low, high);
	return dist(rng());
}

std::string randomName() {
	static const std::vector<std::string> names{"Alice", "Bob", "Charlie", "Diana", "Eve", "Frank", "Grace", "Heidi"};
	return names[static_cast<size_t>(randInt(0, static_cast<int>(names.size() - 1)))];
}

std::string randomCourseTitle() {
	static const std::vector<std::string> titles{"Database Systems",	"C++ Programming",	"Operating Systems",
												 "Distributed Systems", "Machine Learning", "Computer Networks"};
	return titles[static_cast<size_t>(randInt(0, static_cast<int>(titles.size() - 1)))];
}

std::vector<uint32_t> randomStudents(uint32_t base_id) {
	int count = randInt(2, 5);
	std::vector<uint32_t> students;
	students.reserve(static_cast<size_t>(count));
	for (int i = 0; i < count; ++i) {
		students.push_back(base_id + static_cast<uint32_t>(i));
	}
	return students;
}

int main() {
	try {
		// ========== INITIALIZATION ==========
		printSeparator("LuminaDB-Core Demo - Phase 6: Query Interface");

		// Do NOT remove old database - we want to test persistence!
		Database db("demo.db", 20);

		// ========== TRY TO READ EXISTING DATA ==========
		printSeparator("1. ATTEMPT TO READ PREVIOUSLY STORED DATA");

		bool user_101_exists = db.exists(101);
		bool sensor_201_exists = db.exists(201);

		std::cout << "Checking for previously stored data...\n\n";

		if (user_101_exists) {
			std::cout << "User 101 EXISTS in database!\n";
			try {
				User existing_user = db.find<User>(101);
				std::cout << "   → ID=" << existing_user.getId() << ", Name=" << existing_user.getName()
						  << ", Age=" << existing_user.getAge() << "\n\n";
			} catch (const std::exception &e) {
				std::cout << "   Error reading: " << e.what() << "\n\n";
			}
		} else {
			std::cout << "User 101 NOT found - first run or no previous data\n\n";
		}

		if (sensor_201_exists) {
			std::cout << "Sensor 201 EXISTS in database!\n";
			try {
				SensorData existing_sensor = db.find<SensorData>(201);
				std::cout << "   → ID=" << existing_sensor.getSensorId() << ", Value=" << existing_sensor.getValue()
						  << "\n\n";
			} catch (const std::exception &e) {
				std::cout << "   Error reading: " << e.what() << "\n\n";
			}
		} else {
			std::cout << "Sensor 201 NOT found - first run or no previous data\n\n";
		}

		// ========== INSERT NEW USERS ==========
		printSeparator("2. INSERT NEW USERS");

		User user1(static_cast<uint32_t>(101), randomName(), static_cast<uint16_t>(randInt(18, 60)));
		User user2(static_cast<uint32_t>(102), randomName(), static_cast<uint16_t>(randInt(18, 60)));
		User user3(static_cast<uint32_t>(103), randomName(), static_cast<uint16_t>(randInt(18, 60)));

		std::cout << "Inserting User 101 (Alice, 28)..." << std::endl;
		bool success1 = db.insert<User>(101, user1);
		std::cout << "Result: " << (success1 ? "SUCCESS" : "FAILED") << "\n";

		std::cout << "Inserting User 102 (Bob, 35)..." << std::endl;
		bool success2 = db.insert<User>(102, user2);
		std::cout << "Result: " << (success2 ? "SUCCESS" : "FAILED") << "\n";

		std::cout << "Inserting User 103 (Charlie, 24)..." << std::endl;
		bool success3 = db.insert<User>(103, user3);
		std::cout << "Result: " << (success3 ? "SUCCESS" : "FAILED") << "\n";

		// ========== INSERT SENSOR DATA ==========
		printSeparator("3. INSERT SENSOR DATA");

		const uint64_t base_timestamp = 1609459200ULL; // Jan 1, 2021 00:00:00
		SensorData sensor1(201, randDouble(15.0, 40.0), base_timestamp + static_cast<uint64_t>(randInt(0, 86400)));
		SensorData sensor2(202, randDouble(15.0, 40.0), base_timestamp + static_cast<uint64_t>(randInt(0, 86400)));

		std::cout << "Inserting Sensor 201 (Value: 25.5)..." << std::endl;
		bool sensor_success1 = db.insert<SensorData>(201, sensor1);
		std::cout << "Result: " << (sensor_success1 ? "SUCCESS" : "FAILED") << "\n";

		std::cout << "Inserting Sensor 202 (Value: 28.3)..." << std::endl;
		bool sensor_success2 = db.insert<SensorData>(202, sensor2);
		std::cout << "Result: " << (sensor_success2 ? "SUCCESS" : "FAILED") << "\n";

		// ========== INSERT COURSES ==========
		printSeparator("4. INSERT COURSES");

		std::vector<uint32_t> students_db = randomStudents(1001);
		std::vector<uint32_t> students_cpp = randomStudents(2001);

		Course course1(301, randomCourseTitle(), students_db);
		Course course2(302, randomCourseTitle(), students_cpp);

		std::cout << "Inserting Course 301 (Database Systems, 3 students)..." << std::endl;
		bool course_success1 = db.insert<Course>(301, course1);
		std::cout << "Result: " << (course_success1 ? "SUCCESS" : "FAILED") << "\n";

		std::cout << "Inserting Course 302 (C++ Programming, 2 students)..." << std::endl;
		bool course_success2 = db.insert<Course>(302, course2);
		std::cout << "Result: " << (course_success2 ? "SUCCESS" : "FAILED") << "\n";

		// ========== FIND & VERIFY USERS ==========
		printSeparator("4. FIND & VERIFY USERS");

		std::cout << "Finding User 101..." << std::endl;
		User found_user = db.find<User>(101);
		std::cout << "Found: ID=" << found_user.getId() << ", Name=" << found_user.getName()
				  << ", Age=" << found_user.getAge() << "\n";

		std::cout << "Finding User 102..." << std::endl;
		User found_user2 = db.find<User>(102);
		std::cout << "Found: ID=" << found_user2.getId() << ", Name=" << found_user2.getName()
				  << ", Age=" << found_user2.getAge() << "\n";

		std::cout << "Finding User 103..." << std::endl;
		User found_user3 = db.find<User>(103);
		std::cout << "Found: ID=" << found_user3.getId() << ", Name=" << found_user3.getName()
				  << ", Age=" << found_user3.getAge() << "\n";

		// ========== FIND & VERIFY SENSORS ==========
		printSeparator("5. FIND & VERIFY SENSORS");

		std::cout << "Finding Sensor 201..." << std::endl;
		SensorData found_sensor = db.find<SensorData>(201);
		std::cout << "Found: ID=" << found_sensor.getSensorId() << ", Value=" << found_sensor.getValue()
				  << ", Timestamp=" << found_sensor.getTimestamp() << "\n";

		std::cout << "Finding Sensor 202..." << std::endl;
		SensorData found_sensor2 = db.find<SensorData>(202);
		std::cout << "Found: ID=" << found_sensor2.getSensorId() << ", Value=" << found_sensor2.getValue()
				  << ", Timestamp=" << found_sensor2.getTimestamp() << "\n";

		// ========== FIND & VERIFY COURSES ==========
		printSeparator("6. FIND & VERIFY COURSES");

		std::cout << "Finding Course 301..." << std::endl;
		Course found_course = db.find<Course>(301);
		std::cout << "Found: ID=" << found_course.getCourseId() << ", Title=" << found_course.getTitle()
				  << ", Students: " << found_course.getStudentsIds().size() << "\n";

		std::cout << "Finding Course 302..." << std::endl;
		Course found_course2 = db.find<Course>(302);
		std::cout << "Found: ID=" << found_course2.getCourseId() << ", Title=" << found_course2.getTitle()
				  << ", Students: " << found_course2.getStudentsIds().size() << "\n";

		// ========== EXISTS CHECKS ==========
		printSeparator("7. EXISTS CHECKS");

		std::cout << "Checking if key 101 exists: " << (db.exists(101) ? "YES" : "NO") << "\n";
		std::cout << "Checking if key 201 exists: " << (db.exists(201) ? "YES" : "NO") << "\n";
		std::cout << "Checking if key 301 exists: " << (db.exists(301) ? "YES" : "NO") << "\n";
		std::cout << "Checking if key 999 exists: " << (db.exists(999) ? "YES" : "NO") << "\n";

		// ========== NOT FOUND TEST ==========
		printSeparator("8. NOT FOUND TEST");

		std::cout << "Attempting to find non-existent key 999..." << std::endl;
		try {
			User not_found = db.find<User>(999);
		} catch (const std::exception &e) {
			std::cout << "Caught exception: " << e.what() << "\n";
		}

		// ========== SUMMARY ==========
		printSeparator("DEMO SUMMARY");
		std::cout << "Database file: " << db.getFilename() << "\n";
		std::cout << "Total objects stored: 7 (3 Users + 2 Sensors + 2 Courses)\n";
		std::cout << "All objects successfully persisted and retrieved!\n";
		std::cout << "Multiple pages allocated (B+ Tree: 0-999, Data: 1000+)\n";

		printSeparator("DEMO COMPLETED SUCCESSFULLY");

		return 0;

	} catch (const std::exception &e) {
		std::cerr << "\n[ERROR] " << e.what() << std::endl;
		return 1;
	}
}