### Esempio completo (con condizione di variabile)

```c

Lock lock;
Condition dataready;
Queue queue;

AddToQueue(item) {
    lock.Acquire(); // Get Lock
    queue.enqueue(item); // Add item    
    dataready.signal(); // Signal any waiters
    lock.Release(); // Release Lock
}

RemoveFromQueue() {
    lock.Acquire(); // Get Lock
    while (queue.isEmpty()) {
        dataready.wait(&lock); // If nothing, sleep
    }
    item = queue.dequeue(); // Get next item
    lock.Release(); // Release Lock
    return(item);
}
```

Perche l'istruzione *while* al posto dell' *if* nel metodo RemoveFromQueue?

Dipende dal tipo di schedulazione
- Hoare-style (nella maggior parte dei testi, if implementation): <br>
    » Il segnalatore restituisce il lucchetto e la CPU a chi attende; chi attende parte immediatamente <br>
    » Chi attende restituisce il lucchetto e la CPU al segnalatore quando esce dalla sezione critica oppure se torna ad attendere <br>
    NOTA: se non ci sono elementi nella coda possono capitare eccezioni, come 	`Segmentation Fault` 

- Mesa-style (nella maggior parte dei sistemi operativi, while implementation):<br>
    » Il segnalatore tiene il lucchetto e la CPU <br>
    » Chi attende viene messo nella coda ready senza particolare priorità <br>
    » Praticamente, bisogna controllare di nuovo la condizione dopo la wait <br>

### Problema dei Lettori e Scrittori

Definiamo due classi di utenti:
- » Lettori – non modificano mai il database
- » Scrittori – leggono e scrivono nel database

E i seguenti vincoli
1. Lettori possono accedere se non ci sono scrittori
2. Scrittori possono accedere se non ci sono lettori o altri scrittori
3. Solo un thread manipola le variabili di stato: variabili condivise (tipicamente globali) che descrivono la situazione corrente del sistema.  Lock quindi serve per accedere alle variabili di stato

Struttura Base della soluzione 
```
Reader()
    Wait until no writers
    Access database
    Check out – wake up a waiting writer

Writer()
    Wait until no active readers or writers
    Access database
    Check out – wake up waiting readers or writer7
```

In questo caso, abbiamo le seguenti variabili di stato:
1. AR: numero di active Readers
2. AW: numero di active Writers
3. WR: numero di waiting Writers
4. WW: numero di waiting Writer

Tutto inizializzato a 0
```c
Reader() {
    // First check self into system
    lock.Acquire();
    while ((AW + WW) > 0) { // Is it safe to read?
        WR++; // No. Writers exist
        okToRead.wait(&lock); // Sleep on cond var
        WR--; // No longer waiting
    }
    
    AR++; // Now we are active!
    lock.release();
    // Perform actual read-only access AccessDatabase(ReadOnly);

    // Now, check out of system
    lock.Acquire();
    
    AR--; // No longer active
    
    if (AR == 0 && WW > 0) // No other active readers
        okToWrite.signal(); // Wake up one writer
    lock.Release();
}

Writer() {
    // First check self into system
    lock.Acquire();
    while ((AW + AR) > 0) { // Is it safe to write?
        WW++; // No. Active users exist
        okToWrite.wait(&lock); // Sleep on cond var
        WW--; // No longer waiting
    }
    
    AW++; // Now we are active!
    lock.release();
    // Perform actual read/write access
    
    AccessDatabase(ReadWrite);
    
    // Now, check out of system
    lock.Acquire();
    AW--; // No longer active
    if (WW > 0){ // Give priority to writers
        okToWrite.signal(); // Wake up one writer
    } else if (WR > 0) { // Otherwise, wake reader
        okToRead.broadcast(); // Wake all readers
    }
    lock.Release();
}


```



### Costruzione di Monitor su Semafori

### 🔹 Il lock

La parte del **lock** è semplice:
basta usare un **mutex**.


### 🔹 E per le variabili di condizione?

Potremmo pensare di implementarle così:

```c
Wait() { semaphore.P(); }
Signal() { semaphore.V(); }
```

**Non funziona.**
Perché?
La `Wait()` potrebbe sospendersi **mentre il lock è ancora acquisito**, impedendo ad altri thread di accedere alla sezione critica e quindi bloccando tutto (deadlock).


### Possiamo sistemarlo così?

```c
Wait(Lock lock) {
    lock.Release();
    semaphore.P();
    lock.Acquire();
}

Signal() { semaphore.V(); }
```

Sembra meglio, **non funziona ancora correttamente.**


### Il problema: differenza tra *Condition Variables* e *Semaphores*

Le **variabili di condizione** e i **semafori** non si comportano allo stesso modo:


### In pratica

* Con una **variabile di condizione**:

  * Se un thread chiama `signal()` e nessuno è in attesa → **non succede nulla**.
  * Se un thread entra in `wait()` dopo → **si blocca** finché qualcuno non fa un nuovo `signal()`.

* Con un **semaforo**:

  * Se un thread chiama `V()` e nessuno è in attesa → il contatore del semaforo **aumenta**.
  * Se un thread chiama `P()` più tardi → **non si blocca**, perché consuma quel “credito” di segnale.

Questa differenza (“storia” del semaforo) fa sì che una variabile di condizione **non possa essere implementata direttamente** con un semaforo semplice.


###  Riassunto finale

> Possiamo costruire un monitor usando i semafori?

* Il lock (mutua esclusione) sì, è facile da ottenere con un mutex.
* Le variabili di condizione no, non direttamente:
  perché i **semafori hanno memoria del passato**, mentre le **variabili di condizione no**.


