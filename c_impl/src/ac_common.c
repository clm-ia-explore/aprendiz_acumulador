/*
 * Aprendiz Acumulador - Implementación de funciones comunes
 */
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
