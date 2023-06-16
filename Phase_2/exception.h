#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <umps3/umps/arch.h>
#include "interrupt.h"



pcb_t pcbFree_table[MAXPROC];

/* come process id usiamo un intero che aumenta
    e basta (no caso reincarazione)*/
int pid_start = 0;

void foobar();

void passup_ordie(int );

void syscall_handler();

void update_PC_SYS_non_bloccanti();

void update_PC_SYS_bloccanti();

void SYS_create_process(state_t *, support_t *, nsd_t *);

void SYS_terminate_process(int );

void terminate_family(pcb_t *);

void kill_process(pcb_t *);

void SYS_Passeren(int *);

void SYS_Verhogen(int *);

void SYS_Doio(int *, int *);

void SYS_Get_CPU_Time();

void SYS_Clockwait();

void SYS_Get_Support_Data();

void SYS_Get_Process_Id(int );

void SYS_Get_Children(int *, int );

int Check_Kernel_mode();

void P_always(int *);



#endif