#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sys.h"
#include "global.h"
#include "erl_alloc.h"
#include "erl_process_lock.h"
#include "bif.h"

#ifdef ERTS_SMP // focus on ERTS_SMP code at this moment

// defined ERTS_PROC_LOCK_ATOMIC_IMPL

/******************************************************************************
 ***************** Lock Mutex *************************************************
 *****************************************************************************/
// otp_src_R15B03/erts/include/internal/ethr_mutex.h
static ETHR_INLINE void
ETHR_INLINE_MTX_FUNC_NAME_(my_ethr_mutex_lock)(ethr_mutex *mtx)
{
    int res;
    printf("== START [my_ethr_mutex_lock] ==\r\n");
    res = pthread_mutex_lock(&mtx->pt_mtx);
    // wangjia: this function called
    // Don't print in normal case, this function called very often
    /* printf("haha\r\n"); */
    if (res != 0)
       ETHR_FATAL_ERROR__(res);
}


// otp_src_R15B03/erts/emulator/beam/erl_threads.h
ERTS_GLB_INLINE void
#ifdef ERTS_ENABLE_LOCK_COUNT
my_erts_mtx_lock_x(erts_mtx_t *mtx, char *file, unsigned int line)
#else
my_erts_mtx_lock(erts_mtx_t *mtx)
#endif
{
    printf("== START [my_erts_mtx_lock] ==\r\n");
    // wangjia: USE_THREADS defined by external gcc -DUSER_THREADS
#ifdef USE_THREADS
#ifdef ERTS_ENABLE_LOCK_CHECK
    erts_lc_lock(&mtx->lc);
#endif
#ifdef ERTS_ENABLE_LOCK_COUNT
    erts_lcnt_lock(&mtx->lcnt);
#endif
    // wangjia: to call real implementation
    // Actually, it calls
    // ethr_mutex:ETHR_INLINE_MTX_FUNC_NAME_(ethr_mutex_lock)
    my_ethr_mutex_lock(&mtx->mtx);
#ifdef ERTS_ENABLE_LOCK_COUNT
    erts_lcnt_lock_post_x(&mtx->lcnt, file, line);
#endif
#endif // USE_THREADS
}

// otp_src_R15B03/erts/emulator/beam/erl_process_lock.h
// #if ERTS_GLB_INLINE_INCL_FUNC_DEF --> true
ERTS_GLB_INLINE void my_erts_pix_lock(erts_pix_lock_t *pixlck)
{
    printf("== START [my_erts_pix_lock] ==\r\n");
    ERTS_LC_ASSERT(pixlck);
    my_erts_mtx_lock(&pixlck->u.mtx);
}

/*****************************************************************************/

// WANGJIA comment DONE
Process *
my_erts_pid2proc_opt(Process *c_p,
                            ErtsProcLocks c_p_have_locks,
                            Eterm pid,
                            ErtsProcLocks pid_need_locks,
                            int flags)
{
    /**** NOTES ****
     *** I copied this function from erl_send in erl_message to learn process
     *** lock seperately, maybe there are some uselesss steps for this simple
     *** pid2process.
     *** To simplify my studying, focus on SMP version first, only keep SMP
     *** code and ignore non-smp part.
     ***/
    erts_pix_lock_t *pix_lock; // high level pid index lock in Erlang VM level
    ErtsProcLocks need_locks;
    Uint pix;
    Process *proc;



    printf("\r\n=== START [my_erts_pid2proc_opt] ====\r\n");
    erts_printf("                  Running pid is %T\r\n", c_p->id);
    erts_printf("     Try to get Process* for pid %T\r\n", pid);

    // Can't get Process* for external pid
    if (is_not_internal_pid(pid)) {
	proc = NULL;
	goto done;
    }

    // get internal pid index
    // <0.33.0> -> 33
    //   ^pid   -> ^ pix
    pix = internal_pid_index(pid); // pid index
    erts_printf("[internal_pid_index] %ld for pid %T\r\n", pix, pid);

    // ensure valid pix
    if(pix >= erts_max_processes) {
	proc = NULL;
	goto done;
    }

    // locks
    ERTS_LC_ASSERT((pid_need_locks & ERTS_PROC_LOCKS_ALL) == pid_need_locks);
    need_locks = pid_need_locks;
    pix_lock = ERTS_PIX2PIXLOCK(pix);

    printf("== locks ==\r\n");
    printf("ERTS_PROC_LOCKS_ALL: %#x\r\n", (unsigned int)ERTS_PROC_LOCKS_ALL);
    printf("     pid_need_locks: %#x\r\n", (unsigned int)pid_need_locks);
    printf("         need_locks: %#x\r\n", (unsigned int)need_locks);
    printf("           pix_lock: %#x\r\n", (unsigned int)pix_lock);

    // c_p != NULL && c_p->id ...
    if (c_p && c_p->id == pid) { // pid2proc(self())
	ASSERT(c_p->id != ERTS_INVALID_PID);
	ASSERT(c_p == process_tab[pix]);
	if (!(flags & ERTS_P2P_FLG_ALLOW_OTHER_X) && c_p->is_exiting) {
	    proc = NULL;
	    goto done;
	}
	need_locks &= ~c_p_have_locks;
	if (!need_locks) {
	    proc = c_p;
            printf("== pid2proc(self()) ==\r\n");
            printf("c_p: %#x\r\n",  (unsigned int) c_p);
            printf("proc: %#x\r\n", (unsigned int) proc);

	    erts_pix_lock(pix_lock);
	    if (flags & ERTS_P2P_FLG_SMP_INC_REFC)
		proc->lock.refc++;
	    erts_pix_unlock(pix_lock);
	    goto done;
	}
    }

    // lock critical section
    my_erts_pix_lock(pix_lock);

    // process_tab is a global variables
    // Pid to Process*
    proc = process_tab[pix];

    if (proc) { // proc != NULL
        erts_printf("               proc: %#x\r\n", (unsigned int)proc);
        erts_printf("       (target) pid: %T\r\n", pid);
        erts_printf("proc pid (proc->id): %T\r\n", proc->id);

	if (proc->id != pid || (!(flags & ERTS_P2P_FLG_ALLOW_OTHER_X)
				&& ERTS_PROC_IS_EXITING(proc))) {
            // process_tab updated, should happen rarely
	    proc = NULL;
	}
	else if (!need_locks) { // need_locks mean "locks needed"
            printf("need locks is zero (%#x), that means no need for any locks"
                   "\r\n", need_locks);
            printf("                    flags = %#x\r\n", flags);
            printf("ERTS_P2P_FLG_SMP_INC_REFC = %#x\r\n",
                   ERTS_P2P_FLG_SMP_INC_REFC);
	    if (flags & ERTS_P2P_FLG_SMP_INC_REFC) {
                printf("To increase proc->lock.refc (%ld)\r\n",
                       proc->lock.refc);
		proc->lock.refc++;
            } else {
                printf("HELP!!!\r\n");
            }
	}
	else { // Actually need some locks, maybe busy then
	    int busy;
            /* Try a quick trylock to grab all the locks we need. */
            busy = (int) erts_smp_proc_raw_trylock__(proc, need_locks);

	    if (!busy) {
		if (flags & ERTS_P2P_FLG_SMP_INC_REFC)
		    proc->lock.refc++;
	    }
	    else {
		if (flags & ERTS_P2P_FLG_TRY_LOCK)
		    proc = ERTS_PROC_LOCK_BUSY;
		else {
		    erts_pid2proc_safelock(c_p,
					   c_p_have_locks,
					   &proc,
					   pid_need_locks,
					   pix_lock,
					   flags);
		    if (proc && (flags & ERTS_P2P_FLG_SMP_INC_REFC))
			proc->lock.refc++;
		}
	    }
        }
    }

    // unlock
    erts_pix_unlock(pix_lock);
 done:
    printf("Finally, proc = %#x\r\n", (unsigned int) proc);
    printf("====  END  [my_erts_pid2proc_opt] ======\r\n\r\n");
    return proc;
}

/*
** Use this demo function to learn locks in Erlang internals
f(Pid).
Pid = spawn(timer, sleep, [infinity]).
wangjialock:demo_pid_to_process_pointer(Pid).
*/
BIF_RETTYPE wangjialock_demo_pid_to_process_pointer_1(BIF_ALIST_1)
{
    Process* p;      // Running Process
    Process* rp;     // Target Process
    Eterm pid;       // Target Pid

    p = BIF_P;
    pid = BIF_ARG_1;
    erts_printf("I'm %T and I want to get Process* for %T\r\n", p->id, pid);

    // get Process* for another process by its pid
    // => erl_process_lock.h:erts_pid2proc_opt
    // => ethr_mutex.h:ETHR_INLINE_MTX_FUNC_NAME_(ethr_mutex_lock)

    // WORKING, learn this erts_pid2proc_opt
    rp = my_erts_pid2proc_opt(p, // current Process*
                              ERTS_PROC_LOCK_MAIN, // c p have locks, 1
                              pid,                 // Pid
                              0,                   // pid_need_locsk, 0
                              ERTS_P2P_FLG_SMP_INC_REFC); // flags
    printf("  p: %#x\r\n", p);
    printf(" rp: %#x\r\n", rp);
    erts_printf("Now, rp (%T) rp->id (%T)\r\n", rp, rp->id);
    BIF_RET(rp->id);
}

/********* DEMO OUTPUT********************************************************
1> Pid = spawn(timer, sleep, [infinity]).
<0.34.0>
2> wangjialock:demo_pid_to_process_pointer(Pid).
I'm <0.32.0> and I want to get Process* for <0.34.0>

=== START [my_erts_pid2proc_opt] ====
                  Running pid is <0.32.0>
     Try to get Process* for pid <0.34.0>
[internal_pid_index] 34 for pid <0.34.0>
== locks ==
ERTS_PROC_LOCKS_ALL: 0xf
     pid_need_locks: 0
         need_locks: 0
           pix_lock: 0x876520
== START [my_erts_pix_lock] ==
== START [my_erts_mtx_lock] ==
== START [my_ethr_mutex_lock] ==
               proc: 0x156c8d0
       (target) pid: <0.34.0>
proc pid (proc->id): <0.34.0>
need locks is zero (0), that means no need for any locks
                    flags = 0x4
ERTS_P2P_FLG_SMP_INC_REFC = 0x4
To increase proc->lock.refc (1)
Finally, proc = 0x156c8d0
====  END  [my_erts_pid2proc_opt] ======

  p: 0x156b898
 rp: 0x156c8d0
Now, rp (<cp/header:0x000000000156c8d0>) rp->id (<0.34.0>)
<0.34.0>

******** END DEMO ************************************************************/

/* DEMO tmux commands
f().
Pid = spawn(timer, sleep, [infinity]).
register(haha, Pid).
wangjialock:demo_whereis(haha).
*/
BIF_RETTYPE wangjialock_demo_whereis_1(BIF_ALIST_1)
{
    Eterm res;
    if (is_not_atom(BIF_ARG_1)) {
	BIF_ERROR(BIF_P, BADARG);
    }
    // the structure is really simple, just a hash bucket
    // the difficult point is in lock
    // which locks are necessary etc.
    res = erts_whereis_name_to_id(BIF_P, BIF_ARG_1);
    erts_printf("== res : %T\r\n", res);
    BIF_RET(res);
}

      // TODO
      // 2, learn erlang:register/1 maybe better
      // code is short, to learn lock stuff
#else // #ifdef ERTS_SMP, just to make compile work

BIF_RETTYPE wangjialock_demo_pid_to_process_pointer_1(BIF_ALIST_1)
{
     BIF_RET(am_error);
}
BIF_RETTYPE wangjialock_demo_whereis_1(BIF_ALIST_1)
{
     BIF_RET(am_error);
}

#endif // #ifdef ERTS_SMP
