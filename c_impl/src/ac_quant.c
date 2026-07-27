/*
 * Aprendiz Acumulador - Implementación del comando QUANT
 * Cuantiza los valores del acumulador usando varios métodos de normalización
 */
#include "ac_common.h"
#include "ac_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <unistd.h>
static void print_quant_usage(const char *progname) {
    fprintf(stderr, "Uso:\n");
    fprintf(stderr, "  %s quant <input.dat> <output.qdat> [opciones]\n", progname);
    fprintf(stderr, "\nCuantiza datos normalizados de un archivo .dat a un archivo .qdat.\n");
    fprintf(stderr, "\nOpciones:\n");
    fprintf(stderr, "  --method <m>      Método de normalización (default: minmax)\n");
    fprintf(stderr, "                    raw, minmax, posmax, signed01, sum01,\n");
    fprintf(stderr, "                    softmax, sigmoid, tanh01, rank\n");
    fprintf(stderr, "  --min <n>         Valor mínimo cuantizado (default: 0)\n");
    fprintf(stderr, "  --max <n>         Valor máximo cuantizado (default: 1000)\n");
    fprintf(stderr, "  --temperature <t> Temperatura para softmax (default: 1.0)\n");
    fprintf(stderr, "  --scale <s>       Escala para sigmoid/tanh01 (default: 1.0)\n");
    fprintf(stderr, "  --renorm-max      Renormalizar al máximo después de normalizar\n");
    fprintf(stderr, "  --allow-raw       Permitir método 'raw' (sin normalización)\n");
    fprintf(stderr, "  --force           Sobrescribir archivo de salida si existe\n");
}
int ac_quant_main(int argc, char *argv[]) {
    if (argc < 3) {
        print_quant_usage(argv[0]);
        return 1;
    }
    const char *input = argv[1];
    const char *output = argv[2];
    // Opciones por defecto
    ac_norm_method_t method = NORM_MINMAX;
    int32_t qmin = 0;
    int32_t qmax = 1000;
    double temperature = 1.0;
    double scale = 1.0;
    int renorm_max = 0;
    int allow_raw = 0;
    int force = 0;
    // Parsear argumentos
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--method") == 0 && i + 1 < argc) {
            method = ac_parse_norm_method(argv[++i]);
        } else if (strcmp(argv[i], "--min") == 0 && i + 1 < argc) {
            qmin = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--max") == 0 && i + 1 < argc) {
            qmax = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--temperature") == 0 && i + 1 < argc) {
            temperature = atof(argv[++i]);
        } else if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc) {
            scale = atof(argv[++i]);
        } else if (strcmp(argv[i], "--renorm-max") == 0) {
            renorm_max = 1;
        } else if (strcmp(argv[i], "--allow-raw") == 0) {
            allow_raw = 1;
        } else if (strcmp(argv[i], "--force") == 0) {
            force = 1;
        } else {
            fprintf(stderr, "Opción desconocida: %s\n", argv[i]);
            print_quant_usage(argv[0]);
            return 1;
        }
    }
    // Validar método raw
    if (method == NORM_RAW && !allow_raw) {
        ac_error("El método 'raw' está deshabilitado para cuantización; use --allow-raw");
        return 1;
    }
    // Leer valores desde archivo .dat
    int64_t *values = NULL;
    uint64_t n;
    if (ac_read_dat_values(input, &values, &n) != 0) {
        return 1;
    }
    // Convertir a doubles para normalización
    double *doubles = malloc(n * sizeof(double));
    if (!doubles) {
        free(values);
        ac_error("out of memory");
        return 1;
    }
    for (uint64_t i = 0; i < n; i++) {
        doubles[i] = (double)values[i];
    }
    free(values);
    // Normalizar valores
    double *normalized = malloc(n * sizeof(double));
    if (!normalized) {
        free(doubles);
        ac_error("out of memory");
        return 1;
    }
    if (ac_normalize_values(doubles, normalized, n, method, temperature, scale) != 0) {
        free(doubles);
        free(normalized);
        return 1;
    }
    free(doubles);
    // Cuantizar valores
    int32_t *qvalues = malloc(n * sizeof(int32_t));
    if (!qvalues) {
        free(normalized);
        ac_error("out of memory");
        return 1;
    }
    if (ac_quantize_floats(normalized, qvalues, n, qmin, qmax, renorm_max) != 0) {
        free(normalized);
        free(qvalues);
        return 1;
    }
    free(normalized);
    // Escribir archivo .qdat
    if (ac_write_qdat_file(output, n, method, qmin, qmax, qvalues, force) != 0) {
        free(qvalues);
        return 1;
    }
    free(qvalues);
    ac_info("Cuantizado: %s -> %s (%lu elementos, método: %s)",
            input, output, (unsigned long)n, ac_get_norm_method_name(method));
    return 0;
}
