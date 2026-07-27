/*
 * Aprendiz Acumulador - Implementación en C
 * Archivo principal tipo BusyBox
 *
 * Uso:
 *   ./acumulador init <nombre> <tamaño>
 *   ./acumulador stim <nombre> <índice> <valor>
 *   ./acumulador query <nombre> <índice>
 *   ./acumulador view <nombre> [formato]
 *   ./acumulador mem <nombre>
 *   ./acumulador quant <nombre> <min> <max>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include "ac_common.h"
#include "ac_init.h"
#include "ac_commands.h"
static void print_usage(const char *progname) {
    fprintf(stderr, "Uso: %s <comando> [argumentos...]\n", progname);
    fprintf(stderr, "\nComandos disponibles:\n");
    fprintf(stderr, "  init <nombre> <tamaño>       Inicializar acumulador\n");
    fprintf(stderr, "  stim <nombre> <índice> <valor>  Estimular posición\n");
    fprintf(stderr, "  query <nombre> <índice>      Consultar valor\n");
    fprintf(stderr, "  view <nombre> [formato]      Visualizar contenido\n");
    fprintf(stderr, "  mem <nombre>                 Información de memoria\n");
    fprintf(stderr, "  quant <nombre> <min> <max>   Cuantizar valores\n");
    fprintf(stderr, "\nFormatos de view: matrix, csv, heatmap\n");
}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    const char *cmd = argv[1];
    // Desplazar argumentos para que cada comando reciba sus propios args
    argc--;
    argv++;
    if (strcmp(cmd, "init") == 0) {
        return ac_init_main(argc, argv);
    } else if (strcmp(cmd, "stim") == 0) {
        return ac_stim_main(argc, argv);
    } else if (strcmp(cmd, "query") == 0) {
        return ac_query_main(argc, argv);
    } else if (strcmp(cmd, "view") == 0) {
        return ac_view_main(argc, argv);
    } else if (strcmp(cmd, "mem") == 0) {
        return ac_mem_main(argc, argv);
    } else if (strcmp(cmd, "quant") == 0) {
        return ac_quant_main(argc, argv);
    } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    } else {
        fprintf(stderr, "Error: Comando desconocido '%s'\n", cmd);
        print_usage(argv[0]);
        return 1;
    }
}
