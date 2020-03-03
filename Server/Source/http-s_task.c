/*
*********************************************************************************************************
*                                               uC/HTTP
*                                     Hypertext Transfer Protocol
*
*                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                       HTTP SERVER TASK MODULE
*
* Filename : http-s_task.c
* Version  : V3.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    HTTPs_TASK_MODULE

#include  "http-s_task.h"
#include  "http-s_sock.h"
#include  "http-s_conn.h"
#include  "http-s_mem.h"
#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED)
#if defined(HTTPs_CfgFS_Dyn)
#include  <Source/fs.h>
#endif
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPs_TASK_STR_NAME_TASK                  "HTTP Instance"
#define  HTTPs_TASK_STR_NAME_LOCK                  "HTTP Instance Lock"
#define  HTTPs_TASK_STR_NAME_SEM_STOP_REQ          "HTTP Instance Stop req"
#define  HTTPs_TASK_STR_NAME_SEM_STOP_COMPLETED    "HTTP Instance Stop compl"
#define  HTTPs_TASK_STR_NAME_TMR                   "HTTP Conn Timeout"

#define  HTTPs_OS_LOCK_ACQUIRE_FAIL_DLY_MS                    5u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  HTTPsTask_InstanceTask        (void            *p_data);

static  void  HTTPsTask_InstanceTaskHandler (HTTPs_INSTANCE  *p_start_cfg);


/*
*********************************************************************************************************
*                                       HTTPsTask_LockCreate()
*
* Description : Create OS lock object.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           HTTPs_ERR_NONE              Lock object successfully created.
*                           HTTPs_ERR_OS_OBJ_CREATE     Lock object allocation failed.
*
* Return(s)   : Pointer to the OS lock object, if no error(s)
*
*               Null pointer,                  otherwise.
*
* Caller(s)   : HTTPs_InstanceInit().
*
* Note(s)     : none.
*********************************************************************************************************
*/

KAL_LOCK_HANDLE  HTTPsTask_LockCreate (HTTPs_ERR  *p_err)
{
    KAL_LOCK_HANDLE  lock_handle;
    RTOS_ERR         err_rtos;

                                                                /* ---------- ACQUIRE OS_LOCK_OBJ MEM SPACE ----------- */
    lock_handle = KAL_LockCreate((const CPU_CHAR *)HTTPs_TASK_STR_NAME_LOCK,
                                                   DEF_NULL,
                                                  &err_rtos);

    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = HTTPs_ERR_TASK_LOCK_CREATE;
        goto exit;
    }

   *p_err = HTTPs_ERR_NONE;

exit:
    return (lock_handle);
}


/*
*********************************************************************************************************
*                                        HTTPsTask_LockAcquire()
*
* Description : Acquire exclusive resource access.
*
* Argument(s) : p_lock_obj  Pointer to OS lock object.
*               ----------  Argument validated in HTTPs_InstanceInit().
**
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                           HTTPs_ERR_NONE              Exclusive lock access successfully acquired
*                           HTTPs_ERR_OS_LOCK_ACQUIRE   Exclusive lock access failed.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_InstanceStart(),
*               HTTPs_InstanceStop(),
*               HTTPs_InstanceTaskHandler().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPsTask_LockAcquire (KAL_LOCK_HANDLE  os_lock_obj,
                             HTTPs_ERR       *p_err)
{
    RTOS_ERR  err_rtos;


    KAL_LockAcquire(os_lock_obj,
                    KAL_OPT_PEND_BLOCKING,
                    0u,
                   &err_rtos);
    switch (err_rtos) {
        case RTOS_ERR_NONE:
            *p_err = HTTPs_ERR_NONE;
             break;

        default:
            *p_err = HTTPs_ERR_TASK_LOCK_ACQUIRE;
             break;
    }
}


/*
*********************************************************************************************************
*                                      HTTPsTask_LockRelease()
*
* Description : Release exclusive resource access.
*
* Argument(s) : p_lock_obj  Pointer to OS lock object.
*               ----------  Argument validated in HTTPs_InstanceInit().
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_ConnTimeout(),
*               HTTPs_InstanceStart(),
*               HTTPs_InstanceStop(),
*               HTTPs_InstanceTaskHandler().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPsTask_LockRelease (KAL_LOCK_HANDLE  os_lock_obj)
{
    RTOS_ERR  err_rtos;


    KAL_LockRelease(os_lock_obj,
                   &err_rtos);

   (void)err_rtos;                                              /* Prevent 'variable unused' compiler warning.          */
}


/*
*********************************************************************************************************
*                                      HTTPsTask_InstanceObjInit()
*
* Description : (1) Initialize OS objects:
*
*                   (a) Validate remaining memory available
*                   (b) Acquire memory space for task control block
*                   (c) Acquire memory space for task stack
*                   (d) Create semaphore for instance stop request
*                   (e) Create semaphore for instance stop request completed
*
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*               -----       Argument validated in HTTPs_InstanceStart().
*
*                               HTTPs_ERR_NONE                      Instance object successfully initialized.
*                               HTTPs_ERR_OS_INIT_POOL_REM_MEM      Not enough remaining memory on the heap.
*                               HTTPs_ERR_OS_OBJ_CREATE             Failed to create instance pool.
*                               HTTPs_ERR_OS_SEM_CREATE             Failed to create semaphore object.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_InstanceInit().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPsTask_InstanceObjInit (HTTPs_INSTANCE  *p_instance,
                                 HTTPs_ERR       *p_err)
{
           HTTPs_OS_TASK_OBJ  *p_os_task_obj;
    const  NET_TASK_CFG       *p_task_cfg;
           RTOS_ERR            err_rtos;


    p_task_cfg = p_instance->TaskCfgPtr;

    p_os_task_obj = HTTPsMem_InstanceTaskInit(p_err);
    if (p_os_task_obj == DEF_NULL) {
        goto exit;
    }

    p_instance->OS_TaskObjPtr = p_os_task_obj;

                                                                /* ------- ACQUIRE TASK AND TASK STACK MEM SPACE ------ */
    p_os_task_obj->TaskHandle = KAL_TaskAlloc(HTTPs_TASK_STR_NAME_TASK,
                                              p_task_cfg->StkPtr,
                                              p_task_cfg->StkSizeBytes * sizeof(CPU_STK),
                                              DEF_NULL,
                                             &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = HTTPs_ERR_TASK_OBJ_CREATE;
        goto exit;
    }


                                                                /* --------------- CREATE STOP REQ SEM ---------------- */
    p_os_task_obj->SemStopReq = KAL_SemCreate(HTTPs_TASK_STR_NAME_SEM_STOP_REQ,
                                              DEF_NULL,
                                             &err_rtos);

    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = HTTPs_ERR_TASK_SEM_CREATE;
        goto exit;
    }


                                                                /* ------------- CREATE STOP COMPLETE SEM ------------- */
    p_os_task_obj->SemStopCompleted = KAL_SemCreate(HTTPs_TASK_STR_NAME_SEM_STOP_COMPLETED,
                                                    DEF_NULL,
                                                   &err_rtos);

    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = HTTPs_ERR_TASK_SEM_CREATE;
        goto exit;
    }


   *p_err = HTTPs_ERR_NONE;

exit:
    return;
}


/*
*********************************************************************************************************
*                                     HTTPsTask_InstanceTaskCreate()
*
* Description : Create and start instance HTTP server task.
*
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*               -----       Argument validated in HTTPs_InstanceStart().
*
*                               HTTPs_ERR_NONE                      Task successfully created and started.
*                               HTTPs_ERR_OS_TASK_CREATE            Task create failed.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_InstanceStart().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPsTask_InstanceTaskCreate (HTTPs_INSTANCE  *p_instance,
                                    HTTPs_ERR       *p_err)
{
           HTTPs_OS_TASK_OBJ  *p_os_task_obj;
    const  NET_TASK_CFG       *p_task_cfg;
           RTOS_ERR            err_rtos;


    p_os_task_obj = (HTTPs_OS_TASK_OBJ *)p_instance->OS_TaskObjPtr;
    p_task_cfg    =  p_instance->TaskCfgPtr;

                                                                /* Create HTTP server task.                             */
    KAL_TaskCreate(              p_os_task_obj->TaskHandle,
                                 HTTPsTask_InstanceTask,
                   (void       *)p_instance,
                   (CPU_INT08U  )p_task_cfg->Prio,
                                 DEF_NULL,
                                &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
        p_instance->Started = DEF_NO;
       *p_err               = HTTPs_ERR_TASK_CREATE;
        return;
    }

    p_instance->Started = DEF_YES;

   *p_err = HTTPs_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      HTTPsTask_InstanceTaskDel()
*
* Description : Stop and delete instance task.
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
* Return(s)   : none.
*
* Created by  : HTTPs_InstanceTaskHandler().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPsTask_InstanceTaskDel (HTTPs_INSTANCE  *p_instance)
{
           RTOS_ERR            err_rtos;
#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED) && \
    (defined(HTTPs_CfgFS_Dyn))
    const  HTTPs_CFG          *p_cfg;
#endif
           HTTPs_OS_TASK_OBJ  *p_os_task_obj;


    p_instance->Started =  DEF_NO;                              /* Stop Instance.                                       */
    p_os_task_obj       = (HTTPs_OS_TASK_OBJ *)p_instance->OS_TaskObjPtr;

    KAL_TaskDel( p_os_task_obj->TaskHandle,
                &err_rtos);

#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED) && \
    (defined(HTTPs_CfgFS_Dyn))
    p_cfg = p_instance->CfgPtr;
    FS_WorkingDirObjFree(((HTTPs_CFG_FS_DYN *)p_cfg->FS_CfgPtr)->WorkingFolderNamePtr);
#endif

   (void)err_rtos;                                              /* Ignore err(s).                                       */
}


/*
*********************************************************************************************************
*                                   HTTPsTask_InstanceStopReqSignal()
*
* Description : Signal that the instance must be stopped.
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStop().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*               -----       Argument validated in HTTPs_InstanceStop().
*
*                               HTTPs_ERR_NONE          Stop request successfully signaled.
*                               HTTPs_ERR_OS_STOP_REQ   Stop request failed.
*
* Return(s)   : none
*
* Caller(s)   : HTTPs_InstanceStop().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPsTask_InstanceStopReqSignal (HTTPs_INSTANCE  *p_instance,
                                       HTTPs_ERR       *p_err)
{
    HTTPs_OS_TASK_OBJ  *p_os_task_obj;
    RTOS_ERR            err_rtos;


    p_os_task_obj = (HTTPs_OS_TASK_OBJ *)p_instance->OS_TaskObjPtr;



    KAL_SemPost(p_os_task_obj->SemStopReq,                      /* Signal instance that stop is requested.              */
                KAL_OPT_POST_NONE,
               &err_rtos);

    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = HTTPs_ERR_TASK_STOP_REQ_SIGNAL;
        return;
    }

   *p_err = HTTPs_ERR_NONE;
}


/*
*********************************************************************************************************
*                                   HTTPsTask_InstanceStopReqPending()
*
* Description : Get stop request.
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
* Return(s)   : DEF_YES, if stop is requested or a fatal error occurred.
*               DEF_NO,  otherwise.
*
* Caller(s)   : HTTPs_InstanceTaskHandler().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsTask_InstanceStopReqPending (HTTPs_INSTANCE  *p_instance)
{
    HTTPs_OS_TASK_OBJ  *p_os_task_obj;
    RTOS_ERR            err_rtos;


    p_os_task_obj = (HTTPs_OS_TASK_OBJ *)p_instance->OS_TaskObjPtr;

    KAL_SemPend(p_os_task_obj->SemStopReq,
                KAL_OPT_PEND_NON_BLOCKING,
                1u,
               &err_rtos);

    switch (err_rtos) {
        case RTOS_ERR_WOULD_BLOCK:
        case RTOS_ERR_OS:
        case RTOS_ERR_TIMEOUT:
             return (DEF_NO);


        case RTOS_ERR_NONE:                                     /* Event occurred.                                      */
        case RTOS_ERR_NULL_PTR:                                 /* Should not be null.                                  */
        case RTOS_ERR_NOT_AVAIL:
        case RTOS_ERR_INVALID_ARG:
        case RTOS_ERR_ABORT:
        case RTOS_ERR_ISR:
        default:
             return DEF_YES;
    }
}


/*
*********************************************************************************************************
*                                HTTPsTask_InstanceStopCompletedSignal()
*
* Description : Signal that the stop request has been completed.
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_InstanceTaskHandler().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPsTask_InstanceStopCompletedSignal (HTTPs_INSTANCE  *p_instance)
{
    HTTPs_OS_TASK_OBJ  *p_os_task_obj;
    RTOS_ERR            err_rtos;


    p_os_task_obj = (HTTPs_OS_TASK_OBJ *)p_instance->OS_TaskObjPtr;

    KAL_SemPost(p_os_task_obj->SemStopCompleted,
                KAL_OPT_POST_NONE,
               &err_rtos);
}


/*
*********************************************************************************************************
*                                HTTPsTask_InstanceStopCompletedPending()
*
* Description : Wait until the stop request has been completed.
*
* Argument(s) : p_instance  Pointer to the instance structure variable.
*               ----------  Argument validated in HTTPs_InstanceStop().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*               -----       Argument validated in HTTPs_InstanceStop().
*
*                               HTTPs_ERR_NONE               Stop request successfully completed.
*                               HTTPs_ERR_OS_STOP_COMPLETED  Complete pending failed.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_InstanceStop().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPsTask_InstanceStopCompletedPending (HTTPs_INSTANCE  *p_instance,
                                              HTTPs_ERR       *p_err)
{
    HTTPs_OS_TASK_OBJ  *p_os_task_obj;
    RTOS_ERR            err_rtos;


    p_os_task_obj = (HTTPs_OS_TASK_OBJ *)p_instance->OS_TaskObjPtr;

    KAL_SemPend(p_os_task_obj->SemStopCompleted,
                KAL_OPT_PEND_BLOCKING,
                0u,                                             /* Infinite timeout.                                    */
               &err_rtos);

    switch (err_rtos) {
        case RTOS_ERR_WOULD_BLOCK:
        case RTOS_ERR_OS:
        case RTOS_ERR_TIMEOUT:
            *p_err = HTTPs_ERR_TASK_STOP_COMPLETED_PEND;
             return;

        case RTOS_ERR_NONE:                                     /* Event occurred.                                      */
        case RTOS_ERR_NULL_PTR:                                 /* Should not be null.                                  */
        case RTOS_ERR_INVALID_ARG:
        case RTOS_ERR_ABORT:
        case RTOS_ERR_ISR:
        default:
             break;
    }


   *p_err = HTTPs_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         HTTPsTask_TimeDly_ms()
*
* Description : Delay for specified time, in milliseconds.
*
* Argument(s) : time_dly_ms     Time delay value, in millisecond.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_InstanceTaskHandler().
*
*               This function is an INTERNAL HTTP server function & SHOULD NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPsTask_TimeDly_ms (CPU_INT32U  time_dly_ms)
{
    KAL_Dly(time_dly_ms);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         HTTPs_InstanceTask()
*
* Description : Shell task to schedule & run HTTP instance task handler.
*
*               (1) Shell task's primary purpose is to schedule & run HTTPs_InstanceTaskHandler() forever.
*
* Argument(s) : p_data  Pointer to task initialization (required by uC/OS-III).
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_InstanceTaskCreate().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  HTTPsTask_InstanceTask (void  *p_data)
{
    HTTPs_INSTANCE  *p_instance;


    p_instance = (HTTPs_INSTANCE *)p_data;

    while (DEF_ON) {
        HTTPsTask_InstanceTaskHandler(p_instance);              /* Call HTTP server task handler.                       */
    }
}


/*
*********************************************************************************************************
*                                    HTTPsTask_InstanceTaskHandler()
*
* Description : HTTP server main loop.
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_InstanceTask().
*
*               This function is an INTERNAL HTTPs server function & MUST NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  HTTPsTask_InstanceTaskHandler (HTTPs_INSTANCE  *p_instance)
{
    const  HTTPs_CFG             *p_cfg         = p_instance->CfgPtr;
           NET_SOCK_QTY           sock_nbr_rdy  = 0;
           CPU_BOOLEAN            close_pending = DEF_NO;
           CPU_BOOLEAN            closed        = DEF_NO;
           CPU_BOOLEAN            sel_abort     = DEF_NO;
           CPU_BOOLEAN            accept;
           HTTPs_ERR              err;
#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED)
    const  NET_FS_API            *p_fs_api;
           CPU_CHAR              *p_working_folder;
           CPU_BOOLEAN            result;
#endif
           HTTPs_INSTANCE_ERRS   *p_ctr_err;


    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);
                                                                /* ---------------- SET WORKING FOLDER ---------------- */
    switch (p_cfg->FS_Type) {
        case HTTPs_FS_TYPE_NONE:
             break;

        case HTTPs_FS_TYPE_STATIC:
             break;

        case HTTPs_FS_TYPE_DYN:
#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED)
             p_fs_api = ((HTTPs_CFG_FS_DYN *)p_cfg->FS_CfgPtr)->FS_API_Ptr;
             p_working_folder = ((HTTPs_CFG_FS_DYN *)p_cfg->FS_CfgPtr)->WorkingFolderNamePtr;
             if (p_fs_api != DEF_NULL) {
                 if ((p_fs_api->WorkingFolderSet != DEF_NULL) &&
                     (p_working_folder    != DEF_NULL)) {
                     result = p_fs_api->WorkingFolderSet(p_working_folder);
                     if (result != DEF_OK) {
                         HTTPs_ERR_INC(p_ctr_err->FS_ErrWorkingFolderInvalidCtr);
                         CPU_SW_EXCEPTION(;);
                     }
                 }
             }
#else
             HTTPs_ERR_INC(p_ctr_err->FS_ErrNoEnCtr);
             CPU_SW_EXCEPTION(HTTPs_ERR_CFG_INVALID_FS_EN);
#endif
             break;

        default:
             HTTPs_ERR_INC(p_ctr_err->FS_ErrTypeInvalidCtr);
             CPU_SW_EXCEPTION(;);
             break;
    }

   (void)p_ctr_err;

    while (DEF_ON) {
                                                                /* -------------------- DELAY TASK -------------------- */
        if (p_cfg->OS_TaskDly_ms > 0) {
            HTTPsTask_TimeDly_ms(p_cfg->OS_TaskDly_ms);
        }

                                                                /* ------------------- SEL CONN RDY ------------------- */
        accept = !closed;

        do {                                                    /* Try to obtain HTTP instance's select abort           */
            HTTPsTask_LockAcquire(p_instance->ConnSelAbortLockObj, &err);
            if (err != HTTPs_ERR_NONE) {                        /* If failed to acquire instance lock ...               */
                                                                /* ... Delay the task ...                               */
                HTTPsTask_TimeDly_ms(HTTPs_OS_LOCK_ACQUIRE_FAIL_DLY_MS);
            }
        } while (err != HTTPs_ERR_NONE);

        sel_abort = p_instance->SelAbortReq;

        HTTPsTask_LockRelease(p_instance->ConnSelAbortLockObj);
        if (sel_abort == DEF_NO) {
            sock_nbr_rdy = HTTPsSock_ConnSel(p_instance, accept);
        }

                                                                /* -------------- ACQUIRE INSTANCE LOCK --------------- */
        do {
            HTTPsTask_LockAcquire(p_instance->OS_LockObj, &err);
            if (err != HTTPs_ERR_NONE) {                        /* If failed to acquire instance lock ...               */
                                                                /* ... Delay the task ...                               */
                HTTPsTask_TimeDly_ms(HTTPs_OS_LOCK_ACQUIRE_FAIL_DLY_MS);
            }
        } while (err != HTTPs_ERR_NONE);

                                                                /* -------------- CHECK FOR ASYNC CLOSE --------------- */
        if (close_pending == DEF_NO) {                          /* If close NOT already signaled.                       */
            close_pending  = HTTPsTask_InstanceStopReqPending(p_instance);
        }

        if (close_pending == DEF_YES) {                         /* If stop is requested.                                */

            if (closed == DEF_NO) {                             /* If not closed.                                       */

                                                                /* ---------------- CLOSE LISTEN SOCK ----------------- */
                switch (p_cfg->SockSel) {
                    case HTTPs_SOCK_SEL_IPv4:
#ifdef   NET_IPv4_MODULE_EN
                         HTTPsSock_ListenClose(p_instance, p_instance->SockListenID_IPv4);
#endif
                         break;

                    case HTTPs_SOCK_SEL_IPv6:
#ifdef   NET_IPv6_MODULE_EN
                         HTTPsSock_ListenClose(p_instance, p_instance->SockListenID_IPv6);
#endif
                         break;

                    case HTTPs_SOCK_SEL_IPv4_IPv6:
#ifdef   NET_IPv4_MODULE_EN
                         HTTPsSock_ListenClose(p_instance, p_instance->SockListenID_IPv4);
#endif
#ifdef   NET_IPv6_MODULE_EN
                         HTTPsSock_ListenClose(p_instance, p_instance->SockListenID_IPv6);
#endif
                         break;

                    default:
                        break;
                }
                                                                    /* Close listen sock: discard incoming conn.        */
                closed = DEF_YES;

            } else if (p_instance->ConnActiveCtr == 0) {            /* If no more active connection:                    */

                HTTPsTask_InstanceStopCompletedSignal(p_instance);  /* Signal that the stop is completed.               */
                HTTPsTask_LockRelease(p_instance->OS_LockObj);      /* Release instance lock.                           */
                HTTPsTask_TimeDly_ms(500);                          /* Add delay to permit context switch.              */
                return;
            }
        }

                                                                /* ----------------- PROCESS CONN RDY ----------------- */
        if (sock_nbr_rdy > 0) {
            HTTPsConn_Process(p_instance);
        } else {
            if (p_instance->ConnActiveCtr > 0 && closed) {      /* HTTP connection may have been closed forcefully  ... */
                HTTPsMem_ConnRelease(p_instance,                /* ... due to non-graceful instance stop. Free conn ... */
                                     p_instance->ConnFirstPtr); /* ... resources.                                       */
            }
        }

                                                                /* -------------- RELEASE INSTANCE LOCK --------------- */
        HTTPsTask_LockRelease(p_instance->OS_LockObj);
    }
}
