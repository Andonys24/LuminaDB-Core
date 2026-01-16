#include "luminadb/buffer/BufferPoolManager.hpp"
#include "luminadb/model/Course.hpp"
#include "luminadb/model/ModelFactory.hpp"
#include "luminadb/model/SensorData.hpp"
#include "luminadb/model/User.hpp"
#include "luminadb/storage/DiskManager.hpp"
#include <iostream>
#include <memory>
#include <vector>

using namespace LuminaDB;

int main() {
	// 1. Setup: Disk file and a Pool restricted to 3 pages to force eviction
	DiskManager dm("lumina_v4.db");
	BufferPoolManager bpm(3, &dm);

	uint32_t user_page_id, sensor_page_id, course_page_id;

	std::cout << "--- Phase 4: Serialization and Model Factory Test ---" << std::endl;

	// --- PART A: CREATE AND SAVE A USER (Variable String) ---
	{
		Page *page = bpm.newPage(user_page_id, static_cast<uint32_t>(ModelType::USER));

		User user(101, "Satoshi Nakamoto", 45);
		std::vector<char> buffer(user.getSerializedSize());
		user.serializeToBuffer(buffer.data());

		if (page->insertRecord(buffer.data(), static_cast<uint16_t>(buffer.size()))) {
			std::cout << "[OK] User serialized into Page " << user_page_id << std::endl;
		}
		bpm.unpinPage(user_page_id, true); // Mark as dirty
	}

	// --- PART B: CREATE AND SAVE SENSOR DATA (Fixed Primitives) ---
	{
		Page *page = bpm.newPage(sensor_page_id, static_cast<uint32_t>(ModelType::SENSOR));

		SensorData sensor(777, 24.5, 1705416000);
		std::vector<char> buffer(sensor.getSerializedSize());
		sensor.serializeToBuffer(buffer.data());

		page->insertRecord(buffer.data(), static_cast<uint16_t>(buffer.size()));
		std::cout << "[OK] Sensor serialized into Page " << sensor_page_id << std::endl;
		bpm.unpinPage(sensor_page_id, true);
	}

	// --- PART C: CREATE AND SAVE A COURSE (Complex Vector + String) ---
	{
		Page *page = bpm.newPage(course_page_id, static_cast<uint32_t>(ModelType::COURSE));

		std::vector<uint32_t> initial_students = {101, 202, 303, 404, 505};
		Course course(501, "Database Systems Architecture", initial_students);

		std::vector<char> buffer(course.getSerializedSize());
		course.serializeToBuffer(buffer.data());

		page->insertRecord(buffer.data(), static_cast<uint16_t>(buffer.size()));
		std::cout << "[OK] Course serialized into Page " << course_page_id << std::endl;
		bpm.unpinPage(course_page_id, true);
	}

	// --- PART D: FORCE EVICTION (Corrected) ---
	std::cout << "\n--- Forcing Eviction: Filling the Pool to push data to Disk ---" << std::endl;

	uint32_t d1, d2, d3;
	// 1. Create the pages and IMMEDIATELY get their IDs
	bpm.newPage(d1, 0);
	bpm.newPage(d2, 0);
	bpm.newPage(d3, 0);

	// 2. Unpin them so the Replacer (LRU) knows they can be evicted
	bpm.unpinPage(d1, false);
	bpm.unpinPage(d2, false);
	bpm.unpinPage(d3, false);

	// Now the pool is full of "unpinned" pages.
	// When we call fetchPage next, the BPM will successfully evict one of these.

	// --- PART E: RECOVERY USING FACTORY ---
	std::cout << "\n--- Verifying Object Reconstruction via ModelFactory ---" << std::endl;

	// 1. Recover User
	{
		Page *p_rec = bpm.fetchPage(user_page_id);
		auto obj = ModelFactory::create(static_cast<ModelType>(p_rec->getHeader()->object_type));

		uint16_t size;
		const char *raw = p_rec->getRecord(0, size);
		obj->deserializeFromBuffer(raw);

		User *u = static_cast<User *>(obj.get());
		std::cout << "SUCCESS: Recovered User: " << u->getName() << " (ID: " << u->getId() << ")" << std::endl;
		bpm.unpinPage(user_page_id, false);
	}

	// 2. Recover Sensor
	{
		Page *p_sen = bpm.fetchPage(sensor_page_id);
		auto obj_sen = ModelFactory::create(static_cast<ModelType>(p_sen->getHeader()->object_type));

		uint16_t size;
		obj_sen->deserializeFromBuffer(p_sen->getRecord(0, size));
		SensorData *s = static_cast<SensorData *>(obj_sen.get());
		std::cout << "SUCCESS: Recovered Sensor: Value=" << s->getValue() << ", TS=" << s->getTimestamp() << std::endl;
		bpm.unpinPage(sensor_page_id, false);
	}

	// 3. Recover Course (The Complex One)
	{
		Page *p_crs = bpm.fetchPage(course_page_id);
		auto obj_crs = ModelFactory::create(static_cast<ModelType>(p_crs->getHeader()->object_type));

		uint16_t size;
		obj_crs->deserializeFromBuffer(p_crs->getRecord(0, size));
		Course *c = static_cast<Course *>(obj_crs.get());

		std::cout << "SUCCESS: Recovered Course: " << c->getTitle() << std::endl;
		std::cout << "         Students enrolled: " << c->getStudentsIds().size() << std::endl;
		bpm.unpinPage(course_page_id, false);
	}

	std::cout << "\n--- PHASE 4 COMPLETED SUCCESSFULLY ---" << std::endl;
	return 0;
}