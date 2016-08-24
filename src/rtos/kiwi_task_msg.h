/* =========================================================
*
*     filename :  kiwi_task_msg.h
*       author :  Qiang Chen
*         date :  12/09/2015
*
*  description :  the header file of structure message
*
*===========================================================*/
#ifndef KIWI_TAKS_MSG_H
#define KIWI_TAKS_MSG_H

#include <stdlib.h>
#include "kiwi_data_types.h"

// every message starts with 0xAAAA
#define MESSAGE_HEADER 0xAAAA
/* this is the message buffer.
   Note, buffer does not contain any storage space.
   You need to allcoate buffer array outsize and 
   make buffer point to the address of the static array.
*/
typedef struct kiwi_msg_s {
  S32 head;   // head index
  S32 tail;   // tail index
  S32 size;
  char *buffer;
} kiwi_msg_t;


/* initiate message buffer */
void kiwi_init_msg(kiwi_msg_t *p_msg, char *p_buffer);

#endif
