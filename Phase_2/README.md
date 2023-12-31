# PANDOS PHASE 2

Nella seconda fase del progetto l'obiettivo è quello di realizzare il kernel del S.O. Panda+.  
Le funzionalità che deve gestire sono:

- Inizializzazione del sistema
- Scheduling dei processi
- Gestione delle eccezioni:
    - Interrupt
    - Syscall
    - Trap

## DIVISIONE FILE

I file principali della fase 2 sono divisi come segue:

- ### initial.c : 
    - Inizializza le variabili globali del nucleo e le varie strutture dati
    - Crea ed inizializza il primo processo
    - Invoca lo scheduler

- ### scheduler.c :
    Il ruolo dello scheduler e’ di gestire l'avvicendamento dei processi.

- ### exception.c :
    Distingue il tipo di eccezione del sistema e la risolve attraverso il gestore opportuno.  
    In particolare, i tipi di eccezione possono essere:
    - Interrupt
    - TLB Trap
    - Program Trap
    - Syscall

- ### interrupt.c :
    Si occupa di determinare l'interrupt facendo distinzione tra:
    - Processor Local Timer interrupt
    - Interval Timer interrupt
    - Device interrupt  

    E successivamente le affida ai vari gestori di interrupt.


## SCELTE PROGETTUALI

### Variabili aggiuntive

- __IOValues__:
    tra i parametri della system call DoIO l'utente passa l'indirizzo di un array dentro il quale scrive i comandi da dare al device, al completamento dell'operazione lo status del device va copiato in questo array.  
    Il campo IOValues che e' stato inserito al descrittore dei processi salva l'indirizzo di questo array durante la system call per poterlo poi riusare durante la gestione dell'interrupt.

- __startNstop__: 
    in questa variabile viene salvato il TOD al momento del lancio di un nuovo processo.  
    Nel momento in cui il processo viene interrotto per un eccezione la funzione UpdateCPUtime utilizza il valore salvato in startNstop per calcolare il tempo di utilizzo del processore da parte dell'ultimo processo in esecuzione, e aggiornare cosi' il relativo campo p_time.

- __is_waiting__:
    quando viene conclusa la gestione di un interrupt ci sono due scenari possibili:
    - L'interrupt e' arrivato durante l'esecuzione di un processo e dunque il controllo va ritornato ad esso 
    - Non c'erano processi in esecuzione e dunque il controllo va ritornato allo scheduler (fatta eccezione per PLT_interrupt_handler che richiama sempre lo scheduler).   
    
    La variabile is_waiting e' utilizzata per stabilire quale delle due operazioni si debba effettuare, infatti, quando la gestione di un interrupt termina, se is_waiting è true allora la variabile viene settata a false e viene invocato lo scheduler, altrimenti si esegue un LDST sul processo che era in esecuzione.  
    
    N.B. Lo scheduler, prima di entrare nello stato WAIT, imposta is_waiting a true.

### Semaforo per i terminali

Mentre per gli altri dispositivi utilizziamo un array di semafori di dimensione 8 (uno per ogni device), per i terminali abbiamo scelto di utilizzare un array di semafori di dimensione 16, considerando i primi 8 (da 0 a 7) per la ricezione e gli ultimi 8 (da 8 a 15) per la trasmissione.  
In questo modo ciascun terminale _n_ avrà i suoi rispettivi semafori alle posizioni _n_ e _n_ + 8.

### Interrupt handler per i device

Data la diversità dei terminali dagli altri tipi di device si e' deciso di  utilizzare due funzioni distinte per gestire i relativi interrupt:  
- terminal_interrupt_handler per i terminali
- general_interrupt_handler per tutti gli altri device

### Calcolo device address

Nella SYSCall DoIO, si calcola il device sul quale l'utente vuole effettuare l'operazione partendo dall'address fornito in input.  
   
```C
int devReg = ((memaddr)cmdAddr - DEV_REG_START) / DEV_REG_SIZE;
int typeDevice = devReg/8;
```
Sottraendo l'indirizzo di partenza dei device register e dividendo per 16 (dimensione di ogni device register) si ottiene un valore compreso tra 0 e 39 (8 device di 5 tipi, quindi 40 device in tutto).  
Dividendo per 8 si determina il tipo di device richiesto, da 0 a 4 rispettivamente disk, tape, network, printer e terminal.

## Compilazione 

Per compilare il progetto si utilizza il comando  `make`.  
Per eliminare i file creati si adopera il comando `make clean`.

I file di umps sono situati nella cartella "machine".  
Nel caso in cui si volesse realizzare una configurazione di una nuova macchina su umps3 bisognerà selezionare la directory di "machine".

