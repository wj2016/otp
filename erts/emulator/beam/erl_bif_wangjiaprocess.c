// header files see erl_bif_lists.c
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif


#include "sys.h"
#include "erl_vm.h"
#include "global.h"
#include "erl_process.h"
#include "erl_message.h"
#include "erl_binary.h"
#include "error.h"
#include "bif.h"
#include "erl_debug.h"

// TODO
// processes() is a good start point to learn ProcessQueue
// spawn() to see which Queue to change

#ifdef ERTS_SMP // focus on ERTS_SMP code at this moment

/*
Pid = spawn(timer, sleep, [infinity]).
Pid2 = spawn(fun() -> F = fun(F) -> F(F) end, F(F) end).
wangjiaprocess:demo_pcb(Pid).
**
*/
BIF_RETTYPE wangjiaprocess_demo_pcb_1(BIF_ALIST_1)
{
    Eterm bif_info_tuple;
    Process* p;     // Target Process
    Process* rp;     // Target Process
    Eterm self_pid;
    Eterm target_pid;
    printf("\r\n");

    p = BIF_P;
    self_pid = BIF_P->id;
    target_pid = BIF_ARG_1;

    rp = erts_pid2proc_opt(p, // current Process*
                           ERTS_PROC_LOCK_MAIN, // c p have locks, 1
                           target_pid,                 // Pid
                           0,                   // pid_need_locsk, 0
                           ERTS_P2P_FLG_SMP_INC_REFC); // flags

    erts_printf("|    self()      |    remote      |*************\r\n");
    // pid()
    erts_printf("|   %T     |    %T    | pid()\r\n", self_pid, target_pid);
    // id in PCB
    erts_printf("|   %T     |    %T    | pid in PCB\r\n", p->id, rp->id);
    // parent pid
    erts_printf("|   %T     |    %T    | parent\r\n", p->parent, rp->parent);
    // erlang:group_leader/0
    erts_printf("|   %T     |    %T    | group_leader()\r\n",
                p->group_leader, rp->group_leader);

    // prev Process*
    if(p->prev != NULL) {
        erts_printf("|   %T     ", (p->prev)->id);
    } else {
        erts_printf("|     NULL       ");
    }
    if(rp->prev != NULL) {
        erts_printf("|   %T     | prev\r\n", (p->prev)->id);
    } else {
        erts_printf("|     NULL       | prev\r\n");
    }

    // next Process*
    if(p->next != NULL) {
        erts_printf("|   %T     ", (p->next)->id);
    } else {
        erts_printf("|     NULL       ");
    }
    if(rp->next != NULL) {
        erts_printf("|   %T     | next\r\n", (p->next)->id);
    } else {
        erts_printf("|     NULL       | next\r\n");
    }

    printf("---------- Intergers Info  -----------------------\r\n");
    printf("|%16u|%16u| heap_sz\r\n", p->heap_sz, rp->heap_sz);
    printf("|%16d|%16d| heap - heap\r\n", p->hend - p->heap, rp->hend - rp->heap);
    printf("|%16u|%16u| min_heap_size\r\n", p->min_heap_size, rp->min_heap_size);
    printf("|%16u|%16u| min_vheap_size\r\n", p->min_vheap_size, rp->min_vheap_size);

    printf("|%16u|%16u| status, STATE\r\n", p->status, rp->status);
    printf("|%16u|%16u| gcstatus, GC STATE\r\n", p->gcstatus, rp->gcstatus);
    printf("|%16u|%16u| rstatus\r\n", p->rstatus, rp->rstatus);
    printf("|%16u|%16u| rcount\r\n", p->rcount, rp->rcount);
    printf("|%16d|%16d| prio\r\n", p->prio, rp->prio);
    printf("|%16d|%16d| skipped\r\n", p->skipped, rp->skipped);
    printf("|%16u|%16u| reds\r\n", p->reds, rp->reds);
    printf("|-----%6u-----|%16u| arity\r\n", p->arity, rp->arity);
    printf("|%16u|%16u| max_arg_reg\r\n", p->max_arg_reg, rp->max_arg_reg);
    printf("|%16u|%16u| catches\r\n", p->catches, rp->catches);
    printf("|%16u|-----%6u-----| fcalls, reds left\r\n", p->fcalls, rp->fcalls);
    printf("|%#16x|%#16x| flags\r\n", p->flags, rp->flags);
    printf("|%#16x|%#16x| runq_flags\r\n", p->runq_flags, rp->runq_flags);
    printf("|%#16x|%#16x| status_flags\r\n", p->status_flags, rp->status_flags);

    printf("---------- Memory Addresses ----------------------\r\n");
    printf("|%#016x|%#016x| htop, heap top\r\n", p->htop, rp->htop);
    printf("|%#016x|%#016x| stop, steak top\r\n", p->stop, rp->stop);
    printf("|%#016x|%#016x| heap, heap start\r\n", p->heap, rp->heap);
    printf("|%#016x|%#016x| hend, heap end\r\n", p->hend, rp->hend);
    printf("|%#016x|%#016x| &heap, where is Process*\r\n", &(p->heap), &(rp->heap));

    printf("---------- Other Info ----------------------\r\n");
    //    erts_printf("|%T|%T| Sequential trace token\r\n", p->seq_trace_token, rp->seq_trace_token);
    BIF_RET(am_ok);
}

/************** DEMO
1> Pid = spawn(timer, sleep, [infinity]).
<0.34.0>
2> wangjiaprocess:demo_pcb(Pid).

|    self()      |    remote      |*************
|   <0.32.0>     |    <0.34.0>    | pid()
|   <0.32.0>     |    <0.34.0>    | pid in PCB
|   <0.26.0>     |    <0.32.0>    | parent
|   <0.25.0>     |    <0.25.0>    | group_leader()
|     NULL       |     NULL       | prev
|     NULL       |     NULL       | next
---------- Intergers Info  -----------------------
|            1597|             233| heap_sz
|            1597|             233| heap - heap
|             233|             233| min_heap_size
|           46368|           46368| min_vheap_size
|               3|               2| status, STATE
|               3|               0| gcstatus, GC STATE
|               0|               0| rstatus
|               0|               0| rcount
|               2|               2| prio
|               0|               0| skipped
|            1870|               9| reds
|-----     0-----|               0| arity
|               6|               6| max_arg_reg
|               1|               0| catches
|             369|-----  1999-----| fcalls, reds left
|               0|             0x4| flags
|             0x1|               0| runq_flags
|             0x8|               0| status_flags
---------- Memory Addresses ----------------------
|0x0000004f339b88|0x0000004f33be90| htop, heap top
|0x0000004f33a350|0x0000004f33c540| stop, steak top
|0x0000004f337228|0x0000004f33be08| heap, heap start
|0x0000004f33a410|0x0000004f33c550| hend, heap end
|0x00000001e9f968|0x00000001ea09a0| &heap, where is Process*
---------- Other Info ----------------------
ok

*/

#else

BIF_RETTYPE wangjiaprocess_demo_pcb_1(BIF_ALIST_1)
{
    BIF_RET(am_error);
}

#endif
