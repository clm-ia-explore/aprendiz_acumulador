/*
 * Aprendiz Acumulador - Implementación del comando QUANT
 * Cuantiza los valores del acumulador a un rango especificado
 */
#include "ac_common.h"
#include "ac_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
static void print_quant_usage(const char *progname) {
    fprintf(stderr, "Uso: %s quant <nombre> <min> <max>\n", progname);
    fprintf(stderr, "\nCuantiza los valores del acumulador al rango [min, max].\n");
    fprintf(stderr, "  nombre   Nombre del acumulador\n");
    fprintf(stderr, "  min      Valor mínimo del rango de salida\n");
    fprintf(stderr, "  max      Valor máximo del rango de salida\n");
}
int ac_quant_main(int argc, char *argv[]) {
    if (argc != 4) {
        print_quant_usage(argv[0]);
        return 1;
    }
    const char *name = argv[1];
    char *endptr;
    double qmin = strtod(argv[2], &endptr);
    if (*endptr != '\0') {
        ac_error("Valor mínimo inválido");
        return 1;
    }
    double qmax = strtod(argv[3], &endptr);
    if (*endptr != '\0') {
        ac_error("Valor máximo inválido");
        return 1;
    }
    if (qmin >= qmax) {
        ac_error("El mínimo debe ser menor que el máximo");
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
    // Mapear archivo
    size_t file_size;
    int fd;
    double *data = ac_mmap_file(filepath, &file_size, &fd, 1);
    if (data == MAP_FAILED) {
        ac_error("No se pudo mapear el archivo");
        return 1;
    }
    // Los datos comienzan después de la cabecera
    double *array = (double *)((char *)data + sizeof(ac_header_t));
    // Encontrar min y max actuales
    double cur_min = array[0], cur_max = array[0];
    for (size_t i = 1; i < header.size; i++) {
        if (array[i] < cur_min) cur_min = array[i];
        if (array[i] > cur_max) cur_max = array[i];
    }
    double cur_range = cur_max - cur_min;
    // Cuantizar valores
    size_t changed = 0;
    for (size_t i = 0; i < header.size; i++) {
        double old_val = array[i];
        if (cur_range == 0) {
            // Todos los valores son iguales, asignar punto medio
            array[i] = (qmin + qmax) / 2.0;
        } else {
            // Normalizar a [0, 1] y escalar a [qmin, qmax]
            double normalized = (array[i] - cur_min) / cur_range;
            array[i] = qmin + normalized * (qmax - qmin);
        }
        if (old_val != array[i]) changed++;
    }
    // Sincronizar y desmapear
    msync(array, sizeof(double) * header.size, MS_SYNC);
    ac_munmap_file(data, file_size, fd);
    ac_info("Cuantización completada: [%g, %g] → [%g, %g]",
            cur_min, cur_max, qmin, qmax);
    ac_info("Elementos modificados: %zu de %lu", changed, (unsigned long)header.size);
    return 0;
}
