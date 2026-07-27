/*
 * Aprendiz Acumulador - Implementación del comando INIT
 * Inicializa un nuevo acumulador con tamaño especificado
 */
#include "ac_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

static void print_init_usage(const char *progname) {
    fprintf(stderr, "Uso: %s init <nombre> <tamaño>\n", progname);
    fprintf(stderr, "\nCrea un nuevo acumulador con el tamaño especificado.\n");
    fprintf(stderr, "  nombre   Nombre del acumulador (sin extensión)\n");
    fprintf(stderr, "  tamaño   Número de elementos (entero positivo)\n");
}

int ac_init_main(int argc, char *argv[]) {
    if (argc != 3) {
        print_init_usage(argv[0]);
        return 1;
    }
    
    const char *name = argv[1];
    char *endptr;
    long size = strtol(argv[2], &endptr, 10);
    
    if (*endptr != '\0' || size <= 0) {
        ac_error("Tamaño inválido: debe ser un entero positivo");
        return 1;
    }
    
    if (strlen(name) == 0 || strlen(name) >= AC_MAX_NAME_LEN) {
        ac_error("Nombre inválido: debe tener entre 1 y %d caracteres", AC_MAX_NAME_LEN - 1);
        return 1;
    }
    
    char filepath[512];
    if (ac_build_path(filepath, sizeof(filepath), name, ".bin") != 0) {
        ac_error("Ruta demasiado larga");
        return 1;
    }
    
    if (access(filepath, F_OK) == 0) {
        ac_error("El acumulador '%s' ya existe", name);
        return 1;
    }
    
    // Crear cabecera compatible con Python (24 bytes)
    ac_bin_header_t header;
    memcpy(header.magic, "ACCSHM01", 8);
    header.version = AC_VERSION;
    header.n = (uint32_t)size;
    header.flags = 0;
    header.reserved2 = 0;
    
    FILE *f = fopen(filepath, "wb");
    if (!f) {
        ac_error("No se pudo crear el archivo '%s': %s", filepath, strerror(errno));
        return 1;
    }
    
    // Escribir cabecera (24 bytes)
    if (fwrite(&header, sizeof(ac_bin_header_t), 1, f) != 1) {
        ac_error("Error escribiendo cabecera");
        fclose(f);
        unlink(filepath);
        return 1;
    }
    
    // Escribir datos inicializados a 0 (double array)
    size_t data_size = sizeof(double) * (size_t)size;
    double *zeros = calloc((size_t)size, sizeof(double));
    if (!zeros) {
        ac_error("No hay memoria suficiente para %ld elementos", size);
        fclose(f);
        unlink(filepath);
        return 1;
    }
    
    if (fwrite(zeros, sizeof(double), (size_t)size, f) != (size_t)size) {
        ac_error("Error escribiendo datos");
        free(zeros);
        fclose(f);
        unlink(filepath);
        return 1;
    }
    
    free(zeros);
    fclose(f);
    
    // Escribir metadatos en caché
    ac_meta_cache_t meta;
    meta.size = (uint64_t)size;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    meta.mtime = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    meta.checksum = ac_simple_checksum(&header, sizeof(ac_bin_header_t));
    
    if (ac_write_metadata(name, &meta) != 0) {
        ac_info("Nota: No se pudo escribir caché de metadatos (no crítico)");
    }
    
    ac_info("Acumulador '%s' creado exitosamente con %ld elementos", name, size);
    ac_info("Archivo: %s", filepath);
    ac_info("Tamaño en disco: ~%zu bytes", sizeof(ac_bin_header_t) + data_size);
    
    return 0;
}
