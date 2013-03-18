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
