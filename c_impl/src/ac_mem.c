/*
 * Aprendiz Acumulador - Implementación del comando MEM
 * Muestra información de memoria y estadísticas del acumulador
 */
#include "ac_common.h"
#include "ac_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
static void print_mem_usage(const char *progname) {
    fprintf(stderr, "Uso: %s mem <nombre>\n", progname);
    fprintf(stderr, "\nMuestra información de memoria y estadísticas del acumulador.\n");
    fprintf(stderr, "  nombre   Nombre del acumulador\n");
}
int ac_mem_main(int argc, char *argv[]) {
    if (argc != 2) {
        print_mem_usage(argv[0]);
        return 1;
    }
    const char *name = argv[1];
    // Construir ruta
    char filepath[512];
    if (ac_build_path(filepath, sizeof(filepath), name, ".bin") != 0) {
        ac_error("Ruta demasiado larga");
        return 1;
    }
    // Obtener información del archivo
    struct stat st;
    if (stat(filepath, &st) != 0) {
        ac_error("El acumulador '%s' no existe", name);
        return 1;
    }
    // Leer cabecera
    ac_header_t header;
    if (ac_read_header(filepath, &header) != 0) {
        ac_error("El acumulador '%s' es inválido o está corrupto", name);
        return 1;
    }
    // Calcular estadísticas básicas mapeando el archivo
    size_t file_size;
    int fd;
    double *data = ac_mmap_file(filepath, &file_size, &fd, 0);
    if (data == MAP_FAILED) {
        ac_error("No se pudo mapear el archivo");
        return 1;
    }
    double *array = (double *)((char *)data + sizeof(ac_header_t));
    // Calcular min, max, sum
    double min = array[0], max = array[0], sum = 0;
    size_t non_zero = 0;
    for (size_t i = 0; i < header.size; i++) {
        if (array[i] < min) min = array[i];
        if (array[i] > max) max = array[i];
        sum += array[i];
        if (array[i] != 0.0) non_zero++;
    }
    ac_munmap_file(data, file_size, fd);
    // Intentar leer metadatos en caché
    ac_meta_cache_t meta;
    int has_meta = (ac_read_metadata(name, &meta) == 0);
    // Imprimir información
    printf("=== Información del Acumulador: %s ===\n", name);
    printf("Archivo:           %s\n", filepath);
    printf("Tamaño elementos:  %lu\n", (unsigned long)header.size);
    printf("Versión formato:   %u\n", header.version);
    printf("\n");
    printf("=== Uso de Memoria ===\n");
    printf("Tamaño en disco:   %ld bytes (%.2f KB)\n", (long)st.st_size, st.st_size / 1024.0);
    printf("Datos (double):    %zu bytes (%.2f KB)\n",
           (size_t)(header.size * sizeof(double)),
           (header.size * sizeof(double)) / 1024.0);
    printf("Cabecera:          %zu bytes\n", sizeof(ac_header_t));
    printf("Metadatos caché:   %s\n", has_meta ? "disponible" : "no disponible");
    printf("\n");
    printf("=== Estadísticas ===\n");
    printf("Mínimo:            %g\n", min);
    printf("Máximo:            %g\n", max);
    printf("Suma total:        %g\n", sum);
    printf("Elementos no cero: %zu (%.2f%%)\n", non_zero,
           header.size > 0 ? (non_zero * 100.0 / header.size) : 0.0);
    printf("Promedio:          %g\n", header.size > 0 ? sum / header.size : 0.0);
    return 0;
}
