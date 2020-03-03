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
*                                     HTTP SERVER MEMORY LIBRARY
*
* Filename : http-s_mem.h
* Version  : V3.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This main network protocol suite header file is protected from multiple pre-processor
*               inclusion through use of the HTTP memory module present pre-processor macro definition.
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  HTTPs_MEM_PRESENT                                          /* See Note #1.                                         */
#define  HTTPs_MEM_PRESENT

/*
*********************************************************************************************************
*********************************************************************************************************
*                                         HTTPs INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "http-s.h"

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef   HTTPs_MEM_MODULE
#define  HTTPs_MEM_EXT
#else
#define  HTTPs_MEM_EXT  extern
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* Instance functionalities.            */
void                 HTTPsMem_InstanceInit                (MEM_SEG             *p_mem_seg,
                                                           HTTPs_ERR           *p_err);

HTTPs_OS_TASK_OBJ   *HTTPsMem_InstanceTaskInit            (HTTPs_ERR           *p_err);

HTTPs_INSTANCE      *HTTPsMem_InstanceGet                 (HTTPs_ERR           *p_err);

void                 HTTPsMem_InstanceRelease             (HTTPs_INSTANCE      *p_instance);

                                                                /* Conn functionalities.                */
void                 HTTPsMem_ConnPoolInit                (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_ERR           *p_err);

HTTPs_CONN          *HTTPsMem_ConnGet                     (HTTPs_INSTANCE      *p_instance,
                                                           NET_SOCK_ID          sock_id,
                                                           NET_SOCK_ADDR        client_addr);

void                 HTTPsMem_ConnRelease                 (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_CONN          *p_conn);

void                 HTTPsMem_ConnClr                     (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_CONN          *p_conn);

#if (HTTPs_CFG_TOKEN_PARSE_EN == DEF_ENABLED)
CPU_BOOLEAN          HTTPsMem_TokenGet                    (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_CONN          *p_conn,
                                                           HTTPs_ERR           *p_err);

void                 HTTPsMem_TokenRelease                (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_CONN          *p_conn);
#endif

#if (HTTPs_CFG_QUERY_STR_EN == DEF_ENABLED)
HTTPs_KEY_VAL       *HTTPsMem_QueryStrKeyValBlkGet        (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_CONN          *p_conn,
                                                           HTTPs_ERR           *p_err);

void                 HTTPsMem_QueryStrKeyValBlkReleaseAll (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_CONN          *p_conn);
#endif

#if (HTTPs_CFG_FORM_EN == DEF_ENABLED)
HTTPs_KEY_VAL       *HTTPsMem_FormKeyValBlkGet            (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_CONN          *p_conn,
                                                           HTTPs_ERR           *p_err);

void                 HTTPsMem_FormKeyValBlkReleaseAll     (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_CONN          *p_conn);
#endif

#if (HTTPs_CFG_HDR_RX_EN == DEF_ENABLED)
HTTP_HDR_BLK        *HTTPsMem_ReqHdrGet                   (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_CONN          *p_conn,
                                                           HTTP_HDR_FIELD       hdr_fied,
                                                           HTTP_HDR_VAL_TYPE    val_type,
                                                           HTTPs_ERR           *p_err);

void                 HTTPsMem_ReqHdrRelease               (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_CONN          *p_conn);
#endif

#if (HTTPs_CFG_HDR_TX_EN == DEF_ENABLED)
HTTP_HDR_BLK        *HTTPsMem_RespHdrGet                  (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_CONN          *p_conn,
                                                           HTTP_HDR_FIELD       hdr_fied,
                                                           HTTP_HDR_VAL_TYPE    val_type,
                                                           HTTPs_ERR           *p_err);

void                 HTTPsMem_RespHdrRelease              (HTTPs_INSTANCE      *p_instance,
                                                           HTTPs_CONN          *p_conn);
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of HTTPs mem module include.                     */

