/* my C code for wangjia:hiwangjia/1
 * To implement this file, I simply copy from other existing code
 * header files see erl_bif_lists.c
 */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sys.h"
#include "erl_vm.h"
#include "global.h"
#include "erl_process.h"
#include "error.h"
#include "bif.h"

/* Args 0 see erl_db.c ets_all_0
 * Return Value see erl_bif_lists.c lists_member_2
 */
BIF_RETTYPE wangjia_hiwangjia_1(BIF_ALIST_1)
{
  //printf("Hello, my erl_bif_jiawang.c\r\n");
  printf("RAW: %d\r\n", BIF_ARG_1);
  if (is_nil(BIF_ARG_1)) {
    erts_printf("Empty List (NIL): %T\r\n", BIF_ARG_1);
  }
  else if (is_atom(BIF_ARG_1)) {
    erts_printf("Atom : %T\r\n", BIF_ARG_1);
  }
  else if (is_list(BIF_ARG_1)) {
    printf("RAW: %d\r\n", BIF_ARG_1);
    printf("BIF_ARG_1 address: %d\n", &BIF_ARG_1);
    printf("========= Tagged Memory ==========\n");
    print_tagged_memory(ptr_val(BIF_ARG_1), ptr_val(BIF_ARG_1) + 100);

    /* printf("========= Untagged Memory ==========\n"); */
    /* print_untagged_memory(ptr_val(BIF_ARG_1), ptr_val(BIF_ARG_1) + 20); */

    /* print_untagged_memory(&BIF_ARG_1, &BIF_ARG_1 + 10); */
    erts_printf("jiawang List: %T\r\n", BIF_ARG_1);
  }
  else if (is_pid(BIF_ARG_1)) {
    erts_printf("Pid: %T\r\n", BIF_ARG_1);
    printf("RAW: %d\n", BIF_ARG_1);
    print_memory(BIF_P);
  }
  else {
    erts_printf("Others: %T\r\n", BIF_ARG_1);
  }
  BIF_RET(am_false);
}


/* convert an integer to a list of ascii integers */

/*
wangjia:integer2list(3456).
*/

BIF_RETTYPE wangjia_integer2list_1(BIF_ALIST_1)
{
    Eterm* hp;
    Uint need;

    if (is_not_integer(BIF_ARG_1)) {
	BIF_ERROR(BIF_P, BADARG);
    }

    if (is_small(BIF_ARG_1)) {
	char *c;
	int n;
        // MacOS defined(ARCH_64) && !HALFWORD_HEAP
	struct Sint_buf ibuf;

        erts_printf("Small Integer: %T\r\n", BIF_ARG_1);
        int val = signed_val(BIF_ARG_1);
	c = Sint_to_buf(val, &ibuf);
	n = sys_strlen(c);
        printf("String: %s\r\n", c);
        erts_printf("String Length: %d\r\n", n);
	need = 2*n;
	hp = HAlloc(BIF_P, need);
	BIF_RET(buf_to_intlist(&hp, c, n, NIL));
    }
    else {
      printf("ERROR, can only handle small integers\r\n");
      BIF_RET(am_false);
    }
}

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

/*
wangjia:mylength([1,2,3]).
*/
BIF_RETTYPE wangjia_mylength_1(BIF_ALIST_1)
{
  int i;
  i = list_length(BIF_ARG_1);
  printf("Length: %d\r\n", i);
  BIF_RET(am_false);
}


int
my_print_list(Eterm list)
{
    int i = 0;

    while(is_list(list)) {
      printf("is list\r\n");
      i++;
      /* This doesn't work! */
      // erts_print("Elem: %d :%T\r\n", i, list_val(list));
      list = CDR(list_val(list));
    }
    if (is_not_nil(list)) {
	return -1;
    }
    return i;
}


/*
wangjia:print_list([1,2,3]).
*/
BIF_RETTYPE wangjia_print_list_1(BIF_ALIST_1)
{
  int i;
  printf("my function to print a list\r\n");
  i = my_print_list(BIF_ARG_1);
  printf("Length: %d\r\n", i);
  BIF_RET(am_true);
}


