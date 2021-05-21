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
*                                      HTTP SERVER SOCKET MODULE
*
* Filename : http-s_sock.h
* Version  : V3.01.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This main network protocol suite header file is protected from multiple pre-processor
*               inclusion through use of the HTTPs module present pre-processor macro definition.
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  HTTPs_SOCK_MODULE_PRESENT                          /* See Note #1.                                         */
#define  HTTPs_SOCK_MODULE_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <cpu_core.h>

#include  "http-s.h"

/*
*********************************************************************************************************
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

NET_SOCK_ID   HTTPsSock_ListenInit   (const  HTTPs_CFG                 *p_cfg,
                                             NET_SOCK_PROTOCOL_FAMILY   family,
                                             HTTPs_ERR                 *p_err);

void          HTTPsSock_ListenClose  (       HTTPs_INSTANCE            *p_instance,
                                             NET_SOCK_ID                sock_listen_id);

NET_SOCK_QTY  HTTPsSock_ConnSel      (       HTTPs_INSTANCE            *p_instance,
                                             CPU_BOOLEAN                accept);

CPU_BOOLEAN   HTTPsSock_ConnDataRx   (       HTTPs_INSTANCE            *p_instance,
                                             HTTPs_CONN                *p_conn);

CPU_BOOLEAN   HTTPsSock_ConnDataTx   (       HTTPs_INSTANCE            *p_instance,
                                             HTTPs_CONN                *p_conn);

void          HTTPsSock_ConnClose    (       HTTPs_INSTANCE            *p_instance,
                                             HTTPs_CONN                *p_conn);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* HTTPs_SOCK_MODULE_PRESENT  */
