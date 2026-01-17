# LuminaDB-Core

Motor de almacenamiento embebido en C++20 con índice B+ Tree, buffer pool y páginas con formato slotted para persistir objetos serializados. Incluye un demo interactivo que muestra inserción, búsqueda y persistencia entre ejecuciones.

## Características
- Índice B+ Tree sobre IDs de 32 bits, con splits de hojas e internas y raíz persistente.
- Buffer Pool con reemplazo LRU, pines y flush a disco para páginas de 4 KB.
- Páginas slotted con header (`page_id`, `object_type`, `slot_count`, `free_ptr`) y almacenamiento compacto de registros.
- DiskManager para lectura/escritura de páginas, creación del archivo y zero-fill en páginas cortas.
- Modelos de ejemplo (`User`, `SensorData`, `Course`) que implementan `Storable` y se serializan/deserializan vía `ModelFactory`.
- Demo CLI que persiste en `demo.db`, reabre en ejecuciones posteriores y rellena datos aleatorios para validar splits y múltiples páginas.

## Arquitectura rápida
- `Database`: fachada de alto nivel para `insert`, `find`, `exists`. Ensambla `DiskManager`, `BufferPoolManager` y `BPlusTree`. ([include/luminadb/database/Database.hpp](include/luminadb/database/Database.hpp))
- `BPlusTree` y `BPlusTreePage`: nodos de índice y lógica de búsqueda/inserción. ([include/luminadb/index](include/luminadb/index))
- `BufferPoolManager`: gestiona páginas en RAM, LRU, pin/unpin, y asignación de nuevas páginas. ([include/luminadb/buffer/BufferPoolManager.hpp](include/luminadb/buffer/BufferPoolManager.hpp))
- `Page` y slotted layout: header + slots + registros. Tamaño fijo de 4096 bytes. ([include/luminadb/storage/Page.hpp](include/luminadb/storage/Page.hpp))
- `DiskManager`: E/S de páginas fijas en el archivo y reserva inicial. ([src/storage/DiskManager.cpp](src/storage/DiskManager.cpp))
- Modelos: `User`, `SensorData`, `Course` y la fábrica de serialización. ([include/luminadb/model](include/luminadb/model))
- Demo: flujo completo de inserción/búsqueda/existencia con claves fijas y contenido aleatorio en cada corrida. ([main.cpp](main.cpp))

## Construcción
Requiere CMake y un compilador C++20.

```bash
# Configurar
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Compilar (MSVC: usa --config Release)
cmake --build build --config Release
```

## Ejecución del demo

```bash
cd build
./LuminaDB.exe   # en Windows: .\LuminaDB.exe
```

Comportamiento del demo:
- Si `demo.db` existe, lee datos previos y reanuda el árbol desde la página raíz persistida.
- Inserta tres usuarios (IDs 101, 102, 103), dos sensores (201, 202) y dos cursos (301, 302) con valores aleatorios en cada corrida.
- Hace búsquedas y comprobaciones de existencia; muestra splits de hojas/internas en consola.
- El archivo crece en cada ejecución porque se reinsertan las mismas claves (no hay deduplicación/updates).

## Layout de páginas
- Páginas de índice: raíz en la página 0; el árbol crece con nuevas páginas conforme ocurren splits.
- Páginas de datos: comienzan en 1000 y se asignan secuencialmente para los registros almacenados.

## Limitaciones conocidas
- No hay actualización ni borrado; las inserciones repetidas añaden más páginas.
- No hay WAL ni recuperación ante fallos; consistencia best-effort en una sola escritura.
- No se implementa `delete` en el B+ Tree; no se reciclan páginas de datos.

## Estructura del repositorio
- [`main.cpp`](main.cpp): demo CLI.
- [`include/luminadb`](include/luminadb): headers de API y estructuras core.
- [`src`](src): implementaciones.
- [`sandbox`](sandbox): archivos de salida y pruebas manuales.
- [`build`](build): artefactos generados por CMake (no se versionan normalmente).

## Troubleshooting
- Si ves caracteres raros en consola (p. ej. flechas), es un tema de codificación de consola; los datos están correctos.
- Para empezar limpio, borra `build/demo.db` (o recrea `build/`) y vuelve a compilar.
