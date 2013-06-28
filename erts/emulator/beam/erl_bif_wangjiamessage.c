#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sys.h"
#include "global.h"
#include "erl_alloc.h"
#include "erl_process_lock.h"
#include "bif.h"

#ifdef ERTS_SMP // focus on ERTS_SMP code at this moment

// otp_src_R15B03/erts/emulator/beam/erl_process_lock.h
ERTS_GLB_INLINE void
my_erts_smp_proc_unlock__(Process *p,
                          erts_pix_lock_t *pix_lck,
                          ErtsProcLocks locks)
{
    ErtsProcLocks old_lflgs;
    ETHR_COMPILER_BARRIER;

    old_lflgs = ERTS_PROC_LOCK_FLGS_READ_(&p->lock);

    ERTS_LC_ASSERT((locks & ~ERTS_PROC_LOCKS_ALL) == 0);
    ERTS_LC_ASSERT(locks == (old_lflgs & locks));

    while (1) {
        /*
         * We'll atomically unlock every lock that has no waiter.
         * If any locks with waiters remain we'll let
         * erts_proc_unlock_failed() deal with them.
         */
        ErtsProcLocks wait_locks =
            (old_lflgs >> ERTS_PROC_LOCK_WAITER_SHIFT) & locks;

        /* What p->lock will look like with all non-waited locks released. */
        ErtsProcLocks want_lflgs = old_lflgs & (wait_locks | ~locks);

        if (want_lflgs != old_lflgs) {
            ErtsProcLocks new_lflgs =
                ERTS_PROC_LOCK_FLGS_CMPXCHG_RELB_(&p->lock, want_lflgs, old_lflgs);

            if (new_lflgs != old_lflgs) {
                /* cmpxchg failed, try again. */
                old_lflgs = new_lflgs;
                continue;
            }
        }

        /* We have successfully unlocked every lock with no waiter. */

        if (want_lflgs & locks) {
            /* Locks with waiters remain. */
            /* erts_proc_unlock_failed() returns with pix_lck unlocked. */
            erts_proc_unlock_failed(p, pix_lck, want_lflgs & locks);
        }
        else {
        }

        break;
    }
}

ERTS_GLB_INLINE void
my_erts_smp_proc_unlock(Process *p, ErtsProcLocks locks)
{
#ifdef ERTS_SMP
    erts_smp_proc_unlock__(p, NULL, locks);
#endif
}

/*********** DEMO  send a message to another process **********
** Basically, there are two ways to send a message:
** 1) Pid ! Message    =====>>>>> ebif_bang_2
         BIF_RETTYPE
         ebif_bang_2(BIF_ALIST_2)
         {
             return erl_send(BIF_P, BIF_ARG_1, BIF_ARG_2);
         }

** 2) erlang:send(Pid, Message) -->> send_2
         BIF_RETTYPE send_2(BIF_ALIST_2)
         {
             erts_printf("process: %T\r\n", BIF_P->id);
             erts_printf("Arg1: %T\r\n", BIF_ARG_1);
             erts_printf("Arg2: %T\r\n", BIF_ARG_2);
             return erl_send(BIF_P, BIF_ARG_1, BIF_ARG_2);
         }

    4> erlang:send(Pid, kkkkkkkkkkkk).
    process: <0.31.0>
    Arg1: <0.35.0>
    Arg2: kkkkkkkkkkkk
**
*/
/**** Demo tmux commands
f(Pid).
Pid = spawn(timer, sleep, [infinity]).
wangjiamessage:demo_send_message(Pid, 3).
wangjiamessage:demo_send_message(Pid, 4).
wangjiamessage:demo_send_message(Pid, 5).
wangjia:demo_print_message_queue(Pid).

*/
// All functions used to send messages
Eterm my_erl_send(Process *p, Eterm to, Eterm msg);
Sint   my_do_send(Process *p, Eterm to, Eterm msg, int suspend);
#define SEND_BADARG (-4)
void
my_erts_send_message(Process* sender,
                     Process* receiver,
                     ErtsProcLocks *receiver_locks,
                     Eterm message,
                     unsigned flags);

// otp_src_R15B03/erts/emulator/beam/bif.c [bif:send_2]
// my implementation for send_2
BIF_RETTYPE wangjiamessage_demo_send_message_2(BIF_ALIST_2)
{
    erts_printf("Sending Message (%T) from %T to %T\r\n",
                BIF_ARG_2, BIF_P->id, BIF_ARG_1);
    my_erl_send(BIF_P, BIF_ARG_1, BIF_ARG_2);
    BIF_RET(am_ok);
}

// This function is just a wrapper and call real
Eterm my_erl_send(Process *p, Eterm to, Eterm msg)
{
    Sint result = my_do_send(p, to, msg, !0);
    printf("== my_erl_send result: %d\r\n", result);
    if (result > 0) {
	ERTS_VBUMP_REDS(p, result);
	BIF_RET(msg);
    }
    ASSERT(! "Wangjiamessage Ignores Errors");
    BIF_ERROR(p, BADARG);
}

// mainly copied from bif:do_send/4, but simplified and only possible
// to send message to pid
Sint
my_do_send(Process *p, Eterm to, Eterm msg, int suspend)
{
    Process* rp;
    DistEntry *dep;
    Eterm* tp;

    // Only considers internal pid now, get receiver's Process*
    if (is_internal_pid(to)) {
	if (IS_TRACED(p)) // trace only works if you tell it
	    trace_send(p, to, msg);
	if (ERTS_PROC_GET_SAVED_CALLS_BUF(p))
	    save_calls(p, &exp_send);
	if (internal_pid_index(to) >= erts_max_processes)
	    return SEND_BADARG;

        // get Process* for receiver, detailed analysis see my_ function
	rp = erts_pid2proc_opt(p, ERTS_PROC_LOCK_MAIN,
			       to, 0, ERTS_P2P_FLG_SMP_INC_REFC);
	if (!rp) {
	    ERTS_SMP_ASSERT_IS_NOT_EXITING(p);
	    return 0;
	}
        erts_printf("rp pid: %T\r\n", rp->id);
    }
 send_message: {
        ErtsProcLocks rp_locks = 0;
        Sint res;
#ifdef ERTS_SMP
        // Usually ERTS_SMP
        if (p == rp) // self() send message to self()
            rp_locks |= ERTS_PROC_LOCK_MAIN;
#endif
        /* send to local process */
        my_erts_send_message(p, rp, &rp_locks, msg, 0);
        printf("== After send message, rp_locks == %#x\r\n", rp_locks);
        if (!erts_use_sender_punish)
            res = 0;
        else {
#ifdef ERTS_SMP
            // Process * -> msg_inq is special for ERTS_SMP
            // But what is it?
            res = rp->msg_inq.len*4;
            printf("some punishment (%d) for sender process\r\n", res);
            printf("ERTS_PROC_LOCK_MAIN : %#x\r\n", ERTS_PROC_LOCK_MAIN);
            printf("           rp_locks : %#x\r\n", rp_locks);
            printf("LOCKMAIN & rp_locsk : %#x\r\n",
                   ERTS_PROC_LOCK_MAIN & rp_locks);
            if (ERTS_PROC_LOCK_MAIN & rp_locks)
                res += rp->msg.len*4;
            // it becomes more expensive to send a message to a process with
            // a really long message queue
            printf("      rp->msg.len*4 : %#x\r\n", rp->msg.len*4);
#else
            res = rp->msg.len*4;
#endif
        }
        // unlock
        my_erts_smp_proc_unlock(rp,
                                p == rp
                                ? (rp_locks & ~ERTS_PROC_LOCK_MAIN)
                                : rp_locks); // if not self()
        // Aha, here, it will decrease the refc
        erts_smp_proc_dec_refc(rp);
        return res;
    }
}
/***** message_alloc() and message_free(), see erl_alloc.h:326 **********
 ***** But message_alloc() is actually not called in my erl    *********
static ERTS_INLINE TYPE *						\
NAME##_alloc(void)							\
{									\
    TYPE *res = NAME##_pre_alloc();					\
    if (!res)								\
	res = erts_alloc(ALCT, sizeof(TYPE));				\
    return res;								\
}									\
static ERTS_INLINE void							\
NAME##_free(TYPE *p)							\
{									\
    if (!NAME##_pre_free(p))						\
	erts_free(ALCT, (void *) p);					\
}
************************************************************************/
void
my_erts_send_message(Process* sender,
                     Process* receiver,
                     ErtsProcLocks *receiver_locks,
                     Eterm message,
                     unsigned flags)
{
    Uint msize;
    ErlHeapFragment* bp = NULL;
    Eterm token = NIL;
    printf("my function to send messages! haha?\r\n");
    // some benchmark stuff
    BM_STOP_TIMER(system);
    BM_MESSAGE(message,sender,receiver);
    BM_START_TIMER(send);
    ErlOffHeap *ohp;
    Eterm *hp;
    BM_SWAP_TIMER(send,size);
    // object size
    msize = size_object(message);
    BM_SWAP_TIMER(size,send);
    // allocate memory
    hp = erts_alloc_message_heap(msize,&bp,&ohp,receiver,receiver_locks);
    BM_SWAP_TIMER(send,copy);
    // message copied, copy.c
    message = copy_struct(message, msize, &hp, ohp);
    BM_MESSAGE_COPIED(msz);
    BM_SWAP_TIMER(copy,send);
    DTRACE6(message_send, sender_name, receiver_name,
            msize, tok_label, tok_lastcnt, tok_serial);
    // add message to receiver's msg queue
    erts_queue_message(receiver, receiver_locks, bp, message, token);
    BM_SWAP_TIMER(send,system);
    return;
}

#else

BIF_RETTYPE wangjiamessage_demo_send_message_2(BIF_ALIST_2)
{
    BIF_RET(am_error);
}

#endif
