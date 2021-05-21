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
*                                             HTTP SERVER
*
* Filename : http-s.c
* Version  : V3.01.01
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
#define    HTTPs_MODULE
#include  "http-s.h"
#include  "http-s_mem.h"
#include  "http-s_task.h"
#include  "http-s_str.h"
#include  "http-s_sock.h"
#include  "http-s_req.h"
#include  "http-s_resp.h"
#include  <Source/net_cfg_net.h>

#ifdef  NET_IPv4_MODULE_EN
#include  <IP/IPv4/net_ipv4.h>
#endif
#ifdef  NET_IPv6_MODULE_EN
#include  <IP/IPv6/net_ipv6.h>
#endif

#include  <Source/net_app.h>
#include  <Source/net_bsd.h>
#include  <Source/net_tcp.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_InitActive = DEF_ACTIVE;

const  HTTP_DICT  HTTPs_DictionaryTokenInternal[] = {
    { HTTPs_TOKEN_INTERNAL_STATUS_CODE,   HTTPs_STR_TOKEN_INTERNAL_STATUS_CODE,   (sizeof(HTTPs_STR_TOKEN_INTERNAL_STATUS_CODE)   - 1) },
    { HTTPs_TOKEN_INTERNAL_REASON_PHRASE, HTTPs_STR_TOKEN_INTERNAL_REASON_PHRASE, (sizeof(HTTPs_STR_TOKEN_INTERNAL_REASON_PHRASE) - 1) },
};

CPU_SIZE_T  HTTPs_DictionarySizeTokenInternal = sizeof(HTTPs_DictionaryTokenInternal);


/*
*********************************************************************************************************
*                                             HTTPs_Init()
*
* Description : (1) Initialize HTTP server suite:
*
*                   (a) Validate error file length.
*                   (b) Initialize instance pool
*
* Argument(s) : p_mem_seg   Pointer to the memory segment to use to allocate necessary objects.
*                           Set to DEF_NULL to allocate objects on the HEAP configured in uC/LIB.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPs_ERR_NONE
*                               HTTPs_ERR_CFG_INVALID_FILE_LEN
*                               HTTPs_ERR_INIT
*
*                               ------------ RETURNED BY HTTPsMem_InstanceInit() -------------
*                               See HTTPsMem_InstanceInit() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : Your Product's Application.
*
*               This function is a HTTPs server initialization function & MUST be called by
*               application/initialization function(s).
*
* Note(s)     : (1) 'HTTPs_InitActive' MUST be accessed exclusively in critical sections during initialization.
*********************************************************************************************************
*/

void  HTTPs_Init (MEM_SEG    *p_mem_seg,
                  HTTPs_ERR  *p_err)
{
    CPU_SR_ALLOC();


#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
                                                                /* -------------- VALIDATE ERR PAGE LEN --------------- */
    if (HTTPs_HTML_DFLT_ERR_LEN == 0) {
       *p_err = HTTPs_ERR_CFG_INVALID_ERR_PAGE_LEN;
        return;
    }
#endif


                                                                /* ------------- INIT HTTPs INSTANCE POOL ------------- */
    CPU_CRITICAL_ENTER();                                       /* See Note #1.                                         */
    if (HTTPs_InitActive == DEF_INACTIVE) {
        CPU_CRITICAL_EXIT();
       *p_err = HTTPs_ERR_INIT;
        return;
    }
    CPU_CRITICAL_EXIT();

    HTTPsMem_InstanceInit(p_mem_seg, p_err);
    if (*p_err != HTTPs_ERR_NONE) {
         return;
    }

    CPU_CRITICAL_ENTER();                                       /* See Note #1.                                         */
    HTTPs_InitActive = DEF_INACTIVE;                            /* Block http-s fncts/tasks until init complete.        */
    CPU_CRITICAL_EXIT();

   *p_err = HTTPs_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         HTTPs_InstanceInit()
*
* Description : (1) Initialize a HTTP server instance :
*
*                   (a) Validate return err pointer.
*                   (b) Validate pointers (configuration & file system api).
*                   (c) Validate HTTP server configuration.
*                   (d) Get HTTPs server instance.
*                   (f) Initialize instance OS object.
*                   (g) Initialize connection pool.
*
*
* Argument(s) : p_cfg       Pointer to the instance configuration object.
*
*               p_task_cfg  Pointer to the instance task configuration object.
*
*               p_fs_api    Pointer to specific file system API.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPs_ERR_NONE                          HTTPs instance server successfully
*                                                                           initialized and started.
*
*                               HTTPs_ERR_INIT                          HTTP server suite not initialized.
*
*                               HTTPs_ERR_NULL_PTR                      Argument 'p_cfg'/'p_fs_api' passed a
*                                                                           NULL pointer.
*
*                               HTTPs_ERR_CFG_INVALID_NBR_CONN          Invalid number of connection.
*                               HTTPs_ERR_CFG_INVALID_DFLT_FILE         Default file is a null pointer.
*                               HTTPs_ERR_CFG_INVALID_SECURE_EN         Invalid secure configuration.
*                               HTTPs_ERR_CFG_INVALID_HOST_LEN          Invalid host length.
*                               HTTPs_ERR_CFG_INVALID_FS_PATH_LEN       Invalid file system path length.
*                               HTTPs_ERR_CFG_INVALID_FS_SEP_CHAR       Invalid file system separator character.
*                               HTTPs_ERR_CFG_INVALID_TOKEN_EN          Invalid token configuration.
*                               HTTPs_ERR_CFG_INVALID_TOKEN_PARAM       Invalid token parameter.
*                               HTTPs_ERR_CFG_INVALID_QUERY_STR_EN      Invalid Query String configuration.
*                               HTTPs_ERR_CFG_INVALID_QUERY_STR_PARAM   Invalid Query String parameter.
*                               HTTPs_ERR_CFG_INVALID_FORM_EN           Invalid Form configuration.
*                               HTTPs_ERR_CFG_INVALID_FORM_PARAM        Invalid Form parameter.
*                               HTTPs_ERR_CFG_INVALID_HDR_EN            Invalid header configuration.
*                               HTTPs_ERR_CFG_INVALID_HDR_PARAM         Invalid header parameter.
*                               HTTPs_ERR_CFG_INVALID_BUF_LEN           Invalid connection buffer length.
*                               HTTPs_ERR_CFG_INVALID_FILE_LEN          Invalid static error file configuration.
*
*                               ---------------------- RETURNED BY HTTPsMem_InstanceGet() ------------------------
*                               See HTTPsMem_InstanceGet() for additional return error codes.
*
*                               ----------------------- RETURNED BY HTTPsTask_LockCreate() -----------------------
*                               See HTTPsTask_LockCreate() for additional return error codes.
*
*                               --------------------- RETURNED BY HTTPsTask_InstanceObjInit() --------------------
*                               See HTTPsTask_InstanceObjInit() for additional return error codes.
*
*                               ---------------------- RETURNED BY HTTPsMem_ConnPoolInit() -----------------------
*                               See HTTPsMem_ConnPoolInit() for additional return error codes.
*
* Return(s)   : Pointer to the instance handler, if NO error(s).
*
*               NULL pointer,                    otherwise.
*
* Caller(s)   : Application.
*
*               This function is a HTTPs server application programming interface (API) function
*               & MAY be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

HTTPs_INSTANCE  *HTTPs_InstanceInit (const  HTTPs_CFG      *p_cfg,
                                     const  NET_TASK_CFG   *p_task_cfg,
                                            HTTPs_ERR      *p_err)
{
#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED)
    const  NET_FS_API      *p_fs_api;
#endif
           HTTPs_INSTANCE  *p_instance;
           CPU_BOOLEAN      init_active;
           CPU_BOOLEAN      hook_def;
           CPU_BOOLEAN      result;
           KAL_ERR          kal_err;
#if ((HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED) && \
     (HTTPs_CFG_FS_PRESENT_EN  == DEF_ENABLED))
           CPU_INT32U       path_len_max;
           CPU_CHAR         path_sep_char;
#endif
    CPU_SR_ALLOC();


                                                                /* --------------- VALIDATE RTN ERR PTR --------------- */
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    CPU_CRITICAL_ENTER();
    init_active = HTTPs_InitActive;
    CPU_CRITICAL_EXIT();


    if (init_active == DEF_INACTIVE) {
#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
                                                                /* ------------------- VALIDATE PTR ------------------- */
        if (p_cfg == DEF_NULL) {                                /* Validate cfg ptr.                                    */
           *p_err = HTTPs_ERR_NULL_PTR;
            return (DEF_NULL);
        }

        if (p_task_cfg == DEF_NULL) {                           /* Validate task cfg ptr.                               */
           *p_err = HTTPs_ERR_NULL_PTR;
            return (DEF_NULL);
        }

        switch (p_cfg->SockSel) {
            case HTTPs_SOCK_SEL_IPv4:
#ifndef   NET_IPv4_MODULE_EN
                *p_err = HTTPs_ERR_CFG_INVALID_SOCK_SEL;
                 return (DEF_NULL);
#else
                 break;
#endif  /* NET_IPv4_MODULE_EN */


            case HTTPs_SOCK_SEL_IPv6:
#ifndef   NET_IPv6_MODULE_EN
                *p_err = HTTPs_ERR_CFG_INVALID_SOCK_SEL;
                 return (DEF_NULL);
#else
                 break;
#endif  /* NET_IPv6_MODULE_EN */


            case HTTPs_SOCK_SEL_IPv4_IPv6:
#ifndef   NET_IPv4_MODULE_EN
                *p_err = HTTPs_ERR_CFG_INVALID_SOCK_SEL;
                 return (DEF_NULL);
#endif  /* NET_IPv4_MODULE_EN */

#ifndef   NET_IPv6_MODULE_EN
                *p_err = HTTPs_ERR_CFG_INVALID_SOCK_SEL;
                 return (DEF_NULL);
#else
                 break;
#endif  /* NET_IPv6_MODULE_EN */


            default:
                *p_err = HTTPs_ERR_CFG_INVALID_SOCK_SEL;
                 return (DEF_NULL);
        }

                                                                /* ------------------- VALIDATE CFG ------------------- */

        if (p_cfg->SecurePtr != DEF_NULL) {                     /* Validate secure cfg.                                 */
#ifndef  NET_SECURE_MODULE_EN
           *p_err = HTTPs_ERR_CFG_INVALID_SECURE_EN;
            return (DEF_NULL);
#else
            if (p_cfg->SecurePtr->CertPtr == DEF_NULL) {
                *p_err = HTTPs_ERR_CFG_INVALID_SECURE_CERT_INVALID;
                 return (DEF_NULL);
            }

            if (p_cfg->SecurePtr->CertLen == 0u) {
                *p_err = HTTPs_ERR_CFG_INVALID_SECURE_CERT_INVALID;
                 return (DEF_NULL);
            }

            if (p_cfg->SecurePtr->KeyPtr == DEF_NULL) {
                *p_err = HTTPs_ERR_CFG_INVALID_SECURE_KEY_INVALID;
                 return (DEF_NULL);
            }

            if (p_cfg->SecurePtr->KeyLen == 0u) {
                *p_err = HTTPs_ERR_CFG_INVALID_SECURE_KEY_INVALID;
                 return (DEF_NULL);
            }

            switch (p_cfg->SecurePtr->Fmt) {
                case NET_SOCK_SECURE_CERT_KEY_FMT_PEM:
                case NET_SOCK_SECURE_CERT_KEY_FMT_DER:
                     break;


                case NET_SOCK_SECURE_CERT_KEY_FMT_NONE:
                default:
                    *p_err = HTTPs_ERR_CFG_INVALID_SECURE_CERT_INVALID;
                     return (DEF_NULL);
            }
#endif  /* NET_SECURE_MODULE_EN */
        }

        if (p_cfg->ConnNbrMax <= 0u) {                          /* Validate nbr conn.                                   */
           *p_err = HTTPs_ERR_CFG_INVALID_NBR_CONN;
            return (DEF_NULL);
        }

        if (p_cfg->BufLen < HTTPs_BUF_LEN_MIN) {                /* Validate buf len.                                    */
           *p_err = HTTPs_ERR_CFG_INVALID_BUF_LEN;
            return (DEF_NULL);
        }

        if (p_cfg->FS_CfgPtr == DEF_NULL) {                     /* Validate FS configuration pointer.                   */
           *p_err = HTTPs_ERR_CFG_NULL_PTR_FS;
            return (DEF_NULL);
        }

        if (p_cfg->DfltResourceNamePtr == DEF_NULL) {
           *p_err = HTTPs_ERR_CFG_INVALID_DFLT_RESOURCE;
            return (DEF_NULL);
        }

        switch (p_cfg->FS_Type) {
            case HTTPs_FS_TYPE_NONE:
                                                                /* Validate path len max.                               */
                 if (((HTTPs_CFG_FS_NONE *)p_cfg->FS_CfgPtr)->PathLenMax <= 0) {
                    *p_err = HTTPs_ERR_CFG_INVALID_PATH_LEN;
                     return (DEF_NULL);
                 }
                 break;


            case HTTPs_FS_TYPE_STATIC:
#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED)
                 p_fs_api = ((HTTPs_CFG_FS_STATIC *)p_cfg->FS_CfgPtr)->FS_API_Ptr;
                 if (p_fs_api == DEF_NULL) {
                    *p_err = HTTPs_ERR_CFG_INVALID_FS_API;
                     return (DEF_NULL);
                 }

                 path_len_max = p_fs_api->CfgPathGetLenMax();
                 if (path_len_max <= 0) {                       /* Validate FS path len max.                            */
                    *p_err = HTTPs_ERR_CFG_INVALID_FS_PATH_LEN;
                     return (DEF_NULL);
                 }

                 path_sep_char = p_fs_api->CfgPathGetSepChar();
                 if (path_sep_char == ASCII_CHAR_NULL) {        /* Validate path sep char.                              */
                    *p_err = HTTPs_ERR_CFG_INVALID_FS_SEP_CHAR;
                     return (DEF_NULL);
                 }
#else
                *p_err = HTTPs_ERR_CFG_INVALID_FS_EN;
                 return (DEF_NULL);
#endif
                 break;

            case HTTPs_FS_TYPE_DYN:
#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED)
                 p_fs_api = ((HTTPs_CFG_FS_DYN *)p_cfg->FS_CfgPtr)->FS_API_Ptr;
                 if (p_fs_api == DEF_NULL) {
                    *p_err = HTTPs_ERR_CFG_INVALID_FS_API;
                     return (DEF_NULL);
                 }

                 path_len_max = p_fs_api->CfgPathGetLenMax();
                 if (path_len_max <= 0) {                       /* Validate FS path len max.                            */
                    *p_err = HTTPs_ERR_CFG_INVALID_FS_PATH_LEN;
                     return (DEF_NULL);
                 }

                 path_sep_char = p_fs_api->CfgPathGetSepChar();
                 if (path_sep_char == ASCII_CHAR_NULL) {        /* Validate path sep char.                              */
                    *p_err = HTTPs_ERR_CFG_INVALID_FS_SEP_CHAR;
                     return (DEF_NULL);
                 }
#else
                *p_err = HTTPs_ERR_CFG_INVALID_FS_EN;
                 return (DEF_NULL);
#endif
                 break;

            default:
                *p_err = HTTPs_ERR_CFG_INVALID_FS_TYPE;
                 return (DEF_NULL);
        }


#if (HTTPs_CFG_ABSOLUTE_URI_EN == DEF_ENABLED)
        if (p_cfg->HostNameLenMax <= 0) {                       /* Validate host len max.                               */
           *p_err = HTTPs_ERR_CFG_INVALID_HOST_LEN;
            return (DEF_NULL);
        }
#endif  /* HTTPs_CFG_ABSOLUTE_URI_EN */

                                                                /* Validate hdr field param.                            */
        if (p_cfg->HdrRxCfgPtr != DEF_NULL) {
#if (HTTPs_CFG_HDR_RX_EN == DEF_ENABLED)

            if (p_cfg->HdrRxCfgPtr->DataLenMax <= 0) {          /* Validate Req hdr field val len.                      */
               *p_err = HTTPs_ERR_CFG_INVALID_HDR_PARAM;
                return (DEF_NULL);
            }

            if (p_cfg->HooksPtr == DEF_NULL) {                  /* The Header Rx hook must be defined.                  */
               *p_err = HTTPs_ERR_CFG_INVALID_HDR_PARAM;
                return (DEF_NULL);
            } else {
                if (p_cfg->HooksPtr->OnReqHdrRxHook == DEF_NULL) {
                   *p_err = HTTPs_ERR_CFG_INVALID_HDR_PARAM;
                    return (DEF_NULL);
                }
            }

#else
           *p_err = HTTPs_ERR_CFG_INVALID_HDR_EN;               /* Validate hdr field en param.                         */
            return (DEF_NULL);
#endif  /* HTTPs_CFG_HDR_RX_EN */
        }

        if (p_cfg->HdrTxCfgPtr != DEF_NULL) {
#if (HTTPs_CFG_HDR_TX_EN == DEF_ENABLED)

            if (p_cfg->HdrTxCfgPtr->DataLenMax <= 0) {          /* Validate Resp hdr field val len.                     */
               *p_err = HTTPs_ERR_CFG_INVALID_HDR_PARAM;
                return (DEF_NULL);
            }

            if (p_cfg->HooksPtr == DEF_NULL) {                  /* The Header Tx hook must be defined.                  */
               *p_err = HTTPs_ERR_CFG_INVALID_HDR_PARAM;
                return (DEF_NULL);
            } else {
                if (p_cfg->HooksPtr->OnRespHdrTxHook == DEF_NULL) {
                   *p_err = HTTPs_ERR_CFG_INVALID_HDR_PARAM;
                    return (DEF_NULL);
                }
            }

#else
           *p_err = HTTPs_ERR_CFG_INVALID_HDR_EN;               /* Validate hdr field en param.                         */
            return (DEF_NULL);
#endif  /* HTTPs_CFG_HDR_TX_EN */
        }

        if (p_cfg->TokenCfgPtr != DEF_NULL) {                   /* Validate token param.                                */
#if (HTTPs_CFG_TOKEN_PARSE_EN == DEF_ENABLED)

            if (p_cfg->TokenCfgPtr->ValLenMax <= 0u) {          /* Validate token val len max.                          */
               *p_err = HTTPs_ERR_CFG_INVALID_TOKEN_PARAM;
                return (DEF_NULL);
            }

            if (p_cfg->HooksPtr == DEF_NULL) {                  /* Validate token hook function.                        */
               *p_err = HTTPs_ERR_CFG_INVALID_TOKEN_PARAM;
                return (DEF_NULL);
            } else {
                if (p_cfg->HooksPtr->OnRespTokenHook == DEF_NULL) {
                   *p_err = HTTPs_ERR_CFG_INVALID_TOKEN_PARAM;
                    return (DEF_NULL);
                }
            }
#else
           *p_err = HTTPs_ERR_CFG_INVALID_TOKEN_EN;             /* Validate token en param.                             */
            return (DEF_NULL);
#endif  /* HTTPs_CFG_TOKEN_PARSE_EN */
        }

        if (p_cfg->QueryStrCfgPtr != DEF_NULL) {
#if (HTTPs_CFG_QUERY_STR_EN == DEF_ENABLED)

            if (p_cfg->QueryStrCfgPtr->KeyLenMax <= 0u) {       /* Validate Key length max.                             */
               *p_err = HTTPs_ERR_CFG_INVALID_FORM_PARAM;
                return (DEF_NULL);
            }

            if (p_cfg->QueryStrCfgPtr->ValLenMax <= 0u) {       /* Validate Value length max.                           */
               *p_err = HTTPs_ERR_CFG_INVALID_FORM_PARAM;
                return (DEF_NULL);
            }
#else
            *p_err = HTTPs_ERR_CFG_INVALID_QUERY_STR_EN;        /* Validate Query String en param.                      */
             return (DEF_NULL);
#endif
        }

        if (p_cfg->FormCfgPtr != DEF_NULL) {                    /* Validate Form param.                                 */
#if (HTTPs_CFG_FORM_EN == DEF_ENABLED)

            if (p_cfg->FormCfgPtr->KeyLenMax <= 0u) {           /* Validate Key length max.                             */
               *p_err = HTTPs_ERR_CFG_INVALID_FORM_PARAM;
                return (DEF_NULL);
            }

            if (p_cfg->FormCfgPtr->ValLenMax <= 0u) {           /* Validate Value length max.                           */
               *p_err = HTTPs_ERR_CFG_INVALID_FORM_PARAM;
                return (DEF_NULL);
            }

                                                                /* Validate multipart param.                            */
            if ((p_cfg->FormCfgPtr->MultipartEn                  == DEF_ENABLED) &&
                (p_cfg->FormCfgPtr->MultipartFileUploadEn        == DEF_ENABLED) &&
                (p_cfg->FormCfgPtr->MultipartFileUploadFolderPtr == DEF_NULL)  ) {
                *p_err = HTTPs_ERR_CFG_INVALID_FORM_PARAM;
                 return (DEF_NULL);
            }

            if ((p_cfg->FormCfgPtr->MultipartFileUploadEn == DEF_ENABLED)        &&
                (p_cfg->FS_Type                           != HTTPs_FS_TYPE_DYN)) {
                *p_err = HTTPs_ERR_CFG_INVALID_FS_API;
                 return (DEF_NULL);
            }

            if (p_cfg->HooksPtr == DEF_NULL) {                  /* Validate Form hook function.                         */
               *p_err = HTTPs_ERR_CFG_INVALID_FORM_PARAM;
                return (DEF_NULL);
            } else {
                if (p_cfg->HooksPtr->OnReqRdySignalHook == DEF_NULL) {
                   *p_err = HTTPs_ERR_CFG_INVALID_FORM_PARAM;
                    return (DEF_NULL);
                }
            }
                                                                /* Validate buf len for Form.                           */
            if (p_cfg->BufLen < (p_cfg->FormCfgPtr->KeyLenMax + p_cfg->FormCfgPtr->ValLenMax)) {
               *p_err = HTTPs_ERR_CFG_INVALID_BUF_LEN;
                return (DEF_NULL);
            }
#else
           *p_err = HTTPs_ERR_CFG_INVALID_FORM_EN;              /* Validate Form en param.                              */
            return (DEF_NULL);
#endif  /* HTTPs_CFG_FORM_EN */
        }
#endif  /* HTTPs_CFG_ARG_CHK_EXT_EN */


                                                                /* ------------------- GET INSTANCE ------------------- */
        p_instance = HTTPsMem_InstanceGet(p_err);
        if ((*p_err      != HTTPs_ERR_NONE) ||
            ( p_instance == DEF_NULL)      ) {
            return (DEF_NULL);
        }

                                                                /* ---------- INITIALIZE INSTANCE PARAMETERS ---------- */
        p_instance->CfgPtr         = p_cfg;
        p_instance->TaskCfgPtr     = p_task_cfg;
        p_instance->Started        = DEF_NO;
        p_instance->SelAbortReq    = DEF_NO;


        switch (p_cfg->FS_Type) {
            case HTTPs_FS_TYPE_NONE:
                 break;

            case HTTPs_FS_TYPE_STATIC:
#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED)
                 p_fs_api = ((HTTPs_CFG_FS_STATIC *)p_cfg->FS_CfgPtr)->FS_API_Ptr;
                 p_instance->FS_PathLenMax  = p_fs_api->CfgPathGetLenMax();
                 p_instance->FS_PathSepChar = p_fs_api->CfgPathGetSepChar();
#endif
                 break;


            case HTTPs_FS_TYPE_DYN:
#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED)
                 p_fs_api = ((HTTPs_CFG_FS_DYN *)p_cfg->FS_CfgPtr)->FS_API_Ptr;
                 p_instance->FS_PathLenMax  = p_fs_api->CfgPathGetLenMax();
                 p_instance->FS_PathSepChar = p_fs_api->CfgPathGetSepChar();
#endif
                 break;

            default:
                *p_err = HTTPs_ERR_CFG_INVALID_FS_TYPE;
                 return (DEF_NULL);
        }


        p_instance->OS_LockObj = HTTPsTask_LockCreate(p_err);
        if (*p_err != HTTPs_ERR_NONE){
             p_instance->OS_TaskObjPtr = (HTTPs_OS_TASK_OBJ *)DEF_NULL;
             HTTPsMem_InstanceRelease(p_instance);
             return (DEF_NULL);
        }


        p_instance->ConnSelAbortLockObj = KAL_LockCreate ("HTTPs Conn Sel Abort Mtx",
                                                           DEF_NULL,
                                                          &kal_err);

        if (kal_err != RTOS_ERR_NONE) {
           *p_err = HTTPs_ERR_TASK_LOCK_CREATE;
            HTTPsMem_InstanceRelease(p_instance);
            return (DEF_NULL);
        }

                                                                /* ------------------- INIT OS OBJ -------------------- */
        HTTPsTask_InstanceObjInit(p_instance, p_err);
        if (*p_err != HTTPs_ERR_NONE) {
             HTTPsMem_InstanceRelease(p_instance);
             return (DEF_NULL);
        }


                                                                /* ------------------ INIT CONN POOL ------------------ */
        HTTPsMem_ConnPoolInit(p_instance,
                               p_err);
        if (*p_err != HTTPs_ERR_NONE) {
             HTTPsMem_InstanceRelease(p_instance);
             return (DEF_NULL);
        }

        hook_def = HTTPs_HOOK_DEFINED(p_cfg->HooksPtr, OnInstanceInitHook);
        if (hook_def == DEF_YES) {
                                                                /* If Instance conn objs init handler is not null ...   */
                                                                /* ... call Instance conn objs init handler.            */
            result = p_cfg->HooksPtr->OnInstanceInitHook(p_instance,
                                                         p_cfg->Hooks_CfgPtr);
            if (result != DEF_OK) {
                HTTPsMem_InstanceRelease(p_instance);
               *p_err = HTTPs_ERR_INIT_INSTANCE_HOOK_FAULT;
                return (DEF_NULL);
            }
        }


    } else {                                                    /* HTTPs_InitActive != DEF_INACTIVE                     */
       *p_err = HTTPs_ERR_INIT;
        return (DEF_NULL);
    }

    CPU_CRITICAL_ENTER();
    ++HTTPs_InstanceInitializedNbr;
    CPU_CRITICAL_EXIT();

   *p_err = HTTPs_ERR_NONE;

    return (p_instance);
}


/*
*********************************************************************************************************
*                                        HTTPs_InstanceStart()
*
* Description : (1) Start a specific HTTPs server instance which has been previously initialized:
*
*                   (a) Validate return error pointer.
*                   (b) Initialize instance listen socket.
*                   (c) Create and start HTTP server instance task.
*
* Argument(s) : p_instance  Pointer to specific HTTP server instance handler.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                           HTTPs_ERR_NONE      Instance HTTPs server is started.
*
*                           --------------- RETURNED BY HTTPsTask_LockAcquire() -----------------
*                           See HTTPsTask_LockAcquire() for additional return error codes.
*
*                           --------------- RETURNED BY HTTPsSock_ListenInit() ------------------
*                           See HTTPsSock_ListenInit() for additional return error codes.
*
*                           ----------- RETURNED BY HTTPsTask_InstanceTaskCreate() --------------
*                           See HTTPsTask_InstanceTaskCreate() for additional return error codes.
*
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
*               This function is a HTTPs server application programming interface (API) function
*               & MAY be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPs_InstanceStart (HTTPs_INSTANCE  *p_instance,
                           HTTPs_ERR       *p_err)
{
    const  HTTPs_CFG    *p_cfg;
#ifdef   NET_IPv4_MODULE_EN
           NET_SOCK_ID   sock_listen_ipv4;
#endif
#ifdef   NET_IPv6_MODULE_EN
           NET_SOCK_ID   sock_listen_ipv6;
#ifdef NET_IPv4_MODULE_EN
           NET_ERR       err;
#endif
#endif
           CPU_SR_ALLOC();


                                                                /* --------------- VALIDATE RTN ERR PTR --------------- */
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

                                                                /* Acquire Instance lock.                               */
    HTTPsTask_LockAcquire(p_instance->OS_LockObj, p_err);
    if (*p_err != HTTPs_ERR_NONE) {
         return;
    }

    if (p_instance->Started == DEF_YES) {
        HTTPsTask_LockRelease(p_instance->OS_LockObj);
       *p_err = HTTPs_ERR_NONE;
        return;
    }

    p_cfg                   = p_instance->CfgPtr;
    p_instance->SelAbortReq = DEF_NO;

                                                                /* ----------------- INIT LISTEN SOCK ----------------- */
    switch (p_cfg->SockSel) {
        case HTTPs_SOCK_SEL_IPv4:
#ifdef   NET_IPv4_MODULE_EN
             sock_listen_ipv4 = HTTPsSock_ListenInit(p_cfg, NET_SOCK_PROTOCOL_FAMILY_IP_V4, p_err);
             if (*p_err != HTTPs_ERR_NONE){
                  return;
             }
             p_instance->SockListenID_IPv4 = sock_listen_ipv4;
#else
            *p_err = HTTPs_ERR_CFG_INVALID_SOCK_SEL;
#endif
             break;



        case HTTPs_SOCK_SEL_IPv6:
#ifdef   NET_IPv6_MODULE_EN
             sock_listen_ipv6 = HTTPsSock_ListenInit(p_cfg, NET_SOCK_PROTOCOL_FAMILY_IP_V6, p_err);
             if (*p_err != HTTPs_ERR_NONE){
                  return;
             }

             p_instance->SockListenID_IPv6 = sock_listen_ipv6;
             break;
#else
            *p_err = HTTPs_ERR_CFG_INVALID_SOCK_SEL;
             return;
#endif


        case HTTPs_SOCK_SEL_IPv4_IPv6:
#ifndef  NET_IPv4_MODULE_EN
            *p_err = HTTPs_ERR_CFG_INVALID_SOCK_SEL;
             return;
#elif (!defined(NET_IPv6_MODULE_EN))
             *p_err = HTTPs_ERR_CFG_INVALID_SOCK_SEL;
              return;
#else
              sock_listen_ipv4 = HTTPsSock_ListenInit(p_cfg, NET_SOCK_PROTOCOL_FAMILY_IP_V4, p_err);
              if (*p_err != HTTPs_ERR_NONE){
                   return;
              }

              p_instance->SockListenID_IPv4 = sock_listen_ipv4;

              sock_listen_ipv6 = HTTPsSock_ListenInit(p_cfg, NET_SOCK_PROTOCOL_FAMILY_IP_V6, p_err);

              if (*p_err != HTTPs_ERR_NONE){
                  (void)NetSock_Close(sock_listen_ipv4, &err);  /* Close IPv4 sock.                                     */
                  return;
              }

              p_instance->SockListenID_IPv6 = sock_listen_ipv6;
              break;
#endif


        default:
            *p_err = HTTPs_ERR_CFG_INVALID_SOCK_SEL;
             return;
    }

                                                                /* -------------- CREATE & START OS TASK -------------- */
    HTTPsTask_InstanceTaskCreate(p_instance, p_err);            /* return err of sub-fcnts.                             */
    if (*p_err != HTTPs_ERR_NONE) {
         HTTPsTask_LockRelease(p_instance->OS_LockObj);         /* Release instance lock.                               */
         return;
    }

    CPU_CRITICAL_ENTER();
    ++HTTPs_InstanceRunningNbr;
    CPU_CRITICAL_EXIT();

    HTTPsTask_LockRelease(p_instance->OS_LockObj);              /* Release instance lock.                               */

   *p_err = HTTPs_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         HTTPs_InstanceStop()
*
* Description : (1) Stop a specific HTTPs server instance:
*
*                   (a) Validate return error pointer.
*                   (b) Search instance structure.
*                   (c) Reset any idle connections.
*                   (d) Signal instance to stop the task.
*                   (e) Wait for completion of the stopping routine.
*
* Argument(s) : p_instance        Pointer to Instance handler.
*
*               graceful_stop_en  Boolean that determines whether the server will wait for established idle connections to time out before
*                                 the instance can be stopped by this function.
*
*                                     DEF_TRUE     Allows for the HTTP connection(s) to close once the configured inactivity timeout expires.
*
*                                     DEF_FALSE    Forcibly close listen socket(s) and child socket(s) associated with the connected HTTP
*                                                  client(s) & free the connection resources.
*
*               p_err             Pointer to variable that will receive the return error code from this function :
*
*                                     HTTPs_ERR_NONE                    Instance HTTPs server is started.
*
*                                     HTTPs_ERR_TASK_STOP_REQ_SIGNAL    NetSock_SelAbort() could not interrupt blocking
*                                                                       function call to HTTPsSock_ConnSel() in
*                                                                       HTTPsTask_InstanceTaskHandler(). Caller should
*                                                                       try again.
*
*                                     --------------------- RETURNED BY HTTPsTask_LockAcquire() ---------------------
*                                     See HTTPsTask_LockAcquire() for additional return error codes.
*
*                                     ---------------- RETURNED BY HTTPsTask_InstanceStopReqSignal() ----------------
*                                     See HTTPsTask_InstanceStopReqSignal() for additional return error codes.
*
*                                     ----------- RETURNED BY HTTPsTask_InstanceStopCompletedPending() --------------
*                                     See HTTPsTask_InstanceStopCompletedPending() for additional return error codes.
*
*                                     ------------------------ RETURNED BY NetSock_SelAbort() -----------------------
*                                     See NetSock_SelAbort() for additional return error codes.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
*               This function is an application interface (API) function & MAY be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPs_InstanceStop (HTTPs_INSTANCE  *p_instance,
                          CPU_BOOLEAN      graceful_stop_en,
                          HTTPs_ERR       *p_err)
{
    HTTPs_SOCK_SEL   sock_sel;
    CPU_INT08U       conns;
    HTTPs_CONN      *p_https_conn;
    NET_SOCK_ID      sock_id;
    NET_ERR          net_err;
    NET_ERR          net_err_ipv6;

    CPU_SR_ALLOC();

                                                                /* --------------- VALIDATE RTN ERR PTR --------------- */
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }
                                                                /* Acquire Instance lock.                               */
    HTTPsTask_LockAcquire(p_instance->OS_LockObj, p_err);

    if (*p_err != HTTPs_ERR_NONE) {
        return;
    }

    if (p_instance->Started != DEF_YES) {
        HTTPsTask_LockRelease(p_instance->OS_LockObj);
       *p_err = HTTPs_ERR_NONE;
        return;
    }
    net_err      = NET_SOCK_ERR_NONE;
    net_err_ipv6 = NET_SOCK_ERR_NONE;


    sock_sel = p_instance->CfgPtr->SockSel;

    if (p_instance->ConnActiveCtr == DEF_NULL) {                /* Prevent HTTPsSock_ConnSel() from pending on sock ... */
                                                                /* ... operations indefinitely if client never connects.*/
                                                                /* Abort blocking call on instance's listening socket(s)*/
                                                                /* Acquire the conn sel abort lock.                     */
        HTTPsTask_LockAcquire(p_instance->ConnSelAbortLockObj, p_err);
        if (*p_err != HTTPs_ERR_NONE) {
            return;
        }
        switch (sock_sel) {
            case HTTPs_SOCK_SEL_IPv4:
                 NetSock_SelAbort(p_instance->SockListenID_IPv4, &net_err);
                 break;


#if defined(NET_IPv6_MODULE_EN)
            case HTTPs_SOCK_SEL_IPv6:
                 NetSock_SelAbort(p_instance->SockListenID_IPv6, &net_err_ipv6);
                 break;


            case HTTPs_SOCK_SEL_IPv4_IPv6:
                 NetSock_SelAbort(p_instance->SockListenID_IPv4, &net_err);
                 NetSock_SelAbort(p_instance->SockListenID_IPv6, &net_err_ipv6);
                 break;
#endif


            default:
                 break;
        }
                                                                /* Check error codes.                                   */
        if (net_err      != NET_SOCK_ERR_NONE ||
            net_err_ipv6 != NET_SOCK_ERR_NONE) {
           *p_err = HTTPs_ERR_TASK_STOP_REQ_SIGNAL;             /* Internal socket error or select() function not ready.*/
            HTTPsTask_LockRelease(p_instance->ConnSelAbortLockObj);
            return;
        }
        p_instance->SelAbortReq = DEF_TRUE;                     /* Request to abort HTTPsSock_ConnSel() was successful. */
    } else {
        p_https_conn = p_instance->ConnFirstPtr;
        sock_id      = p_https_conn->SockID;
        conns        = p_instance->ConnActiveCtr;

        if (graceful_stop_en != DEF_TRUE) {
#if (HTTPs_CFG_PERSISTENT_CONN_EN == DEF_ENABLED)
            DEF_BIT_CLR(p_instance->ConnFirstPtr->Flags, HTTPs_FLAG_CONN_PERSISTENT);
#endif
                                                                /* Close listening socket(s).                           */
            switch (sock_sel) {
                case HTTPs_SOCK_SEL_IPv4:
                     NetSock_Close(p_instance->SockListenID_IPv4, &net_err);
                     break;


#if defined(NET_IPv6_MODULE_EN)
                case HTTPs_SOCK_SEL_IPv6:
                     NetSock_Close(p_instance->SockListenID_IPv6, &net_err);
                     break;


                case HTTPs_SOCK_SEL_IPv4_IPv6:
                     NetSock_Close(p_instance->SockListenID_IPv4, &net_err);
                     NetSock_Close(p_instance->SockListenID_IPv6, &net_err);
                     break;
#endif


                default:
                     break;
            }

            while (conns > 0u) {
                NetSock_Close(sock_id, &net_err);               /* Close HTTP conn socket.                              */

                if (net_err == NET_SOCK_ERR_NONE) {             /* Release conn resources.                              */
                    HTTPsMem_ConnRelease(p_instance, p_https_conn);
                }

                if (conns) {
                    p_https_conn = p_https_conn->ConnNextPtr;
                    sock_id      = p_https_conn->SockID;
                }
                conns--;
            }
        }
    }
    HTTPsTask_LockRelease(p_instance->ConnSelAbortLockObj);     /* Release the conn sel abort lock.                     */
                                                                /* ------------- SIGNAL INSTANCE TO STOP -------------- */
    HTTPsTask_InstanceStopReqSignal(p_instance, p_err);
    if (*p_err != HTTPs_ERR_NONE) {
         HTTPsTask_LockRelease(p_instance->OS_LockObj);
         return;
    }

    HTTPsTask_LockRelease(p_instance->OS_LockObj);              /* Release Instance lock before pending.                */

                                                                /* --------------- WAIT STOP COMPLETED ---------------- */
    HTTPsTask_InstanceStopCompletedPending(p_instance, p_err);
    if (*p_err != HTTPs_ERR_NONE) {
         return;
    }

                                                                /* Re-acquire Instance lock.                            */
    HTTPsTask_LockAcquire(p_instance->OS_LockObj, p_err);
    if (*p_err != HTTPs_ERR_NONE) {
         return;
    }

                                                                /* ---------------- DEL INSTANCE TASK ----------------- */
    HTTPsTask_InstanceTaskDel(p_instance);

    CPU_CRITICAL_ENTER();
    --HTTPs_InstanceRunningNbr;
    CPU_CRITICAL_EXIT();

    HTTPsTask_LockRelease(p_instance->OS_LockObj);              /* Release Instance lock.                               */

   *p_err = HTTPs_ERR_NONE;
}


/*
*********************************************************************************************************
*                                          HTTPs_RespHdrGet()
*
* Description : Acquire a new response header block.
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*               ------      Argument validated in HTTPsSock_ConnAccept().
*
*               hdr_field   Type of the response header value :
*
*                               See enumeration HTTPs_HDR_FIELD.
*
*               val_type    Data type of the response header field value :
*
*                               HTTP_HDR_VAL_TYPE_NONE      Header field doesn't require value.
*                               HTTP_HDR_VAL_TYPE_STR       Header value type is a string.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPs_ERR_NONE                  If returned a valid block.
*
*                               ------------- RETURNED BY HTTPsMem_RespHdrGet() -------------
*                               See HTTPsMem_RespHdrGet() for additional return error codes.
*
* Return(s)   : Pointer to the response header block that can be filled with data, if no error(s).
*
*               Null pointer, otherwise
*
* Caller(s)   : Application. (Inside Hooks)
*
*               This function is a HTTPs server application programming interface (API) function
*               & MAY be called by application function(s).
*
* Note(s)     : (1) Must be called from a callback function only. Should be called by the function pointed by
*                   RespHdrTxFnctPtr in the instance configuration.
*
*                   (a) The instance lock must be acquired before calling this function. It's why this function
*                       must be called from a callback function.
*
*               (2) The header block is automatically added to the header blocks list. Thus the caller has not to
*                   add the block to the list. Only filling the value and value length should be required by the
*                   caller.
*********************************************************************************************************
*/
#if (HTTPs_CFG_HDR_TX_EN == DEF_ENABLED)
HTTP_HDR_BLK  *HTTPs_RespHdrGet (const HTTPs_INSTANCE      *p_instance,
                                       HTTPs_CONN          *p_conn,
                                       HTTP_HDR_FIELD       hdr_field,
                                       HTTP_HDR_VAL_TYPE    val_type,
                                       HTTPs_ERR           *p_err)
{
    HTTP_HDR_BLK  *p_blk;


    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_instance == DEF_NULL) {
       *p_err = HTTPs_ERR_NULL_PTR;
        p_blk = DEF_NULL;
        goto exit;
    }

    if (p_conn == DEF_NULL) {
       *p_err = HTTPs_ERR_NULL_PTR;
        p_blk = DEF_NULL;
        goto exit;
    }
#endif

    p_blk = HTTPsMem_RespHdrGet((HTTPs_INSTANCE *)p_instance,
                                                  p_conn,
                                                  hdr_field,
                                                  val_type,
                                                  p_err);

#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
exit:
#endif
    return (p_blk);
}
#endif


/*
*********************************************************************************************************
*                                     HTTPs_RespBodySetParamFile()
*
* Description : Set the parameters for the response body when the body's data is a file inside a File System
*               infrastructure.
*
* Argument(s) : p_instance      Pointer to the instance.
*               ----------       Argument validated in HTTPs_InstanceStart().
*
*               p_conn          Pointer to the connection.
*               ------           Argument validated in HTTPsSock_ConnAccept().
*
*               p_path          Pointer to the string file path.
*
*               content_type    Content type of the file.
*                                   If unknown, can be set to HTTP_CONTENT_TYPE_UNKNOWN. The server core
*                                   will found it with the file extension.
*                                   See HTTP_CONTENT_TYPE enum in http.h for possible content types.
*
*               token_en        DEF_YES, if the file contents tokens the server needs to replace.
*                               DEF_NO, otherwise.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
* Return(s)   : None.
*
* Caller(s)   : Application. (Inside Hooks)
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPs_RespBodySetParamFile (const  HTTPs_INSTANCE        *p_instance,
                                         HTTPs_CONN            *p_conn,
                                         CPU_CHAR              *p_path,
                                         HTTP_CONTENT_TYPE      content_type,
                                         CPU_BOOLEAN            token_en,
                                         HTTPs_ERR             *p_err)
{
#if (HTTPs_CFG_TOKEN_PARSE_EN == DEF_ENABLED)
    const  HTTPs_CFG  *p_cfg = p_instance->CfgPtr;
#endif


    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_instance == DEF_NULL) {
       *p_err = HTTPs_ERR_NULL_PTR;
        goto exit;
    }

    if (p_conn == DEF_NULL) {
       *p_err = HTTPs_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_conn->RespBodyDataType = HTTPs_BODY_DATA_TYPE_FILE;

    if (p_path != DEF_NULL) {
        Mem_Copy(p_conn->PathPtr, p_path, p_conn->PathLenMax);
    } else {
        *p_err = HTTPs_ERR_RESP_PATH_PTR_INVALID;
         goto exit;
    }

    if (content_type != HTTP_CONTENT_TYPE_UNKNOWN) {
        p_conn->RespContentType = content_type;
    }

    if (token_en == DEF_YES) {
#if (HTTPs_CFG_TOKEN_PARSE_EN == DEF_ENABLED)
        if (p_cfg->TokenCfgPtr != DEF_NULL) {
            DEF_BIT_SET(p_conn->Flags, HTTPs_FLAG_RESP_CHUNKED);
        } else {
            DEF_BIT_CLR(p_conn->Flags, HTTPs_FLAG_RESP_CHUNKED);
           *p_err = HTTPs_ERR_CFG_INVALID_TOKEN_EN;
            goto exit;
         }
#else
        DEF_BIT_CLR(p_conn->Flags, HTTPs_FLAG_RESP_CHUNKED);
       *p_err = HTTPs_ERR_CFG_INVALID_TOKEN_EN;
        goto exit;
#endif
    } else {
        DEF_BIT_CLR(p_conn->Flags, HTTPs_FLAG_RESP_CHUNKED);
    }

   *p_err = HTTPs_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                  HTTPs_RespBodySetParamStaticData()
*
* Description : Set the parameters for the response body when the body's data is a static data contains in
*               a memory space.
*
* Argument(s) : p_instance      Pointer to the instance.
*               ----------       Argument validated in HTTPs_InstanceStart().
*
*               p_conn          Pointer to the connection.
*               ------           Argument validated in HTTPsSock_ConnAccept().
*
*               content_type    Content type of the file.
*                                   See HTTP_CONTENT_TYPE enum in http.h for possible content types.
*
*               p_data          Pointer to memory section containing data.
*                               DEF_NULL, if data will added to the response with the 'OnRespChunkHook' Hook.
*
*               data_len        Data length.
*
*               token_en        DEF_YES, if the data contents tokens the server needs to replace.
*                               DEF_NO, otherwise.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   HTTPs_ERR_NONE
*                                   HTTPs_ERR_NULL_PTR
*                                   HTTPs_ERR_RESP_BODY_DATA_LEN_INVALID
*
* Return(s)   : None.
*
* Caller(s)   : Application. (Inside Hooks)
*
* Note(s)     : (1) This function can be used when the data to put in the response body is not inside a file
*                   within a File System.
*
*               (2) If all the data to send is inside a memory space, the 'p_data' parameter can be set
*                   to point to the memory space and the 'data_len' must be set since the data length is
*                   known.
*
*               (3) When the data to send is a stream of unknown size, the Chunked Transfer Encoding must be
*                   used. In that case, the function can be used with the parameter 'p_data' set to DEF_NULL.
*                   This will tell the server to use the hook function 'p_cfg->p_hooks->OnRespChunkHook' to
*                   retrieve the data to put in the HTTP response.
*********************************************************************************************************
*/

void  HTTPs_RespBodySetParamStaticData (const  HTTPs_INSTANCE        *p_instance,
                                               HTTPs_CONN            *p_conn,
                                               HTTP_CONTENT_TYPE      content_type,
                                               void                  *p_data,
                                               CPU_INT32U             data_len,
                                               CPU_BOOLEAN            token_en,
                                               HTTPs_ERR             *p_err)
{
#if (HTTPs_CFG_TOKEN_PARSE_EN == DEF_ENABLED)
    const  HTTPs_CFG  *p_cfg = p_instance->CfgPtr;
#endif


    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_instance == DEF_NULL) {
       *p_err = HTTPs_ERR_NULL_PTR;
        goto exit;
    }

    if (p_conn == DEF_NULL) {
       *p_err = HTTPs_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_conn->RespBodyDataType = HTTPs_BODY_DATA_TYPE_STATIC_DATA;

#if 0
    if (content_type == HTTP_CONTENT_TYPE_UNKNOWN) {
       *p_err = HTTPs_ERR_RESP_CONTENT_TYPE_INVALID;
        goto exit;
    }
#endif

    p_conn->RespContentType  = content_type;

    if (p_data != DEF_NULL) {
        if (data_len <= 0) {
           *p_err = HTTPs_ERR_RESP_BODY_DATA_LEN_INVALID;
            goto exit;
        }
        p_conn->DataPtr = p_data;
        p_conn->DataLen = data_len;
        DEF_BIT_CLR(p_conn->Flags, HTTPs_FLAG_RESP_CHUNKED_HOOK);
    } else {
        DEF_BIT_SET(p_conn->Flags, HTTPs_FLAG_RESP_CHUNKED_HOOK);
    }

    if (token_en == DEF_YES) {
#if (HTTPs_CFG_TOKEN_PARSE_EN == DEF_ENABLED)
        if (p_cfg->TokenCfgPtr != DEF_NULL) {
            DEF_BIT_SET(p_conn->Flags, HTTPs_FLAG_RESP_CHUNKED);
        } else {
            DEF_BIT_CLR(p_conn->Flags, HTTPs_FLAG_RESP_CHUNKED);
           *p_err = HTTPs_ERR_CFG_INVALID_TOKEN_EN;
            goto exit;
         }
#else
        DEF_BIT_CLR(p_conn->Flags, HTTPs_FLAG_RESP_CHUNKED);
       *p_err = HTTPs_ERR_CFG_INVALID_TOKEN_EN;
        goto exit;
#endif
    } else {
        DEF_BIT_CLR(p_conn->Flags, HTTPs_FLAG_RESP_CHUNKED);
    }

   *p_err = HTTPs_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                    HTTPs_RespBodySetParamNoBody()
*
* Description : Set the parameters to let the server know that no body is necessary in the response.
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------   Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*               ------       Argument validated in HTTPsSock_ConnAccept().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPs_ERR_NONE                  Parameter successfully set.
*                               HTTPs_ERR_NULL_PTR              NULL argument(s) was passed.
*
* Return(s)   : None.
*
* Caller(s)   : Application. (Inside Hooks)
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPs_RespBodySetParamNoBody (const HTTPs_INSTANCE  *p_instance,
                                          HTTPs_CONN      *p_conn,
                                          HTTPs_ERR       *p_err)
{
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_instance == DEF_NULL) {
       *p_err = HTTPs_ERR_NULL_PTR;
        goto exit;
    }

    if (p_conn == DEF_NULL) {
       *p_err = HTTPs_ERR_NULL_PTR;
        goto exit;
    }
#endif

    p_conn->RespBodyDataType = HTTPs_BODY_DATA_TYPE_NONE;
    p_conn->RespContentType  = HTTP_CONTENT_TYPE_UNKNOWN;
    p_conn->DataPtr = DEF_NULL;
    p_conn->DataLen = 0;

    DEF_BIT_CLR(p_conn->Flags, HTTPs_FLAG_RESP_CHUNKED);
    DEF_BIT_CLR(p_conn->Flags, HTTPs_FLAG_RESP_CHUNKED_HOOK);

   *p_err = HTTPs_ERR_NONE;


#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
exit:
#endif
    return;
}

