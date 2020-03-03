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
*                                   HTTP CLIENT CONNECTIION MODULE
*
* Filename : http-c_conn.c
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
#define    HTTPc_CONN_MODULE
#include  <Source/net_ascii.h>
#include  <Source/net_util.h>

#include  <Source/net.h>
#include  <Source/net_cfg_net.h>
#include  <Source/net_app.h>
#include  <Source/net_sock.h>

#include  "http-c_conn.h"
#include  "http-c_sock.h"
#include  "http-c_req.h"
#include  "http-c_resp.h"
#ifdef HTTPc_TASK_MODULE_EN
#include  "http-c_task.h"
#endif
#ifdef HTTPc_WEBSOCK_MODULE_EN
#include  "http-c_websock.h"
#endif


/*
*********************************************************************************************************
*                                          HTTPcConn_Process()
*
* Description : Process a connection in its CONNECT state.
*
* Argument(s) : p_conn  Pointer to current HTTPc Connection object.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcTask_Handler().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPcConn_Process (HTTPc_CONN  *p_conn)
{
    HTTPc_CONN_OBJ    *p_conn_const;
    CPU_BOOLEAN        is_connect;
    CPU_BOOLEAN        result;
#ifdef HTTPc_TASK_MODULE_EN
    CPU_BOOLEAN        no_block;
#endif
    HTTPc_ERR          err;


    p_conn_const = (HTTPc_CONN_OBJ *)p_conn;

    switch (p_conn->State) {
        case HTTPc_CONN_STATE_NONE:
             break;

        case HTTPc_CONN_STATE_CONNECT:                          /* CONNECT STATE                                        */
             is_connect = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_CONNECT);
             if (is_connect == DEF_NO) {

                 HTTPcConn_Connect(p_conn, &err);               /* Connection Connect.                                  */
                 if (err != HTTPc_ERR_NONE) {
                     p_conn->ErrCode     = err;
                     p_conn->CloseStatus = HTTPc_CONN_CLOSE_STATUS_NONE;
                     p_conn->State       = HTTPc_CONN_STATE_NONE;
                     result              = DEF_FAIL;
                     HTTPcConn_Remove(p_conn);
                 } else {
                     p_conn->State   = HTTPc_CONN_STATE_PARAM_VALIDATE;
                     result          = DEF_OK;
                 }

#ifdef HTTPc_TASK_MODULE_EN
                                                                /* No-Blocking mode: notify app with callback that ...  */
                                                                /* ... connect is done.                                 */
                 no_block = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_NO_BLOCK);
                 if ((no_block          == DEF_YES ) &&
                     (p_conn->OnConnect != DEF_NULL)) {
                      p_conn->OnConnect(p_conn_const, result);
                 }

#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
                                                                /* Blocking mode: Signal app that connect is done.      */
                 if (p_conn->ConnectSignal.SemObjPtr != DEF_NULL) {
                     HTTPcTask_ConnConnectSignal(p_conn, &err);
                     if (err != HTTPc_ERR_NONE) {
                         CPU_SW_EXCEPTION(;);
                     }
                 }
#endif
#endif
             } else {
                 p_conn->State = HTTPc_CONN_STATE_PARAM_VALIDATE;
             }
             break;


        default:
             CPU_SW_EXCEPTION(;);
             break;
    }
 
    (void)p_conn_const;
    (void)result;
}


/*
*********************************************************************************************************
*                                          HTTPcConn_Connect()
*
* Description : (1) Connect to an HTTP server.
*                   (a) Validate Server Socket Address.
*                   (b) Connect to Server.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                             Connection to server successful.
*                               HTTPc_ERR_CONN_PARAM_PORT_INVALID          Invalid HTTP Server port number.
*                               HTTPc_ERR_CONN_PARAM_ADDR_INVALID          Invalid HTTP Server address.
*                               HTTPc_ERR_CONN_PARAM_ADDR_FAMILY_INVALID   Invalid HTTP Server address family.
*
*                               ------------ RETURNED BY HTTPcSock_Connect() ------------
*                               See HTTPcSock_Connect() for additional return error codes.
*
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcTask_Handler(),
*               HTTPc_ConnConnect().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPcConn_Connect (HTTPc_CONN      *p_conn,
                         HTTPc_ERR       *p_err)
{
    CPU_BOOLEAN  is_connect;
#ifdef HTTPc_TASK_MODULE_EN
    CPU_BOOLEAN  no_block;
#endif

    is_connect = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_CONNECT);
    if (is_connect == DEF_YES) {
       *p_err = HTTPc_ERR_CONN_IS_CONNECT;
        goto exit;
    }

                                                                /* ---------------- VALIDATE CALLBACKS ---------------- */
#ifdef HTTPc_TASK_MODULE_EN
    no_block = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_NO_BLOCK);
    if (no_block == DEF_YES) {
        if (p_conn->OnConnect == DEF_NULL) {
           *p_err = HTTPc_ERR_CONN_PARAM_CONNECT_CALLBACK_INVALID;
            goto exit;
        }
    }

    if (p_conn->OnClose == DEF_NULL) {
       *p_err = HTTPc_ERR_CONN_PARAM_CLOSE_CALLBACK_INVALID;
        goto exit;
    }
#endif

                                                                /* -------------- CONNECT TO HTTP SERVER -------------- */
    HTTPcSock_Connect(p_conn, p_err);
    if (*p_err != HTTPc_ERR_NONE) {
         goto exit;
    }

    DEF_BIT_SET(p_conn->Flags, HTTPc_FLAG_CONN_CONNECT);

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                          HTTPcConn_Close()
*
* Description : (1) HTTPc Connection Close Handler:
*                   (a) Call hook function to advertise connection closing.
*                   (b) Close Socket
*                   (c) Release HTTPc Connection related objects.
*                   (d) Remove connection from list.
*
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               ------------ RETURNED BY HTTPcSock_Close() -------------
*                               See HTTPcSock_Close() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcConn_TransProcess(),
*               HTTPc_ConnCloseHandler().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPcConn_Close (HTTPc_CONN  *p_conn,
                       HTTPc_ERR   *p_err)
{
    HTTPc_CONN_OBJ    *p_conn_const;
    HTTPc_REQ         *p_req;
    HTTPc_REQ_OBJ     *p_req_const;


    p_conn_const = (HTTPc_CONN_OBJ *)p_conn;

                                                                /* ---------- CLEAR CONNECTION REQUESTS LIST ---------- */
    p_req        = p_conn->ReqListHeadPtr;
    p_req_const  = (HTTPc_REQ_OBJ *)p_req;
    while (p_req != DEF_NULL) {
#ifdef HTTPc_TASK_MODULE_EN
        if (p_req->OnTransComplete != DEF_NULL) {
            p_req->OnTransComplete(p_conn_const, p_req_const, p_req->RespPtr, DEF_FAIL);
        }
        if (p_req->OnErr != DEF_NULL) {
            p_req->OnErr(p_conn_const, p_req_const, HTTPc_ERR_CONN_SOCK_CLOSED);
        }
#else
        (void)p_conn_const;
        (void)p_req_const;
#endif
        HTTPcConn_ReqRemove(p_conn);
        p_req = p_conn->ReqListHeadPtr;
    }

    DEF_BIT_CLR(p_conn->Flags, HTTPc_FLAG_CONN_CONNECT);
    DEF_BIT_CLR(p_conn->Flags, HTTPc_FLAG_CONN_IN_USE);
                                                                /* ------------- CLOSE CONNECTION SOCKET -------------- */
    HTTPcSock_Close(p_conn, p_err);
    if (*p_err != HTTPc_ERR_NONE) {
        *p_err = HTTPc_ERR_CONN_SOCK_CLOSE_FATAL;
         goto exit;
    }

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                      HTTPcConn_TransProcess()
*
* Description : Process the connection's transaction state (i.e. request or response state).
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPcTask_Handler().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPcConn_TransProcess (HTTPc_CONN  *p_conn)
{
    HTTPc_CONN_OBJ    *p_conn_const;
    HTTPc_REQ         *p_req;
    HTTPc_REQ_OBJ     *p_req_const;
    CPU_BOOLEAN        result;
#ifdef HTTPc_TASK_MODULE_EN
    CPU_BOOLEAN        no_block;
#endif
    CPU_BOOLEAN        success;
    CPU_BOOLEAN        to_close;
    HTTPc_ERR          err;
#ifdef HTTPc_WEBSOCK_MODULE_EN
    CPU_BOOLEAN        is_websocket;
#endif
    p_conn_const = (HTTPc_CONN_OBJ *)p_conn;

    p_req        = p_conn->ReqListHeadPtr;
    p_req_const  = (HTTPc_REQ_OBJ *)p_req;

                                                                /* ------------- CONNECTION STATE MACHINE ------------- */
    switch (p_conn->State) {
        case HTTPc_CONN_STATE_PARAM_VALIDATE:                   /* REQUEST PREPARATION & VALIDATTION STATE              */
             result = HTTPcSock_IsRxClosed(p_conn, &err);       /* Check if connection half-closed by server.           */
             if (result == DEF_YES) {
                 p_conn->ErrCode     = HTTPc_ERR_NONE;
                 p_conn->CloseStatus = HTTPc_CONN_CLOSE_STATUS_SERVER;
                 p_conn->State       = HTTPc_CONN_STATE_COMPLETED;
             } else {
                 if (p_req != DEF_NULL) {
                     HTTPcConn_TransParamReset(p_conn);         /* Clear Connection internal parameters.                */
                     HTTPcReq_Prepare(p_conn, &err);
                     if (err != HTTPc_ERR_NONE) {
                         p_conn->ErrCode     = err;
                         p_conn->CloseStatus = HTTPc_CONN_CLOSE_STATUS_NONE;
                         p_conn->State       = HTTPc_CONN_STATE_ERR;
                     } else {
                         p_conn->State = HTTPc_CONN_STATE_REQ_LINE_METHOD;
                     }
                 }
             }
             break;


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
        case HTTPc_CONN_STATE_REQ_END:                          /* REQUEST PROCESSING STATES                            */
            (void)HTTPcReq(p_conn, &err);
             switch (err) {
                 case HTTPc_ERR_NONE:
                 case HTTPc_ERR_CONN_SOCK_TX:
                 case HTTPc_ERR_TRANS_TX_BUF_FULL:
                      break;

                 case HTTPc_ERR_CONN_SOCK_CLOSED:
                      p_conn->ErrCode     = err;
                      p_conn->CloseStatus = HTTPc_CONN_CLOSE_STATUS_SERVER;
                      p_conn->State       = HTTPc_CONN_STATE_ERR;
                      break;

                 default:
                      p_conn->ErrCode     = err;
                      p_conn->CloseStatus = HTTPc_CONN_CLOSE_STATUS_ERR_INTERNAL;
                      p_conn->State       = HTTPc_CONN_STATE_ERR;
                      break;
             }
             break;


        case HTTPc_CONN_STATE_RESP_INIT:
        case HTTPc_CONN_STATE_RESP_STATUS_LINE:
        case HTTPc_CONN_STATE_RESP_HDR:
        case HTTPc_CONN_STATE_RESP_BODY:
        case HTTPc_CONN_STATE_RESP_BODY_CHUNK_SIZE:
        case HTTPc_CONN_STATE_RESP_BODY_CHUNK_DATA:
        case HTTPc_CONN_STATE_RESP_BODY_CHUNK_CRLF:
        case HTTPc_CONN_STATE_RESP_BODY_CHUNK_LAST:
        case HTTPc_CONN_STATE_RESP_COMPLETED:                   /* RESPONSE PROCESSING STATES                           */
             success = HTTPcResp(p_conn, &err);
             switch (err) {
                 case HTTPc_ERR_NONE:
                 case HTTPc_ERR_CONN_SOCK_RX:
                 case HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED:
                      break;

                 case HTTPc_ERR_CONN_SOCK_CLOSED:
                      p_conn->ErrCode     = err;
                      p_conn->CloseStatus = HTTPc_CONN_CLOSE_STATUS_SERVER;
                      p_conn->State       = HTTPc_CONN_STATE_ERR;
                      break;

                 default:
                      p_conn->ErrCode     = err;
                      p_conn->CloseStatus = HTTPc_CONN_CLOSE_STATUS_ERR_INTERNAL;
                      p_conn->State       = HTTPc_CONN_STATE_ERR;
                      break;
             }

             if (success == DEF_YES) {
                 DEF_BIT_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_COMPLETE_OK);
             } else {
                 DEF_BIT_CLR(p_conn->RespFlags, HTTPc_FLAG_RESP_COMPLETE_OK);
             }
             break;


        case HTTPc_CONN_STATE_ERR:                              /* ERROR STATE                                          */
#ifdef HTTPc_TASK_MODULE_EN
                                                                /* Call callback function for Transaction Error.        */
             if (p_req != DEF_NULL) {
                 if (p_req->OnErr != DEF_NULL) {
                     p_req->OnErr(p_conn_const, p_req_const, p_conn->ErrCode);
                 }
             } else {
                 CPU_SW_EXCEPTION(;);
             }
#endif
             p_conn->State = HTTPc_CONN_STATE_COMPLETED;
             break;


        case HTTPc_CONN_STATE_COMPLETED:                        /* TRANSACTION COMPLETE STATE                           */
             if (p_req != DEF_NULL) {
                 success = DEF_BIT_IS_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_COMPLETE_OK);
#ifdef  HTTPc_WEBSOCK_MODULE_EN
                                                                /* Check if it is a upgrade websocket request.          */
                 result = DEF_BIT_IS_SET(p_req->Flags, HTTPc_FLAG_REQ_UPGRADE_WEBSOCKET);
                 if ((result == DEF_YES) && (success == DEF_YES)) {
                     if ((p_req->WebSockPtr->Flags & HTTPc_FLAG_WEBSOCK_ALL) == HTTPc_FLAG_WEBSOCK_ALL) {
                         if (p_req->RespPtr->StatusCode == HTTP_STATUS_SWITCHING_PROTOCOLS) {
                                                                /* The connection is upgrading to websocket.            */
                             DEF_BIT_SET(p_conn->Flags, HTTPc_FLAG_CONN_WEBSOCKET);
                             p_conn->WebSockPtr = p_req->WebSockPtr->WebSockObjPtr;
                         }
                     }
                 }
                 if (p_req->WebSockPtr != DEF_NULL) {
                     HTTPc_Mem_WebSockReqRelease (p_req->WebSockPtr);
                 }
#endif

                 HTTPcConn_ReqRemove(p_conn);                   /* Remove request from list.                            */

#ifdef HTTPc_TASK_MODULE_EN
                                                                /* No-Blocking mode: notify app with callback that ...  */
                                                                /* ... transaction is done.                             */
                 no_block = DEF_BIT_IS_SET(p_req->Flags, HTTPc_FLAG_REQ_NO_BLOCK);
                 if ((no_block               == DEF_YES ) &&
                     (p_req->OnTransComplete != DEF_NULL)) {
                      p_req->OnTransComplete(p_conn_const, p_req_const, p_req->RespPtr, success);
                 }
#endif
             }

             to_close = HTTPcConn_TransComplete(p_conn);
             if (to_close == DEF_YES) {
                 p_conn->State = HTTPc_CONN_STATE_CLOSE;
             } else {
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
                                                                /* Blocking mode: Signal app that transaction is done.  */
                 if (p_conn->TransDoneSignal.SemObjPtr != DEF_NULL) {
                     HTTPcTask_TransDoneSignal(p_conn, &err);
                     if (err != HTTPc_ERR_NONE) {
                         CPU_SW_EXCEPTION(;);
                     }
                 }
#endif
#ifdef  HTTPc_WEBSOCK_MODULE_EN

                 is_websocket = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_WEBSOCKET);
                 if (is_websocket == DEF_YES) {
                     p_conn->State = HTTPc_CONN_STATE_WEBSOCK_INIT;
                 } else {
                     p_conn->State = HTTPc_CONN_STATE_PARAM_VALIDATE;
                 }
#else
                 p_conn->State = HTTPc_CONN_STATE_PARAM_VALIDATE;
#endif
             }
             break;

        case HTTPc_CONN_STATE_CLOSE:                            /* CONNECTION CLOSE STATE                               */
             HTTPcConn_Close(p_conn, &err);                     /* Close Connection.                                    */
             HTTPcConn_Remove(p_conn);                          /* Release Connection.                                  */

#ifdef HTTPc_TASK_MODULE_EN
                                                                /* Call callback function for Conn Close.               */
             if (p_conn->OnClose != DEF_NULL) {
                 p_conn->OnClose(p_conn_const, p_conn->CloseStatus, p_conn->ErrCode);
             }

#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
                                                                 /* Blocking mode: Signal app that transaction is done.  */
             if (p_conn->TransDoneSignal.SemObjPtr != DEF_NULL) {
                 HTTPcTask_TransDoneSignal(p_conn, &err);
                 if (err != HTTPc_ERR_NONE) {
                     CPU_SW_EXCEPTION(;);
                 }
             }
                                                                /* Blocking mode: Signal App that Conn was closed.      */
             if (p_conn->CloseSignal.SemObjPtr != DEF_NULL) {
                 HTTPcTask_ConnCloseSignal(p_conn, &err);
                 if (err != HTTPc_ERR_NONE) {
                     CPU_SW_EXCEPTION(;);
                 }
             }
#endif
#endif
             p_conn->State = HTTPc_CONN_STATE_NONE;
             break;


        default:
             CPU_SW_EXCEPTION(;);
             break;
    }

    (void)p_conn_const;
    (void)p_req_const;
}


/*
*********************************************************************************************************
*                                      HTTPcConn_TransParamReset()
*
* Description : Clear HTTP Connection object members related to internal usage during an HTTP transaction.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPc_ReqSend(),
*               HTTPcTask_Handle().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPcConn_TransParamReset (HTTPc_CONN  *p_conn)
{
                                                                /* ------------ INIT CONNECTION PARAMETERS ------------ */
    p_conn->SockFlags                = DEF_BIT_NONE;
    p_conn->ErrCode                  = HTTPc_ERR_NONE;
    p_conn->CloseStatus              = HTTPc_CONN_CLOSE_STATUS_NONE;
    DEF_BIT_CLR(p_conn->Flags, HTTPc_FLAG_CONN_TO_CLOSE);

                                                                /* ------------- INIT REQUEST PARAMETERS -------------- */
    p_conn->ReqFlags                 = 0u;
#if (HTTPc_CFG_QUERY_STR_EN == DEF_ENABLED)
    p_conn->ReqQueryStrTxIx          = 0u;
    p_conn->ReqQueryStrTempPtr       = DEF_NULL;
#endif
#if (HTTPc_CFG_HDR_TX_EN == DEF_ENABLED)
    p_conn->ReqHdrTxIx               = 0u;
    p_conn->ReqHdrTempPtr            = DEF_NULL;
#endif
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
    p_conn->ReqFormDataTxIx          = 0u;
#endif
    p_conn->ReqDataOffset            = 0u;

                                                                /* ------------- INIT RESPONSE PARAMETERS ------------- */
    DEF_BIT_CLR(p_conn->RespFlags, HTTPc_FLAG_RESP_RX_MORE_DATA);
    DEF_BIT_CLR(p_conn->RespFlags, HTTPc_FLAG_RESP_BODY_CHUNK_TRANSFER);


                                                                /* --------- INIT CONNECTION BUFFER PARAMETERS -------- */
    p_conn->RxBufPtr                 = p_conn->BufPtr;
    p_conn->RxDataLenRem             = 0u;
    p_conn->RxDataLen                = 0u;
    p_conn->TxBufPtr                 = p_conn->BufPtr;
    p_conn->TxDataLen                = 0u;
    p_conn->TxDataPtr                = DEF_NULL;
}


/*
*********************************************************************************************************
*                                         HTTPcConn_TransComplete()
*
* Description : HTTP Transaction complete checks if a connection must be closed.
*
* Argument(s) : p_conn    Pointer to current HTTPc Connection.
*
* Return(s)   : DEF_YES,  if connection must be closed.
*               DEF_NO,   otherwise.
*
* Caller(s)   : HTTPcTask_Handle(),
*               HTTPc_ReqSend().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcConn_TransComplete (HTTPc_CONN  *p_conn)
{
    CPU_BOOLEAN   close_hdr;
    CPU_BOOLEAN   is_connect;
    CPU_BOOLEAN   to_close;
#ifdef  HTTPc_WEBSOCK_MODULE_EN
    CPU_BOOLEAN   is_websocket;
#endif

    to_close = DEF_NO;

    close_hdr  = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_TO_CLOSE);
    is_connect = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_CONNECT);
    if ((close_hdr  == DEF_YES) &&
        (is_connect == DEF_YES)) {
         to_close = DEF_YES;
    }
#ifdef  HTTPc_WEBSOCK_MODULE_EN
    is_websocket = DEF_BIT_IS_SET(p_conn->Flags,  HTTPc_FLAG_CONN_WEBSOCKET);
    if (is_websocket == DEF_YES) {
        to_close = DEF_NO;
    }
#endif
    if (p_conn->CloseStatus != HTTPc_CONN_CLOSE_STATUS_NONE) {
        to_close = DEF_YES;
    }

    return (to_close);
}


/*
*********************************************************************************************************
*                                            HTTPcConn_Add()
*
* Description : (1) Add current HTTPc Connection to connection list if the task is enabled.
*               (2) Tag Connection as being used.
*
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcTask(),
*               HTTPc_ConnOpen().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPcConn_Add (HTTPc_CONN  *p_conn)
{
    DEF_BIT_SET(p_conn->Flags, HTTPc_FLAG_CONN_IN_USE);
#ifdef HTTPc_TASK_MODULE_EN
    HTTPcTask_ConnAdd(p_conn);
#endif

    p_conn->State = HTTPc_CONN_STATE_CONNECT;
}


/*
*********************************************************************************************************
*                                          HTTPcConn_Remove()
*
* Description : (1) Remove current HTTPc Connection from connection list if the task is enabled.
*               (2) Release Connection.
*
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcConn_Process(),
*               HTTPcConn_TransProcess().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPcConn_Remove (HTTPc_CONN  *p_conn)
{

#ifdef HTTPc_TASK_MODULE_EN
    HTTPcTask_ConnRemove(p_conn);
#endif

    DEF_BIT_CLR(p_conn->Flags, HTTPc_FLAG_CONN_IN_USE);
}


/*
*********************************************************************************************************
*                                          HTTPcConn_ReqAdd()
*
* Description : Add new Request to the Connection Request list.
*
* Argument(s) : p_req       Pointer to the HTTPc Request.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Request added successfully.
*                               HTTPc_ERR_CONN_IS_RELEASED      Connection is not used.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcTask(),
*               HTTPc_ReqSend().
*
* Note(s)     : (1) New Request is added at the end of the list.
*********************************************************************************************************
*/

void  HTTPcConn_ReqAdd (HTTPc_REQ  *p_req,
                        HTTPc_ERR  *p_err)
{
    HTTPc_CONN   *p_conn;
    HTTPc_REQ    *p_req_item;
    CPU_BOOLEAN   in_use;


    p_conn = p_req->ConnPtr;

    in_use = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_IN_USE);

    if (in_use == DEF_YES) {
        DEF_BIT_SET(p_req->Flags, HTTPc_FLAG_REQ_IN_USE);

        if (p_conn->ReqListHeadPtr == DEF_NULL) {
            p_conn->ReqListHeadPtr = p_req;
            p_conn->ReqListEndPtr  = p_req;
            p_req->NextPtr         = 0;
        } else {
            p_req_item             = p_conn->ReqListEndPtr;
            p_req_item->NextPtr    = p_req;
            p_conn->ReqListEndPtr  = p_req;
            p_req->NextPtr         = 0;
        }

       *p_err = HTTPc_ERR_NONE;

    } else {
       *p_err = HTTPc_ERR_CONN_IS_RELEASED;
    }
}


/*
*********************************************************************************************************
*                                         HTTPcConn_ReqRemove()
*
* Description : Remove the first Request from the Connection Request list.
*
* Argument(s) : p_conn      Pointer to the HTTPc Connection.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcConn_Close(),
*               HTTPcConn_TransComplete().
*
* Note(s)     : (1) The Head of the Request list is removed.
*********************************************************************************************************
*/

void  HTTPcConn_ReqRemove (HTTPc_CONN  *p_conn)
{
    HTTPc_REQ  *p_req_item;
    CPU_SR_ALLOC();


    p_req_item = p_conn->ReqListHeadPtr;

    if (p_req_item == DEF_NULL) {
        p_conn->ReqListEndPtr  = DEF_NULL;
    } else if (p_req_item->NextPtr == DEF_NULL) {
        p_conn->ReqListHeadPtr = DEF_NULL;
        p_conn->ReqListEndPtr  = DEF_NULL;
        p_req_item->NextPtr    = 0;
    } else {
        p_conn->ReqListHeadPtr = p_req_item->NextPtr;
        p_req_item->NextPtr    = 0;
    }

    if (p_req_item != DEF_NULL) {
        CPU_CRITICAL_ENTER();
        DEF_BIT_CLR(p_req_item->Flags, HTTPc_FLAG_REQ_IN_USE);
        CPU_CRITICAL_EXIT();
    }
}

