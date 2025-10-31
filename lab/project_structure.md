

# Struttura base progetto C con header

```
project/
├── include/       # Contiene solo i file header (.h)
│   └── math_utils.h
├── src/           # Contiene solo i file sorgente (.c)
│   ├── main.c
│   └── math_utils.c
```

---

## 1. Cartella `include/`

* Solo file `.h`.
* Contiene dichiarazioni di funzioni, costanti, tipi, macro.
* Ogni header deve avere **include guard**:

```c
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

int add(int a, int b);

#endif
```

* Oppure usare `#pragma once`:

```c
#pragma once

int add(int a, int b);
```

---

## 2. Cartella `src/`

* Solo file `.c`.
* Contiene implementazioni delle funzioni e `main.c`.
* Ogni file `.c` include gli header necessari:

```c
#include "math_utils.h"
#include <stdio.h>
```

---

## 3. Compilazione con `gcc`

Se hai `src/file1.c` e `src/file2.c`:

```bash
gcc -Iinclude src/file1.c src/file2.c -o nome_app
```

* `-Iinclude` dice a `gcc` di cercare gli header nella cartella `include`.
* `src/file1.c` e `src/file2.c` contengono il codice sorgente da compilare.

---