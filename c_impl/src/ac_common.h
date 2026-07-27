/*
 * Aprendiz Acumulador - Cabecera común
 * Definiciones compartidas, estructuras y utilidades
 */
#ifndef AC_COMMON_H
#define AC_COMMON_H
#include <stdint.h>
#include <stddef.h>
// Magic numbers para identificar archivos válidos
#define AC_MAGIC      0x41434355  // "ACCU" en ASCII (archivo .bin runtime)
#define AC_DAT_MAGIC  0x41434441  // "ACDA" en ASCII (archivo .dat snapshot)
#define AC_QDAT_MAGIC 0x41435144  // "ACQD" en ASCII (archivo .qdat cuantizado)
// Versión del formato de archivo
#define AC_VERSION 1
// Máximo tamaño de nombre de acumulador
#define AC_MAX_NAME_LEN 256
// Estructura de cabecera binaria para archivos .bin (runtime)
// Debe coincidir con el formato Python: 8sIIII = 8+4+4+4+4 = 24 bytes
typedef struct {
    char     magic[8];          // Magic number (string)
    uint32_t version;           // Versión del formato
    uint32_t n;                 // Número de elementos
    uint32_t flags;             // Reservado
    uint32_t reserved2;         // Reservado
} ac_bin_header_t;
// Estructura de cabecera para archivos .dat (snapshot)
// Formato Python: 8sIIII = 24 bytes
typedef struct {
    char     magic[8];          // MAGIC_DAT (string)
    uint32_t version;           // Versión del formato
    uint32_t n;                 // Número de elementos (n)
    uint32_t flags;             // Reservado para futuro
    uint32_t crc32;             // CRC32 de los datos
} ac_dat_header_t;
// Estructura de cabecera para archivos .qdat (cuantizado)
// Formato Python: 8sIIIiiI = 8+4+4+4+4+4+4 = 32 bytes
typedef struct {
    char     magic[8];          // MAGIC_QDAT (string)
    uint32_t version;           // Versión del formato
    uint32_t n;                 // Número de elementos (n)
    uint32_t method;            // Método de normalización (código)
    int32_t  qmin;              // Valor mínimo cuantizado
    int32_t  qmax;              // Valor máximo cuantizado
    uint32_t crc32;             // CRC32 de los datos
} ac_qdat_header_t;
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
// Leer cabecera de archivo binario (.bin)
int ac_read_header(const char *filepath, ac_bin_header_t *header);
// Escribir cabecera de archivo binario (.bin)
int ac_write_header(const char *filepath, const ac_bin_header_t *header);
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
// Calcular CRC32 (estándar)
uint32_t ac_crc32(const void *data, size_t len);
// Parsear método de normalización desde string
ac_norm_method_t ac_parse_norm_method(const char *str);
// Obtener nombre de método de normalización
const char* ac_get_norm_method_name(ac_norm_method_t method);
// Parsear formato de visualización desde string
ac_view_format_t ac_parse_view_format(const char *str);
// Convertir índice con soporte para negativos (desde el final)
int64_t ac_parse_index(int64_t idx, int64_t size);
// Leer cabecera de archivo .dat
int ac_read_dat_header(const char *filepath, ac_dat_header_t *header);
// Escribir cabecera de archivo .dat
int ac_write_dat_header(const char *filepath, const ac_dat_header_t *header);
// Leer cabecera de archivo .qdat
int ac_read_qdat_header(const char *filepath, ac_qdat_header_t *header);
// Escribir cabecera de archivo .qdat
int ac_write_qdat_header(const char *filepath, const ac_qdat_header_t *header);
// Leer valores desde archivo .dat
int ac_read_dat_values(const char *filepath, int64_t **values, uint64_t *n);
// Escribir valores a archivo .dat
int ac_write_dat_file(const char *filepath, uint64_t n, const int64_t *values, int force);
// Leer valores cuantizados desde archivo .qdat
int ac_read_qdat_values(const char *filepath, int32_t **values, uint64_t *n, 
                        ac_norm_method_t *method, int32_t *qmin, int32_t *qmax);
// Escribir valores cuantizados a archivo .qdat
int ac_write_qdat_file(const char *filepath, uint64_t n, ac_norm_method_t method,
                       int32_t qmin, int32_t qmax, const int32_t *values, int force);
// Normalizar valores según método
int ac_normalize_values(const double *input, double *output, uint64_t n,
                        ac_norm_method_t method, double temperature, double scale);
// Cuantizar valores float a enteros
int ac_quantize_floats(const double *floats, int32_t *output, uint64_t n,
                       int32_t qmin, int32_t qmax, int renorm_max);
// Imprimir mensaje de error estandarizado
void ac_error(const char *msg, ...);
// Imprimir mensaje de información
void ac_info(const char *msg, ...);
#endif // AC_COMMON_H
