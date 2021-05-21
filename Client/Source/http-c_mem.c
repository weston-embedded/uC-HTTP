/*
*********************************************************************************************************
*                                               uC/HTTP
*                                     Hypertext Transfer Protocol
*
*                    Copyright 2004-2021 Silicon Laboratories Inc. www.silabs.com
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
*                                     HTTP CLIENT MEMORY LIBRARY
*
* Filename : http-c_mem.c
* Version  : V3.01.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    HTTPc_MEM_MODULE
#include  "http-c_mem.h"

/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static MEM_DYN_POOL HTTPc_MsgPool;
#ifdef  HTTPc_WEBSOCK_MODULE_EN
static MEM_DYN_POOL HTTPc_WebSockReqPool;
#endif


/*
*********************************************************************************************************
*                                        HTTPc_Mem_MsgPoolInit()
*
* Description : Initialize the Message object pool for the HTTP client task.
*
* Argument(s) : p_cfg  Pointer to the HTTP client configuration.
*
*               p_seg  Pointer to segment from which to allocate memory. Will be allocated from
*                      general-purpose heap if null.
*
*               p_err  Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPc_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPc_Mem_TaskMsgPoolInit (const  HTTPc_CFG  *p_cfg,
                                        MEM_SEG    *p_seg,
                                        HTTPc_ERR  *p_err)
{
    LIB_ERR      err_lib;


    Mem_DynPoolCreate("HTTPc Msg Pool",
                      &HTTPc_MsgPool,
                       p_seg,
                       sizeof(HTTPc_TASK_MSG),
                       sizeof(CPU_SIZE_T),
                       p_cfg->MsgQ_Size,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        default:
        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPc_ERR_MSG_INIT_POOL_FAULT;
             return;
    }

   *p_err = HTTPc_ERR_NONE;
}


/*
*********************************************************************************************************
*                                          HTTPc_Mem_MsgGet()
*
* Description : Get a HTTPc Message object.
*
* Argument(s) : none.
*
* Return(s)   : If operation is successful, Pointer to the allocated Message object.
*               If operation has failed,    DEF_NULL.
*
* Caller(s)   : HTTPcTask_MsgQueue().
*
* Note(s)     : none.
*********************************************************************************************************
*/

HTTPc_TASK_MSG  *HTTPc_Mem_TaskMsgGet (void)
{

    LIB_ERR          err_lib;
    HTTPc_TASK_MSG  *p_msg;


    p_msg = (HTTPc_TASK_MSG *) Mem_DynPoolBlkGet(&HTTPc_MsgPool,
                                            &err_lib);

    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             Mem_Clr(p_msg, sizeof(HTTPc_TASK_MSG));
             break;

        case LIB_MEM_ERR_POOL_EMPTY:
        default:
             return (DEF_NULL);
    }

    return (p_msg);
}


/*
*********************************************************************************************************
*                                        HTTPc_Mem_MsgRelease()
*
* Description : Release an allocated HTTPc Message object.
*
* Argument(s) : p_msg   Pointer to the Message object to free.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPcTask().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPc_Mem_TaskMsgRelease (HTTPc_TASK_MSG  *p_msg)
{
    LIB_ERR  err_lib;


    Mem_DynPoolBlkFree(&HTTPc_MsgPool,
                        p_msg,
                       &err_lib);
    (void)err_lib;
}


/*
*********************************************************************************************************
*                                    HTTPc_Mem_WebSockReqPoolInit()
*
* Description : Initialize the WebSocket Request object pool.
*
* Argument(s) : p_cfg  Pointer to the HTTP client configuration.
*
*               p_seg  Pointer to segment from which to allocate memory. Will be allocated from
*                      general-purpose heap if null.
*
*               p_err  Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPc_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef  HTTPc_WEBSOCK_MODULE_EN
void  HTTPc_Mem_WebSockReqPoolInit (const  HTTPc_CFG  *p_cfg,
                                           MEM_SEG    *p_seg,
                                           HTTPc_ERR  *p_err)
{
    LIB_ERR      err_lib;


    Mem_DynPoolCreate("HTTPc WebSocket Request Pool",
                      &HTTPc_WebSockReqPool,
                       p_seg,
                       sizeof(HTTPc_WEBSOCK_REQ),
                       sizeof(CPU_SIZE_T),
                       0,
                       p_cfg->MsgQ_Size,
                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        default:
        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPc_ERR_WEBSOCK_REQ_INIT_POOL_FAULT;
             return;
    }

   *p_err = HTTPc_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                       HTTPc_Mem_WebSockReqGet()
*
* Description : Get a WebSocket Request object.
*
* Argument(s) : none.
*
* Return(s)   : If operation is successful, Pointer to the allocated WebSocket Request object.
*               If operation has failed,    DEF_NULL.
*
* Caller(s)   : HTTPcWebSock_InitReqObj().
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef  HTTPc_WEBSOCK_MODULE_EN
HTTPc_WEBSOCK_REQ  *HTTPc_Mem_WebSockReqGet (void)
{
    LIB_ERR             err_lib;
    HTTPc_WEBSOCK_REQ  *p_ws_req;


    p_ws_req = (HTTPc_WEBSOCK_REQ *) Mem_DynPoolBlkGet(&HTTPc_WebSockReqPool,
                                                       &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             Mem_Clr(p_ws_req, sizeof(HTTPc_WEBSOCK_REQ ));
             break;

        case LIB_MEM_ERR_POOL_EMPTY:
        default:
             return (DEF_NULL);
    }

    return (p_ws_req);
}
#endif


/*
*********************************************************************************************************
*                                     HTTPc_Mem_WebSockReqRelease()
*
* Description : Release an allocated WebSocket Request object.
*
* Argument(s) : p_ws_req    Pointer to the WebSocket Request object to free.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPcConn_TransProcess(),
*               HTTPcWebSock_InitReqObj().
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef  HTTPc_WEBSOCK_MODULE_EN
void  HTTPc_Mem_WebSockReqRelease (HTTPc_WEBSOCK_REQ  *p_ws_req)
{
    LIB_ERR  err_lib;


    Mem_DynPoolBlkFree(&HTTPc_WebSockReqPool,
                        p_ws_req,
                       &err_lib);
   (void)err_lib;
}
#endif

