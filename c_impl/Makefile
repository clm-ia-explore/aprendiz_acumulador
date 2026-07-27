# Makefile para Aprendiz Acumulador en C
# Soporta versiones dev y release con optimización nivel 3
CC = gcc
CFLAGS_COMMON = -Wall -Wextra -std=c11 -I./include
LDFLAGS = -lm
# Directorios
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include
# Archivos fuente
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
# Nombre del binario
TARGET = acumulador
# Versiones
# dev: sin optimización, con símbolos de debug
# release: optimización O3, sin debug
.PHONY: all dev release clean help

all: dev

dev: CFLAGS = $(CFLAGS_COMMON) -g -O0 -DDEBUG
dev: $(TARGET)
	@echo "Build dev completado: $(TARGET)"

release: CFLAGS = $(CFLAGS_COMMON) -O3 -DNDEBUG
release: $(TARGET)
	@echo "Build release completado: $(TARGET) (optimizado -O3)"

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	rm -f *.bin *.dat *.qdat *.meta
	@echo "Limpieza completada"

help:
	@echo "Makefile para Aprendiz Acumulador (C)"
	@echo ""
	@echo "Uso:"
	@echo "  make          - Compilar versión dev (debug, sin optimizar)"
	@echo "  make dev      - Compilar versión dev (debug, sin optimizar)"
	@echo "  make release  - Compilar versión release (O3, sin debug)"
	@echo "  make clean    - Limpiar archivos compilados"
	@echo "  make help     - Mostrar esta ayuda"
	@echo ""
	@echo "Características:"
	@echo "  - Versión dev: incluye símbolos de debug, sin optimización"
	@echo "  - Versión release: optimización nivel 3 (-O3), máximo rendimiento"
	@echo "  - Arquitectura tipo busybox: un solo binario con múltiples comandos"
