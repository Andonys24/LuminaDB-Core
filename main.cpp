#include "luminadb/buffer/BufferPoolManager.hpp"
#include "luminadb/common/types.hpp"
#include "luminadb/index/BPlusTree.hpp"
#include "luminadb/storage/DiskManager.hpp"
#include <iostream>

using namespace LuminaDB;

int main() {
	// En main.cpp (temporal, solo para probar)
	DiskManager dm("lumina_index.db");
	BufferPoolManager bpm(10, &dm);

	// 1. Crear manualmente un árbol de 2 niveles
	uint32_t root_id, leaf1_id, leaf2_id;

	// Crear nodo raíz (interno)
	Page *root_page = bpm.newPage(root_id, ModelType::B_PLUS_TREE);
	BPlusTreeInternalPage root(const_cast<char *>(root_page->getRawData()));
	root.init(IndexPageType::INTERNAL_NODE, 0, 250);

	// Crear hoja izquierda (keys: 10, 20)
	Page *leaf1 = bpm.newPage(leaf1_id, ModelType::B_PLUS_TREE);
	BPlusTreeLeafPage left(const_cast<char *>(leaf1->getRawData()));
	left.init(IndexPageType::LEAF_NODE, root_id, 250);
	left.insert(10, {1, 1});
	left.insert(20, {1, 2});

	// Crear hoja derecha (keys: 30, 40)
	Page *leaf2 = bpm.newPage(leaf2_id, ModelType::B_PLUS_TREE);
	BPlusTreeLeafPage right(const_cast<char *>(leaf2->getRawData()));
	right.init(IndexPageType::LEAF_NODE, root_id, 250);
	right.insert(30, {1, 3});
	right.insert(40, {1, 4});

	// Configurar el nodo interno
	// Estructura: [leaf1_id] -> key=30 -> [leaf2_id]
	root.setValueAt(0, leaf1_id); // Primer hijo (izquierda)
	root.setKeyAt(0, 30);		  // Llave divisoria
	root.setValueAt(1, leaf2_id); // Segundo hijo (derecha)
	root.setSize(1);			  // Solo 1 key (30)

	bpm.unpinPage(root_id, true);
	bpm.unpinPage(leaf1_id, true);
	bpm.unpinPage(leaf2_id, true);

	// 2. Probar búsqueda
	BPlusTree tree(root_id, &bpm);

	RecordID result;

	std::cout << "\n=== Testing Navigation ===\n";

	// Test con keys en leaf1
	if (tree.getValue(10, result)) {
		std::cout << "Found key=10 -> DATA at page " << result.page_id << ", slot " << result.slot_num
				  << " (searched via B+Tree leaf page 3)" << std::endl;
	}

	if (tree.getValue(20, result)) {
		std::cout << "Found key=20 -> DATA at page " << result.page_id << ", slot " << result.slot_num
				  << " (searched via B+Tree leaf page 3)" << std::endl;
	}

	// Test con keys en leaf2
	if (tree.getValue(30, result)) {
		std::cout << "Found key=30 -> DATA at page " << result.page_id << ", slot " << result.slot_num
				  << " (searched via B+Tree leaf page 4)" << std::endl;
	}

	if (tree.getValue(40, result)) {
		std::cout << "Found key=40 -> DATA at page " << result.page_id << ", slot " << result.slot_num
				  << " (searched via B+Tree leaf page 4)" << std::endl;
	}

	return 0;
}