/*==========================================================
 *      name : kiwi_wrapper.h
 *    author : Qiang Chen
 *      date : 12//09/2015
 *  description: 
 *            this header file defiines the global variables
 ===========================================================*/
#ifndef KIWI_WRAPPER_H
#define KIWI_WRAPPER_H

#include <windows.h>
#include "kiwi_data_types.h"
#include "kiwi_task_msg.h"
#include "kiwi_tasks.h"

#define MAX_TASK_NAME_LENGTH (20)
#define MAX_MSG_BUFFER_LENGTH (4096)

    // define message header
    typedef struct header_struct {
        U32 source;
        U32 type;
        U32 length;
    } msg_header;


    void kiwi_setup(void);


#if _WIN32
    DWORD generic_thread_entry(LPVOID data);
#else
    void *generic_thread_entry(void *data);
#endif

    U16 kiwi_get_taskID(void);

    void kiwi_create_thread(U16 task_id,
        void(*entry_func)(void));

    int kiwi_create_task_thread(U16 task_id,
        const char *task_name,
        void(*entry_func)(void)
    );
    void kiwi_go(void);

    U16 kiwi_get_next_taskID(U16 task_id);

    void kiwi_run_next_task(void);

    int kiwi_get_message_buffer_space(kiwi_msg_t *p_msg);

    void kiwi_send_message(U16 task_id, void *msg_ptr, int bytes);

    void kiwi_get_message(U16 task_id, void *p_des);

#endif
