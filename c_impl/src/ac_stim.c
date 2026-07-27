/*
 * Aprendiz Acumulador - Implementación del comando STIM
 * Estimula una posición del acumulador con un valor
 */
#include "ac_common.h"
#include "ac_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/mman.h>
static void print_stim_usage(const char *progname) {
    fprintf(stderr, "Uso: %s stim <nombre> <índice> <valor>\n", progname);
    fprintf(stderr, "\nAñade un valor a una posición del acumulador.\n");
    fprintf(stderr, "  nombre   Nombre del acumulador\n");
    fprintf(stderr, "  índice   Posición (puede ser negativo para contar desde el final)\n");
    fprintf(stderr, "  valor    Valor a añadir (float/double)\n");
}
int ac_stim_main(int argc, char *argv[]) {
    if (argc != 4) {
        print_stim_usage(argv[0]);
        return 1;
    }
    const char *name = argv[1];
    char *endptr;
    long idx = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0') {
        ac_error("Índice inválido");
        return 1;
    }
    double value = strtod(argv[3], &endptr);
    if (*endptr != '\0') {
        ac_error("Valor inválido");
        return 1;
    }
    // Construir ruta
    char filepath[512];
    if (ac_build_path(filepath, sizeof(filepath), name, ".bin") != 0) {
        ac_error("Ruta demasiado larga");
        return 1;
    }
    // Validar archivo
    if (ac_validate_file(filepath) != 0) {
        ac_error("El acumulador '%s' no existe o es inválido", name);
        return 1;
    }
    // Leer cabecera
    ac_header_t header;
    if (ac_read_header(filepath, &header) != 0) {
        ac_error("No se pudo leer la cabecera");
        return 1;
    }
    // Validar índice
    int64_t actual_idx = ac_parse_index(idx, (int64_t)header.size);
    if (actual_idx < 0 || actual_idx >= (int64_t)header.size) {
        ac_error("Índice fuera de rango (%ld no está en [0, %lu])", idx, (unsigned long)(header.size - 1));
        return 1;
    }
    // Mapear archivo
    size_t file_size;
    int fd;
    void *mapped = ac_mmap_file(filepath, &file_size, &fd, 1);
    if (mapped == MAP_FAILED) {
        ac_error("No se pudo mapear el archivo");
        return 1;
    }
    // Los datos comienzan después de la cabecera
    double *array = (double *)((char *)mapped + sizeof(ac_header_t));
    // Añadir valor
    array[actual_idx] += value;
    double new_value = array[actual_idx];  // Guardar el valor antes de desmapear
    // Sincronizar y desmapear
    msync(mapped, file_size, MS_SYNC);
    ac_munmap_file(mapped, file_size, fd);
    // Actualizar metadatos
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t mtime = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    ac_meta_cache_t meta;
    meta.size = header.size;
    meta.mtime = mtime;
    meta.checksum = ac_simple_checksum(&header, sizeof(ac_header_t));
    ac_write_metadata(name, &meta);
    ac_info("Estimulación aplicada: [%ld] += %g → %g", idx, value, new_value);
    return 0;
}
