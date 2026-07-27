/*
 * Aprendiz Acumulador - Implementación del comando MEM
 * Muestra información de memoria, memoriza y reconstruye estado del acumulador
 */
#define _POSIX_C_SOURCE 200809L
#include "ac_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#define SHM_HEADER_SIZE 24

static void print_mem_usage(const char *progname) {
    fprintf(stderr, "Uso:\n");
    fprintf(stderr, "  %s mem <nombre>                    # Mostrar información\n", progname);
    fprintf(stderr, "  %s mem memorize <nombre> <archivo.dat> [--force]\n", progname);
    fprintf(stderr, "  %s mem reconstruct <nombre> <archivo.dat> [--force]\n", progname);
}

static int ac_mem_info(const char *name) {
    char filepath[512];
    if (ac_build_path(filepath, sizeof(filepath), name, ".bin") != 0) {
        ac_error("Ruta demasiado larga");
        return 1;
    }
    
    struct stat st;
    if (stat(filepath, &st) != 0) {
        ac_error("El acumulador '%s' no existe", name);
        return 1;
    }
    
    ac_bin_header_t header;
    if (ac_read_header(filepath, &header) != 0) {
        ac_error("El acumulador '%s' es inválido o está corrupto", name);
        return 1;
    }
    
    size_t file_size;
    int fd;
    double *data = ac_mmap_file(filepath, &file_size, &fd, 0);
    if (data == MAP_FAILED) {
        ac_error("No se pudo mapear el archivo");
        return 1;
    }
    
    double *array = (double *)((char *)data + SHM_HEADER_SIZE);
    
    double min = array[0], max = array[0], sum = 0;
    size_t non_zero = 0;
    for (size_t i = 0; i < header.n; i++) {
        if (array[i] < min) min = array[i];
        if (array[i] > max) max = array[i];
        sum += array[i];
        if (array[i] != 0.0) non_zero++;
    }
    
    ac_munmap_file(data, file_size, fd);
    
    ac_meta_cache_t meta;
    int has_meta = (ac_read_metadata(name, &meta) == 0);
    
    printf("=== Información del Acumulador: %s ===\n", name);
    printf("Archivo:           %s\n", filepath);
    printf("Tamaño elementos:  %u\n", header.n);
    printf("Versión formato:   %u\n", header.version);
    printf("\n");
    printf("=== Uso de Memoria ===\n");
    printf("Tamaño en disco:   %ld bytes (%.2f KB)\n", (long)st.st_size, st.st_size / 1024.0);
    printf("Datos (double):    %zu bytes (%.2f KB)\n",
           (size_t)(header.n * sizeof(double)),
           (header.n * sizeof(double)) / 1024.0);
    printf("Cabecera:          %d bytes\n", SHM_HEADER_SIZE);
    printf("Metadatos caché:   %s\n", has_meta ? "disponible" : "no disponible");
    printf("\n");
    printf("=== Estadísticas ===\n");
    printf("Mínimo:            %g\n", min);
    printf("Máximo:            %g\n", max);
    printf("Suma total:        %g\n", sum);
    printf("Elementos no cero: %zu (%.2f%%)\n", non_zero,
           header.n > 0 ? (non_zero * 100.0 / header.n) : 0.0);
    printf("Promedio:          %g\n", header.n > 0 ? sum / header.n : 0.0);
    
    return 0;
}

static int ac_mem_memorize(const char *name, const char *datfile, int force) {
    char binpath[512];
    if (ac_build_path(binpath, sizeof(binpath), name, ".bin") != 0) {
        ac_error("Ruta demasiado larga");
        return 1;
    }
    
    if (ac_validate_file(binpath) != 0) {
        ac_error("El acumulador '%s' no existe", name);
        return 1;
    }
    
    ac_bin_header_t header;
    if (ac_read_header(binpath, &header) != 0) {
        ac_error("No se pudo leer la cabecera del acumulador");
        return 1;
    }
    
    size_t file_size;
    int fd;
    double *data = ac_mmap_file(binpath, &file_size, &fd, 0);
    if (data == MAP_FAILED) {
        ac_error("No se pudo mapear el archivo");
        return 1;
    }
    
    int64_t *values = (int64_t *)((char *)data + SHM_HEADER_SIZE);
    
    if (ac_write_dat_file(datfile, header.n, values, force) != 0) {
        ac_munmap_file(data, file_size, fd);
        return 1;
    }
    
    ac_munmap_file(data, file_size, fd);
    
    ac_info("Memorizado: %s -> %s (%u elementos)", name, datfile, header.n);
    return 0;
}

static int ac_mem_reconstruct(const char *name, const char *datfile, int force) {
    char binpath[512];
    if (ac_build_path(binpath, sizeof(binpath), name, ".bin") != 0) {
        ac_error("Ruta demasiado larga");
        return 1;
    }
    
    if (!force && access(binpath, F_OK) == 0) {
        ac_error("El acumulador '%s' ya existe; use --force", name);
        return 1;
    }
    
    int64_t *values = NULL;
    uint64_t n;
    if (ac_read_dat_values(datfile, &values, &n) != 0) {
        return 1;
    }
    
    ac_bin_header_t header;
    memcpy(header.magic, "ACCSHM01", 8);
    header.version = AC_VERSION;
    header.n = (uint32_t)n;
    header.flags = 0;
    header.reserved2 = 0;
    
    FILE *f = fopen(binpath, "wb");
    if (!f) {
        free(values);
        ac_error("No se pudo crear el archivo '%s'", binpath);
        return 1;
    }
    
    fwrite(&header, sizeof(ac_bin_header_t), 1, f);
    
    for (uint64_t i = 0; i < n; i++) {
        double d = (double)values[i];
        fwrite(&d, sizeof(double), 1, f);
    }
    
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    free(values);
    
    ac_meta_cache_t meta;
    meta.size = n;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    meta.mtime = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    meta.checksum = ac_simple_checksum(&header, sizeof(ac_bin_header_t));
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
        if (argc != 2) {
            print_mem_usage(argv[0]);
            return 1;
        }
        return ac_mem_info(argv[1]);
    }
}
