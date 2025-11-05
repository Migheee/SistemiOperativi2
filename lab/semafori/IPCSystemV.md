### IPC System V

#### Cos’è l’IPC

**InterProcess Communication (IPC)** significa comunicazione tra processi.
Rappresenta l’insieme dei meccanismi che permettono a processi diversi di scambiarsi dati o coordinarsi.
Serve, ad esempio, per gestire l’accesso condiviso a una risorsa o per sincronizzare più processi.

#### Tipi di IPC System V

Il System V IPC fornisce tre principali meccanismi di comunicazione:

1. **Code di messaggi:** scambio di dati strutturati tra processi tramite messaggi.
2. **Semafori:** coordinano e sincronizzano l’accesso a risorse condivise.
3. **Memoria condivisa:** più processi leggono e scrivono in un’area di memoria comune.

#### Creazione di un oggetto IPC

Ogni meccanismo si crea tramite una chiamata di sistema simile a `open()`:

1. **Coda di messaggi:** `msgget()`
2. **Semaforo:** `semget()`
3. **Memoria condivisa:** `shmget()`

#i### Concetto di key e id

* La **key** (`key_t`) è come un “nome logico” della risorsa IPC.
* L’**id** è un valore assegnato dal kernel quando la risorsa viene creata.
  Viene poi usato in tutte le altre chiamate per fare riferimento a quella risorsa.

#### Esempio: creazione di un semaforo

```c
// Permessi: rw-------
id = semget(key, 10, IPC_CREAT | S_IRUSR | S_IWUSR);
if (id == -1)
    errExit("semget");
```

Questa chiamata:

* crea un set di 10 semafori se non esiste (`IPC_CREAT`);
* imposta i permessi di lettura/scrittura per l’utente (`S_IRUSR | S_IWUSR`);
* restituisce un id (identificativo univoco nel sistema) o `-1` in caso di errore.

#### Analisi della key

La chiave (`key_t`) identifica logicamente un oggetto IPC.
Serve a permettere a più processi di accedere alla stessa risorsa.

Ci sono due modi per ottenerla:

##### 1. `IPC_PRIVATE`

```c
id = semget(IPC_PRIVATE, 10, S_IRUSR | S_IWUSR);
```

* Il kernel genera automaticamente una chiave unica.
* Utile per oggetti IPC utilizzati solo tra processi imparentati (padre/figlio).

##### 2. `ftok()`

```c
#include <sys/ipc.h>

key_t ftok(const char *pathname, int proj_id);
```

**Parametri:**

* `pathname`: file esistente usato per generare parte della chiave (basata su inode e device number).
* `proj_id`: identificatore del progetto (solo 8 bit usati). Serve a differenziare chiavi basate sullo stesso file.
  Deve essere identico tra processi che condividono la stessa risorsa.

**Esempio:**

```c
key_t key = ftok("/mydir/myfile", 'a');
if (key == -1)
  errExit("ftok failed");

int id = semget(key, 10, S_IRUSR | S_IWUSR);
if (id == -1)
  errExit("semget failed");
```

Se due processi chiamano `ftok()` con lo stesso file e lo stesso `proj_id`, otterranno la stessa chiave IPC.

#### Riepilogo

* **IPC:** comunicazione e sincronizzazione tra processi.
* **System V IPC:** include code di messaggi, semafori e memoria condivisa.
* **key (`key_t`):** identifica logicamente un oggetto IPC.
* **id:** identificatore univoco assegnato dal kernel.
* **`IPC_PRIVATE`:** chiave unica gestita dal kernel.
* **`ftok()`:** genera chiavi basate su file + project ID.

---

### Strutture Dati

Per ogni meccanismo IPC, il kernel associa una struttura dati dedicata
(`msqid_ds`, `semid_ds`, `shmid_ds`).
Per ogni istanza di un oggetto IPC System V viene creata una struttura specifica,
che al suo interno contiene anche la sottostruttura `ipc_perm`, la quale memorizza i permessi.

```c
struct ipc_perm {
  key_t __key;           /* Key, as supplied to ’get’ call */
  uid_t uid;             /* Owner’s user ID */
  gid_t gid;             /* Owner’s group ID */
  uid_t cuid;            /* Creator’s user ID */
  gid_t cgid;            /* Creator’s group ID */
  unsigned short mode;   /* Permissions */
  unsigned short __seq;  /* Sequence number */
};
```

#### Campi della struttura `ipc_perm`

* **uid** e **gid:** specificano i proprietari dell’oggetto IPC.
* **cuid** e **cgid:** contengono gli ID di utente e gruppo del processo che ha creato l’oggetto.
* **mode:** rappresenta la maschera dei permessi per l’oggetto IPC, inizializzata usando i 9 bit meno significativi dei flag specificati nella chiamata di sistema `get` usata per creare l’oggetto.

#### Note importanti su `ipc_perm`

1. I campi **cuid** e **cgid** sono **immutabili**.
2. Solo i permessi di **lettura** e **scrittura** hanno significato per gli oggetti IPC.
   I permessi di **esecuzione** non hanno rilevanza e vengono ignorati.

---

### Comandi IPC

Possono essere eseguiti sul terminale di sistemi UNIX-like (Mac compreso).

#### 1. `ipcs` – Ottenere informazioni sugli oggetti IPC nel sistema

```bash
user@localhost[~]$ ipcs
------ Message Queues --------
key        msqid    owner    perms    used-bytes    messages
0x1235     26       student  620      12            20
------ Shared Memory Segments --------
key        shmid    owner    perms    bytes    nattch    status
0x1234     0        professor 600      8192     2
------ Semaphore Arrays --------
key        semid    owner    perms    nsems
0x1111     102      professor 330      20
```

#### 2. `ipcrm` – Rimuovere oggetti IPC dal sistema

**Rimuovere una coda di messaggi:**

```bash
ipcrm -Q 0x1235   # 0x1235 è la key della coda
ipcrm -q 26       # 26 è l'identificatore della coda
```

**Rimuovere un segmento di memoria condivisa:**

```bash
ipcrm -M 0x1234   # 0x1234 è la key del segmento di memoria
ipcrm -m 0        # 0 è l'identificatore del segmento di memoria
```

**Rimuovere un array di semafori:**

```bash
ipcrm -S 0x1111   # 0x1111 è la key dell'array di semafori
ipcrm -s 102      # 102 è l'identificatore dell'array di semafori
```

---

### Semafori: Creazione e apertura

La chiamata di sistema `semget()` crea un nuovo insieme di semafori oppure ottiene l’ID di un insieme esistente (se la *key* passata è uguale a quella usata in una precedente chiamata a `semget()`).

```c
#include <sys/sem.h>

int semget(key_t key, int nsems, int semflg);
```

#### Argomenti

1. **key**
   Identificatore unico dell’insieme di semafori.

   * Può essere generato con `ftok()`.
   * Se impostato a `IPC_PRIVATE`, viene sempre creato un nuovo insieme.

2. **nsems**
   Numero di semafori da creare all’interno dell’insieme.

   * Deve essere un intero maggiore di 0.
   * Se l’insieme esiste già, questo parametro viene ignorato (ma deve comunque essere > 0).

3. **semflg**
   Bitmask che specifica i permessi e i flag di creazione.

   * I permessi si comportano come il parametro `mode` in `open(2)` (es. `0666` per lettura/scrittura).
   * È possibile combinare con OR (`|`) i seguenti flag:

     * `IPC_CREAT` — crea un nuovo insieme se non esiste uno con la stessa *key*.
     * `IPC_EXCL` — usato insieme a `IPC_CREAT`, fa fallire la chiamata se esiste già un insieme con quella *key* (evita di aprire insiemi esistenti).

#### Valore di ritorno

* Restituisce l’**ID dell’insieme di semafori** in caso di successo.
* Restituisce **-1** in caso di errore (con `errno` impostato per indicare la causa).

#### Esempio
```c
int semid;
ket_t key = //... (generate a key in some way, i.e. with ftok)
// A) delegate the problem of finding a unique key to the kernel
semid = semget(IPC_PRIVATE, 10, S_IRUSR | S_IWUSR);
// B) create a semaphore set with identifier key, if it doesn’t already exist
semid = semget(key, 10, IPC_CREAT | S_IRUSR | S_IWUSR);
//C) create a semaphore set with identifier key, but fail if it exists already
semid = semget(key, 10, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
```

### Operazione di controllo sui semafori
La chiamata di sistema `semctl` svolge alcune operazioni di controllo su un
insieme di semafori o su un semaforo specifico in un insieme.

#### Esempio

```c
#include <sys/sem.h>
// Returns nonnegative integer on success, or -1 on error
int semctl(int semid, int semnum, int cmd, ... /* union semun arg */);
```

L'argomento *semid* è l'id del meccanismo IPC creato (in questo caso semafori). Alcune operazioni di controllo richiedono un quarto argomento: `semun`, la quale deve essere esplicitamente definita dal programmatore prima di chiamare semctl.

```c
#ifndef SEMUN_H
#define SEMUN_H
#include <sys/sem.h>
// definition of the union semun

#endif
```

Per chi non sapesse cosa è una union: ecco una lezione dal goat: [union](https://youtu.be/TM4jgODgdFY?si=E7BEfCbSynItMCoJ&t=145)

### Operaizioni sui semafori
```c
int semctl(semid, 0 /*ignored*/, cmd, arg);
```

- IPC_RMID: rimuove l'insieme dei semafori, qualunque processo fermo si sveglia. Arg non è richiesto
- IPC_STAT: copia la struttura semid associata all'insieme dei semafori a quella punatata da semun (struct semid_ds * buf) 
- IPC_SET: aggiorna i campi della struttura semid_ds con quelli che vengono puntati da buf
```c
struct semid_ds {
  struct ipc_perm sem_perm; /* Ownership and permissions */
  time_t sem_otime; /* Time of last semop() */
  time_t sem_ctime; /* Time of last change */
  unsigned long sem_nsems; /* Number of semaphores in set */
};
```
Solo i sottocampi *uid*, *gid* e *mode* della sotto-struttura sem perm possono
venire aggiornati con IPC SET.

#### Esempio: cambiare i permessi ad un semaforo

```c
ket_t key = //... (generate a key in some way, i.e. with ftok)
// get, or create, the semaphore set
int semid = semget(key, 10, IPC_CREAT | S_IRUSR | S_IWUSR);
// instantiate a semid_ds struct
struct semid_ds ds;
// instantiate a semun union (defined manually somewhere)
union semun arg;
arg.buf = &ds;
// get a copy of semid_ds structure belonging to the kernel
if (semctl(semid, 0 /*ignored*/, IPC_STAT, arg) == -1)
  errExit("semctl IPC_STAT failed");
// update permissions to guarantee read access to the group
arg.buf->sem_perms.mode |= S_IRGRP;
// update the semid_ds structure of the kernel
if (semctl(semid, 0 /*ignored*/, IPC_SET, arg) == -1)
  errExit("semctl IPC_SET failed");
```
#### Esempio: rimozione di un insieme di semafori

```c
if (semctl(semid, 0/*ignored*/, IPC_RMID, 0/*ignored*/) == -1)
  errExit("semctl failed");
else
  printf("semaphore set removed successfully\n");
```
#### Recuperare e inizializzare valori di un semaforo

```c 
int semctl(semid, semnum, cmd, arg);
```
- SETVAL: il valore del semaforo sem-esimo viene inzializzato al valore specificato in arg.val
- GETVAL: come risultato della funzione, semctl restituisce il valore del semaforo semnum-esimo dell’insieme dato da semid. L’argomento arg
non `e richiesto.

#### Esempio che mostra come inizializzare un semaforo specifico di un insieme:
```c
ket_t key = //... (generate a key in some way, i.e. with ftok)
// get, or create, the semaphore set
int semid = semget(key, 10, IPC_CREAT | S_IRUSR | S_IWUSR);
// set the semaphore value to 0
union semun arg;
arg.val = 0;
// initialize the 5-th semaphore to 0
if (semctl(semid, 5, SETVAL, arg) == -1)
  errExit("semctl SETVAL");
```
Un insieme di semafori deve sempre venire inizializzato prima dell’uso

#### Esempio che mostra come inizializzare un insieme di 10 semafori:
```c
ket_t key = //... (generate a key in some way, i.e. with ftok)
// get, or create, the semaphore set
int semid = semget(key, 10, IPC_CREAT | S_IRUSR | S_IWUSR);
// set the first 5 semaphores to 1, and the remaining to 0
int values[] = {1,1,1,1,1,0,0,0,0,0};
union semun arg;
arg.array = values;
// initialize the semaphore set
if (semctl(semid, 0/*ignored*/, SETALL, arg) == -1)
  errExit("semctl SETALL");
```

#### Esempio che mostra come ottenere lo stato corrente di un insieme di 10 semafori:
```c
ket_t key = //... (generate a key in some way, i.e. with ftok)
// get, or create, the semaphore set
int semid = semget(key, 10, IPC_CREAT | S_IRUSR | S_IWUSR);
// declare an array big enougth to store the semaphores’ value
int values[10];
union semun arg;
arg.array = values;
// get the current state of a semaphore set
if (semctl(semid, 0/*ignored*/, GETALL, arg) == -1)
  errExit("semctl GETALL");
```

#### Recuperare informazioni specifiche
```c
int semctl(semid, semnum, cmd, 0);
```
con cmd che corrisponde a uno dei seguenti valori:
1. GETPID: restituisce il *PID* che ha svolto l'ultima semop sul semnum-esimo semforo
2. GETNCNT: restituisce il numero di processi in attesa sull'semnum-esimo semaforo 
3. GETZCNT: resituisce il numero di processi in attesa che il valore del semaforo semnum-esimo diventi 0

### Semaforo: Operazioni base
Come da teoria, è possibile eseguire wait & signal per mezzo della syscall `semop`

```c
#include <sys/sem.h>
// Returns 0 on success, or -1 on error
int semop(int semid, struct sembuf *sops, unsigned int nsops);
```

L'argomento `sops` è un puntatore dad array che contiene una sequenza di operazioni da svolgere atomicamente. Invece,  `nsops` indica la dimensione dell'array. Ogni elemento dell'array puntato da  `sops` ha la seguente forma:
```c
struct sembuf{
  unsigned short sem_num;
  short sem_op;
  short sem_flag;
}
```

#### Analisi della struttura `sembuf`

1. **`sem_num`**
   Indica **l’indice del semaforo** all’interno dell’insieme su cui si vuole eseguire l’operazione.

2. **`sem_op`**
   Specifica **il tipo di operazione** da eseguire sul valore del semaforo (`semval`):

   #### ▪️ Caso `sem_op > 0`

   * L’operazione **incrementa** `semval` di questo valore (rilascio o segnalazione di risorsa).
   * Se è specificato il flag `SEM_UNDO`, il sistema:

     * aggiorna la variabile di aggiustamento `semadj` associata al processo con
       `semadj -= sem_op`;
     * alla terminazione del processo, il kernel **aggiunge** `semadj` a `semval`,
       annullando l’effetto dell’operazione.
   * Questa operazione **non blocca mai** l’esecuzione e richiede i permessi di **alterazione** sul set di semafori.

#### ▪️ Caso `sem_op == 0`

Operazione detta **“wait-for-zero”**:

* Il processo **attende che `semval` diventi 0**.
* Se `semval` è già 0, l’operazione procede immediatamente.
* Se invece `semval ≠ 0` e nel campo `sem_flg` è impostato `IPC_NOWAIT`,
  l’operazione **non viene eseguita** e fallisce con errore `EAGAIN`.
* In questo caso, il processo deve avere i **permessi di lettura** sul semaforo.

#### ▪️ Caso `sem_op < 0`

* Il processo deve avere i **permessi di alterazione** (*alter permission*) sull’insieme di semafori.
* Se il valore corrente del semaforo (`semval`) è **maggiore o uguale** al valore assoluto di `sem_op`,
  l’operazione può procedere immediatamente:

  * il valore assoluto di `sem_op` viene **sottratto** da `semval`;
  * se è specificato il flag `SEM_UNDO`, il sistema **aggiunge** il valore assoluto di `sem_op` alla variabile di aggiustamento (`semadj`) associata al semaforo.
* Se invece il valore assoluto di `sem_op` è **maggiore di `semval`** e nel campo `sem_flg` è impostato `IPC_NOWAIT`,
  la chiamata a `semop()` **fallisce**:

  * viene impostato `errno = EAGAIN`;
  * nessuna delle operazioni presenti nell’array `sops` viene eseguita.
* In caso contrario (cioè se `IPC_NOWAIT` **non** è specificato):

  * il contatore `semncnt` — che rappresenta il numero di thread **in attesa che il valore del semaforo aumenti** — viene incrementato di 1;
  * il thread viene **sospeso (messo in attesa)** finché non si verifica una delle condizioni di risveglio previste dal manuale.

#### Comportamento di `semop` in caso di blocco

Quando una chiamata `semop(...)` **blocca**, per uscire dal blocco deve verificarsi una delle seguenti condizioni:

1. **Modifica del semaforo da parte di un altro processo**
   * Un altro processo modifica il valore del semaforo in modo che l’operazione richiesta possa procedere.

2. **Interruzione tramite segnale**
   * Un segnale interrompe la chiamata a `semop(...)`.
   * In questo caso viene restituito un errore `EINTR`.

3. **Cancellazione del semaforo da parte di un altro processo**
   * Se il semaforo coinvolto viene rimosso, `semop(...)` fallisce con errore `EIDRM`.


#### Evitare il blocco

È possibile evitare che `semop(...)` blocchi durante un’operazione su un semaforo specifico impostando il flag `IPC_NOWAIT` nel corrispondente campo `sem_flg`.

* In questo caso, se `semop(...)` avrebbe dovuto bloccare, invece **restituisce subito l’errore `EAGAIN`**.

#### Esempio 
```c
struct sembuf sops[3];

sops[0].sem_num = 0;
sops[0].sem_op = -1; // subtract 1 from semaphore 0
sops[0].sem_flg = 0;

sops[1].sem_num = 1;
sops[1].sem_op = 2; // add 2 to semaphore 1
sops[1].sem_flg = 0;

sops[2].sem_num = 2;
sops[2].sem_op = 0; // wait for semaphore 2 to equal 0
sops[2].sem_flg = IPC_NOWAIT; // but don’t block if operation cannot be performed immediately

if (semop(semid, sops, 3) == -1) {
  if (errno == EAGAIN) // Semaphore 2 would have blocked
    printf("Operation would have blocked\n");
  else
  errExit("semop"); // Some other error
}
``` 


### Memoria Condivisa

E' il secondo meccanismo di System V IPC, è un segmento di memoria gestito dal kernel, il quale permette a due o più processi di scambiarsi dati


