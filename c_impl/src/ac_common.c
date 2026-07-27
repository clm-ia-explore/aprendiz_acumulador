/*
 * Aprendiz Acumulador - Implementación de funciones comunes
 */
#define _POSIX_C_SOURCE 200809L
#include "ac_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
#include <math.h>
#define _POSIX_C_SOURCE 200809L
// Tabla CRC32 precalculada
static uint32_t crc32_table[256];
static int crc32_table_initialized = 0;
static void init_crc32_table(void) {
    if (crc32_table_initialized) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = 1;
}
// Directorio por defecto para archivos de datos
static const char *DATA_DIR = ".";
const char* ac_get_data_dir(void) {
    const char *dir = getenv("ACUMULADOR_DATA_DIR");
    return dir ? dir : DATA_DIR;
}
int ac_build_path(char *buf, size_t bufsize, const char *name, const char *ext) {
    const char *dir = ac_get_data_dir();
    int ret = snprintf(buf, bufsize, "%s/%s%s", dir, name, ext);
    if (ret < 0 || (size_t)ret >= bufsize) {
        return -1;
    }
    return 0;
}
int ac_build_meta_path(char *buf, size_t bufsize, const char *name) {
    return ac_build_path(buf, bufsize, name, ".meta");
}
int ac_read_header(const char *filepath, ac_header_t *header) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        return -1;
    }
    size_t read = fread(header, sizeof(ac_header_t), 1, f);
    fclose(f);
    if (read != 1) {
        return -1;
    }
    // Validar magic number
    if (header->magic != AC_MAGIC) {
        return -2;
    }
    return 0;
}
int ac_write_header(const char *filepath, const ac_header_t *header) {
    FILE *f = fopen(filepath, "wb");
    if (!f) {
        return -1;
    }
    size_t written = fwrite(header, sizeof(ac_header_t), 1, f);
    fclose(f);
    if (written != 1) {
        return -1;
    }
    return 0;
}
int ac_read_metadata(const char *name, ac_meta_cache_t *meta) {
    char path[512];
    if (ac_build_meta_path(path, sizeof(path), name) != 0) {
        return -1;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }
    size_t read = fread(meta, sizeof(ac_meta_cache_t), 1, f);
    fclose(f);
    if (read != 1) {
        return -1;
    }
    return 0;
}
int ac_write_metadata(const char *name, const ac_meta_cache_t *meta) {
    char path[512];
    if (ac_build_meta_path(path, sizeof(path), name) != 0) {
        return -1;
    }
    // Escritura atómica usando archivo temporal
    char temp_path[512];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
    FILE *f = fopen(temp_path, "wb");
    if (!f) {
        return -1;
    }
    size_t written = fwrite(meta, sizeof(ac_meta_cache_t), 1, f);
    fclose(f);
    if (written != 1) {
        unlink(temp_path);
        return -1;
    }
    if (rename(temp_path, path) != 0) {
        unlink(temp_path);
        return -1;
    }
    return 0;
}
int ac_validate_file(const char *filepath) {
    ac_header_t header;
    int ret = ac_read_header(filepath, &header);
    return (ret == 0) ? 0 : -1;
}
void* ac_mmap_file(const char *filepath, size_t *size, int *fd, int writable) {
    struct stat st;
    *fd = open(filepath, writable ? O_RDWR : O_RDONLY);
    if (*fd < 0) {
        return MAP_FAILED;
    }
    if (fstat(*fd, &st) < 0) {
        close(*fd);
        return MAP_FAILED;
    }
    *size = st.st_size;
    void *addr = mmap(NULL, *size,
                      writable ? (PROT_READ | PROT_WRITE) : PROT_READ,
                      MAP_SHARED, *fd, 0);
    if (addr == MAP_FAILED) {
        close(*fd);
    }
    return addr;
}
int ac_munmap_file(void *addr, size_t size, int fd) {
    if (addr && addr != MAP_FAILED) {
        munmap(addr, size);
    }
    if (fd >= 0) {
        close(fd);
    }
    return 0;
}
uint32_t ac_simple_checksum(const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum = (sum << 1) ^ bytes[i];
    }
    return sum;
}
uint32_t ac_crc32(const void *data, size_t len) {
    init_crc32_table();
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ bytes[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}
ac_norm_method_t ac_parse_norm_method(const char *str) {
    if (strcasecmp(str, "raw") == 0) return NORM_RAW;
    if (strcasecmp(str, "minmax") == 0) return NORM_MINMAX;
    if (strcasecmp(str, "posmax") == 0) return NORM_POSMAX;
    if (strcasecmp(str, "signed01") == 0) return NORM_SIGNED01;
    if (strcasecmp(str, "sum01") == 0) return NORM_SUM01;
    if (strcasecmp(str, "softmax") == 0) return NORM_SOFTMAX;
    if (strcasecmp(str, "sigmoid") == 0) return NORM_SIGMOID;
    if (strcasecmp(str, "tanh01") == 0) return NORM_TANH01;
    if (strcasecmp(str, "rank") == 0) return NORM_RANK;
    return NORM_RAW; // Default
}
const char* ac_get_norm_method_name(ac_norm_method_t method) {
    switch (method) {
        case NORM_RAW: return "raw";
        case NORM_MINMAX: return "minmax";
        case NORM_POSMAX: return "posmax";
        case NORM_SIGNED01: return "signed01";
        case NORM_SUM01: return "sum01";
        case NORM_SOFTMAX: return "softmax";
        case NORM_SIGMOID: return "sigmoid";
        case NORM_TANH01: return "tanh01";
        case NORM_RANK: return "rank";
        default: return "unknown";
    }
}
static double clamp01(double x) {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}
int ac_normalize_values(const double *input, double *output, uint64_t n,
                        ac_norm_method_t method, double temperature, double scale) {
    if (n == 0) return 0;
    if (method == NORM_RAW) {
        for (uint64_t i = 0; i < n; i++) output[i] = input[i];
        return 0;
    }
    if (method == NORM_MINMAX) {
        double mn = input[0], mx = input[0];
        for (uint64_t i = 1; i < n; i++) {
            if (input[i] < mn) mn = input[i];
            if (input[i] > mx) mx = input[i];
        }
        double rng = mx - mn;
        if (rng == 0) {
            for (uint64_t i = 0; i < n; i++) output[i] = 0.0;
        } else {
            for (uint64_t i = 0; i < n; i++) output[i] = (input[i] - mn) / rng;
        }
        return 0;
    }
    if (method == NORM_POSMAX) {
        double mx = 0.0;
        for (uint64_t i = 0; i < n; i++) {
            double v = input[i] > 0 ? input[i] : 0;
            if (v > mx) mx = v;
        }
        if (mx <= 0) {
            for (uint64_t i = 0; i < n; i++) output[i] = 0.0;
        } else {
            for (uint64_t i = 0; i < n; i++) {
                double v = input[i] > 0 ? input[i] : 0;
                output[i] = v / mx;
            }
        }
        return 0;
    }
    if (method == NORM_SIGNED01) {
        double mx = 0.0;
        for (uint64_t i = 0; i < n; i++) {
            double a = fabs(input[i]);
            if (a > mx) mx = a;
        }
        if (mx <= 0) {
            for (uint64_t i = 0; i < n; i++) output[i] = 0.5;
        } else {
            for (uint64_t i = 0; i < n; i++) output[i] = (input[i] / mx + 1.0) / 2.0;
        }
        return 0;
    }
    if (method == NORM_SUM01) {
        double mn = input[0];
        for (uint64_t i = 1; i < n; i++) if (input[i] < mn) mn = input[i];
        double sum = 0;
        for (uint64_t i = 0; i < n; i++) sum += (input[i] - mn);
        if (sum <= 0) {
            for (uint64_t i = 0; i < n; i++) output[i] = 1.0 / n;
        } else {
            for (uint64_t i = 0; i < n; i++) output[i] = (input[i] - mn) / sum;
        }
        return 0;
    }
    if (method == NORM_SOFTMAX) {
        if (temperature <= 0) { ac_error("temperature must be positive"); return -1; }
        double mx = input[0];
        for (uint64_t i = 1; i < n; i++) if (input[i] > mx) mx = input[i];
        double sum = 0;
        for (uint64_t i = 0; i < n; i++) {
            output[i] = exp((input[i] - mx) / temperature);
            sum += output[i];
        }
        if (sum <= 0) {
            for (uint64_t i = 0; i < n; i++) output[i] = 1.0 / n;
        } else {
            for (uint64_t i = 0; i < n; i++) output[i] /= sum;
        }
        return 0;
    }
    if (method == NORM_SIGMOID) {
        if (scale <= 0) { ac_error("scale must be positive"); return -1; }
        for (uint64_t i = 0; i < n; i++) {
            double z = input[i] / scale;
            if (z >= 0) {
                output[i] = 1.0 / (1.0 + exp(-z));
            } else {
                double e = exp(z);
                output[i] = e / (1.0 + e);
            }
        }
        return 0;
    }
    if (method == NORM_TANH01) {
        if (scale <= 0) { ac_error("scale must be positive"); return -1; }
        for (uint64_t i = 0; i < n; i++) {
            output[i] = (tanh(input[i] / scale) + 1.0) / 2.0;
        }
        return 0;
    }
    if (method == NORM_RANK) {
        if (n == 1) { output[0] = 0.0; return 0; }
        // Crear array de índices para ordenar
        uint64_t *indices = malloc(n * sizeof(uint64_t));
        if (!indices) { ac_error("out of memory"); return -1; }
        for (uint64_t i = 0; i < n; i++) indices[i] = i;
        // Ordenar índices por valor (bubble sort simple para claridad)
        for (uint64_t i = 0; i < n - 1; i++) {
            for (uint64_t j = i + 1; j < n; j++) {
                if (input[indices[i]] > input[indices[j]]) {
                    uint64_t tmp = indices[i];
                    indices[i] = indices[j];
                    indices[j] = tmp;
                }
            }
        }
        // Asignar rangos
        double *ranks = calloc(n, sizeof(double));
        if (!ranks) { free(indices); ac_error("out of memory"); return -1; }
        uint64_t i = 0;
        while (i < n) {
            uint64_t j = i;
            while (j + 1 < n && input[indices[j + 1]] == input[indices[i]]) j++;
            double avg = (i + j) / 2.0;
            for (uint64_t k = i; k <= j; k++) ranks[indices[k]] = avg;
            i = j + 1;
        }
        for (uint64_t i = 0; i < n; i++) output[i] = ranks[i] / (n - 1);
        free(indices);
        free(ranks);
        return 0;
    }
    ac_error("unknown normalization method");
    return -1;
}
int ac_quantize_floats(const double *floats, int32_t *output, uint64_t n,
                       int32_t qmin, int32_t qmax, int renorm_max) {
    if (qmin > qmax) { ac_error("min must be <= max"); return -1; }
    double *vals = malloc(n * sizeof(double));
    if (!vals) { ac_error("out of memory"); return -1; }
    for (uint64_t i = 0; i < n; i++) vals[i] = floats[i];
    if (renorm_max) {
        double mx = vals[0];
        for (uint64_t i = 1; i < n; i++) if (vals[i] > mx) mx = vals[i];
        if (mx > 0) {
            for (uint64_t i = 0; i < n; i++) vals[i] /= mx;
        } else {
            for (uint64_t i = 0; i < n; i++) vals[i] = 0;
        }
    }
    int32_t rng = qmax - qmin;
    for (uint64_t i = 0; i < n; i++) {
        double x = clamp01(vals[i]);
        int32_t q = (int32_t)floor(qmin + x * rng + 0.5);
        if (q < qmin) q = qmin;
        if (q > qmax) q = qmax;
        output[i] = q;
    }
    free(vals);
    return 0;
}
ac_view_format_t ac_parse_view_format(const char *str) {
    if (strcasecmp(str, "matrix") == 0) return VIEW_MATRIX;
    if (strcasecmp(str, "csv") == 0) return VIEW_CSV;
    if (strcasecmp(str, "heatmap") == 0) return VIEW_HEATMAP;
    return VIEW_MATRIX; // Default
}
int64_t ac_parse_index(int64_t idx, int64_t size) {
    if (idx < 0) {
        return size + idx;
    }
    return idx;
}
void ac_error(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    va_end(args);
}
void ac_info(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stdout, msg, args);
    fprintf(stdout, "\n");
    va_end(args);
}
// Funciones para archivos .dat
int ac_read_dat_header(const char *filepath, ac_dat_header_t *header) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return -1;
    size_t rd = fread(header, sizeof(ac_dat_header_t), 1, f);
    fclose(f);
    if (rd != 1) return -1;
    if (header->magic != AC_DAT_MAGIC) return -2;
    if (header->version != AC_VERSION) return -3;
    return 0;
}
int ac_write_dat_header(const char *filepath, const ac_dat_header_t *header) {
    FILE *f = fopen(filepath, "wb");
    if (!f) return -1;
    size_t wr = fwrite(header, sizeof(ac_dat_header_t), 1, f);
    fclose(f);
    return (wr == 1) ? 0 : -1;
}
int ac_read_dat_values(const char *filepath, int64_t **values, uint64_t *n) {
    ac_dat_header_t header;
    if (ac_read_dat_header(filepath, &header) != 0) {
        ac_error("archivo .dat inválido");
        return -1;
    }
    FILE *f = fopen(filepath, "rb");
    if (!f) return -1;
    fseek(f, sizeof(ac_dat_header_t), SEEK_SET);
    *n = header.size;
    *values = malloc(header.size * sizeof(int64_t));
    if (!*values) { fclose(f); ac_error("out of memory"); return -1; }
    size_t rd = fread(*values, sizeof(int64_t), header.size, f);
    fclose(f);
    if (rd != header.size) { free(*values); ac_error("lectura incompleta"); return -1; }
    // Verificar CRC32
    uint32_t calc_crc = ac_crc32(*values, header.size * sizeof(int64_t));
    if (calc_crc != header.crc32) { free(*values); ac_error("CRC32 incorrecto"); return -1; }
    return 0;
}
int ac_write_dat_file(const char *filepath, uint64_t n, const int64_t *values, int force) {
    if (!force && access(filepath, F_OK) == 0) {
        ac_error("el archivo ya existe");
        return -1;
    }
    ac_dat_header_t header;
    header.magic = AC_DAT_MAGIC;
    header.version = AC_VERSION;
    header.size = n;
    header.flags = 0;
    header.crc32 = ac_crc32(values, n * sizeof(int64_t));
    char tmppath[512];
    snprintf(tmppath, sizeof(tmppath), "%s.tmp", filepath);
    FILE *f = fopen(tmppath, "wb");
    if (!f) return -1;
    fwrite(&header, sizeof(ac_dat_header_t), 1, f);
    fwrite(values, sizeof(int64_t), n, f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    if (rename(tmppath, filepath) != 0) { unlink(tmppath); return -1; }
    return 0;
}
// Funciones para archivos .qdat
int ac_read_qdat_header(const char *filepath, ac_qdat_header_t *header) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return -1;
    size_t rd = fread(header, sizeof(ac_qdat_header_t), 1, f);
    fclose(f);
    if (rd != 1) return -1;
    if (header->magic != AC_QDAT_MAGIC) return -2;
    if (header->version != AC_VERSION) return -3;
    return 0;
}
int ac_write_qdat_header(const char *filepath, const ac_qdat_header_t *header) {
    FILE *f = fopen(filepath, "wb");
    if (!f) return -1;
    size_t wr = fwrite(header, sizeof(ac_qdat_header_t), 1, f);
    fclose(f);
    return (wr == 1) ? 0 : -1;
}
int ac_read_qdat_values(const char *filepath, int32_t **values, uint64_t *n,
                        ac_norm_method_t *method, int32_t *qmin, int32_t *qmax) {
    ac_qdat_header_t header;
    if (ac_read_qdat_header(filepath, &header) != 0) {
        ac_error("archivo .qdat inválido");
        return -1;
    }
    FILE *f = fopen(filepath, "rb");
    if (!f) return -1;
    fseek(f, sizeof(ac_qdat_header_t), SEEK_SET);
    *n = header.size;
    *values = malloc(header.size * sizeof(int32_t));
    if (!*values) { fclose(f); ac_error("out of memory"); return -1; }
    size_t rd = fread(*values, sizeof(int32_t), header.size, f);
    fclose(f);
    if (rd != header.size) { free(*values); ac_error("lectura incompleta"); return -1; }
    // Verificar CRC32
    uint32_t calc_crc = ac_crc32(*values, header.size * sizeof(int32_t));
    if (calc_crc != header.crc32) { free(*values); ac_error("CRC32 incorrecto"); return -1; }
    if (method) *method = (ac_norm_method_t)header.method;
    if (qmin) *qmin = header.qmin;
    if (qmax) *qmax = header.qmax;
    return 0;
}
int ac_write_qdat_file(const char *filepath, uint64_t n, ac_norm_method_t method,
                       int32_t qmin, int32_t qmax, const int32_t *values, int force) {
    if (!force && access(filepath, F_OK) == 0) {
        ac_error("el archivo ya existe");
        return -1;
    }
    ac_qdat_header_t header;
    header.magic = AC_QDAT_MAGIC;
    header.version = AC_VERSION;
    header.size = n;
    header.method = (uint32_t)method;
    header.qmin = qmin;
    header.qmax = qmax;
    header.crc32 = ac_crc32(values, n * sizeof(int32_t));
    char tmppath[512];
    snprintf(tmppath, sizeof(tmppath), "%s.tmp", filepath);
    FILE *f = fopen(tmppath, "wb");
    if (!f) return -1;
    fwrite(&header, sizeof(ac_qdat_header_t), 1, f);
    fwrite(values, sizeof(int32_t), n, f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    if (rename(tmppath, filepath) != 0) { unlink(tmppath); return -1; }
    return 0;
}
