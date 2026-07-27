/*
 * Aprendiz Acumulador - Implementación del comando VIEW
 * Visualiza el contenido del acumulador en diferentes formatos
 */
#include "ac_common.h"
#include "ac_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
static void print_view_usage(const char *progname) {
    fprintf(stderr, "Uso: %s view <nombre> [formato]\n", progname);
    fprintf(stderr, "\nVisualiza el contenido del acumulador.\n");
    fprintf(stderr, "  nombre   Nombre del acumulador\n");
    fprintf(stderr, "  formato  matrix (por defecto), csv, heatmap\n");
}
// Calcular min y max para normalización
static void find_min_max(double *arr, size_t n, double *min, double *max) {
    *min = arr[0];
    *max = arr[0];
    for (size_t i = 1; i < n; i++) {
        if (arr[i] < *min) *min = arr[i];
        if (arr[i] > *max) *max = arr[i];
    }
}
// Caracteres para heatmap ASCII
static const char *heatmap_chars = " .:-=+*#%@";
static void print_matrix(double *arr, size_t n) {
    printf("Acumulador (%zu elementos):\n", n);
    for (size_t i = 0; i < n; i++) {
        printf("[%4zu] = %12.6g\n", i, arr[i]);
    }
}
static void print_csv(double *arr, size_t n) {
    printf("index,value\n");
    for (size_t i = 0; i < n; i++) {
        printf("%zu,%g\n", i, arr[i]);
    }
}
static void print_heatmap(double *arr, size_t n) {
    double min, max;
    find_min_max(arr, n, &min, &max);
    double range = max - min;
    if (range == 0) range = 1; // Evitar división por cero
    int chars_len = strlen(heatmap_chars) - 1;
    printf("Heatmap (min=%g, max=%g):\n", min, max);
    for (size_t i = 0; i < n; i++) {
        double normalized = (arr[i] - min) / range;
        int idx = (int)(normalized * chars_len);
        if (idx > chars_len) idx = chars_len;
        if (idx < 0) idx = 0;
        printf("%c", heatmap_chars[idx]);
        // Salto de línea cada 80 caracteres para mejor visualización
        if ((i + 1) % 80 == 0) printf("\n");
    }
    printf("\n");
}
int ac_view_main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        print_view_usage(argv[0]);
        return 1;
    }
    const char *name = argv[1];
    ac_view_format_t format = VIEW_MATRIX;
    if (argc == 3) {
        format = ac_parse_view_format(argv[2]);
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
    // Mapear archivo (solo lectura)
    size_t file_size;
    int fd;
    double *data = ac_mmap_file(filepath, &file_size, &fd, 0);
    if (data == MAP_FAILED) {
        ac_error("No se pudo mapear el archivo");
        return 1;
    }
    // Los datos comienzan después de la cabecera
    double *array = (double *)((char *)data + sizeof(ac_header_t));
    // Imprimir según formato
    switch (format) {
        case VIEW_MATRIX:
            print_matrix(array, header.size);
            break;
        case VIEW_CSV:
            print_csv(array, header.size);
            break;
        case VIEW_HEATMAP:
            print_heatmap(array, header.size);
            break;
    }
    // Desmapear
    ac_munmap_file(data, file_size, fd);
    return 0;
}
