#ifndef LUMINADB_MODEL_FACTORY_HPP
#define LUMINADB_MODEL_FACTORY_HPP

#include "Storable.hpp"
#include <memory>

namespace LuminaDB {

class ModelFactory {
  public:
	static std::unique_ptr<Storable> create(ModelType);
};

} // namespace LuminaDB

#endif