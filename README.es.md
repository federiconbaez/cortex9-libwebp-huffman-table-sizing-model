# cortex9-libwebp-huffman-table-sizing-model

Modelo de solo tamaños, en C, de **CVE-2023-4863**: desbordamiento de las
tablas Huffman de libwebp, corregido en 902bc91. Educativo, offline y
determinista. **Cuenta entradas**: nunca asigna ni escribe una tabla, no
lee imágenes, no construye ningún archivo WebP y no enlaza libwebp.

## El fallo

- **Antes del fix:**
  - `kTableSize` suma máximos calculados con `enough.c` para códigos
    **completos** (630, 410, 654);
  - `BuildHuffmanTable` escribe las tablas de segundo nivel a medida que
    llegan los códigos y solo al final comprueba si el árbol está lleno;
  - un código **incompleto** hace que `NextTableBitSize` llegue a la
    longitud 15, escribe más de lo que permite la cota y después devuelve 0.
- **El fix:** primero hace una pasada de conteo sin escribir. Rechaza los
  códigos no llenos antes de escribir y asigna exactamente lo necesario.

**Corrección de la hipótesis:** la causa exacta es *escribir antes de
comprobar que el árbol está lleno*. La tabla de longitudes `table[128]` no
es el buffer que desborda: sus longitudes son de 3 bits (≤ 7) y nunca abre
tablas de segundo nivel. El modelo lo comprueba como control.

## Resultados

- **Caso encontrado por búsqueda (`w05`):** código incompleto de 256
  símbolos. Antes del fix escribe **730** entradas frente a la cota de 630;
  el fix lo rechaza antes de escribir.
- **Alfabeto de distancias (40 símbolos):** la búsqueda encontró códigos
  incompletos con **512** entradas frente a 410.
- **Códigos completos:** los encontrados quedan dentro de la cota.
- **Tabla de longitudes:** nunca supera 128.

`scripts/derive_expected.py` es un **segundo port independiente** en
Python. Detectó un error del port en C (faltaba `offset[len]++`), que se
corrigió.

Ver [LIMITATIONS.md](LIMITATIONS.md), [SOURCES.md](SOURCES.md) y
[PRACTICE.md](PRACTICE.md). Licencia: Apache-2.0.
