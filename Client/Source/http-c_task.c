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
*                                       HTTP CLIENT TASK MODULE
*
* Filename : http-c_task.c
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
#define    HTTPc_TASK_MODULE
#include  <KAL/kal.h>
#include  <Source/net_ascii.h>
#include  <Source/net_util.h>

#include  "http-c.h"
#include  "http-c_mem.h"
#include  "http-c_task.h"
#include  "http-c_conn.h"
#include  "http-c_req.h"
#include  "http-c_resp.h"
#include  "http-c_sock.h"
#ifdef  HTTPc_WEBSOCK_MODULE_EN
#include  "http-c_websock.h"
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef  HTTPc_TASK_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPc_TASK_PRIO_MIN                     1u                    /* Minimum task priority.   */
#define  HTTPc_TASK_STACK_SIZE_MIN               512                   /* Minimum task stack size. */
#define  HTTPc_TASK_NAME                        "HTTPc Task"           /* Task Name.               */

#define  HTTPc_TASK_DLY_MS_DFLT                  5u                    /* Task delay.              */
#define  HTTPc_TASK_DLY_MS_MAX                  20u
#define  HTTPc_TASK_DLY_MS_MIN                   0u

#define  HTTPc_TASK_MSG_Q_NAME                  "HTTPc Msg Q"

#define  HTTPc_TASK_SIGNAL_CONN_CONNECT_DONE     "HTTPc Conn Connect Done"
#define  HTTPc_TASK_SIGNAL_CONN_CLOSE_DONE       "HTTPc Conn Close Done"
#define  HTTPc_TASK_SIGNAL_TRANS_DONE            "HTTPc Transaction Done"

#define  HTTPc_TASK_TIMEOUT_MS_CONN_CONNECT_DFLT  10000u
#define  HTTPc_TASK_TIMEOUT_MS_CONN_CLOSE_DFLT     100u
#define  HTTPc_TASK_TIMEOUT_MS_TRANS_DFLT         30000u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/
static  KAL_Q_HANDLE  HTTPcTask_MsgQ_Handle;

static  CPU_INT16U    HTTPcTask_SignalTimeoutConnConnect_ms;

static  CPU_INT08U    HTTPcTask_Dly_ms;

static  HTTPc_CONN   *HTTPcTask_ConnFirstPtr;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void             HTTPcTask_Handler    (void);

static  HTTPc_TASK_MSG  *HTTPcTask_MsgDequeue (HTTPc_ERR  *p_err);


/*
*********************************************************************************************************
*                                           HTTPcTask_Init()
*
* Description : (1) Initialize signals' timeout.
*               (2) Create HTTP Client Queues.
*               (3) Create HTTP Client Task.
*
*
* Argument(s) : p_cfg       Pointer to HTTPc Instance configuration object.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                              Task successfully created.
*                               HTTPc_ERR_INSTANCE_INIT_TASK_INVALID_ARG    Invalid Task configuration.
*                               HTTPc_ERR_INSTANCE_INIT_TASK_MEM_ALLOC      Insufficient memory space.
*                               HTTPc_ERR_INSTANCE_INIT_TASK_CREATE         Error in task creation.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPc_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPcTask_Init (const  HTTPc_CFG       *p_cfg,
                      const  HTTP_TASK_CFG   *p_task_cfg,
                             HTTPc_ERR       *p_err)
{
    KAL_TASK_HANDLE   task_handle;
    RTOS_ERR          err_rtos;


                                                                /* ----------- INITIALIZE SIGNALS' TIMEOUT ------------ */
    HTTPcTask_SignalTimeoutConnConnect_ms = HTTPc_TASK_TIMEOUT_MS_CONN_CONNECT_DFLT;

    HTTPcTask_MsgQ_Handle = KAL_QCreate(HTTPc_TASK_MSG_Q_NAME,
                                        p_cfg->MsgQ_Size,
                                        DEF_NULL,
                                       &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = HTTPc_ERR_INIT_MSG_Q;
        goto exit;
    }
                                                                /* ------- ALLOCATE MEMORY SPACE FOR HTTPc TASK  ------ */
    task_handle = KAL_TaskAlloc((const  CPU_CHAR *)HTTPc_TASK_NAME,
                                                   p_task_cfg->StkPtr,
                                                   p_task_cfg->StkSizeBytes * sizeof(CPU_STK),
                                                   DEF_NULL,
                                                  &err_rtos);
    switch (err_rtos) {
        case RTOS_ERR_NONE:
             break;

        case RTOS_ERR_INVALID_ARG:
            *p_err = HTTPc_ERR_INIT_TASK_INVALID_ARG;
             goto exit;

        case RTOS_ERR_ALLOC:
        default:
            *p_err = HTTPc_ERR_INIT_TASK_MEM_ALLOC;
             goto exit;
    }

                                                                /* ---------------- CREATE HTTPc TASK ----------------- */
    KAL_TaskCreate(task_handle,
                   HTTPcTask,
                   DEF_NULL,
                   p_task_cfg->Prio,
                   DEF_NULL,
                  &err_rtos);
    switch (err_rtos) {
        case RTOS_ERR_NONE:
             break;

        case RTOS_ERR_INVALID_ARG:
        case RTOS_ERR_ISR:
        case RTOS_ERR_OS:
        default:
            *p_err = HTTPc_ERR_INIT_TASK_CREATE;
             goto exit;
    }

   *p_err = HTTPc_ERR_NONE;


exit:
   return;
}


/*
*********************************************************************************************************
*                                          HTTPcTask()
*
* Description : HTTP Client main loop.
*
* Argument(s) : p_data      Pointer to task initialization (required by uC/OS-III).
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcTask_Init().
*
* Note(s)     : None.
*********************************************************************************************************
*/
void  HTTPcTask (void  *p_data)
{
    HTTPc_CONN         *p_conn;
    HTTPc_REQ          *p_req;
    HTTPc_CONN_OBJ     *p_conn_const;
    HTTPc_REQ_OBJ      *p_req_const;
    HTTPc_ERR           err;
    HTTPc_TASK_MSG     *p_msg;
#ifdef  HTTPc_WEBSOCK_MODULE_EN
    HTTPc_WEBSOCK_MSG  *p_ws_msg;
    HTTPc_WEBSOCK      *p_ws;
#endif

   (void)p_data;

    while (DEF_ON) {
                                                                /* --------------- CHECK FOR CONN READY --------------- */
        p_msg = HTTPcTask_MsgDequeue(&err);
        switch (err) {
            case HTTPc_ERR_NONE:
                 switch (p_msg->Type){

                      case HTTPc_MSG_TYPE_CONN_OPEN:
                           p_conn = (HTTPc_CONN *)p_msg->DataPtr;
                           HTTPcConn_Add(p_conn);
                           break;

                      case HTTPc_MSG_TYPE_CONN_CLOSE:
                           p_conn              = (HTTPc_CONN *)p_msg->DataPtr;
                           p_conn->CloseStatus = HTTPc_CONN_CLOSE_STATUS_APP;
                           p_conn->State       = HTTPc_CONN_STATE_CLOSE;
                           break;

                      case HTTPc_MSG_TYPE_REQ:
                           p_req = (HTTPc_REQ *)p_msg->DataPtr;
                           HTTPcConn_ReqAdd(p_req, &err);
                           if (err != HTTPc_ERR_NONE) {
                               p_conn_const = (HTTPc_CONN_OBJ *)p_req->ConnPtr;
                               p_req_const  = (HTTPc_REQ_OBJ *)p_req;
                               p_req->OnErr(p_conn_const, p_req_const, err);
                           }
                           break;
#ifdef  HTTPc_WEBSOCK_MODULE_EN
                      case HTTPc_MSG_TYPE_WEBSOCK_MSG:
                          p_ws_msg = (HTTPc_WEBSOCK_MSG *)p_msg->DataPtr;
                          HTTPcWebSock_TxMsgAdd(p_ws_msg, &err);
                          if (err != HTTPc_ERR_NONE) {
                              p_conn = p_ws_msg->ConnPtr;
                              p_ws   = p_conn->WebSockPtr;
                              if (p_ws != DEF_NULL) {
                                  if (p_ws->OnErr != DEF_NULL) {
                                      p_conn_const = (HTTPc_CONN_OBJ *)p_ws_msg->ConnPtr;
                                      p_ws->OnErr(p_conn_const, err);
                                  }
                              } else {
                                  CPU_SW_EXCEPTION(;);
                              }
                          }
                          break;
#endif
                      default:
                          CPU_SW_EXCEPTION(;);
                          break;
                 }
                 HTTPc_Mem_TaskMsgRelease(p_msg);
                 break;

            case HTTPc_ERR_MSG_Q_EMPTY:
                 break;

            default:
                 CPU_SW_EXCEPTION(;);
                 break;
        }

                                                                /* -------- SOCKET SELECT ON OPEN CONNECTIONS --------- */
        p_conn = HTTPcTask_ConnFirstPtr;
        HTTPcSock_Sel(p_conn, &err);
        if (err != HTTPc_ERR_NONE) {
            CPU_SW_EXCEPTION(;);
        }
                                                                /* --------------- HTTP CLIENT HANDLER ---------------- */
        HTTPcTask_Handler();

                                                                /* -------------- DELAY HTTP CLIENT TASK -------------- */
        if (HTTPcTask_Dly_ms > 0u) {
            KAL_Dly(HTTPcTask_Dly_ms);
        }
    }
}


/*
*********************************************************************************************************
*                                          HTTPcTask_SetDly()
*
* Description : Set the HTTP Client Task Delay.
*
* Argument(s) : dly_ms      Task delay value.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Task delay successfully set-up.
*                               HTTPc_ERR_CFG_TASK_DLY_INVALID  Invalid delay value.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPc_Init().
*
* Note(s)     : (1) The Task Delay allows other tasks to runs when HTTP client is set in a none blocking
*                   mode.
*********************************************************************************************************
*/

void  HTTPcTask_SetDly (CPU_INT08U   dly_ms,
                        HTTPc_ERR   *p_err)
{
    CPU_SR_ALLOC();


    if (dly_ms > HTTPc_TASK_DLY_MS_MAX) {
       *p_err = HTTPc_ERR_CFG_TASK_DLY_INVALID;
        goto exit;
    }

    CPU_CRITICAL_ENTER();
    HTTPcTask_Dly_ms = dly_ms;
    CPU_CRITICAL_EXIT();

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                         HTTPcTask_MsgQueue()
*
* Description : Signal that a HTTPc message is ready to be process by adding message object to
*               the message queue.
*
* Argument(s) : type    Type of the message obj to queue.
*
*               p_data  Pointer to the Message obj to queue.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Message successfully added to queue.
*                               HTTPc_ERR_MSG_Q_FULL            Error Queue full.
*                               HTTPc_ERR_MSG_Q_SIGNAL_FAULT    Signal Post faulted.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPc_ConnCloseHandler(),
*               HTTPc_ConnOpen(),
*               HTTPc_ReqSend(),
*               HTTPc_WebSockSend().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPcTask_MsgQueue (HTTPc_MSG_TYPE   type,
                          void            *p_data,
                          HTTPc_ERR       *p_err)
{
    HTTPc_TASK_MSG  *p_msg;
    RTOS_ERR         err_rtos;


    p_msg = HTTPc_Mem_TaskMsgGet();
    if (p_msg == DEF_NULL) {
        *p_err = HTTPc_ERR_MSG_Q_SIGNAL_FAULT;
        goto exit;
    }

    p_msg->Type    = type;
    p_msg->DataPtr = p_data;

    KAL_QPost(HTTPcTask_MsgQ_Handle,
              p_msg,
              KAL_OPT_NONE,
             &err_rtos);
    switch (err_rtos) {
        case RTOS_ERR_NONE:
             break;

        case RTOS_ERR_WOULD_OVF:
            *p_err = HTTPc_ERR_MSG_Q_FULL;
             goto exit;

        case RTOS_ERR_NO_MORE_RSRC:
        case RTOS_ERR_OS:
        default:
            *p_err = HTTPc_ERR_MSG_Q_SIGNAL_FAULT;
             goto exit;
    }

   *p_err = HTTPc_ERR_NONE;

exit:
    return;
}


/*
*********************************************************************************************************
*                                          HTTPcTask_ConnAdd()
*
* Description : Add current HTTPc Connection to connection list.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcConn_Add().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPcTask_ConnAdd (HTTPc_CONN  *p_conn)
{
    HTTPc_CONN      *p_conn_item;


                                                                /* ------------ UPDATE INSTANCE CONN LIST ------------- */
    if (HTTPcTask_ConnFirstPtr == DEF_NULL) {
        p_conn->NextPtr        = DEF_NULL;
        HTTPcTask_ConnFirstPtr = p_conn;
    } else {
        p_conn_item            = HTTPcTask_ConnFirstPtr;
        p_conn->NextPtr        = p_conn_item;
        HTTPcTask_ConnFirstPtr = p_conn;
    }
}


/*
*********************************************************************************************************
*                                        HTTPcTask_ConnRemove()
*
* Description : Remove current HTTPc Connection from connection list.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcConn_Remove().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPcTask_ConnRemove (HTTPc_CONN  *p_conn)
{
    HTTPc_CONN      *p_conn_item;
    HTTPc_CONN      *p_conn_item_prev;


    p_conn_item      = HTTPcTask_ConnFirstPtr;
    p_conn_item_prev = DEF_NULL;

    while (p_conn_item != DEF_NULL) {

        if (p_conn == p_conn_item) {

            if (p_conn == HTTPcTask_ConnFirstPtr) {
                HTTPcTask_ConnFirstPtr = p_conn->NextPtr;
            } else {
                p_conn_item_prev->NextPtr = p_conn->NextPtr;
            }

        }

        p_conn_item_prev = p_conn_item;
        p_conn_item      = p_conn_item->NextPtr;
    }

    p_conn->NextPtr = DEF_NULL;
}


/*
*********************************************************************************************************
*                                     HTTPcTask_ConnConnectSignalCreate()
*
* Description : Create the Connection Connect Signal. The signal is used to inform that the connect
*               process is complete.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Signal successfully created.
*                               HTTPc_ERR_CONN_SIGNAL_CREATE    Signal creation faulted.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPc_ConnOpen().
*
* Note(s)     : (1) This signal is used when the HTTP Client task is enabled and use in blocking mode
*                    to advertise that the connect process is done.
*********************************************************************************************************
*/
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
void  HTTPcTask_ConnConnectSignalCreate (HTTPc_CONN  *p_conn,
                                         HTTPc_ERR   *p_err)
{
    RTOS_ERR   err_rtos;


                                                                /* -------- CREATE SIGNAL FOR CONNECT COMPLETE -------- */
    p_conn->ConnectSignal = KAL_SemCreate(HTTPc_TASK_SIGNAL_CONN_CONNECT_DONE,
                                          DEF_NULL,
                                         &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = HTTPc_ERR_CONN_SIGNAL_CREATE;
        goto exit;
    }

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}
#endif


/*
*********************************************************************************************************
*                                    HTTPcTask_ConnConnectSignalDel()
*
* Description : Delete Connection Connect Signal.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Signal successfully deleted.
*                               HTTPc_ERR_CONN_SIGNAL_DEL       Signal deletion faulted.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPc_ConnOpen().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
void  HTTPcTask_ConnConnectSignalDel (HTTPc_CONN  *p_conn,
                                      HTTPc_ERR   *p_err)
{
    RTOS_ERR   err_rtos;


    KAL_SemDel(p_conn->ConnectSignal, &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = HTTPc_ERR_CONN_SIGNAL_DEL;
        goto exit;
    }

    p_conn->ConnectSignal = KAL_SemHandleNull;

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}
#endif


/*
*********************************************************************************************************
*                                   HTTPcTask_ConnCloseSignalCreate()
*
* Description : Create the Connection Close Signal. The signal is used to inform that the connection close
*               process is complete.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Signal successfully created.
*                               HTTPc_ERR_CONN_SIGNAL_CREATE    Signal creation faulted.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPc_ConnCloseHandler().
*
* Note(s)     : (1) This signal is used when the HTTP Client task is enabled and use in blocking mode
*                    to advertise that the close process is done.
*********************************************************************************************************
*/
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
void  HTTPcTask_ConnCloseSignalCreate (HTTPc_CONN  *p_conn,
                                       HTTPc_ERR   *p_err)
{
    RTOS_ERR   err_rtos;


                                                                /* ------- CREATE SIGNAL FOR CONN HTTP CLOSING -------- */
    p_conn->CloseSignal = KAL_SemCreate(HTTPc_TASK_SIGNAL_CONN_CLOSE_DONE,
                                        DEF_NULL,
                                       &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = HTTPc_ERR_CONN_SIGNAL_CREATE;
        goto exit;
    }

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}
#endif


/*
*********************************************************************************************************
*                                    HTTPcTask_ConnCloseSignalDel()
*
* Description : Delete Connection Close Signal.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Signal successfully deleted.
*                               HTTPc_ERR_CONN_SIGNAL_DEL       Signal deletion faulted.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPc_ConnCloseHandler().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
void  HTTPcTask_ConnCloseSignalDel (HTTPc_CONN  *p_conn,
                                    HTTPc_ERR   *p_err)
{
    RTOS_ERR   err_rtos;


    KAL_SemDel(p_conn->CloseSignal, &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = HTTPc_ERR_CONN_SIGNAL_DEL;
        goto exit;
    }

    p_conn->CloseSignal = KAL_SemHandleNull;

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}
#endif

/*
*********************************************************************************************************
*                                     HTTPcTask_TransDoneSignalCreate()
*
* Description : Create the Transaction Done Signal. The signal is used to inform that the HTTP transaction
*               is completed.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Signal successfully created.
*                               HTTPc_ERR_CONN_SIGNAL_CREATE    Signal creation faulted.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPc_ReqSend().
*
* Note(s)     : (1) This signal is used when the HTTP Client task is enabled and use in blocking mode
*                    to advertise that the HTTP transaction is completed.
*********************************************************************************************************
*/
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
void  HTTPcTask_TransDoneSignalCreate (HTTPc_CONN  *p_conn,
                                       HTTPc_ERR   *p_err)
{
    RTOS_ERR  err_rtos;


    p_conn->TransDoneSignal = KAL_SemCreate(HTTPc_TASK_SIGNAL_TRANS_DONE,
                                            DEF_NULL,
                                           &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = HTTPc_ERR_CONN_SIGNAL_CREATE;
        goto exit;
    }

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}
#endif


/*
*********************************************************************************************************
*                                     HTTPcTask_TransDoneSignalDel()
*
* Description : Delete Transaction Done Signal.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Signal successfully deleted.
*                               HTTPc_ERR_CONN_SIGNAL_DEL       Signal deletion faulted.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPc_ReqSend().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
void  HTTPcTask_TransDoneSignalDel (HTTPc_CONN  *p_conn,
                                    HTTPc_ERR   *p_err)
{
    RTOS_ERR  err_rtos;


    KAL_SemDel(p_conn->TransDoneSignal, &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = HTTPc_ERR_CONN_SIGNAL_DEL;
        goto exit;
    }

    p_conn->TransDoneSignal = KAL_SemHandleNull;


   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}
#endif


/*
*********************************************************************************************************
*                                  HTTPcTask_ConnConnectSignal()
*
* Description : Post Connection Connect Signal.
*
* Argument(s) : p_conn      Pointer to current HTTPc connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Signal successfully posted.
*                               HTTPc_ERR_CONN_SIGNAL_FAULT     Signal posting faulted.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcTask_Handler().
*
* Note(s)     : (1) Signal to inform that the connection connect process has been completed.
*********************************************************************************************************
*/
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
void  HTTPcTask_ConnConnectSignal (HTTPc_CONN  *p_conn,
                                   HTTPc_ERR   *p_err)
{
    RTOS_ERR  err_rtos;


    KAL_SemPost(p_conn->ConnectSignal,
                KAL_OPT_PEND_NONE,
               &err_rtos);

    switch (err_rtos) {
        case RTOS_ERR_NONE:
            *p_err = HTTPc_ERR_NONE;
             break;

        default:
            *p_err = HTTPc_ERR_CONN_SIGNAL_FAULT;
             break;
    }
}
#endif


/*
*********************************************************************************************************
*                                      HTTPcTask_ConnCloseSignal()
*
* Description : Post HTTP Connection Close Signal.
*
* Argument(s) : p_conn  Pointer to current HTTPc Connection.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           HTTPc_ERR_NONE                  Signal posted successfully.
*                           HTTPc_ERR_CONN_SIGNAL_FAULT     Error occurred during signal posting.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcTask_Handler().
*
* Note(s)     : (1) Signal to inform that the connection close process has been completed.
*********************************************************************************************************
*/
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
void  HTTPcTask_ConnCloseSignal (HTTPc_CONN  *p_conn,
                                 HTTPc_ERR   *p_err)
{
    RTOS_ERR  err_rtos;


    KAL_SemPost(p_conn->CloseSignal,
                KAL_OPT_PEND_NONE,
               &err_rtos);

    switch (err_rtos) {
        case RTOS_ERR_NONE:
            *p_err = HTTPc_ERR_NONE;
             break;

        default:
            *p_err = HTTPc_ERR_CONN_SIGNAL_FAULT;
             break;
    }
}
#endif


/*
*********************************************************************************************************
*                                   HTTPcTask_TransDoneSignal()
*
* Description : Post HTTP Transaction Done Signal.
*
* Argument(s) : p_conn  Pointer to current HTTPc Connection.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           HTTPc_ERR_NONE                  Signal posted successfully.
*                           HTTPc_ERR_CONN_SIGNAL_FAULT     Error occurred during signal posting.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPs_TaskHandler().
*
* Note(s)     : (1) Signal to inform that the HTTP Transaction has been completed.
*********************************************************************************************************
*/
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
void  HTTPcTask_TransDoneSignal (HTTPc_CONN  *p_conn,
                                 HTTPc_ERR   *p_err)
{
    RTOS_ERR  err_rtos;


    KAL_SemPost(p_conn->TransDoneSignal,
                KAL_OPT_PEND_NONE,
               &err_rtos);

    switch (err_rtos) {
        case RTOS_ERR_NONE:
            *p_err = HTTPc_ERR_NONE;
             break;

        default:
            *p_err = HTTPc_ERR_CONN_SIGNAL_FAULT;
             break;
    }
}
#endif


/*
*********************************************************************************************************
*                                   HTTPcTask_ConnConnectSignalWait()
*
* Description : Wait for Connection Connect signal.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Signal successfully received.
*                               HTTPc_ERR_CONN_SIGNAL_TIMEOUT   Signal pending timed out.
*                               HTTPc_ERR_CONN_SIGNAL_FAULT     Signal pending faulted.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPc_ConnOpen().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
void  HTTPcTask_ConnConnectSignalWait (HTTPc_CONN  *p_conn,
                                       HTTPc_ERR   *p_err)
{
    RTOS_ERR  err_rtos;


    KAL_SemPend(p_conn->ConnectSignal, KAL_OPT_PEND_BLOCKING, HTTPcTask_SignalTimeoutConnConnect_ms,  &err_rtos);
    switch (err_rtos) {
        case RTOS_ERR_NONE:
            *p_err = HTTPc_ERR_NONE;
             break;

        case RTOS_ERR_TIMEOUT:
            *p_err = HTTPc_ERR_CONN_SIGNAL_TIMEOUT;
             break;

        default:
            *p_err = HTTPc_ERR_CONN_SIGNAL_FAULT;
             break;
    }
}
#endif


/*
*********************************************************************************************************
*                                     HTTPcTask_ConnCloseSignalWait()
*
* Description : Wait for HTTP Connection Close signal posting.
*
* Argument(s) : p_conn  Pointer to current HTTPc Connection.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           HTTPc_ERR_NONE                  Signal received.
*                           HTTPc_ERR_CONN_SIGNAL_TIMEOUT   Signal pending timed out.
*                           HTTPc_ERR_CONN_SIGNAL_FAULT     Error occurred during signal pending.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPc_ConnClose().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
void  HTTPcTask_ConnCloseSignalWait (HTTPc_CONN  *p_conn,
                                     HTTPc_ERR   *p_err)
{
    RTOS_ERR  err_rtos;


    KAL_SemPend(p_conn->CloseSignal, KAL_OPT_PEND_BLOCKING, HTTPc_TASK_TIMEOUT_MS_CONN_CLOSE_DFLT,  &err_rtos);
    switch (err_rtos) {
        case RTOS_ERR_NONE:
            *p_err = HTTPc_ERR_NONE;
             break;

        case RTOS_ERR_TIMEOUT:
            *p_err = HTTPc_ERR_CONN_SIGNAL_TIMEOUT;
             break;

        default:
            *p_err = HTTPc_ERR_CONN_SIGNAL_FAULT;
             break;
    }
}
#endif


/*
*********************************************************************************************************
*                                    HTTPcTask_TransDoneSignalWait()
*
* Description : Wait for HTTP Response Ready Signal.
*
* Argument(s) : p_conn  Pointer to current HTTPc Connection.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           HTTPc_ERR_NONE                  Signal received.
*                           HTTPc_ERR_CONN_SIGNAL_TIMEOUT   Signal pending timed out.
*                           HTTPc_ERR_CONN_SIGNAL_FAULT     Error occurred during signal pending.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPc_ReqSend().
*
* Note(s)     : (1) This is a blocking function.
*********************************************************************************************************
*/
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
void  HTTPcTask_TransDoneSignalWait (HTTPc_CONN  *p_conn,
                                     HTTPc_ERR   *p_err)
{
    RTOS_ERR  err_rtos;


    KAL_SemPend(p_conn->TransDoneSignal, KAL_OPT_PEND_BLOCKING, HTTPc_TASK_TIMEOUT_MS_TRANS_DFLT,  &err_rtos);
    switch (err_rtos) {
        case RTOS_ERR_NONE:
            *p_err = HTTPc_ERR_NONE;
             break;

        case RTOS_ERR_TIMEOUT:
            *p_err = HTTPc_ERR_CONN_SIGNAL_TIMEOUT;
             break;

        default:
            *p_err = HTTPc_ERR_CONN_SIGNAL_FAULT;
             break;
    }
}
#endif


/*
*********************************************************************************************************
*                                         HTTPcTask_Wake()
*
* Description : Wake HTTPc Task pending in HTTPcSock_Sel.
*
* Argument(s) : p_conn  Pointer to current HTTPc Connection.
*
*               p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPc_ConnCloseHandler(),
*               HTTPc_ConnOpen(),
*               HTTPc_ReqSend(),
*               HTTPc_WebSockSend().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPcTask_Wake (HTTPc_CONN  *p_conn,
                      HTTPc_ERR   *p_err)
{
    NET_ERR  err;


   *p_err = HTTPc_ERR_NONE;

    if (p_conn->SockID != NET_SOCK_ID_NONE) {
        NetSock_SelAbort(p_conn->SockID, &err);
        if ((err != NET_SOCK_ERR_NONE      ) &&
            (err != NET_SOCK_ERR_NONE_AVAIL)) {
           *p_err = HTTPc_ERR_CONN_SOCK_ABORT_FAILED;
        }
    } else {
        if (HTTPcTask_ConnFirstPtr != DEF_NULL) {
            NetSock_SelAbort(HTTPcTask_ConnFirstPtr->SockID, &err);
            if ((err != NET_SOCK_ERR_NONE      ) &&
                (err != NET_SOCK_ERR_NONE_AVAIL)) {
               *p_err = HTTPc_ERR_CONN_SOCK_ABORT_FAILED;
            }
        }
    }
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
*                                            HTTPcTask_Handler()
*
* Description : HTTP Client main task when HTTPc_CFG_MODE_ASYNC_TASK_EN is enabled.
*               Proceed through the state machine to process all HTTP transactions on each open connection.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcTask().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  HTTPcTask_Handler (void)
{
    HTTPc_CONN  *p_conn;


    p_conn = HTTPcTask_ConnFirstPtr;

    while (p_conn != DEF_NULL) {

        switch (p_conn->State) {
            case HTTPc_CONN_STATE_NONE:
            case HTTPc_CONN_STATE_CONNECT:
                 HTTPcConn_Process(p_conn);
                 break;


            case HTTPc_CONN_STATE_PARAM_VALIDATE:
            case HTTPc_CONN_STATE_REQ_LINE_METHOD:
            case HTTPc_CONN_STATE_REQ_LINE_URI:
            case HTTPc_CONN_STATE_REQ_LINE_QUERY_STR:
            case HTTPc_CONN_STATE_REQ_LINE_PROTO_VER:
            case HTTPc_CONN_STATE_REQ_HDR_HOST:
            case HTTPc_CONN_STATE_REQ_HDR_CONTENT_TYPE:
            case HTTPc_CONN_STATE_REQ_HDR_CONTENT_LEN:
            case HTTPc_CONN_STATE_REQ_HDR_TRANSFER_ENCODE:
            case HTTPc_CONN_STATE_REQ_HDR_CONN:
            case HTTPc_CONN_STATE_REQ_HDR_EXT:
            case HTTPc_CONN_STATE_REQ_HDR_LAST:
            case HTTPc_CONN_STATE_REQ_BODY:
            case HTTPc_CONN_STATE_REQ_BODY_DATA:
            case HTTPc_CONN_STATE_REQ_BODY_DATA_CHUNK_SIZE:
            case HTTPc_CONN_STATE_REQ_BODY_DATA_CHUNK_DATA:
            case HTTPc_CONN_STATE_REQ_BODY_DATA_CHUNK_END:
            case HTTPc_CONN_STATE_REQ_BODY_FORM_APP:
            case HTTPc_CONN_STATE_REQ_BODY_FORM_MULTIPART_BOUNDARY:
            case HTTPc_CONN_STATE_REQ_BODY_FORM_MULTIPART_HDR_CONTENT_DISPO:
            case HTTPc_CONN_STATE_REQ_BODY_FORM_MULTIPART_HDR_CONTENT_TYPE:
            case HTTPc_CONN_STATE_REQ_BODY_FORM_MULTIPART_DATA:
            case HTTPc_CONN_STATE_REQ_BODY_FORM_MULTIPART_DATA_END:
            case HTTPc_CONN_STATE_REQ_BODY_FORM_MULTIPART_BOUNDARY_END:
            case HTTPc_CONN_STATE_REQ_END:
            case HTTPc_CONN_STATE_RESP_INIT:
            case HTTPc_CONN_STATE_RESP_STATUS_LINE:
            case HTTPc_CONN_STATE_RESP_HDR:
            case HTTPc_CONN_STATE_RESP_BODY:
            case HTTPc_CONN_STATE_RESP_BODY_CHUNK_SIZE:
            case HTTPc_CONN_STATE_RESP_BODY_CHUNK_DATA:
            case HTTPc_CONN_STATE_RESP_BODY_CHUNK_CRLF:
            case HTTPc_CONN_STATE_RESP_BODY_CHUNK_LAST:
            case HTTPc_CONN_STATE_RESP_COMPLETED:
            case HTTPc_CONN_STATE_ERR:
            case HTTPc_CONN_STATE_COMPLETED:
            case HTTPc_CONN_STATE_CLOSE:
                 HTTPcConn_TransProcess(p_conn);
                 break;

#ifdef  HTTPc_WEBSOCK_MODULE_EN
            case HTTPc_CONN_STATE_WEBSOCK_INIT:
            case HTTPc_CONN_STATE_WEBSOCK_RXTX:
            case HTTPc_CONN_STATE_WEBSOCK_CLOSE:
            case HTTPc_CONN_STATE_WEBSOCK_ERR:
                 HTTPcWebSock_Process(p_conn);
                 break;
#endif

            default:
                 CPU_SW_EXCEPTION(;);
                 break;
        }

        p_conn = p_conn->NextPtr;
    }
}


/*
*********************************************************************************************************
*                                      HTTPcTask_ConnOpenDequeue()
*
* Description : Check that a HTTPc connection is ready to be connected by looking in the Connection
*               Connect Queue.
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  New connection object in queue.
*                               HTTPc_ERR_CONN_Q_EMPTY          Error Queue empty.
*                               HTTPc_ERR_CONN_Q_SIGNAL_FAULT   Signal check faulted.
*
* Return(s)   : Pointer to new connection received in queue.
*
*               DEF_NULL if queue is empty.
*
* Caller(s)   : HTTPcTask().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  HTTPc_TASK_MSG  *HTTPcTask_MsgDequeue (HTTPc_ERR   *p_err)
{
    HTTPc_TASK_MSG  *p_msg;
    KAL_OPT          option;
    RTOS_ERR         err_rtos;


    if (HTTPcTask_ConnFirstPtr == DEF_NULL) {
       option = KAL_OPT_PEND_BLOCKING;
    } else {
       option = KAL_OPT_PEND_NON_BLOCKING;
    }

    p_msg = (HTTPc_TASK_MSG *)KAL_QPend(HTTPcTask_MsgQ_Handle,
                                        option,
                                        0,
                                       &err_rtos);
    switch (err_rtos) {
        case RTOS_ERR_NONE:
             break;

        case RTOS_ERR_TIMEOUT:
        case RTOS_ERR_WOULD_BLOCK:
             p_msg = DEF_NULL;
            *p_err = HTTPc_ERR_MSG_Q_EMPTY;
             goto exit;

        case RTOS_ERR_ISR:
        case RTOS_ERR_ABORT:
        case RTOS_ERR_OS:
        default:
             p_msg = DEF_NULL;
            *p_err  = HTTPc_ERR_MSG_Q_SIGNAL_FAULT;
             goto exit;
    }

   *p_err = HTTPc_ERR_NONE;


exit:
    return (p_msg);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* HTTPc_TASK_MODULE_EN       */
