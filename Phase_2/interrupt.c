#include <scheduler.c>
#include <umps3/umps/const.h>
#include <umps3/umps/arch.h>
#include <umps3/umps/cp0.h>




/* Restituisce la linea con interrupt in attesa con massima priorità. 
(Se nessuna linea è attiva ritorna 8 ma assumiamo che quando venga
 chiamata ci sia almeno una linea attiva) */
int Get_Interrupt_Line_Max_Prio (){
    unsigned int interrupt_pending = current_process->p_s.cause & CAUSE_IP_MASK;
    /* così abbiamo solo i bit attivi da 8 a 15 del cause register */
    unsigned int intpeg_linee[8];
    for (int i=0; i<8; i++) {
        unsigned mask = ((1<<1)-1)<<i+8;
        intpeg_linee[i] = mask & interrupt_pending;
    }
    /* intpeg_linee[i] indica se la linea i-esima è attiva */
    
    int linea=1; /* questo perché ignoriamo la linea 0*/
    while(linea<8) {
        if(intpeg_linee[linea]!=0) { 
            break;
        }
        linea++;
    }
    /* ritorniamo la quale linea è attiva */
    return linea;
}

/*
The interrupt exception handler’s first step is to determine which device
 or timer with an outstanding interrupt is the highest priority.
 Depending on the device, the interrupt exception handler will 
 perform a number of tasks.*/
void interrupt_handler()
{
    /* sezione 3.6.1 a 3.6.3*/
    switch (Get_Interrupt_Line_Max_Prio())
    {
    /* interrupt processor Local Timer */
    case 1:
        PLT_interrupt_handler();
        break;
    
    /* interrupt Interval Timer */
    case 2:
        IT_interrupt_handler();
        break;

    /* Disk devices */
    case DISKINT:
        DISK_interrupt_handler();
        break;
    
    /* Flash devices */
    case FLASHINT:
        
        break;
    
    /* Network devices*/
    case NETWINT:
        break;

    /* Printer devices */
    case PRNTINT:
        
        break;
    
    /* Terminal devices*/
    case TERMINT:
    
        break;

    default:
        break;
    }
   
}

//3.6.2
void PLT_interrupt_handler() {
    /*Acknowledge the PLT interrupt by loading the timer with a new value.*/
    setTIMER(TIMESLICE);

    /* Copy the processor state at the time of the exception (located at the start of the BIOS Data Page [Section ??-pops]) into the Current Pro- cess’s pcb (p_s). */
    state_t *exc_state = BIOSDATAPAGE;
    current_process->p_s = *exc_state;

    /* Place the Current Process on the Ready Queue; transitioning the Current Process from the “running” state to the “ready” state. */
    insertProcQ(&current_process->p_list, &readyQ);

    /*Call the scheduler*/
    scheduling();
}

//3.6.3
void IT_interrupt_handler(){
    /*Acknowledge the interrupt by loading the Interval Timer with a new value: 100 milliseconds.*/
    LDIT(PSECOND);

    /*Unblock ALL pcbs blocked on the Pseudo-clock semaphore. Hence, the semantics of this semaphore are a bit different than traditional synchronization semaphores*/
    V_all();

    /*Return control to the Current Process: Perform a LDST on the saved exception state*/
    state_t *exc_state = BIOSDATAPAGE;
    LDST(exc_state);
}

/* ritorna la linea del device il cui interrupt è attivo */
int Get_interrupt_device(int intLineNo)
{
    /* Calculate the address for this device’s device register */
    unsigned int interrupt_dev_bit_map = CDEV_BITMAP_ADDR(IntLineNo); 
    /*+indirizzo diverso in base al tipo di device */

    unsigned int int_linee[8];
    for (int i=0; i<8; i++) {
        unsigned mask = ((1<<1)-1)<<i;
        int_linee[i] = mask & interrupt_dev_bit_map;
    }
    
    /* int_linee[i] indica se la linea i-esima è attiva */
    int linea=0;
    while(linea<8) {
        if(int_linee[linea]!=0) { 
            break;
        }
        linea++;
    }
    
return linea;
}

//3.6.1     
void DISK_interrupt_handler(int IntLineNo)
{   /* vedere arch.h */
    int DevNo = Get_interrupt_device(IntLineNo);

    // Forse è possibile fare una funzione comune per tutti i device, passando IntLineNo per parametro
    
    /* Save off the status code from the device’s device register. */
    /*Uso la macro per trovare l'inidirzzo di base del device con la linea di interrupt e il numero di device*/
    int *dev_addr=DEV_REG_ADDR(IntLineNo,DevNo);
    /* Copia del device register*/
    dtpreg_t dev_reg.status = dev_addr->status;

    /* Acknowledge the outstanding interrupt. This is accomplished by writ-
        ing the acknowledge command code in the interrupting device’s device
        register. Alternatively, writing a new command in the interrupting
        device’s device register will also acknowledge the interrupt.*/
    dev_addr->command = ACK;

    /* Perform a V operation on the Nucleus maintained semaphore associ-
        ated with this (sub)device. This operation should unblock the process
        (pcb) which initiated this I/O operation and then requested to wait for
        its completion via a SYS5 operation.*/

    pcb_t *blocked_process = NULL;
    switch(IntLineNo){
        case 3:
            blocked_process = headBlocked(&sem_disk[DevNo]);
            SYS_Verhogen(&sem_disk[DevNo]);
        case 4:
            blocked_process = headBlocked(&sem_tape[DevNo]);
            SYS_Verhogen(&sem_tape[DevNo]);
        case 5:
            blocked_process = headBlocked(&sem_network[DevNo]);
            SYS_Verhogen(&sem_network[DevNo]);
        case 6:
            blocked_process = headBlocked(&sem_printer[DevNo]);
            SYS_Verhogen(&sem_printer[DevNo]);
    }

    /* Place the stored off status code in the newly unblocked pcb’s v0 register.*/
    blocked_process->p_s.reg_v0;
    /* Insert the newly unblocked pcb on the Ready Queue, transitioning this
        process from the “blocked” state to the “ready” state*/

    /* Return control to the Current Process: Perform a LDST on the saved
        exception state (located at the start of the BIOS Data Page */
    state_t *prev_state = BIOSDATAPAGE;
    LDST(prev_state);
}

void V_all(){
    int pid_current = current_process->p_pid;
    if(sem_interval_timer == 1) {
         /* chiamata allo scheduler*/
        scheduling();
    } else {
        while(headBlocked(sem_interval_timer)!=NULL){
            pcb_t* wakedProc = removeBlocked(sem_interval_timer);
            insertProcQ(&readyQ, wakedProc); 
        }
        sem_interval_timer = 1;
    }
}


int Tod_clock()
{   
    /* Permette di calcolare un intervallo di tempo utilizzando la macro per salvare il tempo all'inizio e alla fine di un processo */
    unsigned start,end;
    
    STCK(start);
    scheduling();
    STCK(end);
    return (end-start);
}