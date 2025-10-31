# Pipe in Unix/Linux

Una **pipe** è uno **stream di byte** (tecnicamente un buffer nella memoria del kernel del sistema operativo) che permette ai processi di scambiarsi dei byte.

---

## Proprietà delle pipe

* È **unidirezionale**: i dati scorrono in una sola direzione attraverso la pipe.
* Una estremità della pipe è usata per **scrivere** e l’altra per **leggere**.
* I dati passano attraverso la pipe **sequenzialmente**: i byte sono letti nell’esatto ordine in cui sono stati scritti.
* Non esiste un concetto di messaggi o dimensione di blocco:

  * Il processo che legge può leggere blocchi di dati di qualunque dimensione, **indipendentemente dalla dimensione dei blocchi scritti in origine**.

---

## Creazione di una pipe

La chiamata di sistema `pipe` crea una nuova pipe:

```c
#include <unistd.h>

// Restituisce 0 in caso di successo, -1 in caso di errore
int pipe(int filedes[2]);
```

* Una chiamata riuscita restituisce **due descrittori di file aperti** nell’array `filedes`:

  * `filedes[0]`: read-end (estremità di lettura)
  * `filedes[1]`: write-end (estremità di scrittura)
* Come per ogni descrittore di file, possiamo usare le chiamate di sistema `read` e `write` per fare I/O sulla pipe.

---

## Comunicazione tra processi

Normalmente si usa una pipe per consentire comunicazione fra processi collegati da una **relazione gerarchica** (es. genitore-figlio).

### Esempio: genitore scrive, figlio legge

```c
switch(fork()){
    case 0: // Figlio legge dalla pipe
        char buf[SIZE];
        ssize_t nBys;

        // Chiude write-end inutilizzato
        if (close(fd[1]) == -1)
            errExit("chiuso - figlio");

        // Legge dalla pipe
        nBys = read(fd[0], buf, SIZE); // 0: end-of-file, -1: errore

        if (nBys > 0) {
            buf[nBys] = '\0';
            printf("%s\n", buf);
        }

        // Chiude read-end della pipe
        if (close(fd[0]) == -1)
            errExit("chiuso - figlio");

    default: // Genitore scrive sulla pipe
        char buf[] = "Ciao Mondo\n";
        ssize_t nBys;

        // Chiude read-end inutilizzato
        if (close(fd[0]) == -1)
            errExit("chiuso - genitore");

        // Scrive nella pipe
        nBys = write(fd[1], buf, strlen(buf));

        // Controlla se la scrittura riesce
        if (nBys != strlen(buf)) {
            errExit("scrive - genitore");
        }

        // Chiude write-end della pipe
        if (close(fd[1]) == -1)
            errExit("chiuso - genitore");
}
```

---

## Stati della pipe e comportamento

* **no-wr**: tutti i processi hanno chiuso il write-end
* **si-wr**: almeno un processo ha il write-end aperto
* **no-rd**: tutti i processi hanno chiuso il read-end
* **si-rd**: almeno un processo ha il read-end aperto

### Caso Lettura

* **no-wr**: `read` restituisce 0 byte (EOF)
* **si-wr**, buffer vuoto: lettore va in **sleep**, bloccandolo finché almeno un byte non è stato scritto, oppure ritorna un segnale di non terminazione (`errno = EINTR`).
* **si-wr**, byte disponibili < richiesti: legge solo i byte disponibili
* Se l’estremità di scrittura di una pipe viene chiusa, un processo che legge dalla pipe vedrà **EOF** appena ha letto tutti i dati rimanenti nella pipe.

### Caso Scrittura

* Una scrittura è bloccata finché:

  * c’è sufficiente spazio per completare l’operazione **atomicamente**, oppure
  * avviene un segnale non di terminazione (`errno = EINTR`).
* **no-rd**: `write` fallisce → `SIGPIPE` allo scrittore
* **si-rd**, byte da scrivere < spazio disponibile: scrittura **atomica** (evita race condition)
* **si-rd**, byte da scrivere > spazio disponibile: scrittura **parziale**, non atomica
* Scritture di blocchi di dati più grandi di `PIPE_BUF` byte possono essere **spezzate** in segmenti di dimensione arbitraria (più piccoli di `PIPE_BUF`).
