
## **Coda di messaggi (Message Queue) – Teoria e Implementazione**

### Cos’è una coda di messaggi

Una **coda di messaggi** è una struttura dati gestita dal sistema operativo che permette a più processi di comunicare tra loro **in modo asincrono**, cioè senza dover sincronizzare direttamente lettura e scrittura.

Caratteristiche principali:

* **FIFO (First-In, First-Out)**: i messaggi vengono letti nell’ordine in cui sono stati inseriti, a meno che non si usi la selezione tramite tipo.
* **Decoupling**: il processo mittente e quello ricevente non devono essere attivi contemporaneamente.
* **Tipo di messaggio (`mtype`)**: ogni messaggio ha un tipo intero positivo, che può essere usato per filtrare o ordinare i messaggi.
* **Sistema operativo**: la coda è gestita dal kernel, quindi offre sicurezza e atomicità nelle operazioni di invio e ricezione.

---

### Creazione o ottenimento di una coda di messaggi: `msgget`

La chiamata di sistema `msgget` crea una nuova coda di messaggi oppure ottiene l’identificativo di una coda esistente.

```c
#include <sys/msg.h>

// Restituisce l'identificativo della coda dei messaggi in caso di successo, o -1 in caso di errore
int msgget(key_t key, int msgflg);
```

* `key` è una chiave IPC, mentre `msgflg` è una bit mask che specifica i permessi da porre su una nuova coda di messaggi o da verificare rispetto a una coda esistente.
* In `msgflg` è possibile combinare i seguenti flag:

  * `IPC_CREAT`: se non esiste alcuna coda di messaggi con la chiave specificata, ne crea una nuova
  * `IPC_EXCL`: in congiunzione con `IPC_CREAT`, fa fallire `msgget` qualora esista già una coda di messaggi con la chiave specificata

**Esempio di utilizzo:**

```c
int msqid;
key_t key = //... (generate a key in some way, i.e. with ftok)

// Creazione di una coda privata delegando la chiave unica al kernel
msqid = msgget(IPC_PRIVATE, S_IRUSR | S_IWUSR);

// Creazione di una coda con chiave specifica, se non esiste già
msqid = msgget(key, IPC_CREAT | S_IRUSR | S_IWUSR);

// Creazione di una coda con chiave specifica, fallisce se esiste già
msqid = msgget(key, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
```

---

### Struttura di un messaggio

Un messaggio in una coda di messaggi deve seguire la seguente struttura:

```c
struct mymsg {
    long mtype;    /* tipo di messaggio */
    char mtext[];  /* corpo del messaggio */
};
```

* La prima parte del messaggio contiene il tipo, specificato come intero maggiore di zero.
* Il resto del messaggio può essere strutturato liberamente dal programmatore in termini di lunghezza e contenuto.
* Se non necessario, il corpo del messaggio può anche essere omesso.

---

### Invio di un messaggio: `msgsnd`

La chiamata di sistema `msgsnd` invia un messaggio alla coda:

```c
#include <sys/msg.h>

// Restituisce 0 in caso di successo, -1 in caso di errore
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
```

* `msqid` è l'identificativo della coda dei messaggi
* `msgp` è un puntatore alla struttura del messaggio
* `msgsz` indica il numero di byte del corpo del messaggio, escludendo `mtype`
* `msgflg` può essere 0 oppure `IPC_NOWAIT`

Normalmente, se la coda è piena, `msgsnd` blocca il processo finché non c'è spazio disponibile. Se si specifica `IPC_NOWAIT`, `msgsnd` ritorna immediatamente con errore `EAGAIN`.

---

#### Esempio con stringa

```c
struct mymsg {
    long mtype;
    char mtext[ ] ; 
} m;

m.mtype = 1;

char *text = "Ciao mondo!";
memcpy(m.mtext, text, strlen(text) + 1);  // include il terminatore
size_t mSize = sizeof(struct mymsg) - sizeof(long);

if (msgsnd(msqid, &m, mSize, 0) == -1)
    errExit("msgsnd failed");
```

#### Esempio con interi

```c
struct mymsg {
    long mtype;
    int num1, num2;
} m;

m.mtype = 2;
m.num1 = 34;
m.num2 = 43;

size_t mSize = sizeof(struct mymsg) - sizeof(long);

if (msgsnd(msqid, &m, mSize, 0) == -1)
    errExit("msgsnd failed");
```

#### Messaggio solo con tipo

```c
struct mymsg {
    long mtype;
} m;

m.mtype = 3;
size_t mSize = sizeof(struct mymsg) - sizeof(long);  // zero

if (msgsnd(msqid, &m, mSize, IPC_NOWAIT) == -1) {
    if (errno == EAGAIN)
        printf("La coda era piena\n");
    else
        errExit("msgsnd failed");
}
```

---

#### Ricezione di un messaggio: `msgrcv`

La chiamata di sistema `msgrcv` legge e rimuove un messaggio dalla coda:

```c
#include <sys/msg.h>

// Restituisce il numero di byte copiati in msgp in caso di successo, -1 in caso di errore
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtype, int msgflg);
```

* `msqid` è l'identificativo della coda
* `msgsz` indica lo spazio massimo disponibile per il corpo del messaggio
* `msgtype` seleziona il messaggio da recuperare:

  * 0 → primo messaggio disponibile
  * maggiore di 0 → primo messaggio con `mtype` pari a `msgtype`
  * minore di 0 → primo messaggio con `mtype` minore o uguale al valore assoluto di `msgtype`

**Esempio logico:**

Data una coda con messaggi `(mtype, char)`:

```
(300,'a'); (100,'b'); (200,'c'); (400,'d'); (100,'e')
```

Una serie di chiamate `msgrcv` con `msgtype=-300` restituisce il messaggio con `mtype=100`, cioè il più basso minore o uguale a 300.

L'argomento `msgflg` è una bit mask formata dal bitwise OR di zero o più dei seguenti flag
1. IPC_NOWAIT: se nessun messaggio nella coda è tipo `msgtype`, allora `msgrcv` è bloccante finchè  non arriva un tale messaggio. IPC_NOWAIT non permette al chiamante di bloccarsi e `msgrcv` ritorna immediatamente con errore `ENOMSG`

2. MSG_NOERROR: se la dimensione del campo `mtext` supera lo spazio disponibile allora `msgrcv` fallisce. Aggiungendo `MSG_NOERROR` la syscall `msgrcv` rimuove il messaggio dalla coda, tronca il campo `mtext` al numero di byte specificati da `msgsz` e lo restituisce al chiamante 

#### Esempio 1
```c
// message structure definition
struct mymsg {
    long mtype;
    char mtext[100]; /* array of chars as message body */
} m;

// Get the size of the mtext field.
size_t mSize = sizeof(struct mymsg) - sizeof(long);

// Wait for a message having type equals to 1
if (msgrcv(msqid, &m, mSize, 1, 0) == -1)
    errExit("msgrcv failed");
```

#### Esempio 2
```c
// message structure definition
struct mymsg {
    long mtype;
    char mtext[100]; /* array of chars as message body */
} m;

// Set an arbitrary size for the size.
size_t mSize = sizeof(char) * 50;

// Wait for a message having type equals to 1, but copy its first 50 bytes only
if (msgrcv(msqid, &m, mSize, 1, MSG_NOERROR) == -1)
    errExit("msgrcv failed");
```

#### Esempio 3
```c
// Message structure
struct mymsg {
    long mtype;
} m;

// In polling mode, try to get a message every SEC seconds.
while (1) {
    sleep(SEC);
    // Performing a nonblocking msgrcv.
    if (msgrcv(msqid, &m, 0, 3, IPC_NOWAIT) == -1) {
        if (errno == ENOMSG) {
            printf("No message with type 3 in the queue\n");
        } else {
            errExit("msgrcv failed");
        }   
    } else {
        printf("I found a message with type 3\n");
    }
}
```

### Operaioni di controllo con `msgctl`

La chiamata di sistema msgctl svolge operazioni di controllo sulla coda di messaggi
```c
#include <sys/msg.h>

// Returns 0 on success, or -1 error
int msgctl(int msqid, int cmd, struct msqid_ds *buf);
```

- `msqid` è un identificativo della coda di messaggi
- `cmd` specifica l'operazione da svolgere sulla coda:
    - `IPC_RMID`: rimuove immediatamente la coda. Tutti i messaggi non letti 
    sono persi e qualunque lettore/scrittore bloccato viene risvegliato
    (errno pari a EIDRM). Per questa operazione l’argomento `buf` viene
    ignorato.
    -`IPC_STAT`: posiziona una copia della struttura dati msqid ds associata
    a questa coda nel buffer puntato da buf.
    - `IPC SET`: aggiorna i campi della struttura dati msqid ds associata a
    questa coda usando i valori forniti nel buffer puntato da `buf`

Per ogni coda di messaggi il kernel ha una struttura dati associata `msqid_ds` avente i seguent
campi

```c
struct msqid_ds {
    struct ipc_perm msg_perm; /* Ownership and permissions */
    time_t msg_stime; /* Time of last msgsnd() */
    time_t msg_rtime; /* Time of last msgrcv() */
    time_t msg_ctime; /* Time of last change */
    unsigned long __msg_cbytes; /* Number of bytes in queue */
    msgqnum_t msg_qnum; /* Number of messages in queue */
    msglen_t msg_qbytes; /* Maximum bytes in queue */
    pid_t msg_lspid; /* PID of last msgsnd() */
    pid_t msg_lrpid; /* PID of last msgrcv() */
};
```

I campi modificabili sono solo i seguenti:

- `msg_perm.uid ` — userid del proprietario

- `msg_perm.gid` — group id del proprietario

- `msg_perm.mode` — i permessi (modi) di lettura / scrittura

- `msg_qbytes` — la dimensione massima in byte della coda (il limite)

#### Esempio che consente di cambiare la dimensione massima dei messaggi
```c
struct msqid_ds ds;
// Get the data structure of a message queue
if (msgctl(msqid, IPC_STAT, &ds) == -1)
    errExit("msgctl");

// Change the upper limit on the number of bytes in the mtext
// fields of all messages in the message queue to 1 Kbyte
ds.msg_qbytes = 1024;

// Update associated data structure in kernel
if (msgctl(msqid, IPC_SET, &ds) == -1)
    errExit("msgctl");
```
