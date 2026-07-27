# Aprendiz Acumulador

Sistema de acumulación de valores en memoria compartida con capacidades de normalización, visualización y cuantización.

## Descripción General

**Aprendiz Acumulador** es un conjunto de herramientas de línea de comandos escritas en Python que permiten gestionar acumuladores de valores enteros de 64 bits almacenados en memoria compartida del sistema. El software proporciona mecanismos para:

- Inicializar espacios de memoria compartida para almacenamiento de valores
- Aplicar estímulos (incrementos/decrementos) a posiciones específicas
- Visualizar los valores acumulados en diversos formatos (texto, matriz, mapa de calor)
- Normalizar valores usando múltiples métodos matemáticos
- Persistir y recuperar estados en archivos binarios
- Cuantizar valores normalizados a rangos personalizados

Este sistema está diseñado para escenarios donde se requiere acumular señales, pesos o puntuaciones de forma eficiente, con capacidad de visualización en tiempo real y persistencia de estado.

## Licencia

Este software es de **dominio público** (Unlicense). Véase el archivo `LICENSE` para más detalles.

## Arquitectura

El sistema consta de los siguientes componentes principales:

### Archivos Principales

| Archivo | Propósito |
|---------|-----------|
| `acc_init.py` | Inicializa un nuevo runtime en memoria compartida |
| `acc_stim.py` | Aplica estímulos (valores) a índices específicos |
| `acc_query.py` | Consulta valores con filtrado, ordenamiento y formato |
| `acc_view.py` | Visualiza valores como matriz o valor individual |
| `acc_mem.py` | Memoriza (guarda) y reconstruye (carga) estados |
| `acc_quant.py` | Cuantiza datos normalizados a rangos enteros |
| `acc_common.py` | Funciones y utilidades compartidas |

### Formatos de Archivo

El sistema utiliza tres tipos de archivos binarios:

1. **Runtime (.bin)**: Almacenado en `/dev/shm/` (o directorio configurado via `ACC_SHM_DIR`)
   - Magic: `ACCSHM01`
   - Contiene header + array de int64 en little-endian

2. **Dat (.dat)**: Archivo de persistencia en disco
   - Magic: `ACCDAT01`
   - Contiene header + array de int64 + CRC32

3. **Qdat (.qdat)**: Archivo cuantizado
   - Magic: `ACCQDAT1`
   - Contiene header con método, qmin, qmax + array de int32 + CRC32

## Instalación

No requiere instalación. Los scripts son ejecutables directamente con Python 3.

```bash
chmod +x acc_*.py
```

Asegúrese de tener Python 3.6+ y permisos de escritura en `/dev/shm` (o configure `ACC_SHM_DIR`).

## Uso Básico

### 1. Inicializar un Acumulador

Crea un nuevo espacio de memoria compartida con N posiciones:

```bash
./acc_init.py <nombre> <tamaño> [--force]
```

Ejemplo:
```bash
./acc_init.py demo 16
```

### 2. Aplicar Estímulos

Incrementa o decrementa valores en índices específicos:

```bash
./acc_stim.py <nombre> <índice> <valor> [--quiet]
```

Ejemplo:
```bash
./acc_stim.py demo 0 20      # Suma 20 al índice 0
./acc_stim.py demo 5 8       # Suma 8 al índice 5
./acc_stim.py demo 10 -4     # Resta 4 del índice 10
```

### 3. Visualizar Valores

#### Ver como matriz:
```bash
./acc_view.py <nombre> [opciones]
```

#### Ver valor individual:
```bash
./acc_view.py <nombre> --index <índice> [opciones]
```

### 4. Guardar y Cargar Estados

```bash
# Guardar estado actual a archivo
./acc_mem.py memorize <nombre> <archivo.dat> [--force]

# Cargar estado desde archivo
./acc_mem.py reconstruct <nombre> <archivo.dat> [--force]
```

### 5. Cuantizar Datos

```bash
./acc_quant.py <entrada.dat> <salida.qdat> [opciones]
```

## Métodos de Normalización

El sistema soporta los siguientes métodos de normalización:

| Método | Descripción | Parámetros |
|--------|-------------|------------|
| `raw` | Valores crudos sin normalizar | - |
| `minmax` | Normaliza a [0,1] usando (x-min)/(max-min) | - |
| `posmax` | Solo valores positivos, normaliza por máximo | - |
| `signed01` | Mapea [-max,+max] a [0,1] | - |
| `sum01` | Normaliza por suma (desplazando a positivos) | - |
| `softmax` | Función softmax con temperatura | `--temperature` |
| `sigmoid` | Función sigmoide logística | `--scale` |
| `tanh01` | Tangente hiperbólica mapeada a [0,1] | `--scale` |
| `rank` | Normalización por rango (promedio para empates) | - |

## Opciones de Formato de Salida

| Formato | Descripción | Uso |
|---------|-------------|-----|
| `float` | Valores en punto flotante | Default en query |
| `int` | Enteros (0-100 para normalizados) | Default en view |
| `heat` | Mapa de calor con caracteres | Visualización matricial |
| `csv` | Valores separados por comas | Exportación de datos |

## Ejemplos Detallados

### Ejemplo 1: Flujo Básico

```bash
# Inicializar acumulador de 16 posiciones
./acc_init.py demo 16

# Aplicar varios estímulos
./acc_stim.py demo 0 20
./acc_stim.py demo 5 8
./acc_stim.py demo 10 -4
./acc_stim.py demo 15 40

# Ver como matriz con normalización minmax
./acc_view.py demo --method minmax --format int

# Ver con sigmoid
./acc_view.py demo --method sigmoid --scale 10 --format int

# Ver con softmax
./acc_view.py demo --method softmax --temperature 0.5 --format float

# Ver como mapa de calor
./acc_view.py demo --method posmax --format heat

# Guardar estado
./acc_mem.py memorize demo demo.dat --force

# Cuantizar a 0-255
./acc_quant.py demo.dat demo.qdat --method minmax --min 0 --max 255 --force
```

### Ejemplo 2: Visualización en Tiempo Real

Para monitoreo continuo con actualización automática:

```bash
while true; do 
    clear
    ./acc_view.py demo --method rank --format heat --heat-chars "0123456789"
    sleep 3
done
```

Caracteres recomendados para mapas de calor:

```bash
--heat-chars " ▁▂▃▄▅▆▇█"           # Bloques ASCII
--heat-chars " ▏▎▍▌▋▊▉█"           # Bloques verticales
--heat-chars " ⠁⠃⠇⠏⠟⠿⡿⣿"       # Braille
--heat-chars ".:-=+*#%@"           # Caracteres simples
```

### Ejemplo 3: Consultas Avanzadas

```bash
# Obtener top 5 valores
./acc_query.py demo --k 5 --order desc

# Filtrar por rango
./acc_query.py demo --min 0.5 --max 1.0

# Solo índices
./acc_query.py demo --indices-only --k 10

# Solo valores
./acc_query.py demo --values-only --format float

# Exportar a CSV
./acc_query.py demo --format csv > salida.csv
```

### Ejemplo 4: Matrices Rectangulares

```bash
# Para tamaños no cuadrados, usar --allow-rect
./acc_view.py test --allow-rect --cols 5

# Ver como CSV
./acc_view.py test --format csv --allow-rect --cols 5
```

## Variables de Entorno

| Variable | Descripción | Default |
|----------|-------------|---------|
| `ACC_SHM_DIR` | Directorio para archivos de memoria compartida | `/dev/shm` |

## Casos de Uso

### 1. Sistema de Puntuación Acumulativa

Acumular puntos de múltiples fuentes y visualizar rankings:

```bash
./acc_init.py scores 100

# Cada vez que un usuario realiza una acción
./acc_stim.py scores <user_id> <puntos>

# Ver ranking
./acc_query.py scores --k 10 --order desc --format int
```

### 2. Mapa de Calor de Actividad

Representar actividad en una cuadrícula:

```bash
./acc_init.py heatmap 64  # 8x8

# Actualizar celdas según actividad
./acc_stim.py heatmap <celda> <intensidad>

# Visualizar
./acc_view.py heatmap --method rank --format heat --heat-chars " ▁▂▃▄▅▆▇█"
```

### 3. Pesos de Red Neuronal Simple

Almacenar y ajustar pesos:

```bash
./acc_init.py weights 256

# Ajustar pesos con gradientes
./acc_stim.py weights <idx> <gradiente>

# Exportar para análisis
./acc_mem.py memorize weights pesos.dat --force
./acc_quant.py pesos.dat pesos.qdat --method signed01 --min -127 --max 127
```

### 4. Sistema de Votación

Acumular votos y determinar ganadores:

```bash
./acc_init.py election 10

# Cada voto cuenta
./acc_stim.py election <candidato_id> 1

# Ver resultados
./acc_query.py election --order desc --format int
```

## Referencias Técnicas

### Estructura de Headers

**SHM Header (24 bytes):**
```
magic:      8 bytes  ("ACCSHM01")
version:    4 bytes  (uint32)
n:          4 bytes  (uint32, número de valores)
flags:      4 bytes  (uint32, reservado)
padding:    4 bytes
```

**DAT Header (24 bytes):**
```
magic:      8 bytes  ("ACCDAT01")
version:    4 bytes
n:          4 bytes
flags:      4 bytes
crc32:      4 bytes
```

**QDAT Header (32 bytes):**
```
magic:      8 bytes  ("ACCQDAT1")
version:    4 bytes
n:          4 bytes
method:     4 bytes  (código de método)
qmin:       4 bytes  (int32)
qmax:       4 bytes  (int32)
crc32:      4 bytes
```

### Códigos de Método

| Código | Método |
|--------|--------|
| 0 | raw |
| 1 | minmax |
| 2 | posmax |
| 3 | signed01 |
| 4 | sum01 |
| 5 | softmax |
| 6 | sigmoid |
| 7 | tanh01 |
| 8 | rank |

## Tests y Ejemplos

El directorio `test/` contiene scripts de ejemplo:

- `test/01.sh`: Demostración completa de todas las funcionalidades
- `test/02.sh`: Ejemplo con matriz de 25 elementos
- `test/03/`: Configuración para evmux con 64 posiciones
- `test/04/`: Configuración para cámara con visualización continua

Ejecutar tests:
```bash
cd test
bash 01.sh
```

## Limitaciones

- Tamaño máximo de acumulador: 2^32 - 1 elementos
- Valores acumulados: int64 (-2^63 a 2^63-1)
- Valores cuantizados: int32 (-2^31 a 2^31-1)
- Requiere sistema POSIX con soporte para memoria compartida

## Autor

Software liberado al dominio público.
