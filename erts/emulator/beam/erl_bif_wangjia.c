/*
** my C code to learn erts implementation
 */

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

/**************************************************************
 ****      Basic Types                                 ********
 **************************************************************/
/*
** To show system specific type size and 0 args
wangjia:demo_sizeof_types().
*/
// BIF_RETTYPE == Eterm [bif.h]
BIF_RETTYPE wangjia_demo_sizeof_types_0(BIF_ALIST_0)
{
  erts_printf("small_int : %T\r\n", make_small(558));
  erts_printf("sizeof(int) : %d\r\n", sizeof(int));       // 4
  erts_printf("sizeof(Uint) : %d\r\n", sizeof(Uint));     // 8
  erts_printf("sizeof(long) : %d\r\n", sizeof(long));     // 8
  erts_printf("sizeof(void *) : %d\r\n", sizeof(void *)); // 8
  erts_printf("sizeof(Eterm) : %d\r\n", sizeof(Eterm));   // 8
  // how to return 'ok', erl_atom_table.h
  // BIF_RET(x) == return(x)
  BIF_RET(am_ok);
}

/****************************************************************
 ************ Small Int                **************************
 ***************************************************************/
/*
wangjia:demo_is_small(3).
"Maximum: (1 bsl 59) - 1".
wangjia:demo_is_small((1 bsl 59) - 1).
wangjia:demo_is_small((1 bsl 60) - 1).
wangjia:demo_is_small("hello").
*/
BIF_RETTYPE wangjia_demo_is_small_1(BIF_ALIST_1)
{
  Eterm n = BIF_ARG_1; // BIF_ARG_1 is an Eterm

  if (is_small(n)) { // is_small
    erts_printf("n==(%T) is small\r\n", n);
    printf("RAW: %ld\r\n", n);
    BIF_RET(am_true); // return true
  } else if (is_not_small(n)) {
    erts_printf("n (%T) is NOT small\r\n", n);
    printf("RAW: %ld\r\n", n);
    BIF_RET(am_false); // return false
  }
}

/*
wangjia:demo_small(123).
*/
BIF_RETTYPE wangjia_demo_small_1(BIF_ALIST_1)
{
  Eterm n = BIF_ARG_1;
  // make_small
  erts_printf("small_int : %T\r\n", make_small(3));
  printf("RAW: %#x\r\n", make_small(3));

  printf("Address of BIF_ARG_1 (&BIF_ARG_1) == %ld\r\n", &BIF_ARG_1);

  BIF_RET(am_true);
}

/***********************************************************************
 *************** Lists                  ********************************
 ***********************************************************************/
/*
** This is good to know how memory used to indicate a list
wangjia:demo_integer_to_list(3456).
*/

// copied from utils.c
Eterm
utils_buf_to_intlist(Eterm** hpp, char *buf, size_t len, Eterm tail)
{
    Eterm* hp = *hpp;
    size_t i = len;

    while(i != 0) {
	--i;
        // CONS A | List
	tail = CONS(hp, make_small((Uint)(byte)buf[i]), tail);
        erts_printf("Now tail: %T\r\n", tail);
	hp += 2;
    }

    *hpp = hp;
    return tail;
}

// To learn internal memory structure, use list as a start
BIF_RETTYPE wangjia_demo_integer_to_list_1(BIF_ALIST_1)
{
    Eterm* hp; // heap pointer?
    Uint need;

    if (is_not_integer(BIF_ARG_1)) {
	BIF_ERROR(BIF_P, BADARG);
    }

    if (is_small(BIF_ARG_1)) {
	char *c;
	int n;
        Eterm e;
        Eterm *start;
        // MacOS defined(ARCH_64) && !HALFWORD_HEAP
	struct Sint_buf ibuf;

        erts_printf("Small Integer: %T\r\n", BIF_ARG_1);
        int val = signed_val(BIF_ARG_1);
	c = Sint_to_buf(val, &ibuf);
	n = sys_strlen(c);
        printf("String: %s\r\n", c);
        // 3456 -> "3456\0"
        // n -> strlen^ -> 4
        // need == 8
        erts_printf("String Length: %d\r\n", n);
	need = 2*n;
	hp = HAlloc(BIF_P, need);
        start = hp;
        e = utils_buf_to_intlist(&hp, c, n, NIL);

        printf("RAW hp: %#x\r\n", hp);
        printf("RAW e: %#x\r\n", e);  // e is a list 0xnnnnnnn...1
        printf("RAW list_val(e): %#x\r\n", list_val(e));
        printf("ptr_val: %ld\r\n", ptr_val(*hp));

        printf("== Allocated Memory from %#x to %#x\r\n", start, start+need);
        printf("== Now hp is %#x\r\n", hp);
        print_tagged_memory(start - 1, start + need + 1);
	BIF_RET(e);
    }
    else {
      printf("ERROR, can only handle small integers\r\n");
      BIF_RET(am_false);
    }
}

/**************
1> wangjia:demo_integer_to_list(3456).
Small Integer: 3456
String: 3456
String Length: 4
Now tail: "6"
Now tail: "56"
Now tail: "456"
Now tail: "3456"
RAW hp: 0x5f765748
ptr_val: 268
== Allocated Memory from 0x5f765708 to 0x5f765748
== Now hp is 0x5f765748
+--------------------+--------------------+
| 0x00007fa95f765700 - 0x00007fa95f765748 |
| Address            | Contents           |
|--------------------|--------------------|
| 0x00007fa95f765700 | 0x000000000000ca8b | demo_integer_to_list
| 0x00007fa95f765708 | 0x000000000000036f | 54
| 0x00007fa95f765710 | 0xfffffffffffffffb | []
| 0x00007fa95f765718 | 0x000000000000035f | 53
| 0x00007fa95f765720 | 0x00007fa95f765709 | "6"
| 0x00007fa95f765728 | 0x000000000000034f | 52
| 0x00007fa95f765730 | 0x00007fa95f765719 | "56"
| 0x00007fa95f765738 | 0x000000000000033f | 51
| 0x00007fa95f765740 | 0x00007fa95f765729 | "456"
| 0x00007fa95f765748 | 0x000000000000010f | 16
+--------------------+--------------------+
"3456"
************/

int
my_print_list(Eterm list)
{
    int i = 0;
    Eterm car;
    Eterm cdr;

    while(is_list(list)) {
      i++;
      printf("\r\n\r\n=== is list ===\r\n");
      printf("is_list(list) == %d\r\n", is_list(list)); // is_list
      printf("RAW in list: %#x  <---- should be 0xnnn...1/9\r\n", list);
      erts_printf("Current List: %T\r\n", list);

      // list_val(..) -> a normal address, not an Eterm anymore!
      // WRONG: erts_print("Elem: %d :%T\r\n", i, *(list_val(list)));
      printf("list_val(list) => %#x   <=== should be 0xnnnn...0/8\r\n",
             list_val(list)); // list_val

      car = CAR(list_val(list));
      cdr = CDR(list_val(list));

      printf("RAW [*((Eterm *)list_val(list)), *((Eterm *)list_val(list) + 1)]: [%#x, %#x]\r\n ", *((Eterm *)list_val(list)), *((Eterm *)list_val(list) + 1));
      printf("RAW [CAR(list_val(list)), CDR(list_val(list))] = [%#x, %#x]\r\n", car, cdr);

      // move 'list' point to next cell
      list = CDR(list_val(list));
      if(is_list(list)) {
        printf("list is still a list\r\n");
      } else {
        printf("Haha, list is not a list anymore\r\n");
      }

    }
    if (is_not_nil(list)) {
	return -1;
    }
    return i;
}
/*
wangjia:demo_print_list([1,2,3]).
*/
BIF_RETTYPE wangjia_demo_print_list_1(BIF_ALIST_1)
{
  Eterm list;
  int i;
  list = BIF_ARG_1;
  erts_printf("Input RAW  : %#x\r\n", list);
  erts_printf("Input List : %T\r\n", BIF_ARG_1);
  i = my_print_list(list);
  printf("Length: %d\r\n", i);
  BIF_RET(am_ok);
}

/**************** DEMO ******************************************
1> wangjia:demo_print_list([1,2,3]).
Input RAW  : 0x1fa4cbe1
Input List : [1,2,3]


=== is list ===
is_list(list) == 1
RAW in list: 0x1fa4cbe1  <---- should be 0xnnn...1/9
Current List: [1,2,3]
list_val(list) => 0x1fa4cbe0   <=== should be 0xnnnn...0/8
RAW [*((Eterm *)list_val(list)), *((Eterm *)list_val(list) + 1)]: [0x1f, 0x1fa4cb79]
 RAW [CAR(list_val(list)), CDR(list_val(list))] = [0x1f, 0x1fa4cb79]
list is still a list


=== is list ===
is_list(list) == 1
RAW in list: 0x1fa4cb79  <---- should be 0xnnn...1/9
Current List: [2,3]
list_val(list) => 0x1fa4cb78   <=== should be 0xnnnn...0/8
RAW [*((Eterm *)list_val(list)), *((Eterm *)list_val(list) + 1)]: [0x2f, 0x1fa4cb11]
 RAW [CAR(list_val(list)), CDR(list_val(list))] = [0x2f, 0x1fa4cb11]
list is still a list


=== is list ===
is_list(list) == 1
RAW in list: 0x1fa4cb11  <---- should be 0xnnn...1/9
Current List: [3]
list_val(list) => 0x1fa4cb10   <=== should be 0xnnnn...0/8
RAW [*((Eterm *)list_val(list)), *((Eterm *)list_val(list) + 1)]: [0x3f, 0xfffffffb]
 RAW [CAR(list_val(list)), CDR(list_val(list))] = [0x3f, 0xfffffffb]
Haha, list is not a list anymore
Length: 3
ok
*******************************************************************************/

int
my_list_length_copied_from_utils_c(Eterm list)
{
    int i = 0;

    while(is_list(list)) {
	i++;
	list = CDR(list_val(list));
    }
    if (is_not_nil(list)) {
	return -1;
    }
    return i;
}

BIF_RETTYPE wangjia_demo_length_1(BIF_ALIST_1)
{
  int i;
  i = my_list_length_copied_from_utils_c(BIF_ARG_1);
  printf("Length: %d\r\n", i);
  BIF_RET(make_small(i));
}

/*
wangjia:demo_types(123).
wangjia:demo_types(123.456).
wangjia:demo_types([]).
wangjia:demo_types([1,2,3]).
wangjia:demo_types(self()).
wangjia:demo_types(haha).
wangjia:demo_types(<<"hello, world">>).
*/
BIF_RETTYPE wangjia_demo_types_1(BIF_ALIST_1)
{
  Eterm *p;
  p = &BIF_ARG_1;

  printf("RAW: %#x\r\n", BIF_ARG_1);

  // is_boxed
  if (is_boxed(BIF_ARG_1)) {
    printf("is_boxed\r\n");
  }

  // is_header
  if (is_header(BIF_ARG_1)) {
    printf("is_header\r\n");
  }

  // is_nil
  if (is_nil(BIF_ARG_1)) {
    erts_printf("Empty List (NIL): %T\r\n", BIF_ARG_1);
  }

  // is_atom
  if (is_atom(BIF_ARG_1)) {
    erts_printf("Atom : %T\r\n", BIF_ARG_1);
  }

  // is_list
  if (is_list(BIF_ARG_1)) {
    erts_printf("List: %T\r\n", BIF_ARG_1);
  }

  // is_pid
  if (is_pid(BIF_ARG_1)) {
    Uint pix;
    Eterm pid;
    pid = BIF_ARG_1;
    pix = internal_pid_index(pid);
    printf("pix: %#x\r\n", pix);
    printf("erts_max_processes: %#x\r\n", erts_max_processes);
    erts_printf("Pid: %T\r\n", BIF_ARG_1);
  }

  // is_binary
  if (is_binary(BIF_ARG_1)) {
    erts_printf("Is Binary: %T\r\n", BIF_ARG_1);
  }
  // is_binary_header
  if (is_binary_header(BIF_ARG_1)) {
    erts_printf("Is Binary Header: %T\r\n", BIF_ARG_1);
  }

  print_tagged_memory(p, p + 1);

  BIF_RET(am_ok);
}

/************
1> wangjia:demo_types(123).
RAW: 0x7bf
+--------------------+--------------------+
| 0x00007f8477ce5040 - 0x00007f8477ce5040 |
| Address            | Contents           |
|--------------------|--------------------|
| 0x00007f8477ce5040 | 0x00000000000007bf | 123
+--------------------+--------------------+
ok
2> wangjia:demo_types(123.456).
RAW: 0x77cd8cca
is_boxed
+--------------------+--------------------+
| 0x00007f8477ce5040 - 0x00007f8477ce5040 |
| Address            | Contents           |
|--------------------|--------------------|
| 0x00007f8477ce5040 | 0x00007f8477cd8cca | 1.234560e+02
+--------------------+--------------------+
ok
3> wangjia:demo_types([]).
RAW: 0xfffffffb
Empty List (NIL): []
+--------------------+--------------------+
| 0x00007f8477ce5040 - 0x00007f8477ce5040 |
| Address            | Contents           |
|--------------------|--------------------|
| 0x00007f8477ce5040 | 0xfffffffffffffffb | []
+--------------------+--------------------+
ok

4> wangjia:demo_types([1,2,3]).
RAW: 0x70e5fe19
List: [1,2,3]
+--------------------+--------------------+
| 0x00007f8477ce5040 - 0x00007f8477ce5040 |
| Address            | Contents           |
|--------------------|--------------------|
| 0x00007f8477ce5040 | 0x00007f8470e5fe19 | [1,2,3]
+--------------------+--------------------+
ok
5> wangjia:demo_types(self()).
RAW: 0x203
Pid: <0.32.0>
+--------------------+--------------------+
| 0x00007f8477ce5040 - 0x00007f8477ce5040 |
| Address            | Contents           |
|--------------------|--------------------|
| 0x00007f8477ce5040 | 0x0000000000000203 | <0.32.0>
+--------------------+--------------------+
ok
6> wangjia:demo_types(haha).
RAW: 0x68f8b
Atom : haha
+--------------------+--------------------+
| 0x00007f8477ce5040 - 0x00007f8477ce5040 |
| Address            | Contents           |
|--------------------|--------------------|
| 0x00007f8477ce5040 | 0x0000000000068f8b | haha
+--------------------+--------------------+
ok
7> wangjia:demo_types(<<"hello, world">>).
RAW: 0x70b52512
is_boxed
Is Binary: <<12 bytes>>
+--------------------+--------------------+
| 0x00007f8477ce5040 - 0x00007f8477ce5040 |
| Address            | Contents           |
|--------------------|--------------------|
| 0x00007f8477ce5040 | 0x00007f8470b52512 | <<12 bytes>>
+--------------------+--------------------+
ok
*********************/

/*******************************************************************************
** Learn Tuples
********************************************************************************/
/*
wangjia:demo_is_tuple(haha).
wangjia:demo_is_tuple({1,2,3}).
wangjia:demo_is_tuple({1,2,3,4,5}).
wangjia:demo_is_tuple({1,2,{another,tuple,inside},4,5}).
*/
/*
** From this Tuple demo, I know the tuple implementation and what is HEADER
** and what is boxed type.
** Also, it is clear why it is efficient to use tuple to get a specific element
** internally, a tuple is an array, much faster than a list to do sub-index access
*/
BIF_RETTYPE wangjia_demo_is_tuple_1(BIF_ALIST_1)
{
  Eterm tuple = BIF_ARG_1;
  Eterm *start_p;
  Uint arity;
  // _TAG_PRIMARY_MASK	0x3
  // TAG_PRIMARY_BOXED	0x2
  // _is_not_boxed(x)	((x) & (_TAG_PRIMARY_MASK-TAG_PRIMARY_BOXED))
  // _is_not_boxed(x)	((x) & (1)
  // xxxxxxxx...1 -> not box, is list :)
  // xxxxxxxx...0 -> is box
  printf("RAW: %#x\r\n", tuple);
  if (is_tuple(tuple)) { // is_tuple
    // Basically, box type means something like this: [N=3][X1][X2][X3]
    // [HEADER][V1][V2]...[Vn] where HEADER indicate the size

    //#define is_tuple(x)     (is_boxed((x)) && is_arity_value(*boxed_val((x))))
    // is_boxed ===>>> end with 0
    //#define is_boxed(x)     _ET_APPLY(is_boxed,(x))
    //#define _unchecked_is_boxed(x) (!_is_not_boxed((x)))
    //#define _is_not_boxed(x)	((x) & (_TAG_PRIMARY_MASK-TAG_PRIMARY_BOXED))
    // is_arity_value(*boxed_val(x))
    //#define _unchecked_boxed_val(x)
    //          ((Eterm*) EXPAND_POINTER(((x) - TAG_PRIMARY_BOXED))
    start_p = boxed_val(tuple);
    arity = arityval(*boxed_val(tuple));
    printf("Tuple Arity: %u\r\n", arity);
    print_tagged_memory(start_p, start_p + arity + 1);
    erts_printf("Eterm (%T) is a tuple\r\n", tuple);
    BIF_RET(am_true);
  } else {
    erts_printf("Eterm (%T) is NOT a tuple\r\n", tuple);
    BIF_RET(am_false);
  }
}

/***************** DEMO wangjia:is_tuple/1 *******************************
3> wangjia:demo_is_tuple({1,2,{another,tuple,inside},4,5}).
RAW: 0x1df1a662
Tuple Arity: 5
+--------------------+--------------------+
| 0x000000001df1a660 - 0x000000001df1a688 |
| Address            | Contents           |
|--------------------|--------------------|
| 0x000000001df1a660 | 0x0000000000000140 | Arity(5)
| 0x000000001df1a668 | 0x000000000000001f | 1
| 0x000000001df1a670 | 0x000000000000002f | 2
| 0x000000001df1a678 | 0x000000001df1a4a2 | {another,tuple,inside}
| 0x000000001df1a680 | 0x000000000000004f | 4
| 0x000000001df1a688 | 0x000000000000005f | 5
+--------------------+--------------------+
Eterm ({1,2,{another,tuple,inside},4,5}) is a tuple
true
************** DEMO is_tuple END ***********************************/

/*
wangjia:demo_now().
*/
BIF_RETTYPE wangjia_demo_now_0(BIF_ALIST_0)
{
  Uint megasec, sec, microsec;
  Eterm* hp;

  // get_now is not a system call!
  // Erlang adds some lock on raw system call to make sure now always
  // returns a unique value
  get_now(&megasec, &sec, &microsec);
  hp = HAlloc(BIF_P, 6); // 1 for size, 3 for now, 2 for my dummy
  BIF_RET(TUPLE5(hp,
                 make_small(megasec), make_small(sec), make_small(microsec),
                 make_small(999), make_small(12345)));
}

/*
wangjia:demo_date().
*/
BIF_RETTYPE wangjia_demo_date_0(BIF_ALIST_0)
{
  int year, month, day;
  Eterm* hp;
  get_date(&year, &month, &day);
  hp = HAlloc(BIF_P, 4);  /* {year, month, day}  + arity */
  BIF_RET(TUPLE3(hp, make_small(year), make_small(month), make_small(day)));
}

BIF_RETTYPE wangjia_demo_tuple_1(BIF_ALIST_1)
{
  BIF_RET(am_ok);
}

/************************************************************************
******************* Processes *******************************************
*************************************************************************/
/*
erlang:spawn(fun() -> wangjia:demo_process_basic() end).

'have some message queue'
erlang:send(erlang:spawn(fun() -> timer:sleep(500), wangjia:demo_process_basic() end), 5).
**
*/
BIF_RETTYPE wangjia_demo_process_basic_0(BIF_ALIST_0)
{
  Eterm bif_info_tuple;
  printf("\r\n");
  // try some simple erlang:F/0 to learn process structure

  // erlang:group_leader/0
  erts_printf("group_leader:        %T\r\n", BIF_P->group_leader);
  printf("group_leader (RAW):  %#x\r\n", BIF_P->group_leader);
  // erlang:self/0
  erts_printf("self(): %T\r\n", BIF_P->id);

  // DEBUG functions
  erts_check_heap(BIF_P);

  // struct process in erl_process.h:616
  printf("============ Memory Info ==============\r\n");
  printf("htop: %#x\r\n", BIF_P -> htop); /* Heap top */
  printf("stop: %#x\r\n", BIF_P -> stop); /* Stack top */
  printf("heap: %#x\r\n", BIF_P -> heap); /* Heap start */
  printf("hend: %#x\r\n", BIF_P -> hend); /* Heap end */

  // print memory, will call print_process_memory/1 which is a static function
  print_memory(BIF_P);
  print_memory_info(BIF_P);

  // debug functions in erl_process.c
  bif_info_tuple = erts_debug_processes_bif_info(BIF_P);
  erts_printf("== Process BIF Info ==\r\n%T\r\n", bif_info_tuple);

  BIF_RET(am_ok);
}

/*
** Use this demo function to learn locks in Erlang internals
f(Pid).
Pid = spawn(timer, sleep, [infinity]).
wangjia:demo_pid_to_process_pointer(Pid).
*/
BIF_RETTYPE wangjia_demo_pid_to_process_pointer_1(BIF_ALIST_1)
{
    Process* p;      // Running Process
    Process* rp;     // Target Process
    Eterm pid;       // Target Pid

    p = BIF_P;
    pid = BIF_ARG_1;
    erts_printf("I'm %T and I want to get Process* for %T\r\n", p->id, pid);

    // get Process* for another process by its pid
    // WORKING
    // => erl_process_lock.h:erts_pid2proc_opt
    // => ethr_mutex.h:ETHR_INLINE_MTX_FUNC_NAME_(ethr_mutex_lock)

    printf("\r\n==================== BEGIN =============\r\n");
    rp = erts_pid2proc_opt(p, // current Process*
                           ERTS_PROC_LOCK_MAIN, // c p have locks, 1
                           pid,                 // Pid
                           0,                   // pid_need_locsk, 0
                           ERTS_P2P_FLG_SMP_INC_REFC); // flags
    printf(" p: %#x\r\n", p);
    printf("rp: %#x\r\n", rp);
    printf("\r\n==================== END =============\r\n");
    erts_printf("Now, rp (%T) rp->id (%T)\r\n", rp, rp->id);
    BIF_RET(rp->id);
}


/*
"****** DEMOE have some message queue ****".
begin
f(Pid),
Pid = spawn(fun() -> timer:sleep(200), wangjialock:demo_message_basic() end),
Pid ! 5,
Pid ! "hello",
Pid ! 3
end.

"****** DEMOE empty message queue ****".
spawn(fun() -> wangjia:demo_message_basic() end).

**
*/
BIF_RETTYPE wangjia_demo_message_basic_0(BIF_ALIST_0)
{
  Process *p = BIF_P;

  // erl_process.h:616, struct process {...}
  // ErlMessageQueue msg;  /* Message queue */

  printf("p->msg.first: %#x\r\n", p->msg.first);

  // ErlMessage size
  printf("sizeof(ErlMessage) == %d\r\n", sizeof(ErlMessage));

  if(p->msg.first != NULL) {
    ErlMessage* mp;
    int i = 1;
    printf("Message Queue is NOT empty\r\n");

    // ErlMessageQueue erl_message.h:101
    printf("ErlMessageQueue length: %d\r\n", p->msg.len);

    mp = p->msg.first;
    while (mp != NULL) {
      printf("==== Message %d ======\r\n", i);
      // message term
      printf("Term (RAW):  %#x\r\n", ERL_MESSAGE_TERM(mp));
      erts_printf("Term (pretty):%T\r\n", ERL_MESSAGE_TERM(mp));
      // what is token?
      printf("Token: %#x\r\n", ERL_MESSAGE_TOKEN(mp));
      i++;
      mp = mp->next;
    }
  } else {
    printf("Message Queue is empty\r\n");
  }

  BIF_RET(am_ok);
}

/*
f(Pid).
Pid = spawn(timer, sleep, [infinity]).
wangjia:demo_send_message(Pid, 3).
wangjia:demo_send_message(Pid, 4).
wangjia:demo_send_message(Pid, 5).
wangjia:demo_print_message_queue(Pid).

*/

BIF_RETTYPE wangjia_demo_send_message_2(BIF_ALIST_2)
{
  erts_printf("Sending Message (%T) from %T to %T\r\n",
              BIF_ARG_2, BIF_P->id, BIF_ARG_1);
  // Learn internals
  erl_send(BIF_P, BIF_ARG_1, BIF_ARG_2);
  BIF_RET(am_ok);
}

/*
self() ! haha.
wangjia:demo_print_message_queue(self()).
*/
BIF_RETTYPE wangjia_demo_print_message_queue_1(BIF_ALIST_1)
{
    Process* p;      // Running Process
    Process* rp;     // Target Process
    Eterm pid;       // Target Pid

    p = BIF_P;
    pid = BIF_ARG_1;
    erts_printf("== Show message queue for %T\r\n", pid);

    // use locks to get Process* for pid
    rp = erts_pid2proc_opt(p, ERTS_PROC_LOCK_MAIN, pid,
                           0, ERTS_P2P_FLG_SMP_INC_REFC);
    if (rp == NULL) {
        erts_printf("%T not exists\r\n", pid);
        BIF_RET(am_ok);
    }
    erts_printf("Now, rp (%T) rp->id (%T)\r\n", rp, rp->id);
    if(rp->msg.first != NULL) {
        ErlMessage* mp;
        int i = 1;
        printf("Message Queue is NOT empty\r\n");

        mp = rp->msg.first;
        while (mp != NULL) {
            printf("Message %d: ", i);
            erts_printf("Term (pretty):%T\r\n", ERL_MESSAGE_TERM(mp));
            i++;
            mp = mp->next;
        }
    } else {
        printf("Message Queue is empty\r\n");
    }
    BIF_RET(am_ok);
}
