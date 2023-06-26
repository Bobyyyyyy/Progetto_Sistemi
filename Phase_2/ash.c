#include "ash.h"

/* variabili globali */
HIDDEN semd_t semd_table[MAXPROC]; 
HIDDEN LIST_HEAD(semdFree_h);
HIDDEN DECLARE_HASHTABLE(semd_h, 5);

extern void aaaBreakTest();

void initASH()
{
   for(int i=0; i<MAXPROC; i++){
                list_add(&semd_table[i].s_freelink, &semdFree_h);
    }
}


int insertBlocked(int* semAdd, pcb_t* p)
{
    if(p->p_semAdd==NULL) {
        /* p non è associato ad altri semafori */
        struct semd_t* corrente; 
        hash_for_each_possible(semd_h, corrente, s_link, (u32)semAdd) {
            /* semaforo già in semd_h */
            p->p_semAdd = semAdd;
            insertProcQ(&corrente->s_procq,p);
            return FALSE;
        }

        if (!list_empty(&semdFree_h)) {
            /* allochiamo un semd libero per p e lo inseriamo in semd_h */
            semd_t* newSemd = list_first_entry(&semdFree_h,semd_t,s_freelink);

            newSemd->s_key = semAdd;
            mkEmptyProcQ(&newSemd->s_procq); 
            p->p_semAdd = semAdd; 
            insertProcQ(&newSemd->s_procq, p);

            list_del(&newSemd->s_freelink);
            hash_add(semd_h,&newSemd->s_link, (u32)newSemd->s_key);
            return FALSE;
        }
    }
    return TRUE;
}


pcb_t* outBlocked(pcb_t *p)
{
    semd_t *semdP=NULL;
    struct list_head *corrente, *temp = NULL;
    int bkt;
    semd_t *semTemp=NULL;

    hash_for_each(semd_h, bkt, semdP, s_link) {
        /* se entriamo nel ciclo semdP punta al semaforo che blocca il processo p */
        if(semdP->s_key == p->p_semAdd){
            semTemp = semdP;
            break;
        }
    }

    semdP = semTemp;
    if(semdP!=NULL){

        list_for_each_safe(corrente, temp, &semdP->s_procq) 
        {
            pcb_t *eliminato = list_entry(corrente, pcb_t, p_list);

            if(eliminato==p) 
            {
                /* trovato il processo da eliminare */
                list_del_init(&eliminato->p_list);
                if (list_empty(&semdP->s_procq))
                {
                    /* la coda dei semafori bloccati è vuota quindi inseriamo semdP in semdFree_h */
                    hash_del(&semdP->s_link);
                    list_add(&semdP->s_freelink,&semdFree_h);
                    p->p_semAdd = NULL;
                }
                return p;
            }
        }
    }
    return NULL;
}


pcb_t* removeBlocked(int *semAdd)
{
    semd_t *semdP=NULL;
    int bkt;
    hash_for_each(semd_h,bkt,semdP,s_link) {
        /* se entriamo nel ciclo semdP punta al semaforo con chiave semAdd */
        if (semdP->s_key == semAdd ) {
            pcb_t *trovato = list_first_entry(&semdP->s_procq, pcb_t, p_list);
            return outBlocked(trovato);
        }
    }

    return NULL;
}


pcb_t* headBlocked(int *semAdd)
{
    semd_t *semdP=NULL;
    int bkt;
    hash_for_each(semd_h, bkt, semdP,s_link) {
        /* se entriamo nel ciclo semdP punta al semaforo con chiave semAdd */
        if (semdP->s_key == semAdd ) {
            return headProcQ(&semdP->s_procq);
        }
    }

    return NULL;
}