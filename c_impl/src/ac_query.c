/*
 * Aprendiz Acumulador - Implementación del comando QUERY
 * Consulta el valor de una posición del acumulador
 */
#include "ac_common.h"
#include "ac_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
static void print_query_usage(const char *progname) {
    fprintf(stderr, "Uso: %s query <nombre> <índice>\n", progname);
    fprintf(stderr, "\nConsulta el valor de una posición del acumulador.\n");
    fprintf(stderr, "  nombre   Nombre del acumulador\n");
    fprintf(stderr, "  índice   Posición (puede ser negativo para contar desde el final)\n");
}
int ac_query_main(int argc, char *argv[]) {
    if (argc != 3) {
        print_query_usage(argv[0]);
        return 1;
    }
    const char *name = argv[1];
    char *endptr;
    long idx = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0') {
        ac_error("Índice inválido");
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
    ac_bin_header_t header;
    if (ac_read_header(filepath, &header) != 0) {
        ac_error("No se pudo leer la cabecera");
        return 1;
    }
    // Validar índice
    int64_t actual_idx = ac_parse_index(idx, (int64_t)header.n);
    if (actual_idx < 0 || actual_idx >= (int64_t)header.n) {
        ac_error("Índice fuera de rango (%ld no está en [0, %lu])", idx, (unsigned long)(header.n - 1));
        return 1;
    }
    // Mapear archivo (solo lectura)
    size_t file_size;
    int fd;
    double *data = ac_mmap_file(filepath, &file_size, &fd, 0);
    if (data == MAP_FAILED) {
        ac_error("No se pudo mapear el archivo");
        return 1;
    }
    // Los datos comienzan después de la cabecera
    double *array = (double *)((char *)data + sizeof(ac_bin_header_t));
    // Leer valor
    double value = array[actual_idx];
    // Desmapear
    ac_munmap_file(data, file_size, fd);
    printf("%g\n", value);
    return 0;
}
