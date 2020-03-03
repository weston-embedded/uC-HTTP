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
*                                             HTTP CLIENT
*
* Filename : http-c.c
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
#define    HTTPc_MODULE

#include  <Source/net_ascii.h>
#include  <Source/net_util.h>

#include  "http-c.h"
#include  "http-c_type.h"
#include  "http-c_sock.h"
#include  "http-c_conn.h"
#include  "http-c_req.h"
#include  "http-c_resp.h"
#include  "http-c_mem.h"
#ifdef HTTPc_TASK_MODULE_EN
#include  "http-c_task.h"
#endif
#ifdef  HTTPc_WEBSOCK_MODULE_EN
#include  "http-c_websock.h"
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPc_BUF_LEN_MIN                  256u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static         CPU_BOOLEAN   HTTPc_InitDone = DEF_NO;

static  const  HTTPc_CFG    *HTTPc_CfgPtr;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#if ((HTTPc_CFG_PERSISTENT_EN == DEF_ENABLED) || \
      defined(HTTPc_SIGNAL_TASK_MODULE_EN)       )
static  void  HTTPc_ConnCloseHandler (HTTPc_CONN   *p_conn,
                                      HTTPc_FLAGS   flags,
                                      HTTPc_ERR    *p_err);
#endif


/*
*********************************************************************************************************
*                                        HTTPc_Init()
*
* Description : (1) Initialize HTTP Client Suite :
*                   (a) Validate Configuration.
*                   (c) Create HTTP Client Task if necessary.
*
*
* Argument(s) : p_cfg       Pointer to HTTP Client Instance Configuration Object.
*
*               p_task_cfg  Pointer to HTTP Client task configuration object.
*
*               p_mem_seg   For future usage: Set to DEF_NULL.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                                  HTTPc Initialization successful.
*                               HTTPc_ERR_NULL_PTR                              function passed a null pointer as argument(s).
*                               HTTPc_ERR_CFG_CONN_INACTIVITY_TIMEOUT_INVALID   Invalid inactivity timeout value.
*                               HTTPc_ERR_CFG_CONN_Q_SIZE_INVALID               Invalid Conn Q size value.
*                               HTTPc_ERR_CFG_REQ_Q_SIZE_INVALID                Invalid Request Q size value.
*                               HTTPc_ERR_CFG_TASK_PTR_NULL                     Null Task configuration's pointer.
*                               HTTPc_ERR_INIT                                  HTTPc Initialization failed.
**
*                               ----------- RETURNED BY HTTPcTask_Init() : ------------
*                               See HTTPcTask_Init() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPc_Init (const  HTTPc_CFG       *p_cfg,
                  const  HTTP_TASK_CFG   *p_task_cfg,
                         MEM_SEG         *p_mem_seg,
                         HTTPc_ERR       *p_err)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    if (HTTPc_InitDone == DEF_NO) {
        CPU_CRITICAL_EXIT();

#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)

        if (p_err == DEF_NULL) {                                /* Validate p_err ptr.                                  */
            CPU_SW_EXCEPTION();
        }

        if (p_cfg == DEF_NULL) {                                /* Validate cfg ptr.                                    */
           *p_err = HTTPc_ERR_NULL_PTR;
            goto exit;
        }

        if (p_cfg->MsgQ_Size <= 0) {
           *p_err = HTTPc_ERR_CFG_MSG_Q_SIZE_INVALID;
            goto exit;
        }

#ifdef HTTPc_TASK_MODULE_EN
                                                                /* Validate HTTPc Task parameters.                      */
        if (p_task_cfg == DEF_NULL) {
           *p_err = HTTPc_ERR_CFG_TASK_PTR_NULL;
            goto exit;
        }

        HTTPcTask_SetDly (p_cfg->TaskDly_ms, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             goto exit;
        }
#endif
#endif

        HTTPc_CfgPtr = p_cfg;                                   /* Set global Cfg pointer variable.                     */


#ifdef HTTPc_TASK_MODULE_EN
        HTTPc_Mem_TaskMsgPoolInit(p_cfg, p_mem_seg, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             goto exit;
        }
#ifdef  HTTPc_WEBSOCK_MODULE_EN
        HTTPc_Mem_WebSockReqPoolInit(p_cfg, p_mem_seg, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             goto exit;
        }
#endif
        HTTPcTask_Init(p_cfg,                                   /* HTTPc Task creation.                                 */
                       p_task_cfg,
                       p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             goto exit;
        }
#endif

    } else {
        CPU_CRITICAL_EXIT();
       *p_err = HTTPc_ERR_INIT;
        goto exit;
    }

    CPU_CRITICAL_ENTER();
    HTTPc_InitDone = DEF_YES;
    CPU_CRITICAL_EXIT();

   (void)p_mem_seg;

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                           HTTPc_ConnClr()
*
* Description : Clear an HTTPc Connection before the first usage.
*
* Argument(s) : p_conn_obj  Pointer to the current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Connection object clear successfully.
*                               HTTPc_ERR_INIT_NOT_COMPLETED    HTTPc Initialization is not completed.
*                               HTTPc_ERR_NULL_PTR              Function passed Null pointer as argument(s).
*                               HTTPc_ERR_CONN_IS_USED          Connection is already used.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) This function MUST be called before the HTTPc_CONN object is used for the first time.
*********************************************************************************************************
*/

void  HTTPc_ConnClr (HTTPc_CONN_OBJ    *p_conn_obj,
                     HTTPc_ERR         *p_err)
{
    const HTTPc_CFG    *p_cfg;
          HTTPc_CONN   *p_conn;
          CPU_BOOLEAN   in_use;
          CPU_SR_ALLOC();


                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    if (p_conn_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_cfg  = HTTPc_CfgPtr;
    p_conn = (HTTPc_CONN *)p_conn_obj;

                                                                /* ------ VALIDATE THAT CONN IS NOT USED ALREADY ------ */
    CPU_CRITICAL_ENTER();
    in_use = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_IN_USE);
    CPU_CRITICAL_EXIT();
    if (in_use == DEF_YES) {
       *p_err = HTTPc_ERR_CONN_IS_USED;
        goto exit;
    }

                                                                /* ------------ INIT CONNECTION PARAMETERS ------------ */
    Mem_Clr(&p_conn->ServerSockAddr, sizeof (NET_SOCK_ADDR));
    p_conn->SockID                      = NET_SOCK_ID_NONE;
    p_conn->SockFlags                   = DEF_BIT_NONE;
    p_conn->ConnectTimeout_ms           = p_cfg->ConnConnectTimeout_ms;
    p_conn->InactivityTimeout_s         = p_cfg->ConnInactivityTimeout_s;
    p_conn->ServerPort                  = HTTP_DFLT_PORT_NBR;
#ifdef NET_SECURE_MODULE_EN
    p_conn->SockSecureCfg.CommonName    = DEF_NULL;
    p_conn->SockSecureCfg.TrustCallback = DEF_NULL;
#endif
    p_conn->HostNamePtr                 = DEF_NULL;
    p_conn->HostNameLen                 = 0;
    p_conn->State                       = HTTPc_CONN_STATE_NONE;
    p_conn->Flags                       = DEF_BIT_NONE;
    p_conn->ErrCode                     = HTTPc_ERR_NONE;
    p_conn->CloseStatus                 = HTTPc_CONN_CLOSE_STATUS_NONE;
#ifdef HTTPc_TASK_MODULE_EN
    p_conn->OnConnect                   = DEF_NULL;
    p_conn->OnClose                     = DEF_NULL;
#endif
    p_conn->IsKeepAlive                 = p_cfg->ConnIsKeepAlive;

                                                                /* ------------- INIT REQUEST PARAMETERS -------------- */
    p_conn->ReqListHeadPtr              = DEF_NULL;
    p_conn->ReqListEndPtr               = DEF_NULL;
    p_conn->ReqFlags                    = 0u;
#if (HTTPc_CFG_QUERY_STR_EN == DEF_ENABLED)
    p_conn->ReqQueryStrTxIx             = 0u;
    p_conn->ReqQueryStrTempPtr          = DEF_NULL;
#endif
#if (HTTPc_CFG_HDR_TX_EN == DEF_ENABLED)
    p_conn->ReqHdrTxIx                  = 0u;
    p_conn->ReqHdrTempPtr               = DEF_NULL;
#endif
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
    p_conn->ReqFormDataTxIx             = 0u;
#endif
    p_conn->ReqDataOffset               = 0u;

                                                                /* ------------- INIT RESPONSE PARAMETERS ------------- */
    p_conn->RespFlags                   = 0u;


                                                                /* --------- INIT CONNECTION BUFFER PARAMETERS -------- */
    p_conn->RxBufPtr                    = p_conn->BufPtr;
    p_conn->RxDataLenRem                = 0u;
    p_conn->RxDataLen                   = 0u;
    p_conn->TxBufPtr                    = p_conn->BufPtr;
    p_conn->TxDataLen                   = 0u;
    p_conn->TxDataPtr                   = DEF_NULL;
    p_conn->BufPtr                      = DEF_NULL;
    p_conn->BufLen                      = 0;
    p_conn->NextPtr                     = DEF_NULL;

   *p_err                               = HTTPc_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                         HTTPc_ConnSetParam()
*
* Description : Set parameters related to the HTTP Client Connection.
*
* Argument(s) : p_conn      Pointer to the current HTTPc Connection.
*
*               type        Parameter type :
*
*                               HTTPc_PARAM_TYPE_SERVER_PORT
*                               HTTPc_PARAM_TYPE_PERSISTENT
*                               HTTPc_PARAM_TYPE_CONNECT_TIMEOUT
*                               HTTPc_PARAM_TYPE_INACTIVITY_TIMEOUT
*                               HTTPc_PARAM_TYPE_SECURE_COMMON_NAME
*                               HTTPc_PARAM_TYPE_SECURE_TRUST_CALLBACK
*                               HTTPc_PARAM_TYPE_CONN_CONNECT_CALLBACK
*                               HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK
*
*               p_param     Pointer to parameter
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Parameter set successfully.
*                               HTTPc_ERR_INIT_NOT_COMPLETED    HTTP Client Initialization not completed.
*                               HTTPc_ERR_NULL_PTR              Function passed Null argument(s).
*                               HTTPc_ERR_CONN_IS_USED          HTTPc Connection currently used.
*                               HTTPc_ERR_PARAM_INVALID         Invalid parameter passed.
*                               HTTPc_ERR_FEATURE_DIS           Parameter is related with a disabled feature.
*
* Return(s)   : None
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPc_ConnSetParam (HTTPc_CONN_OBJ    *p_conn_obj,
                          HTTPc_PARAM_TYPE   type,
                          void              *p_param,
                          HTTPc_ERR         *p_err)
{
    HTTPc_CONN       *p_conn;
    CPU_BOOLEAN       in_use;
#if (HTTPc_CFG_PERSISTENT_EN == DEF_ENABLED)
    CPU_BOOLEAN       persistent;
#endif
    CPU_SR_ALLOC();


                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    if (p_conn_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_param == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_conn = (HTTPc_CONN *)p_conn_obj;

                                                                /* ------ VALIDATE THAT CONN IS NOT USED ALREADY ------ */
    CPU_CRITICAL_ENTER();
    in_use = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_IN_USE);
    CPU_CRITICAL_EXIT();
    if (in_use == DEF_YES) {
       *p_err = HTTPc_ERR_CONN_IS_USED;
        goto exit;
    }

    switch (type) {
        case HTTPc_PARAM_TYPE_SERVER_PORT:
             p_conn->ServerPort = *(NET_PORT_NBR *)p_param;
             break;


        case HTTPc_PARAM_TYPE_PERSISTENT:
#if (HTTPc_CFG_PERSISTENT_EN == DEF_ENABLED)
             persistent = *(CPU_BOOLEAN *)p_param;
             if (persistent == DEF_YES) {
                 DEF_BIT_SET(p_conn->Flags, HTTPc_FLAG_CONN_PERSISTENT);
             }
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif


        case HTTPc_PARAM_TYPE_CONNECT_TIMEOUT:
             p_conn->ConnectTimeout_ms = *(CPU_INT16U *)p_param;
             break;


        case HTTPc_PARAM_TYPE_INACTIVITY_TIMEOUT:
             p_conn->InactivityTimeout_s = *(CPU_INT16U *)p_param;
             break;


        case HTTPc_PARAM_TYPE_SECURE_COMMON_NAME:
#ifdef NET_SECURE_MODULE_EN
             p_conn->SockSecureCfg.CommonName = (CPU_CHAR *)p_param;
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif


        case HTTPc_PARAM_TYPE_SECURE_TRUST_CALLBACK:
#ifdef NET_SECURE_MODULE_EN
             p_conn->SockSecureCfg.TrustCallback = (NET_SOCK_SECURE_TRUST_FNCT)p_param;
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif


        case HTTPc_PARAM_TYPE_CONN_CONNECT_CALLBACK:
#ifdef HTTPc_TASK_MODULE_EN
             p_conn->OnConnect = (HTTPc_CONNECT_CALLBACK)p_param;
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif


        case HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK:
#ifdef HTTPc_TASK_MODULE_EN
             p_conn->OnClose = (HTTPc_CONN_CLOSE_CALLBACK)p_param;
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif

        default:
           *p_err = HTTPc_ERR_PARAM_INVALID;
            goto exit;
    }

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                            HTTPc_ConnOpen()
*
* Description : Open a new HTTP connection.
*
* Argument(s) : p_conn_obj          Pointer to HTTPc Connection object to open.
*
*               p_buf               Pointer to the HTTP buffer that will be used to Tx and Rx data.
*
*               buf_len             Length of the HTTP buffer.
*
*               p_hostname_str      Pointer to the hostname string.
*
*               hostname_str_len    Length of the hostname string.
*
*               flags               Configuration flags :
*
*                                       HTTPc_FLAG_CONN_NO_BLOCK
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                          Connection Open successfully started.
*                               HTTPc_ERR_INIT_NOT_COMPLETED            HTTPc Initialization is not completed.
*                               HTTPc_ERR_NULL_PTR                      Function passed null pointer as argument(s).
*                               HTTPc_ERR_CFG_INVALID_BUF_LEN           Buffer Length is to small.
*                               HTTPc_ERR_CONN_PARAM_HOSTNAME_INVALID   Hostname Pointer is null.
*                               HTTPc_ERR_CONN_IS_USED                  Connection is already used.
*
* Return(s)   : DEF_OK,   if Connection opening successfully completed.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPc_ConnOpen (HTTPc_CONN_OBJ    *p_conn_obj,
                             CPU_CHAR          *p_buf,
                             CPU_INT16U         buf_len,
                             CPU_CHAR          *p_hostname_str,
                             CPU_INT16U         hostname_str_len,
                             HTTPc_FLAGS        flags,
                             HTTPc_ERR         *p_err)
{
    HTTPc_CONN      *p_conn;
    CPU_BOOLEAN      result;
    CPU_BOOLEAN      in_use;
    CPU_BOOLEAN      no_block;
#ifdef HTTPc_TASK_MODULE_EN
    HTTPc_ERR        err;
#endif
    CPU_SR_ALLOC();


    result = DEF_FAIL;

                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_FAIL);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    if (p_conn_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_buf == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (buf_len < HTTPc_BUF_LEN_MIN) {
       *p_err = HTTPc_ERR_CFG_INVALID_BUF_LEN;
        goto exit;
    }

    if (p_hostname_str == DEF_NULL) {
       *p_err = HTTPc_ERR_CONN_PARAM_HOSTNAME_INVALID;
        goto exit;
    }

    if (hostname_str_len <= 0) {
       *p_err = HTTPc_ERR_CONN_PARAM_HOSTNAME_INVALID;
        goto exit;
    }
#endif

    p_conn = (HTTPc_CONN *)p_conn_obj;
                                                                /* ------ VALIDATE THAT CONN IS NOT USED ALREADY ------ */
    CPU_CRITICAL_ENTER();
    in_use = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_IN_USE);
    CPU_CRITICAL_EXIT();
    if (in_use == DEF_YES) {
       *p_err = HTTPc_ERR_CONN_IS_USED;
        goto exit;
    }

                                                                /* -------------- SET BUFFER PARAMETERS --------------- */
    p_conn->BufPtr   = p_buf;
    p_conn->TxBufPtr = p_buf;
    p_conn->RxBufPtr = p_buf;
    p_conn->BufLen   = buf_len;
    p_conn->NextPtr  = DEF_NULL;

                                                                /* ------------- SET HOSTNAME PARAMETERS -------------- */
    p_conn->HostNamePtr = p_hostname_str;
    p_conn->HostNameLen = hostname_str_len;

                                                                /* ---------------- SET BLOCKING MODE ----------------- */
    no_block = DEF_BIT_IS_SET(flags, HTTPc_FLAG_CONN_NO_BLOCK);
    if (no_block == DEF_YES) {
        DEF_BIT_SET(p_conn->Flags, HTTPc_FLAG_CONN_NO_BLOCK);
    } else {
        DEF_BIT_CLR(p_conn->Flags, HTTPc_FLAG_CONN_NO_BLOCK);
    }

#ifdef HTTPc_TASK_MODULE_EN

    if (no_block == DEF_NO) {
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
        HTTPcTask_ConnConnectSignalCreate(p_conn, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             goto exit;
        }
#else
       *p_err = HTTPc_ERR_FEATURE_DIS;
        goto exit;
#endif
    }

    HTTPcTask_MsgQueue(         HTTPc_MSG_TYPE_CONN_OPEN,
                       (void *) p_conn,
                                p_err);
    if (*p_err != HTTPc_ERR_NONE) {
        goto exit;
    }

    HTTPcTask_Wake(p_conn, &err);
    if (*p_err != HTTPc_ERR_NONE) {
         CPU_SW_EXCEPTION(DEF_FAIL);
    }


    if (no_block == DEF_NO) {
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
        HTTPcTask_ConnConnectSignalWait(p_conn, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             HTTPc_ConnCloseHandler(p_conn, HTTPc_FLAG_NONE, &err);
             goto exit;
        }

        HTTPcTask_ConnConnectSignalDel(p_conn, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             HTTPc_ConnCloseHandler(p_conn, HTTPc_FLAG_NONE, &err);
             goto exit;
        }

        CPU_CRITICAL_ENTER();
        result = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_CONNECT);
        CPU_CRITICAL_EXIT();
        if (result == DEF_FAIL) {
           *p_err = p_conn->ErrCode;
            goto exit;
        }

#else
       *p_err = HTTPc_ERR_FEATURE_DIS;
        goto exit;
#endif
    }

#else

    if (no_block == DEF_YES) {
       *p_err  = HTTPc_ERR_FEATURE_DIS;
        goto exit;
    }

    HTTPcConn_Add(p_conn);

    while ((p_conn->State != HTTPc_CONN_STATE_PARAM_VALIDATE) &&
           (p_conn->State != HTTPc_CONN_STATE_NONE)) {
        HTTPcConn_Process(p_conn);
    }

   *p_err = p_conn->ErrCode;
    if (*p_err != HTTPc_ERR_NONE) {
         goto exit;
    }

    result = DEF_OK;

#endif

   *p_err = HTTPc_ERR_NONE;
exit:
    return (result);
}


/*
*********************************************************************************************************
*                                          HTTPc_ConnClose()
*
* Description : Close a persistent HTTPc Connection.
*
* Argument(s) : p_conn_obj  Pointer to HTTPc Connection to close.
*
*               flags       Configuration flags :
*
*                               HTTPc_FLAG_CONN_NO_BLOCK
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Connection closing successfully started.
*                               HTTPc_ERR_INIT_NOT_COMPLETED    HTTPc Initialization is not completed.
*                               HTTPc_ERR_NULL_PTR              Function passed a null pointer as argument.
*                               HTTPc_ERR_FEATURE_DIS           Connection Close function is unavailable.
*
*                               ------------ RETURNED BY HTTPc_ConnCloseHandler() ------------
*                               See HTTPc_ConnCloseHandler() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPc_ConnClose (HTTPc_CONN_OBJ    *p_conn_obj,
                       HTTPc_FLAGS        flags,
                       HTTPc_ERR         *p_err)
{
    HTTPc_CONN  *p_conn;


#if (HTTPc_CFG_PERSISTENT_EN == DEF_ENABLED)
                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    if (p_conn_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_conn = (HTTPc_CONN *)p_conn_obj;

    HTTPc_ConnCloseHandler(p_conn, flags, p_err);
    if (*p_err != HTTPc_ERR_NONE) {
         goto exit;
    }

   *p_err = HTTPc_ERR_NONE;

    goto exit;

#else
   (void)p_conn;

   *p_err = HTTPc_ERR_FEATURE_DIS;

    goto exit;
#endif

exit:
    return;
}


/*
*********************************************************************************************************
*                                            HTTPc_ReqClr()
*
* Description : Clear Request object members.
*
* Argument(s) : p_req_obj   Pointer to request object to clear.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Request cleared successfully.
*                               HTTPc_ERR_INIT_NOT_COMPLETED    HTTP Client Initialization not completed.
*                               HTTPc_ERR_NULL_PTR              Function passed a Null arguments.
*                               HTTPc_ERR_REQ_IS_USED           Request object already in usage.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPc_ReqClr (HTTPc_REQ_OBJ  *p_req_obj,
                    HTTPc_ERR      *p_err)
{
    HTTPc_REQ    *p_req;
    CPU_BOOLEAN   in_use;
    CPU_SR_ALLOC();


                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    if (p_req_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_req = (HTTPc_REQ *)p_req_obj;

                                                                /* ------ VALIDATE THAT REQ IS NOT USED ALREADY ------- */
    CPU_CRITICAL_ENTER();
    in_use = DEF_BIT_IS_SET(p_req->Flags, HTTPc_FLAG_REQ_IN_USE);
    CPU_CRITICAL_EXIT();
    if (in_use == DEF_YES) {
       *p_err = HTTPc_ERR_REQ_IS_USED;
        goto exit;
    }

    p_req->Flags                 = DEF_BIT_NONE;
    p_req->HdrFlags              = DEF_BIT_NONE;
    p_req->Method                = HTTP_METHOD_UNKNOWN;
    p_req->ResourcePathPtr       = DEF_NULL;
    p_req->ResourcePathLen       = 0;
    p_req->ContentType           = HTTP_CONTENT_TYPE_UNKNOWN;
    p_req->ContentLen            = 0u;
#if (HTTPc_CFG_QUERY_STR_EN == DEF_ENABLED)
    p_req->QueryStrTbl           = DEF_NULL;
    p_req->QueryStrNbr           = 0u;
    p_req->OnQueryStrTx          = DEF_NULL;
#endif
#if (HTTPc_CFG_HDR_TX_EN == DEF_ENABLED)
    p_req->HdrTbl                = DEF_NULL;
    p_req->HdrNbr                = 0;
    p_req->OnHdrTx               = DEF_NULL;
#endif
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
    p_req->FormFieldTbl          = DEF_NULL;
    p_req->FormFieldNbr          = 0;
#endif
    p_req->DataPtr               = DEF_NULL;
#if (HTTPc_CFG_CHUNK_TX_EN == DEF_ENABLED)
    p_req->OnBodyTx              = DEF_NULL;
#endif

#if (HTTPc_CFG_HDR_RX_EN == DEF_ENABLED)
    p_req->OnHdrRx               = DEF_NULL;
#endif
    p_req->OnBodyRx              = DEF_NULL;

#ifdef HTTPc_TASK_MODULE_EN
    p_req->OnTransComplete       = DEF_NULL;
    p_req->OnErr                 = DEF_NULL;
#endif

    p_req->ConnPtr               = DEF_NULL;
    p_req->RespPtr               = DEF_NULL;

    p_req->NextPtr               = DEF_NULL;

   *p_err = HTTPc_ERR_NONE;

    goto exit;


exit:
    return;
}


/*
*********************************************************************************************************
*                                          HTTPc_ReqSetParam()
*
* Description : Set a parameter related to a given HTTP Request.
*
* Argument(s) : p_req_obj   Pointer to request object.
*
*               type        Parameter type :
*
*                               HTTPc_PARAM_TYPE_REQ_QUERY_STR_TBL
*                               HTTPc_PARAM_TYPE_REQ_QUERY_STR_HOOK
*                               HTTPc_PARAM_TYPE_REQ_HDR_TBL
*                               HTTPc_PARAM_TYPE_REQ_HDR_HOOK
*                               HTTPc_PARAM_TYPE_REQ_FORM_TBL
*                               HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_TYPE
*                               HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_LEN
*                               HTTPc_PARAM_TYPE_REQ_BODY_CHUNK
*                               HTTPc_PARAM_TYPE_REQ_BODY_HOOK
*                               HTTPc_PARAM_TYPE_RESP_HDR_HOOK
*                               HTTPc_PARAM_TYPE_RESP_BODY_HOOK
*                               HTTPc_PARAM_TYPE_TRANS_COMPLETE_CALLBACK
*                               HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK
*
*               p_param     Pointer to parameter.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Parameter set successfully.
*                               HTTPc_ERR_INIT_NOT_COMPLETED    HTTP Client Initialization not completed.
*                               HTTPc_ERR_NULL_PTR              Function passed a null argument(s).
*                               HTTPc_ERR_REQ_IS_USED           Request object already in usage.
*                               HTTPc_ERR_FEATURE_DIS           Parameter is related to a feature disabled.
*                               HTTPc_ERR_PARAM_INVALID         Invalid request parameter.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPc_ReqSetParam (HTTPc_REQ_OBJ     *p_req_obj,
                         HTTPc_PARAM_TYPE   type,
                         void              *p_param,
                         HTTPc_ERR         *p_err)
{
    HTTPc_REQ        *p_req;
    HTTPc_PARAM_TBL  *p_tbl_obj;
    CPU_BOOLEAN       in_use;
    CPU_BOOLEAN       chunk_en;
    CPU_SR_ALLOC();


                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    if (p_req_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_param == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_req = (HTTPc_REQ *)p_req_obj;

                                                                /* ------ VALIDATE THAT REQ IS NOT USED ALREADY ------- */
    CPU_CRITICAL_ENTER();
    in_use = DEF_BIT_IS_SET(p_req->Flags, HTTPc_FLAG_REQ_IN_USE);
    CPU_CRITICAL_EXIT();
    if (in_use == DEF_YES) {
       *p_err = HTTPc_ERR_REQ_IS_USED;
        goto exit;
    }

                                                                /* ------------- REQUEST PARAMETER SETUP -------------- */
    switch (type) {
        case HTTPc_PARAM_TYPE_REQ_QUERY_STR_TBL:
             p_tbl_obj = (HTTPc_PARAM_TBL *)p_param;
#if (HTTPc_CFG_QUERY_STR_EN == DEF_ENABLED)
             p_req->QueryStrTbl  = (HTTPc_KEY_VAL *)p_tbl_obj->TblPtr;
             p_req->QueryStrNbr  = p_tbl_obj->EntryNbr;
             p_req->OnQueryStrTx = DEF_NULL;
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif


        case HTTPc_PARAM_TYPE_REQ_QUERY_STR_HOOK:
#if (HTTPc_CFG_QUERY_STR_EN == DEF_ENABLED)
             p_req->OnQueryStrTx = (HTTPc_REQ_QUERY_STR_HOOK)p_param;
             p_req->QueryStrTbl  = DEF_NULL;
             p_req->QueryStrNbr  = 0;
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif


        case HTTPc_PARAM_TYPE_REQ_HDR_TBL:
             p_tbl_obj = (HTTPc_PARAM_TBL *)p_param;
#if (HTTPc_CFG_HDR_TX_EN == DEF_ENABLED)
             p_req->HdrNbr     =  p_tbl_obj->EntryNbr;
             p_req->HdrTbl     = (HTTPc_HDR *)p_tbl_obj->TblPtr;
             p_req->OnHdrTx    =  DEF_NULL;
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif


        case HTTPc_PARAM_TYPE_REQ_HDR_HOOK:
#if (HTTPc_CFG_HDR_TX_EN == DEF_ENABLED)
             p_req->OnHdrTx    = (HTTPc_REQ_HDR_HOOK)p_param;
             p_req->HdrNbr     =  0;
             p_req->HdrTbl     =  DEF_NULL;
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif


        case HTTPc_PARAM_TYPE_REQ_FORM_TBL:
             p_tbl_obj = (HTTPc_PARAM_TBL *)p_param;
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
             DEF_BIT_SET(p_req->Flags, HTTPc_FLAG_REQ_BODY_PRESENT);
             DEF_BIT_SET(p_req->Flags, HTTPc_FLAG_REQ_FORM_PRESENT);
             p_req->FormFieldNbr = p_tbl_obj->EntryNbr;
             p_req->FormFieldTbl = p_tbl_obj->TblPtr;
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif


        case HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_TYPE:
             DEF_BIT_SET(p_req->Flags, HTTPc_FLAG_REQ_BODY_PRESENT);
             p_req->ContentType = *(HTTP_CONTENT_TYPE *)p_param;
             break;


        case HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_LEN:
             DEF_BIT_SET(p_req->Flags, HTTPc_FLAG_REQ_BODY_PRESENT);
             p_req->ContentLen = *(CPU_INT32U *)p_param;
             break;


        case HTTPc_PARAM_TYPE_REQ_BODY_CHUNK:
#if (HTTPc_CFG_CHUNK_TX_EN == DEF_ENABLED)
             chunk_en = *(CPU_BOOLEAN *)p_param;
             if (chunk_en == DEF_YES) {
                 DEF_BIT_SET(p_req->Flags, HTTPc_FLAG_REQ_BODY_CHUNK_TRANSFER);
             }
             DEF_BIT_SET(p_req->Flags, HTTPc_FLAG_REQ_BODY_PRESENT);
             break;
#else
            (void)chunk_en;
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif


        case HTTPc_PARAM_TYPE_REQ_BODY_HOOK:
             p_req->OnBodyTx = (HTTPc_REQ_BODY_HOOK)p_param;
             break;


        case HTTPc_PARAM_TYPE_RESP_HDR_HOOK:
#if (HTTPc_CFG_HDR_RX_EN == DEF_ENABLED)
             p_req->OnHdrRx = (HTTPc_RESP_HDR_HOOK)p_param;
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif


        case HTTPc_PARAM_TYPE_RESP_BODY_HOOK:
             p_req->OnBodyRx = (HTTPc_RESP_BODY_HOOK)p_param;
             break;


        case HTTPc_PARAM_TYPE_TRANS_COMPLETE_CALLBACK:
#ifdef HTTPc_TASK_MODULE_EN
             p_req->OnTransComplete = (HTTPc_COMPLETE_CALLBACK)p_param;
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif


        case HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK:
#ifdef HTTPc_TASK_MODULE_EN
             p_req->OnErr = (HTTPc_TRANS_ERR_CALLBACK)p_param;
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif

        case HTTPc_PARAM_TYPE_REQ_UPGRADE_WEBSOCKET:
#ifdef  HTTPc_WEBSOCK_MODULE_EN
             DEF_BIT_SET(p_req->Flags, HTTPc_FLAG_REQ_UPGRADE_WEBSOCKET);
             break;
#else
            *p_err = HTTPc_ERR_FEATURE_DIS;
             goto exit;
#endif
        default:
           *p_err = HTTPc_ERR_PARAM_INVALID;
            goto exit;
    }

   (void)p_tbl_obj;

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                            HTTPc_ReqSend()
*
* Description : Send a HTTP request.
*
* Argument(s) : p_conn_obj          Pointer to valid HTTPc Connection on which request will occurred.
*
*               p_req_obj           Pointer to request to send.
*
*               p_resp_obj          Pointer to response object that will be filled with the received response.
*
*               method              HTTP method of the request.
*
*               p_resource_path     Pointer to complete URI (or only resource path) of the request.
*
*               resource_path_len   Resource path length.
*
*               flags               Configuration flags :
*
*                                       HTTPc_FLAG_REQ_NO_BLOCK
*
*               p_err               Pointer to variable that will receive the return error code from this function :
*
*                                       HTTPc_ERR_NONE                              Request Sending successfully started.
*                                       HTTPc_ERR_INIT_NOT_COMPLETED                HTTPc Initialization is not completed.
*                                       HTTPc_ERR_NULL_PTR                          Null pointer(s) was passed as argument(s).
*                                       HTTPc_ERR_CONN_IS_RELEASED                  Connection is not active.
*                                       HTTPc_ERR_REQ_IS_USED                       Request is already in usage.
*                                       HTTPc_ERR_REQ_PARAM_METHOD_INVALID          Invalid HTTP Method.
*                                       HTTPc_ERR_REQ_PARAM_RESOURCE_PATH_INVALID   Resource path is null.
*                                       HTTPc_ERR_FEATURE_DIS                       A needed feature is disabled.
*
*
*                                       ------------ RETURNED BY HTTPcTask_TransDoneSignalCreate() ------------
*                                       See HTTPcTask_TransDoneSignalCreate() for additional return error codes.
*
*                                       ------------ RETURNED BY HTTPcTask_TransDoneSignalWait() ------------
*                                       See HTTPcTask_TransDoneSignalWait() for additional return error codes.
*
*                                       ------------ RETURNED BY HTTPcTask_TransDoneSignalDel() ------------
*                                       See HTTPcTask_TransDoneSignalDel() for additional return error codes.
*
*                                       ------------ RETURNED BY HTTPcReq_Prepare() ------------
*                                       See HTTPcReq_Prepare() for additional return error codes.
*
*                                       ------------ RETURNED BY HTTPcSock_Sel() ------------
*                                       See HTTPcSock_Sel() for additional return error codes.
*
*                                       ------------ RETURNED BY HTTPcReq() ------------
*                                       See HTTPcReq() for additional return error codes.
*
*                                       ------------ RETURNED BY HTTPcResp() ------------
*                                       See HTTPcResp() for additional return error codes.
*
* Return(s)   : DEF_YES, if HTTP Response received successfully.
*               DEF_NO,  otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPc_ReqSend (HTTPc_CONN_OBJ    *p_conn_obj,
                            HTTPc_REQ_OBJ     *p_req_obj,
                            HTTPc_RESP_OBJ    *p_resp_obj,
                            HTTP_METHOD        method,
                            CPU_CHAR          *p_resource_path,
                            CPU_INT16U         resource_path_len,
                            HTTPc_FLAGS        flags,
                            HTTPc_ERR         *p_err)
{
    HTTPc_CONN   *p_conn;
    HTTPc_REQ    *p_req;
    CPU_BOOLEAN   result;
    CPU_BOOLEAN   in_use;
    CPU_BOOLEAN   no_block;
#ifdef HTTPc_TASK_MODULE_EN
    HTTPc_ERR     err;
#endif
    CPU_SR_ALLOC();


    result = DEF_FAIL;

                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_FAIL);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    if (p_conn_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_req_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_resp_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_conn = (HTTPc_CONN *)p_conn_obj;
    p_req  = (HTTPc_REQ  *)p_req_obj;


                                                                /* ------ VALIDATE THAT REQ IS NOT USED ALREADY ------- */
    CPU_CRITICAL_ENTER();
    in_use = DEF_BIT_IS_SET(p_req->Flags, HTTPc_FLAG_REQ_IN_USE);
    if (in_use != DEF_YES) {
        DEF_BIT_SET(p_req->Flags, HTTPc_FLAG_REQ_IN_USE);
        CPU_CRITICAL_EXIT();
    } else {
        CPU_CRITICAL_EXIT();
       *p_err = HTTPc_ERR_REQ_IS_USED;
        goto exit;
    }

    p_req->ConnPtr = p_conn;
    p_req->RespPtr = p_resp_obj;

                                                                /* ----------------- SET HTTP METHOD ------------------ */
    switch (method) {
        case HTTP_METHOD_GET:
        case HTTP_METHOD_POST:
        case HTTP_METHOD_HEAD:
        case HTTP_METHOD_PUT:
        case HTTP_METHOD_DELETE:
        case HTTP_METHOD_TRACE:
        case HTTP_METHOD_CONNECT:
        case HTTP_METHOD_OPTIONS:
             p_req->Method = method;
             break;

        case HTTP_METHOD_UNKNOWN:
        default:
            *p_err = HTTPc_ERR_REQ_PARAM_METHOD_INVALID;
             goto exit;
    }

                                                                /* ---------------- SET RESOURCE PATH ----------------- */
    if (p_resource_path == DEF_NULL) {
       *p_err = HTTPc_ERR_REQ_PARAM_RESOURCE_PATH_INVALID;
        goto exit;
    }
    p_req->ResourcePathPtr = p_resource_path;
    p_req->ResourcePathLen = resource_path_len;

                                                                /* ---------------- SET BLOCKING MODE ----------------- */
    no_block = DEF_BIT_IS_SET(flags, HTTPc_FLAG_REQ_NO_BLOCK);
    if (no_block == DEF_YES) {
        DEF_BIT_SET(p_req->Flags, HTTPc_FLAG_REQ_NO_BLOCK);
    } else {
        DEF_BIT_CLR(p_req->Flags, HTTPc_FLAG_REQ_NO_BLOCK);
    }

#ifdef HTTPc_TASK_MODULE_EN
    if (no_block == DEF_NO) {
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
        HTTPcTask_TransDoneSignalCreate(p_conn, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             goto exit_conn_close;
        }
#else
       *p_err = HTTPc_ERR_FEATURE_DIS;
        goto exit;
#endif
    }

    HTTPcTask_MsgQueue(         HTTPc_MSG_TYPE_REQ,
                       (void *) p_req,
                                p_err);
    if (*p_err != HTTPc_ERR_NONE) {
        goto exit;
    }


    HTTPcTask_Wake(p_conn, &err);
    if (*p_err != HTTPc_ERR_NONE) {
         goto exit;
    }

    if (no_block == DEF_NO) {
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
                                                                /* -------- WAIT FOR RESPONSE IN BLOCKING MODE -------- */
        HTTPcTask_TransDoneSignalWait(p_conn, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             goto exit_conn_close;
        }

        CPU_CRITICAL_ENTER();
        result = DEF_BIT_IS_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_COMPLETE_OK);
        CPU_CRITICAL_EXIT();
        if (result == DEF_FAIL) {
           *p_err = p_conn->ErrCode;
            goto exit;
        }

        HTTPcTask_TransDoneSignalDel(p_conn, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             goto exit_conn_close;
        }

#else
       *p_err = HTTPc_ERR_FEATURE_DIS;
        goto exit;
#endif
    }

   *p_err = HTTPc_ERR_NONE;

    goto exit;
#else

    if (no_block == DEF_YES) {
       *p_err  = HTTPc_ERR_FEATURE_DIS;
        goto exit;
    }

    HTTPcConn_ReqAdd(p_req, p_err);                             /* Add Request to Connection.                           */
    if (*p_err != HTTPc_ERR_NONE) {
         goto exit;
    }

    do {
        HTTPcSock_Sel(p_conn, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             goto exit;
        }
        HTTPcConn_TransProcess(p_conn);
    } while ((p_conn->State != HTTPc_CONN_STATE_PARAM_VALIDATE) &&
             (p_conn->State != HTTPc_CONN_STATE_NONE));

    result = DEF_BIT_IS_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_COMPLETE_OK);

   *p_err = p_conn->ErrCode;

    goto exit;

#endif


#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
exit_conn_close:
    HTTPc_ConnCloseHandler(p_conn, HTTPc_FLAG_NONE, &err);
#endif

exit:
    return (result);
}


/*
*********************************************************************************************************
*                                         HTTPc_FormAppFmt()
*
* Description : Format an application type form from a form field's table.
*
* Argument(s) : p_buf           Pointer to buffer where the form will be written.
*
*               buf_len         Buffer length.
*
*               p_form_tbl      Pointer to form field's table.
*
*               form_tbl_size   Table size.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   HTTPc_ERR_NONE                  Form successfully created.
*                                   HTTPc_ERR_NULL_PTR              Null pointer argument(s) passed.
*                                   HTTPc_ERR_FORM_TYPE_INVALID     Invalid form field type.
*                                   HTTPc_ERR_FORM_BUF_LEN_INVALID  buffer size to small for form.
*                                   HTTPc_ERR_FORM_CREATE           Form creation faulted.
*
* Return(s)   : Length of the formatted form, if no errors.
*
*               0 , otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Only standard Key-Value Pair object are supported in the table to be able to format
*                   the form.
*********************************************************************************************************
*/
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
CPU_INT32U  HTTPc_FormAppFmt (CPU_CHAR              *p_buf,
                              CPU_INT16U             buf_len,
                              HTTPc_FORM_TBL_FIELD  *p_form_tbl,
                              CPU_INT16U             form_tbl_size,
                              HTTPc_ERR             *p_err)
{
    CPU_CHAR              *p_buf_wr;
    HTTPc_FORM_TBL_FIELD  *p_tbl_field;
    HTTPc_KEY_VAL         *p_form_field;
    CPU_INT16U             key_char_encode_nbr;
    CPU_INT16U             val_char_encode_nbr;
    CPU_SIZE_T             str_len_key;
    CPU_SIZE_T             str_len_val;
    CPU_SIZE_T             data_size;
    CPU_BOOLEAN            url_encode;
    CPU_INT16U             i;


    data_size = 0;

                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(0);
    }

    if (p_buf == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_form_tbl == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_buf_wr  = p_buf;

                                                                /* ----------------- PARSE FORM TABLE ----------------- */
    for (i = 0u; i < form_tbl_size; i++) {

        p_tbl_field = &p_form_tbl[i];

        switch (p_tbl_field->Type) {
            case HTTPc_FORM_FIELD_TYPE_KEY_VAL:
                 break;

            case HTTPc_FORM_FIELD_TYPE_KEY_VAL_EXT:
            case HTTPc_FORM_FIELD_TYPE_FILE:
            default:
                 data_size = 0;
                *p_err     = HTTPc_ERR_FORM_TYPE_INVALID;
                 goto exit;
        }

        p_form_field = p_tbl_field->FieldObjPtr;

                                                                /* Calculate length of key and value without encoding.  */
        str_len_key = p_form_field->KeyLen;
        str_len_val = p_form_field->ValLen;

                                                                 /* Found number of character needing URL encoding.      */
        key_char_encode_nbr = HTTP_URL_CharEncodeNbr(p_form_field->KeyPtr, str_len_key);
        val_char_encode_nbr = HTTP_URL_CharEncodeNbr(p_form_field->ValPtr, str_len_val);

                                                                /* Calculate total size needed for key & value encoded. */
        data_size += str_len_key +
                     str_len_val +
                     HTTP_URL_ENCODING_JUMP * key_char_encode_nbr +
                     HTTP_URL_ENCODING_JUMP * val_char_encode_nbr +
                     1;                                         /* + 1 for the "=" between key & value.                 */

        if (i < (form_tbl_size - 1)) {
            data_size++;                                        /* + 1 for the "&" between each key&value pair.         */
        }


        if (data_size > buf_len) {                              /* Return if no more space in buf.                      */
            data_size = 0;
           *p_err = HTTPc_ERR_FORM_BUF_LEN_INVALID;
            goto exit;
        }

        url_encode = HTTP_URL_EncodeStr(p_form_field->KeyPtr,   /* Encode and Write to buffer the Key.                  */
                                        p_buf_wr,
                                       &str_len_key,
                                        buf_len);
        if (url_encode == DEF_FAIL) {
            data_size = 0;
           *p_err = HTTPc_ERR_FORM_CREATE;
            goto exit;
        }

        p_buf_wr += str_len_key;
        buf_len  -= str_len_key;

       *p_buf_wr = ASCII_CHAR_EQUALS_SIGN;                      /* Write the "=" sign.                                  */
        p_buf_wr++;
        buf_len--;

        url_encode = HTTP_URL_EncodeStr(p_form_field->ValPtr,   /* Encode and Write to buffer the Value.                */
                                        p_buf_wr,
                                       &str_len_val,
                                        buf_len);
        if (url_encode == DEF_FAIL) {
            data_size = 0;
           *p_err = HTTPc_ERR_FORM_CREATE;
            goto exit;
        }

        p_buf_wr += str_len_val;
        buf_len  -= str_len_val;

        if (i < (form_tbl_size - 1)) {
           *p_buf_wr = ASCII_CHAR_AMPERSAND;                    /* Write the "&" sign between pairs.                    */
            p_buf_wr++;
            buf_len--;
        }

        p_form_field++;
    }

   *p_err = HTTPc_ERR_NONE;


exit:
    return (data_size);
}
#endif


/*
*********************************************************************************************************
*                                        HTTPc_FormMultipartFmt()
*
* Description : Format an multipart type form from a form field's table.
*
* Argument(s) : p_buf           Pointer to buffer where the form will be written.
*
*               buf_len         Buffer length.
*
*               p_form_tbl      Pointer to form field's table.
*
*               form_tbl_size   Table size.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   HTTPc_ERR_NONE                  Form successfully created.
*                                   HTTPc_ERR_NULL_PTR              Null pointer argument(s) passed.
*                                   HTTPc_ERR_FORM_TYPE_INVALID     Invalid form field type.
*                                   HTTPc_ERR_FORM_BUF_LEN_INVALID  buffer size to small for form.
*                                   HTTPc_ERR_FORM_CREATE           Form creation faulted.
*
* Return(s)   : Length of the formatted form, if no errors.
*
*               0 , otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Only standard Key-Value Pair object are supported in the table to be able to format
*                   the form.
*********************************************************************************************************
*/
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
CPU_INT32U  HTTPc_FormMultipartFmt (CPU_CHAR              *p_buf,
                                    CPU_INT16U             buf_len,
                                    HTTPc_FORM_TBL_FIELD  *p_form_tbl,
                                    CPU_INT16U             form_tbl_size,
                                    HTTPc_ERR             *p_err)
{
    CPU_CHAR              *p_buf_wr;
    CPU_CHAR              *p_str;
    HTTPc_FORM_TBL_FIELD  *p_tbl_field;
    HTTPc_KEY_VAL         *p_form_field;
    CPU_SIZE_T             data_size = 0;
    CPU_SIZE_T             name_field_len;
    CPU_INT08U             name_char_encode_nbr;
    CPU_INT16U             tot_buf_len;
    CPU_INT16U             i;
    CPU_BOOLEAN            url_encode;


                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(0);
    }

    if (p_buf == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_form_tbl == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    tot_buf_len = buf_len;
    p_buf_wr    = p_buf;

                                                                /* ----------------- PARSE FORM TABLE ----------------- */
    for (i = 0u; i < form_tbl_size; i++) {

        p_tbl_field = &p_form_tbl[i];

        switch (p_tbl_field->Type) {
            case HTTPc_FORM_FIELD_TYPE_KEY_VAL:
                 break;

            case HTTPc_FORM_FIELD_TYPE_KEY_VAL_EXT:
            case HTTPc_FORM_FIELD_TYPE_FILE:
            default:
                 data_size = 0;
                *p_err     = HTTPc_ERR_FORM_TYPE_INVALID;
                 goto exit;
        }

        p_form_field = p_tbl_field->FieldObjPtr;


        data_size += HTTPc_STR_BOUNDARY_START_LEN + STR_CR_LF_LEN;

                                                                /* Find nbr of chars to encode in name.                 */
        name_field_len       = p_form_field->KeyLen;
        name_char_encode_nbr = HTTP_URL_CharEncodeNbr(p_form_field->KeyPtr, name_field_len);

        data_size += HTTP_STR_HDR_FIELD_CONTENT_DISPOSITION_LEN +           /* "Content-Disposition"  */
                     2 +                                                    /* ": "                   */
                     HTTP_STR_CONTENT_DISPOSITION_FORM_DATA_LEN +           /* "form-data"            */
                     2 +                                                    /* "; "                   */
                     HTTP_STR_MULTIPART_FIELD_NAME_LEN +                    /* "name"                 */
                     3 +                                                    /* "=\"\""                */
                     name_field_len +                                       /* "name_rx_in_form"      */
                     HTTP_URL_ENCODING_JUMP * name_char_encode_nbr +        /* nbr of char to encode. */
                     STR_CR_LF_LEN;                                         /* "\r\n"                 */

        data_size += 2 * STR_CR_LF_LEN +
                     p_form_field->ValLen;


        if (i == (form_tbl_size -1)) {
            data_size += (HTTPc_STR_BOUNDARY_END_LEN + STR_CR_LF_LEN);
        }

        if (data_size > buf_len) {                             /* Return if no more space in buf.                      */
            data_size = 0;
           *p_err = HTTPc_ERR_FORM_BUF_LEN_INVALID;
            goto exit;
        }


                                                                /* Write start of boundary.                            */
        p_str = Str_Copy_N(p_buf_wr, HTTPc_STR_BOUNDARY_START, data_size);
        if (p_str == DEF_NULL) {
           *p_err = HTTPc_ERR_FORM_CREATE;
            goto exit;
        }

        p_str += HTTPc_STR_BOUNDARY_START_LEN;

                                                                /* Write CRLF after boundary.                           */
        p_str = Str_Copy_N(p_str, STR_CR_LF, STR_CR_LF_LEN);
        if (p_str == DEF_NULL) {
           *p_err = HTTPc_ERR_FORM_CREATE;
            goto exit;
        }

        p_str   += STR_CR_LF_LEN;
        buf_len -= (p_str - p_buf_wr);
        p_buf_wr = p_str;

                                                                /* Write Content-Disposition header.                    */
        p_str = HTTPcReq_HdrCopyToBuf(p_buf_wr,
                                      tot_buf_len,
                                      buf_len,
                                      HTTP_HDR_FIELD_CONTENT_DISPOSITION,
                                      HTTP_STR_CONTENT_DISPOSITION_FORM_DATA,
                                      HTTP_STR_CONTENT_DISPOSITION_FORM_DATA_LEN,
                                      DEF_NO,
                                      p_err);
        if (p_str == DEF_NULL) {
        *p_err = HTTPc_ERR_FORM_CREATE;
         goto exit;
        }

       *p_str = ASCII_CHAR_SEMICOLON;
        p_str++;
       *p_str = ASCII_CHAR_SPACE;
        p_str++;

                                                                /* Write string "name".                                 */
        p_str = Str_Copy_N(p_str, HTTP_STR_MULTIPART_FIELD_NAME, HTTP_STR_MULTIPART_FIELD_NAME_LEN);
        if (p_str == DEF_NULL) {
           *p_err = HTTPc_ERR_FORM_CREATE;
            goto exit;
        }

        p_str += HTTP_STR_MULTIPART_FIELD_NAME_LEN;
       *p_str = ASCII_CHAR_EQUALS_SIGN;
        p_str++;
       *p_str = ASCII_CHAR_QUOTATION_MARK;
        p_str++;

                                                                /* Write name's value.                                  */
        url_encode = HTTP_URL_EncodeStr(p_form_field->KeyPtr,
                                        p_str,
                                       &name_field_len,
                                        buf_len);
        if (url_encode == DEF_FAIL) {
           *p_err = HTTPc_ERR_FORM_CREATE;
            goto exit;
        }

        p_str += name_field_len;
       *p_str  = ASCII_CHAR_QUOTATION_MARK;
        p_str++;

                                                                /* Write 2 CRLF.                                        */
        p_str = Str_Copy_N(p_str, STR_CR_LF, STR_CR_LF_LEN);
        if (p_str == DEF_NULL) {
           *p_err = HTTPc_ERR_REQ_FORM_CREATE;
            goto exit;
        }

        p_str    += STR_CR_LF_LEN;

        buf_len  -= (p_str- p_buf_wr);
        p_buf_wr  = p_str;

        p_str = Str_Copy_N(p_str, STR_CR_LF, STR_CR_LF_LEN);
        if (p_str == DEF_NULL) {
           *p_err = HTTPc_ERR_REQ_FORM_CREATE;
            goto exit;
        }

        p_str    += STR_CR_LF_LEN;
        buf_len  -= (p_str - p_buf_wr);
        p_buf_wr  = p_str;

                                                                /* Write Data.                                          */
        Mem_Copy(p_buf_wr, p_form_field->ValPtr, p_form_field->ValLen);

        p_str    += p_form_field->ValLen;
        buf_len  -= (p_str - p_buf_wr);
        p_buf_wr  = p_str;

                                                                /* Write CRLF.                                          */
        p_str = Str_Copy_N(p_buf_wr, STR_CR_LF, STR_CR_LF_LEN);
        if (p_str == DEF_NULL) {
           *p_err = HTTPc_ERR_REQ_FORM_CREATE;
            goto exit;
        }

        p_str    += STR_CR_LF_LEN;
        buf_len  -= (p_str - p_buf_wr);
        p_buf_wr  = p_str;

        if (i == (form_tbl_size -1)) {
                                                                /* Write end of boundary.                            */
            p_str = Str_Copy_N(p_buf_wr, HTTPc_STR_BOUNDARY_END, HTTPc_STR_BOUNDARY_END_LEN);
            if (p_str == DEF_NULL) {
               *p_err = HTTPc_ERR_REQ_FORM_CREATE;
                goto exit;
            }

            p_str += HTTPc_STR_BOUNDARY_END_LEN;
                                                                /* Write CRLF after last boundary.                      */
            p_str = Str_Copy_N(p_str, STR_CR_LF, STR_CR_LF_LEN);
            if (p_str == DEF_NULL) {
               *p_err = HTTPc_ERR_REQ_FORM_CREATE;
                goto exit;
            }

            p_str    += STR_CR_LF_LEN;
            buf_len  -= (p_str - p_buf_wr);
            p_buf_wr  = p_str;
        }
    }

   *p_err = HTTPc_ERR_NONE;


exit:
    return (data_size);
}
#endif


/*
*********************************************************************************************************
*                                          HTTPc_FormAddKeyVal()
*
* Description : Add a Key-Value Pair object to the form table.
*
* Argument(s) : p_form_tbl  Pointer to the form table.
*
*               p_kvp       Pointer to Key-Value Pair object to put in table.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Object successfully added to Form table.
*                               HTTPc_ERR_NULL_PTR              Null argument(s) passed.
*                               HTTPc_ERR_FORM_FIELD_INVALID    Invalid form object.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
void  HTTPc_FormAddKeyVal (HTTPc_FORM_TBL_FIELD  *p_form_tbl,
                           HTTPc_KEY_VAL         *p_key_val,
                           HTTPc_ERR             *p_err)
{
                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (p_form_tbl == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_key_val == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

                                                                /* --------------- VALIDATE KVP OBJECT ---------------- */
    if (p_key_val->KeyPtr == DEF_NULL) {
       *p_err = HTTPc_ERR_FORM_FIELD_INVALID;
        goto exit;

    }

    if (p_key_val->ValPtr == DEF_NULL) {
       *p_err = HTTPc_ERR_FORM_FIELD_INVALID;
        goto exit;
    }

                                                                /* ----------------- SET TABLE FIELD ------------------ */
    p_form_tbl->Type        = HTTPc_FORM_FIELD_TYPE_KEY_VAL;
    p_form_tbl->FieldObjPtr = (void *)p_key_val;

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}
#endif


/*
*********************************************************************************************************
*                                        HTTPc_FormAddKeyValExt()
*
* Description : Add an Extended Key-Value Pair object to the form table.
*
* Argument(s) : p_form_tbl      p_key_val_ext
*
*               p_key_val_ext   Pointer to the Extended Key-Value Pair object to put in table.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Object successfully added to Form table.
*                               HTTPc_ERR_NULL_PTR              Null argument(s) passed.
*                               HTTPc_ERR_FORM_FIELD_INVALID    Invalid form object.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) HTTPc_KVP_BIG type object allows you to setup a Hook function that will be called when
*                   the form is sent to let the Application write the value directly into the buffer.
*********************************************************************************************************
*/
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
void  HTTPc_FormAddKeyValExt (HTTPc_FORM_TBL_FIELD  *p_form_tbl,
                              HTTPc_KEY_VAL_EXT     *p_key_val_ext,
                              HTTPc_ERR             *p_err)
{
                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (p_form_tbl == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_key_val_ext == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

                                                                /* --------------- VALIDATE KVP OBJECT ---------------- */
    if (p_key_val_ext->KeyPtr == DEF_NULL) {
       *p_err = HTTPc_ERR_FORM_FIELD_INVALID;
        goto exit;

    }

    if (p_key_val_ext->OnValTx == DEF_NULL) {
       *p_err = HTTPc_ERR_FORM_FIELD_INVALID;
        goto exit;
    }

                                                                /* ----------------- SET TABLE FIELD ------------------ */
    p_form_tbl->Type        = HTTPc_FORM_FIELD_TYPE_KEY_VAL_EXT;
    p_form_tbl->FieldObjPtr = (void *)p_key_val_ext;

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}
#endif


/*
*********************************************************************************************************
*                                          HTTPc_FormAddFile()
*
* Description : Add a multipart file object to the form table.
*
* Argument(s) : p_form_tbl  Pointer to the form table.
*
*               p_file_obj  Pointer to the multipart file object to put in table.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Object successfully added to Form table.
*                               HTTPc_ERR_NULL_PTR              Null argument(s) passed.
*                               HTTPc_ERR_FORM_FIELD_INVALID    Invalid form object.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
void  HTTPc_FormAddFile (HTTPc_FORM_TBL_FIELD  *p_form_tbl,
                         HTTPc_MULTIPART_FILE  *p_file_obj,
                         HTTPc_ERR             *p_err)
{
                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (p_form_tbl == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_file_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

                                                                /* ---------- VALIDATE MULTIPART FILE OBJECT ---------- */
    if (p_file_obj->NamePtr == DEF_NULL) {
       *p_err = HTTPc_ERR_FORM_FIELD_INVALID;
        goto exit;

    }

    if (p_file_obj->FileNamePtr == DEF_NULL) {
       *p_err = HTTPc_ERR_FORM_FIELD_INVALID;
        goto exit;
    }

    if (p_file_obj->OnFileTx == DEF_NULL) {
       *p_err = HTTPc_ERR_FORM_FIELD_INVALID;
        goto exit;
    }

                                                                /* ----------------- SET TABLE FIELD ------------------ */
    p_form_tbl->Type        = HTTPc_FORM_FIELD_TYPE_FILE;
    p_form_tbl->FieldObjPtr = (void *)p_file_obj;

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}
#endif


/*
*********************************************************************************************************
*                                        HTTPc_WebSockSetParam()
*
* Description : Set a parameter related to a given WebSocket Obj.
*
* Argument(s) : p_ws_obj    Pointer to the WebSocket Obj.
*
*               type        Parameter type :
*
*                               HTTPc_PARAM_TYPE_WEBSOCK_ON_OPEN
*                               HTTPc_PARAM_TYPE_WEBSOCK_ON_CLOSE
*                               HTTPc_PARAM_TYPE_WEBSOCK_ON_MSG_RX_INIT
*                               HTTPc_PARAM_TYPE_WEBSOCK_ON_MSG_RX_DATA
*                               HTTPc_PARAM_TYPE_WEBSOCK_ON_MSG_RX_COMPLETE
*                               HTTPc_PARAM_TYPE_WEBSOCK_ON_ERR
*                               HTTPc_PARAM_TYPE_WEBSOCK_ON_PONG
*
*               p_param     Pointer to parameter.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Parameter set successfully.
*                               HTTPc_ERR_INIT_NOT_COMPLETED    HTTPc Initialization is not completed.
*                               HTTPc_ERR_NULL_PTR              Null argument(s) passed.
*                               HTTPc_ERR_WEBSOCK_IS_USED       WebSocket Obj is already in use.
*                               HTTPc_ERR_PARAM_INVALID         Invalid request parameter.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef  HTTPc_WEBSOCK_MODULE_EN
void  HTTPc_WebSockSetParam (HTTPc_WEBSOCK_OBJ  *p_ws_obj,
                             HTTPc_PARAM_TYPE    type,
                             void               *p_param,
                             HTTPc_ERR          *p_err)
{
    HTTPc_WEBSOCK  *p_ws;
    CPU_BOOLEAN     in_use;
    CPU_SR_ALLOC();

                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    if (p_ws_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_param == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_ws = (HTTPc_WEBSOCK *)p_ws_obj;
                                                                /* ------ VALIDATE THAT REQ IS NOT USED ALREADY ------- */
    CPU_CRITICAL_ENTER();
    in_use = p_ws->Flags.IsWebsockUsed;
    CPU_CRITICAL_EXIT();
    if (in_use == DEF_YES) {
       *p_err = HTTPc_ERR_WEBSOCK_IS_USED;
        goto exit;
    }

                                                                /* ------------- WEBSOCK PARAMETER SETUP -------------- */
    switch (type) {

        case HTTPc_PARAM_TYPE_WEBSOCK_ON_OPEN:
             p_ws->OnOpen = (HTTPc_WEBSOCK_ON_OPEN)p_param;
             break;

        case HTTPc_PARAM_TYPE_WEBSOCK_ON_CLOSE:
             p_ws->OnClose = (HTTPc_WEBSOCK_ON_CLOSE)p_param;
             break;

        case HTTPc_PARAM_TYPE_WEBSOCK_ON_MSG_RX_INIT:
             p_ws->OnMsgRxInit = (HTTPc_WEBSOCK_ON_RX_INIT)p_param;
             break;

        case HTTPc_PARAM_TYPE_WEBSOCK_ON_MSG_RX_DATA:
             p_ws->OnMsgRxData = (HTTPc_WEBSOCK_ON_RX_DATA)p_param;
             break;

        case HTTPc_PARAM_TYPE_WEBSOCK_ON_MSG_RX_COMPLETE:
             p_ws->OnMsgRxComplete = (HTTPc_WEBSOCK_ON_RX_COMPLETE)p_param;
             break;

        case HTTPc_PARAM_TYPE_WEBSOCK_ON_ERR:
             p_ws->OnErr = (HTTPc_WEBSOCK_ON_ERR)p_param;
             break;

        case HTTPc_PARAM_TYPE_WEBSOCK_ON_PONG:
             p_ws->OnPong = (HTTPc_WEBSOCK_ON_PONG)p_param;
             break;

        default:
            *p_err = HTTPc_ERR_PARAM_INVALID;
             goto exit;
    }

   *p_err = HTTPc_ERR_NONE;
exit:
     return;
}
#endif


/*
*********************************************************************************************************
*                                      HTTPc_WebSockMsgSetParam()
*
* Description : Set a parameter related to a given WebSocket Msg Obj.
*
* Argument(s) : p_msg_obj   Pointer to the WebSocket Msg Obj.
*
*               type        Parameter type :
*
*                               HTTPc_PARAM_TYPE_WEBSOCK_MSG_USER_DATA
*                               HTTPc_PARAM_TYPE_WEBSOCK_MSG_ON_TX_INIT
*                               HTTPc_PARAM_TYPE_WEBSOCK_MSG_ON_TX_DATA
*                               HTTPc_PARAM_TYPE_WEBSOCK_MSG_ON_TX_COMPLETE
*
*               p_param     Pointer to parameter.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Parameter set successfully.
*                               HTTPc_ERR_INIT_NOT_COMPLETED    HTTPc Initialization is not completed.
*                               HTTPc_ERR_NULL_PTR              Null argument(s) passed.
*                               HTTPc_ERR_WEBSOCK_MSG_IS_USED   WebSocket Obj is already in use.
*                               HTTPc_ERR_PARAM_INVALID         Invalid request parameter.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef  HTTPc_WEBSOCK_MODULE_EN
void  HTTPc_WebSockMsgSetParam (HTTPc_WEBSOCK_MSG_OBJ  *p_msg_obj,
                                HTTPc_PARAM_TYPE        type,
                                void                   *p_param,
                                HTTPc_ERR              *p_err)
{
    HTTPc_WEBSOCK_MSG  *p_msg;
    CPU_BOOLEAN         in_use;
    CPU_SR_ALLOC();

                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    if (p_msg_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_param == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_msg = (HTTPc_WEBSOCK_MSG *)p_msg_obj;
                                                                /* ------ VALIDATE THAT REQ IS NOT USED ALREADY ------- */
    CPU_CRITICAL_ENTER();
    in_use = p_msg->Flags.IsUsed;
    CPU_CRITICAL_EXIT();
    if (in_use == DEF_YES) {
       *p_err = HTTPc_ERR_WEBSOCK_MSG_IS_USED;
        goto exit;
    }
                                                                /* ------------- WEBSOCK PARAMETER SETUP -------------- */
    switch (type) {

        case HTTPc_PARAM_TYPE_WEBSOCK_MSG_USER_DATA:
             p_msg->UserDataPtr = p_param;
             break;

        case HTTPc_PARAM_TYPE_WEBSOCK_MSG_ON_TX_INIT:
             p_msg->OnMsgTxInit = (HTTPc_WEBSOCK_ON_TX_INIT)p_param;
             break;

        case HTTPc_PARAM_TYPE_WEBSOCK_MSG_ON_TX_DATA:
             p_msg->OnMsgTxData = (HTTPc_WEBSOCK_ON_TX_DATA)p_param;
             break;

        case HTTPc_PARAM_TYPE_WEBSOCK_MSG_ON_TX_COMPLETE:
             p_msg->OnMsgTxComplete = (HTTPc_WEBSOCK_ON_TX_COMPLETE)p_param;
             break;

        default:
            *p_err = HTTPc_ERR_PARAM_INVALID;
             goto exit;
    }

   *p_err = HTTPc_ERR_NONE;
exit:
     return;
}
#endif


/*
*********************************************************************************************************
*                                        HTTPcWebSock_Upgrade()
*
* Description : Upgrade a HTTP client connection to a WebSocket.
*
* Argument(s) : p_conn_obj          Pointer to valid HTTPc Connection on which request will occurred.
*
*               p_req_obj           Pointer to request to send.
*
*               p_resp_obj          Pointer to response object that will be filled with the received response.
*
*               p_ws_obj            Pointer to WebSocket object.
*
*               p_resource_path     Pointer to complete URI (or only resource path) of the request.
*
*               resource_path_len   Resource path length.
*
*               flags               Configuration flags :
*
*                                       HTTPc_FLAG_WEBSOCK_NO_BLOCK
*
*               p_err               Pointer to variable that will receive the return error code from this function :
*
*                                       HTTPc_ERR_NONE                              Request Sending successfully started.
*                                       HTTPc_ERR_WEBSOCK_CONN_ALREADY_WEBSOCKET    The connection is already a websocket.
*                                       HTTPc_ERR_INIT_NOT_COMPLETED                HTTPc Initialization is not completed.
*                                       HTTPc_ERR_NULL_PTR                          Null pointer(s) was passed as argument(s).
*                                       HTTPc_ERR_CONN_IS_RELEASED                  Connection is not active.
*                                       HTTPc_ERR_REQ_IS_USED                       Request is already in usage.
*                                       HTTPc_ERR_REQ_PARAM_METHOD_INVALID          Invalid HTTP Method.
*                                       HTTPc_ERR_REQ_PARAM_RESOURCE_PATH_INVALID   Resource path is null.
*                                       HTTPc_ERR_FEATURE_DIS                       A needed feature is disabled.
*                                       HTTPc_ERR_PARAM_INVALID                     Invalid request parameter.
*                                       HTTPc_ERR_MSG_Q_FULL                        Error Queue full.
*                                       HTTPc_ERR_MSG_Q_SIGNAL_FAULT                Signal Post faulted.
*                                       HTTPc_ERR_CONN_SIGNAL_CREATE                Signal creation faulted.
*                                       HTTPc_ERR_CONN_SIGNAL_TIMEOUT               Signal pending timed out.
*                                       HTTPc_ERR_CONN_SIGNAL_FAULT                 Error occurred during signal pending.
*                                       HTTPc_ERR_CONN_SIGNAL_DEL                   Signal deletion faulted.
*                                       HTTPc_ERR_WEBSOCK_REQ_MEM_ERR               WebSock Req Obj allocation failed.
*                                       HTTPc_ERR_WEBSOCK_SHA1_ENCODE_FAIL          SHA1 encoding failed.
*                                       HTTPc_ERR_WEBSOCK_BASE64_ENCODE_FAIL        Base64 encoding failed.
*.-
* Return(s)   : DEF_OK,   if the operation is successful.
*               DEF_FAIL, if the operation has failed.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Connection Object:
*
*                   (a) Since the WebSocket Upgrade handshake is based on a HTTP request, the connection object
*                       p_conn_obj MUST have already established a connection to the desired host server.
*
*               (2) Request and Response Object.
*
*                   (a) Any HTTP request/response features available can be used during the WebSocket Upgrade Request.
*
*               (3) WebSocket Upgrade related Header field
*
*                   (a) During a WebSocket Upgrade handshake, the following mandatory header fields are managed by the
*                       WebSocket module and MUST NOT be set by the Application:
*                           Connection
*                           Upgrade
*                           Sec-WebSocket-Key
*                           Sec-WebSocket-Accept
*                           Sec-WebSocket-Version
*
*                   (b) During a WebSocket Upgrade Handshake, the following optional header fields MAY be managed by
*                       the Application using the standard HTTPc Request/Response API:
*                           Sec-WebSocket-Protocol
*                           Sec-WebSocket-Extensions
*
*               (4) If the WebSocket Upgrade Handshake is successful, Status Code in the response obj should be '101'.
*                   This means that the Host server has switched to the WebSocket protocol. Otherwise, the response
*                   object will TYPICALLY described the reason of the failure.
*
*               (5) When the WebSocket Upgrade is completed, any HTTP request is no more allowed and it is not posssible
*                   to switch the connection protocol to HTTP. However, if a HTTP request is required to be sent by the
*                   Application, it SHOULD do one of the following procedure:
*
*                   (1) Close the current WebSocket Connection by sending a Close Frame and open again the HTTP
*                       connection.
*
*                   (2) Open a different HTTP connection with the Host Server.
*********************************************************************************************************
*/
#ifdef  HTTPc_WEBSOCK_MODULE_EN
CPU_BOOLEAN  HTTPc_WebSockUpgrade (HTTPc_CONN_OBJ        *p_conn_obj,
                                   HTTPc_REQ_OBJ         *p_req_obj,
                                   HTTPc_RESP_OBJ        *p_resp_obj,
                                   HTTPc_WEBSOCK_OBJ     *p_ws_obj,
                                   CPU_CHAR              *p_resource_path,
                                   CPU_INT16U             resource_path_len,
                                   HTTPc_FLAGS            flags,
                                   HTTPc_ERR             *p_err)
{
    HTTPc_REQ          *p_req;
    HTTPc_WEBSOCK      *p_ws;
    HTTPc_WEBSOCK_REQ  *p_ws_req;
    HTTPc_CONN         *p_conn;
    CPU_BOOLEAN         is_websocket;
    CPU_BOOLEAN         is_connected;
    CPU_BOOLEAN         no_block;
    CPU_BOOLEAN         result;
    CPU_SR_ALLOC();


    p_req    = (HTTPc_REQ     *)p_req_obj;
    p_ws     = (HTTPc_WEBSOCK *)p_ws_obj;
    p_conn   = (HTTPc_CONN    *)p_conn_obj;
                                                                /* -------------------- VALIDATION -------------------- */
                                                                /* Validate if the conn is eligible to upgrade.         */
                                                                /* Check if the conn is already upgraded to Websock.    */
    CPU_CRITICAL_ENTER();
    is_websocket = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_WEBSOCKET);
    CPU_CRITICAL_EXIT();
    if (is_websocket == DEF_YES) {
       *p_err  = HTTPc_ERR_WEBSOCK_CONN_ALREADY_WEBSOCKET;
        result = DEF_NO;
        goto exit;
    }
                                                                /* Check if the connn is still open.                    */
    CPU_CRITICAL_ENTER();
    is_connected = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_CONNECT);
    CPU_CRITICAL_EXIT();
    if (is_connected == DEF_NO) {
       *p_err  = HTTPc_ERR_WEBSOCK_CONN_IS_NOT_CONNECTED;
        result = DEF_NO;
        goto exit;
    }
                                                                /* If no blocking mode, validate callbacks.             */
    no_block = DEF_BIT_IS_SET(flags, HTTPc_FLAG_WEBSOCK_NO_BLOCK);
    if (no_block == DEF_YES) {
        if (p_ws->OnOpen == DEF_NULL) {
           *p_err  = HTTPc_ERR_REQ_PARAM_WEBSOCK_ON_OPEN_CALLBACK_INVALID;
            result = DEF_NO;
            goto exit;
        }
    }
                                                                /* -------------- REQUEST INTIALIZATION --------------- */
                                                                /* Set as a Upgrade Request to WebSocket.               */
    is_websocket = DEF_YES;
    HTTPc_ReqSetParam( p_req_obj,
                       HTTPc_PARAM_TYPE_REQ_UPGRADE_WEBSOCKET,
                      &is_websocket,
                       p_err);
    if (*p_err != HTTPc_ERR_NONE) {
         result = DEF_FAIL;
         goto exit;
    }
                                                               /* Initialize the WebSocket Request object.              */
    p_ws_req = HTTPcWebSock_InitReqObj(p_err);
    if (*p_err != HTTPc_ERR_NONE) {
         result = DEF_FAIL;
         goto exit;
    }

    p_ws_req->WebSockObjPtr = p_ws;
    p_req->WebSockPtr       = p_ws_req;
                                                                /* ---------------- SEND HTTP REQUEST ----------------- */
    result = HTTPc_ReqSend(p_conn_obj,
                           p_req_obj,
                           p_resp_obj,
                           HTTP_METHOD_GET,
                           p_resource_path,
                           resource_path_len,
                           flags,
                           p_err);
    if (*p_err != HTTPc_ERR_NONE) {
         result = DEF_FAIL;
    }

exit:
    return (result);
}
#endif

/*
*********************************************************************************************************
*                                          HTTPc_WebSockSend()
*
* Description : Send a WebSocket message.
*
* Argument(s) : p_conn_obj      Pointer to valid HTTPc Connection upgraded to WebSocket.
*
*               p_msg_obj       Pointer to a valid WebSocket message object.
*
*               msg_type        Type of message to send.
*
*               p_data          Pointer to the payload to send.
*
*               data_len        Length of the payload to send.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   HTTPc_ERR_NONE                              Request Sending successfully started.
*                                   HTTPc_ERR_INIT_NOT_COMPLETED                HTTPc Initialization is not completed.
*                                   HTTPc_ERR_NULL_PTR                          Null pointer(s) was passed as argument(s).
*                                   HTTPc_ERR_FEATURE_DIS                       A needed feature is disabled.
*                                   HTTPc_ERR_WEBSOCK_INVALID_CONN              The connection is not upgraded to websocket.
*                                   HTTPc_ERR_WEBSOCK_INVALID_TX_PARAM          Tx msg params is invalid.
*                                   HTTPc_ERR_WEBSOCK_MSG_IS_USED               The WebSock msg object is currently used.
*                                   HTTPc_ERR_MSG_Q_FULL                        Error Queue full.
*                                   HTTPc_ERR_MSG_Q_SIGNAL_FAULT                Signal Post faulted.
*                                   HTTPc_ERR_CONN_SIGNAL_CREATE                Signal creation faulted.
*                                   HTTPc_ERR_CONN_SIGNAL_TIMEOUT               Signal pending timed out.
*                                   HTTPc_ERR_CONN_SIGNAL_FAULT                 Error occurred during signal pending.
*                                   HTTPc_ERR_CONN_SIGNAL_DEL                   Signal deletion faulted.
*
* Return(s)   : DEF_OK,     the operation is successful.
*               DEF_FAIL,   the operation has failed.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) The connection object p_conn MUST has been previouslyupgraded to WebSocket to use this
*                   function. Refer to the HTTPc_WebSockUpgrade() function.
*
*               (2) The available Message Types are :
*
*                       HTTPc_WEBSOCK_MSG_TYPE_TXT_FRAME (Data msg)
*                       HTTPc_WEBSOCK_MSG_TYPE_BIN_FRAME (Data msg)
*                       HTTPc_WEBSOCK_MSG_TYPE_CLOSE     (Ctrl msg)
*                       HTTPc_WEBSOCK_MSG_TYPE_PING      (Ctrl msg)
*                       HTTPc_WEBSOCK_MSG_TYPE_PONG      (Ctrl msg)
*
*               (3) Payload Content:
*
*                   (a) The "Payload data" (argument p_data) is defined as "Extension data" concatenated
*                       with "Application data".
*
*                   (b) Extension data length is 0 Bytes unless an extension has been negotiated during
*                       the handshake.
*
*                   (c) "Extension data" content and length are defines by the extension negotiated.
*
*                   (d) If negotiated, the "Extension data" must be handled by the Application.
*
*               (4) Data message Restrictions:
*
*                   (b) Even if the RFC6455 allow to sent message payload up to 2^64 bytes, Only 2^32 bytes is
*                       allowed in the current implementation.
*
*               (5) Control message Restrictions:
*
*                   (a) According to the RFC 6455 section 5.5:
*
*                          "All control frames MUST have a payload length of 125 bytes or less
*                           and MUST NOT be fragmented."
*
*                   (b) Close frame have a specific payload format. Refer to the HTTPc_WebSockFmtCloseMsg()
*                       function for more information.
*
*               (6) Client-to-Server Masking
*
*                   (a) According to the RFC 6455 section 5.2:
*
*                       "All frames sent from the client to the server are masked by a 32-bit value that is
*                        contained within the frame."
*
*                   (b) The Application DO NOT need to mask the payload since it's handled by the WebSocket
*                       module.
*********************************************************************************************************
*/
#ifdef  HTTPc_WEBSOCK_MODULE_EN
CPU_BOOLEAN  HTTPc_WebSockSend (HTTPc_CONN_OBJ          *p_conn_obj,
                                HTTPc_WEBSOCK_MSG_OBJ   *p_msg_obj,
                                HTTPc_WEBSOCK_MSG_TYPE   msg_type,
                                CPU_CHAR                *p_data,
                                CPU_INT32U               payload_len,
                                HTTPc_FLAGS              flags,
                                HTTPc_ERR               *p_err)
{
    CPU_BOOLEAN         result;
    CPU_BOOLEAN         is_websocket;
    CPU_BOOLEAN         no_block;
    HTTPc_CONN         *p_conn;
    HTTPc_WEBSOCK_MSG  *p_msg;
    CPU_BOOLEAN         in_use;
    HTTPc_ERR           err;
    CPU_SR_ALLOC();


    result = DEF_NO;
                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_FAIL);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    if (p_conn_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }

    if (p_msg_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_conn = (HTTPc_CONN        *)p_conn_obj;
    p_msg  = (HTTPc_WEBSOCK_MSG *)p_msg_obj;
                                                                /* Validate the connection state.                       */
    CPU_CRITICAL_ENTER();
    is_websocket = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_WEBSOCKET);
    CPU_CRITICAL_EXIT();
    if (is_websocket == DEF_NO) {
       *p_err = HTTPc_ERR_WEBSOCK_INVALID_CONN;
        result = DEF_NO;
        goto exit;
    }
                                                                /* Check if the Websock msg obj is already used.        */
    CPU_CRITICAL_ENTER();
    in_use = p_msg->Flags.IsUsed;
    CPU_CRITICAL_EXIT();
    if (in_use == DEF_YES) {
       *p_err = HTTPc_ERR_WEBSOCK_MSG_IS_USED;
        goto exit;
    }
                                                                /* Validate callback if no blocking mode.               */
    no_block = DEF_BIT_IS_SET(flags, HTTPc_FLAG_WEBSOCK_NO_BLOCK);
    if (no_block == DEF_YES) {
        if (p_msg->OnMsgTxComplete == DEF_NULL) {
           *p_err  = HTTPc_ERR_WEBSOCK_PARAM_ON_TX_COMPLETE_CALLBACK_INVALID;
            result = DEF_NO;
            goto exit;
        }
    }
                                                                /* Check if the message will be set on callback.        */
    if (p_data == DEF_NULL) {
        if (p_msg->OnMsgTxData == DEF_NULL) {
           *p_err  = HTTPc_ERR_WEBSOCK_PARAM_ON_TX_CALLBACK_INVALID;
            result = DEF_NO;
            goto exit;
        }
    }
                                                                /* Validate the opcode.                                 */
    switch (msg_type) {
        case HTTPc_WEBSOCK_MSG_TYPE_TXT_FRAME:
        case HTTPc_WEBSOCK_MSG_TYPE_BIN_FRAME:
             break;

        case HTTPc_WEBSOCK_MSG_TYPE_CLOSE:
        case HTTPc_WEBSOCK_MSG_TYPE_PING:
        case HTTPc_WEBSOCK_MSG_TYPE_PONG:
                                                                /* Control msg can't be longer than 125 bytes.          */
             if (payload_len > HTTPc_WEBSOCK_MAX_CTRL_FRAME_LEN) {
                *p_err  = HTTPc_ERR_WEBSOCK_INVALID_TX_PARAM;
                 result = DEF_NO;
                 goto exit;
             }
             break;

        default:
            *p_err  = HTTPc_ERR_WEBSOCK_INVALID_TX_PARAM;
             result = DEF_NO;
             goto exit;
    }

                                                                /* --------------- SET WEBSOCK MESSAGE ---------------- */
    p_msg->DataPtr = p_data;
    p_msg->Len     = payload_len;
    p_msg->LenSent = 0u;
    p_msg->OpCode  = (HTTPc_WEBSOCK_MSG_TYPE) msg_type;
    p_msg->ConnPtr = p_conn;

    p_msg->Flags.IsFin       = DEF_YES;
    p_msg->Flags.IsMasked    = DEF_YES;
    p_msg->Flags.IsHdrSet    = DEF_NO;
    p_msg->Flags.IsCompleted = DEF_NO;

                                                                /* ---------------- SET BLOCKING MODE ----------------- */
    no_block = DEF_BIT_IS_SET(flags, HTTPc_FLAG_WEBSOCK_NO_BLOCK );
    if (no_block == DEF_NO) {
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
        HTTPcTask_TransDoneSignalCreate(p_conn, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             goto exit_conn_close;
        }
#else
       *p_err = HTTPc_ERR_FEATURE_DIS;
        goto exit;
#endif
    }
    HTTPcTask_MsgQueue(         HTTPc_MSG_TYPE_WEBSOCK_MSG,
                       (void *) p_msg,
                                p_err);
    if (*p_err != HTTPc_ERR_NONE) {
         goto exit;
    }

    HTTPcTask_Wake(p_conn, &err);
    if (*p_err != HTTPc_ERR_NONE) {
         goto exit;
    }

    if (no_block == DEF_NO) {
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
                                                    /* -------- WAIT FOR RESPONSE IN BLOCKING MODE -------- */
        HTTPcTask_TransDoneSignalWait(p_conn, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             goto exit_conn_close;
        }
        CPU_CRITICAL_ENTER();
        result = p_msg->Flags.IsCompleted;
        CPU_CRITICAL_EXIT();
        if (result == DEF_FAIL) {
            if (p_conn->ErrCode == HTTPc_ERR_NONE) {
               *p_err = HTTPc_ERR_WEBSOCK_SEND_ERROR;
            } else {
               *p_err = p_conn->ErrCode ;
            }
            goto exit;
        }
        HTTPcTask_TransDoneSignalDel(p_conn, p_err);
        if (*p_err != HTTPc_ERR_NONE) {
             goto exit_conn_close;
        }
#else
       *p_err = HTTPc_ERR_FEATURE_DIS;
        goto exit;
#endif
    }
    result = DEF_OK;
   *p_err  = HTTPc_ERR_NONE;

exit:
    return (result);

#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
exit_conn_close:
    HTTPc_ConnCloseHandler(p_conn, HTTPc_FLAG_NONE, p_err);
    return (result);
#endif
}
#endif


/*
*********************************************************************************************************
*                                          HTTPc_WebSockClr()
*
* Description : Clear an HTTPc WebSock object before the first usage.
*
* Argument(s) : p_ws_obj    Pointer to the current HTTPc Websock Object.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  WebSock object clear successfully.
*                               HTTPc_ERR_INIT_NOT_COMPLETED    HTTPc Initialization is not completed.
*                               HTTPc_ERR_NULL_PTR              Function passed Null pointer as argument(s).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef  HTTPc_WEBSOCK_MODULE_EN
void  HTTPc_WebSockClr (HTTPc_WEBSOCK_OBJ  *p_ws_obj,
                        HTTPc_ERR          *p_err)
{
    HTTPc_WEBSOCK  *p_ws;

                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    if (p_ws_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_ws = (HTTPc_WEBSOCK *)p_ws_obj;

                                                                /* -------------- WEBSOCK INITIALIZATION -------------- */
    p_ws->CloseCode             = HTTPc_WEBSOCK_CLOSE_CODE_NONE;
    p_ws->CloseReason.DataPtr   = DEF_NULL;
    p_ws->CloseReason.Len       = 0u;
    p_ws->Flags.IsCloseStarted  = DEF_NO;
    p_ws->Flags.IsPongStarted   = DEF_NO;
    p_ws->Flags.IsRxDataCached  = DEF_NO;
    p_ws->Flags.IsTxMsgCtrlUsed = DEF_NO;
    p_ws->Flags.IsWebsockUsed   = DEF_NO;
    p_ws->OnClose               = DEF_NULL;
    p_ws->OnMsgRxInit           = DEF_NULL;
    p_ws->OnMsgRxData           = DEF_NULL;
    p_ws->OnMsgRxComplete       = DEF_NULL;
    p_ws->OnOpen                = DEF_NULL;
    p_ws->OnErr                 = DEF_NULL;
    p_ws->OnPong                = DEF_NULL;
    p_ws->RxMsgLen              = 0u;
    p_ws->RxMsgLenRead          = 0u;
    p_ws->RxMsgOpCode           = HTTPc_WEBSOCK_OPCODE_NONE;
    p_ws->RxState               = HTTPc_WEBSOCK_RX_STATE_WAIT;
    p_ws->TxMsgListEndPtr       = DEF_NULL;
    p_ws->TxMsgListHeadPtr      = DEF_NULL;
    p_ws->TxState               = HTTPc_WEBSOCK_TX_STATE_MSG_INIT;
    p_ws->TotalMsgLen           = 0u;
    p_ws->OrigOpCode            = HTTPc_WEBSOCK_OPCODE_NONE;

    Mem_Clr(&p_ws->TxMsgCtrl, sizeof(HTTPc_WEBSOCK_MSG));

   *p_err = HTTPc_ERR_NONE;

exit:
    return;
}
#endif


/*
*********************************************************************************************************
*                                         HTTPc_WebSockMsgClr()
*
* Description : Clear an HTTPc WebSock Msg object before the first usage.
*
* Argument(s) : p_msg_obj   Pointer to the WebSock Msg obj to clear.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  WebSock object clear successfully.
*                               HTTPc_ERR_INIT_NOT_COMPLETED    HTTPc Initialization is not completed.
*                               HTTPc_ERR_NULL_PTR              Function passed Null pointer as argument(s).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef  HTTPc_WEBSOCK_MODULE_EN
void  HTTPc_WebSockMsgClr (HTTPc_WEBSOCK_MSG_OBJ *p_msg_obj,
                           HTTPc_ERR             *p_err)
{
    HTTPc_WEBSOCK_MSG   *p_msg;

                                                                /* --------------- ARGUMENTS VALIDATION --------------- */
#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    if (p_msg_obj == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_msg = (HTTPc_WEBSOCK_MSG *)p_msg_obj;
                                                                /* ------------ WEBSOCK REQ INITIALIZATION ------------ */
    p_msg->ConnPtr            = DEF_NULL;
    p_msg->DataPtr            = DEF_NULL;
    p_msg->Len                = 0u;
    p_msg->Flags.IsFin        = DEF_NO;
    p_msg->Flags.IsMasked     = DEF_NO;
    p_msg->Flags.IsUsed       = DEF_NO;
    p_msg->Flags.IsNoBlock    = DEF_NO;
    p_msg->Flags.IsHdrSet     = DEF_NO;
    p_msg->LenSent            = 0u;
    p_msg->MskKey             = 0u;
    p_msg->NextPtr            = DEF_NULL;
    p_msg->OpCode             = HTTPc_WEBSOCK_OPCODE_NONE;
    p_msg->UserDataPtr        = DEF_NULL;
    p_msg->DataLen            = 0u;
    p_msg->OnMsgTxInit        = DEF_NULL;
    p_msg->OnMsgTxData        = DEF_NULL;
    p_msg->OnMsgTxComplete    = DEF_NULL;

   *p_err = HTTPc_ERR_NONE;

exit:
    return;
}
#endif


/*
*********************************************************************************************************
*                                      HTTPc_WebSockFmtCloseMsg()
*
* Description : Format a Close Frame.
*
* Argument(s) : close_code  Value that defines the origin of connection closure.
*
*               p_reason    Pointer to a string that contains a reason of the connection closure.
*
*               p_buf       Pointer to the destination buffer
*
*               buf_len     Length of the destination buffer'
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                            WebSock object clear successfully.
*                               HTTPc_ERR_NULL_PTR                        Function passed Null pointer as argument(s).
*                               HTTPc_ERR_WEBSOCK_INVALID_CLOSE_CODE      Close code argument is an unknown value.
*                               HTTPc_ERR_WEBSOCK_INVALID_CTRL_FRAME_LEN  Total frame is passed are invalid.
*
* Return(s)   : Payload length, If operation is successful.
*
*               0u,             If operation has failed.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Close Frame have a specific format. According to the RFC 6455 section :
*
*                      "If there is a body, the first two bytes of the body MUST be a 2-byte unsigned
*                       integer (in network byte order) representing a status code with value /code/
*                       defined in Section 7.4. Following the 2-byte integer,the body MAY contain
*                       UTF-8-encoded data with value /reason/, the interpretation of which is not
*                       defined by this specification."
*
*               (2) Close Codes available:
*
*                      HTTPc_WEBSOCK_CLOSE_CODE_NORMAL
*                      HTTPc_WEBSOCK_CLOSE_CODE_GOING_AWAY
*                      HTTPc_WEBSOCK_CLOSE_CODE_PROTOCOL_ERR
*                      HTTPc_WEBSOCK_CLOSE_CODE_DATA_TYPE_NOT_ALLOWED
*                      HTTPc_WEBSOCK_CLOSE_CODE_DATA_TYPE_ERR
*                      HTTPc_WEBSOCK_CLOSE_CODE_POLICY_VIOLATION
*                      HTTPc_WEBSOCK_CLOSE_CODE_MSG_TOO_BIG
*                      HTTPc_WEBSOCK_CLOSE_CODE_INVALID_EXT
*                      HTTPc_WEBSOCK_CLOSE_CODE_UNEXPECTED_CONDITION
*
*               (3) Close Reason:
*
*                   (a) Except for its length, the reason string has no restriction and it is user-definable.
*                       This should be used for debug purpose.
*
*                   (b) This field can be empty.
*********************************************************************************************************
*/
#if (HTTPc_CFG_WEBSOCKET_EN == DEF_ENABLED)
CPU_INT16U  HTTPc_WebSockFmtCloseMsg (HTTPc_WEBSOCK_CLOSE_CODE    close_code,
                                      CPU_CHAR                   *p_reason,
                                      CPU_CHAR                   *p_buf,
                                      CPU_INT16U                  buf_len,
                                      HTTPc_ERR                  *p_err)
{
    CPU_INT16U  str_len;
    CPU_INT16U  msg_len;


    msg_len = 0u;

#if (HTTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(0);
    }

    if (HTTPc_InitDone != DEF_YES) {
       *p_err = HTTPc_ERR_INIT_NOT_COMPLETED;
        goto exit;
    }

    switch (close_code) {

        case HTTPc_WEBSOCK_CLOSE_CODE_NORMAL:
        case HTTPc_WEBSOCK_CLOSE_CODE_GOING_AWAY:
        case HTTPc_WEBSOCK_CLOSE_CODE_PROTOCOL_ERR:
        case HTTPc_WEBSOCK_CLOSE_CODE_DATA_TYPE_NOT_ALLOWED:
        case HTTPc_WEBSOCK_CLOSE_CODE_DATA_TYPE_ERR:
        case HTTPc_WEBSOCK_CLOSE_CODE_POLICY_VIOLATION:
        case HTTPc_WEBSOCK_CLOSE_CODE_MSG_TOO_BIG:
        case HTTPc_WEBSOCK_CLOSE_CODE_INVALID_EXT:
        case HTTPc_WEBSOCK_CLOSE_CODE_UNEXPECTED_CONDITION:
             break;

        default:
            *p_err = HTTPc_ERR_WEBSOCK_INVALID_CLOSE_CODE;
             goto exit;
    }
    if (p_buf == DEF_NULL) {
       *p_err = HTTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    str_len = Str_Len(p_reason);
    if (str_len > HTTPc_WEBSOCK_MAX_CLOSE_REASON_LEN) {
        *p_err = HTTPc_ERR_WEBSOCK_INVALID_CTRL_FRAME_LEN;
         goto exit;
    }

    close_code = MEM_VAL_GET_INT16U_BIG(&close_code);
    Mem_Copy(p_buf,(CPU_CHAR *)&close_code, sizeof(CPU_INT16U));

    p_buf   += sizeof(CPU_INT16U);
    buf_len -= HTTPc_WEBSOCK_CLOSE_CODE_LEN;

    if (buf_len < str_len) {
       *p_err = HTTPc_ERR_WEBSOCK_INVALID_CTRL_FRAME_LEN;
        goto exit;
    }

    Str_Copy_N(p_buf, p_reason, str_len);
    msg_len = str_len + sizeof(CPU_INT16U);

exit:
    return (msg_len);
}
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       HTTPc_ConnCloseHandler()
*
* Description : Close an HTTPc Connection.
*
* Argument(s) : p_conn  Pointer to Connection to close.
*
*               flags   Configuration flags :
*
*                               HTTPc_FLAG_CONN_NO_BLOCK
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           HTTPc_ERR_NONE                  Connection closing successfully started.
*                           HTTPc_ERR_CONN_IS_RELEASED      Connection is not used.
*                           HTTPc_ERR_FEATURE_DIS           A needed feature is disabled.
*
*                           ------------ RETURNED BY HTTPcTask_MsgQueue() ------------
*                           See HTTPcTask_MsgQueue() for additional return error codes.
*
*                           ------------ RETURNED BY HTTPcTask_ConnCloseSignalCreate() ------------
*                           See HTTPcTask_ConnCloseSignalCreate() for additional return error codes.
*
*                           ------------ RETURNED BY HTTPcTask_ConnCloseSignalWait() ------------
*                           See HTTPcTask_ConnCloseSignalWait() for additional return error codes.
*
*                           ------------ RETURNED BY HTTPcTask_ConnCloseSignalDel() ------------
*                           See HTTPcTask_ConnCloseSignalDel() for additional return error codes.
*
*                           ------------ RETURNED BY HTTPcConn_Close() ------------
*                           See HTTPcConn_Close() for additional return error codes.
* Return(s)   : None.
*
* Caller(s)   : HTTPc_ConnClose(),
*               HTTPc_ConnOpen(),
*               HTTPc_ReqSend().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if ((HTTPc_CFG_PERSISTENT_EN == DEF_ENABLED) || \
      defined(HTTPc_SIGNAL_TASK_MODULE_EN)       )
static  void  HTTPc_ConnCloseHandler (HTTPc_CONN   *p_conn,
                                      HTTPc_FLAGS   flags,
                                      HTTPc_ERR    *p_err)
{
    CPU_BOOLEAN      in_use;
    CPU_BOOLEAN      no_block;
#ifdef HTTPc_TASK_MODULE_EN
    HTTPc_ERR        err;
#endif
    CPU_SR_ALLOC();


                                                                /* ------------ VALIDATE THAT CONN IS USED ------------ */
    CPU_CRITICAL_ENTER();
    in_use = DEF_BIT_IS_SET(p_conn->Flags, HTTPc_FLAG_CONN_IN_USE);
    CPU_CRITICAL_EXIT();
    if (in_use == DEF_NO) {
       *p_err = HTTPc_ERR_CONN_IS_RELEASED;
        goto exit;
    }

                                                                /* ---------------- SET BLOCKING MODE ----------------- */
    no_block = DEF_BIT_IS_SET(flags, HTTPc_FLAG_CONN_NO_BLOCK);
    if (no_block == DEF_YES) {
        DEF_BIT_SET(p_conn->Flags, HTTPc_FLAG_CONN_NO_BLOCK);
    } else {
        DEF_BIT_CLR(p_conn->Flags, HTTPc_FLAG_CONN_NO_BLOCK);
    }

#ifdef HTTPc_TASK_MODULE_EN
    if (no_block == DEF_NO) {
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN

        HTTPcTask_ConnCloseSignalCreate(p_conn, &err);
        if(err != HTTPc_ERR_NONE) {
            CPU_SW_EXCEPTION(;);
        }
#else
       *p_err = HTTPc_ERR_FEATURE_DIS;
        goto exit;
#endif
    }

    HTTPcTask_MsgQueue(        HTTPc_MSG_TYPE_CONN_CLOSE,
                       (void*) p_conn,
                               p_err);
    if (*p_err != HTTPc_ERR_NONE) {
        goto exit;
    }

    HTTPcTask_Wake(p_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        goto exit;
    }

    if (no_block == DEF_NO) {
#ifdef HTTPc_SIGNAL_TASK_MODULE_EN
        HTTPcTask_ConnCloseSignalWait(p_conn, &err);
        if(err != HTTPc_ERR_NONE) {
            CPU_SW_EXCEPTION(;);
        }

        HTTPcTask_ConnCloseSignalDel(p_conn, &err);
        if(err != HTTPc_ERR_NONE) {
            CPU_SW_EXCEPTION(;);
        }

#else
       *p_err  = HTTPc_ERR_FEATURE_DIS;
        goto exit;
#endif
    }

#else

    HTTPcConn_Close(p_conn, p_err);
    if (*p_err != HTTPc_ERR_NONE) {
         goto exit;
    }

    HTTPcConn_Remove(p_conn);

#endif
   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}
#endif


