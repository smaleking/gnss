/* =========================================================
*
*     filename :  kiwi_task_msg.c
*       author :  Qiang Chen
*         date :  12/09/2015
*
*  description :  this source file define the msg related
*                 function
*
*===========================================================*/

#include "kiwi_task_msg.h"

/* initiate message buffer */
void kiwi_init_msg(kiwi_msg_t *p_msg, char *p_buffer)
{
  p_msg->head = 0;
  p_msg->tail = 0;
  p_msg->size = 0;
  p_msg->buffer = p_buffer;
}
