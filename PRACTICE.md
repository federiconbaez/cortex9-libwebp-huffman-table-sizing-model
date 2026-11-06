# Guía práctica (8 niveles)

Ejercicios **offline y locales** sobre histogramas de longitudes de código.
No se construye ninguna imagen ni se escribe ninguna tabla. Cada respuesta
está comprobada en `tests/test_model.c` (`practice_level_N`), que se
compila con ASan/UBSan.

Comandos: `sh scripts/build.sh test`; `./build/huffsize case <alfabeto>
<root_bits> <c1,...,c15>` (después de `sh scripts/build.sh results /tmp/r`).

1. **Un solo símbolo.** ¿Cuántas entradas escribe? Busca el caso especial
   en las líneas 131-139. (`practice_level_1`: 256, la raíz replicada)
2. **Todo longitud 8.** ¿Se abre alguna tabla de segundo nivel?
   (`practice_level_2`: no, 256 entradas)
3. **Completo o incompleto.** Calcula la suma de Kraft del caso `w05`.
   (`practice_level_3`: < 1, incompleto)
4. **Escribir antes de comprobar.** ¿Cuántas entradas escribe el código
   anterior al fix para `w05` y cuántas sobran respecto a 630?
   (`practice_level_4`: 730, 100 de más)
5. **El fix.** ¿Qué hace la pasada de conteo con `w05`?
   (`practice_level_5`: lo rechaza sin escribir)
6. **Código completo profundo.** Una cadena hasta la longitud 15, ¿cabe?
   (`practice_level_6`: 384, asignación exacta)
7. **Control: la tabla de longitudes.** ¿Por qué `table[128]` no desborda?
   Mira la lectura de 3 bits de la línea 346. (`practice_level_7`: longitudes
   ≤ 7, sin segundo nivel)
8. **Entrada inválida.** Tres códigos de longitud 1, ¿qué devuelve?
   (`practice_level_8`: `BAD_LENGTH`, nada inventado)

Más allá: añade un histograma a `fixtures/cases.tsv`, deriva su
expectativa con `python3 scripts/derive_expected.py > fixtures/expected.tsv`
y comprueba con `sh scripts/build.sh test`.

*English summary: eight offline levels on writing 2nd-level tables before
the full-tree check; every answer is asserted by `tests/test_model.c`.*
