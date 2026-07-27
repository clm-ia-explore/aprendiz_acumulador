/*
 * Aprendiz Acumulador - Implementación del comando MEM
 * Muestra información de memoria, memoriza y reconstruye estado del acumulador
 */
#define _POSIX_C_SOURCE 200809L
#include "ac_common.h"
#include "ac_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
static void print_mem_usage(const char *progname) {
    fprintf(stderr, "Uso:\n");
    fprintf(stderr, "  %s mem <nombre>                    # Mostrar información\n", progname);
    fprintf(stderr, "  %s mem memorize <nombre> <archivo.dat> [--force]\n", progname);
    fprintf(stderr, "  %s mem reconstruct <nombre> <archivo.dat> [--force]\n", progname);
}
static int ac_mem_info(const char *name) {
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
static int ac_mem_memorize(const char *name, const char *datfile, int force) {
    // Construir ruta del runtime
    char binpath[512];
    if (ac_build_path(binpath, sizeof(binpath), name, ".bin") != 0) {
        ac_error("Ruta demasiado larga");
        return 1;
    }
    // Verificar que el runtime existe
    if (ac_validate_file(binpath) != 0) {
        ac_error("El acumulador '%s' no existe", name);
        return 1;
    }
    // Leer cabecera del runtime
    ac_header_t header;
    if (ac_read_header(binpath, &header) != 0) {
        ac_error("No se pudo leer la cabecera del acumulador");
        return 1;
    }
    // Mapear archivo para leer valores
    size_t file_size;
    int fd;
    double *data = ac_mmap_file(binpath, &file_size, &fd, 0);
    if (data == MAP_FAILED) {
        ac_error("No se pudo mapear el archivo");
        return 1;
    }
    int64_t *values = (int64_t *)((char *)data + sizeof(ac_header_t));
    // Escribir archivo .dat
    if (ac_write_dat_file(datfile, header.size, values, force) != 0) {
        ac_munmap_file(data, file_size, fd);
        return 1;
    }
    ac_munmap_file(data, file_size, fd);
    ac_info("Memorizado: %s -> %s (%lu elementos)", name, datfile, (unsigned long)header.size);
    return 0;
}
static int ac_mem_reconstruct(const char *name, const char *datfile, int force) {
    // Construir ruta del runtime
    char binpath[512];
    if (ac_build_path(binpath, sizeof(binpath), name, ".bin") != 0) {
        ac_error("Ruta demasiado larga");
        return 1;
    }
    // Verificar si ya existe
    if (!force && access(binpath, F_OK) == 0) {
        ac_error("El acumulador '%s' ya existe; use --force", name);
        return 1;
    }
    // Leer valores desde .dat
    int64_t *values = NULL;
    uint64_t n;
    if (ac_read_dat_values(datfile, &values, &n) != 0) {
        return 1;
    }
    // Crear archivo .bin con cabecera
    ac_header_t header;
    header.magic = AC_MAGIC;
    header.version = AC_VERSION;
    header.size = n;
    header.timestamp = 0;
    memset(header.reserved, 0, sizeof(header.reserved));
    // Escribir cabecera
    FILE *f = fopen(binpath, "wb");
    if (!f) {
        free(values);
        ac_error("No se pudo crear el archivo '%s'", binpath);
        return 1;
    }
    fwrite(&header, sizeof(ac_header_t), 1, f);
    // Escribir valores como doubles
    for (uint64_t i = 0; i < n; i++) {
        double d = (double)values[i];
        fwrite(&d, sizeof(double), 1, f);
    }
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    free(values);
    // Escribir metadatos
    ac_meta_cache_t meta;
    meta.size = n;
    meta.mtime = 0;
    meta.checksum = ac_simple_checksum(&header, sizeof(header));
    ac_write_metadata(name, &meta);
    ac_info("Reconstruido: %s <- %s (%lu elementos)", name, datfile, (unsigned long)n);
    return 0;
}
int ac_mem_main(int argc, char *argv[]) {
    if (argc < 2) {
        print_mem_usage(argv[0]);
        return 1;
    }
    const char *subcmd = argv[1];
    if (strcmp(subcmd, "memorize") == 0) {
        if (argc != 4 && argc != 5) {
            fprintf(stderr, "Uso: %s mem memorize <nombre> <archivo.dat> [--force]\n", argv[0]);
            return 1;
        }
        int force = (argc == 5 && strcmp(argv[4], "--force") == 0);
        return ac_mem_memorize(argv[2], argv[3], force);
    } else if (strcmp(subcmd, "reconstruct") == 0) {
        if (argc != 4 && argc != 5) {
            fprintf(stderr, "Uso: %s mem reconstruct <nombre> <archivo.dat> [--force]\n", argv[0]);
            return 1;
        }
        int force = (argc == 5 && strcmp(argv[4], "--force") == 0);
        return ac_mem_reconstruct(argv[2], argv[3], force);
    } else {
        // Sin subcomando, mostrar información
        if (argc != 2) {
            print_mem_usage(argv[0]);
            return 1;
        }
        return ac_mem_info(argv[1]);
    }
}
