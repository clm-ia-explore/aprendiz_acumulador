/*
 * Aprendiz Acumulador - Cabecera común
 * Definiciones compartidas, estructuras y utilidades
 */
#ifndef AC_COMMON_H
#define AC_COMMON_H
#include <stdint.h>
#include <stddef.h>
// Magic number para identificar archivos válidos
#define AC_MAGIC 0x41434355  // "ACCU" en ASCII
// Versión del formato de archivo
#define AC_VERSION 2
// Máximo tamaño de nombre de acumulador
#define AC_MAX_NAME_LEN 256
// Estructura de cabecera binaria (se escribe al inicio del archivo .bin/.dat)
typedef struct {
    uint32_t magic;           // Magic number
    uint32_t version;         // Versión del formato
    uint64_t size;            // Número de elementos (n)
    uint64_t timestamp;       // Timestamp de creación/modificación
    char reserved[32];        // Reservado para futuro uso
} ac_header_t;
// Estructura para metadatos en caché (.meta)
typedef struct {
    uint64_t size;
    uint64_t mtime;           // Modificación del archivo binario
    uint32_t checksum;        // Simple checksum para validación
} ac_meta_cache_t;
// Métodos de normalización disponibles
typedef enum {
    NORM_RAW = 0,
    NORM_MINMAX,
    NORM_POSMAX,
    NORM_SIGNED01,
    NORM_SUM01,
    NORM_SOFTMAX,
    NORM_SIGMOID,
    NORM_TANH01,
    NORM_RANK
} ac_norm_method_t;
// Formatos de visualización
typedef enum {
    VIEW_MATRIX = 0,
    VIEW_CSV,
    VIEW_HEATMAP
} ac_view_format_t;
// Funciones de utilidad comunes
// Obtener ruta del directorio de datos (respecto al cwd o variable de entorno)
const char* ac_get_data_dir(void);
// Construir ruta completa para un archivo de acumulador
int ac_build_path(char *buf, size_t bufsize, const char *name, const char *ext);
// Construir ruta para archivo de metadatos (.meta)
int ac_build_meta_path(char *buf, size_t bufsize, const char *name);
// Leer cabecera de archivo binario
int ac_read_header(const char *filepath, ac_header_t *header);
// Escribir cabecera de archivo binario
int ac_write_header(const char *filepath, const ac_header_t *header);
// Leer metadatos desde caché (.meta)
int ac_read_metadata(const char *name, ac_meta_cache_t *meta);
// Escribir metadatos a caché (.meta)
int ac_write_metadata(const char *name, const ac_meta_cache_t *meta);
// Validar si un archivo binario es válido (magic number correcto)
int ac_validate_file(const char *filepath);
// Mapear archivo en memoria (mmap)
void* ac_mmap_file(const char *filepath, size_t *size, int *fd, int writable);
// Desmapear archivo
int ac_munmap_file(void *addr, size_t size, int fd);
// Calcular checksum simple para validación de metadatos
uint32_t ac_simple_checksum(const void *data, size_t len);
// Parsear método de normalización desde string
ac_norm_method_t ac_parse_norm_method(const char *str);
// Parsear formato de visualización desde string
ac_view_format_t ac_parse_view_format(const char *str);
// Convertir índice con soporte para negativos (desde el final)
int64_t ac_parse_index(int64_t idx, int64_t size);
// Imprimir mensaje de error estandarizado
void ac_error(const char *msg, ...);
// Imprimir mensaje de información
void ac_info(const char *msg, ...);
#endif // AC_COMMON_H
