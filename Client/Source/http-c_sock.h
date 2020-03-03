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
*                                      HTTP CLIENT SOCKET MODULE
*
* Filename : http-c_sock.h
* Version  : V3.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This main network protocol suite header file is protected from multiple pre-processor
*               inclusion through use of the HTTPc module present pre-processor macro definition.
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  HTTPc_SOCK_MODULE_PRESENT                          /* See Note #1.                                         */
#define  HTTPc_SOCK_MODULE_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         HTTPc INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <cpu_core.h>

#include  <lib_def.h>
#include  <lib_str.h>
#include  <lib_ascii.h>

#include  <http-c_cfg.h>
#include  "../../Common/http.h"
#include  "http-c.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

void          HTTPcSock_Connect              (HTTPc_CONN  *p_conn,
                                              HTTPc_ERR   *p_err);

CPU_BOOLEAN   HTTPcSock_ConnDataRx           (HTTPc_CONN  *p_conn,
                                              CPU_INT32U   max_len,
                                              HTTPc_ERR   *p_err);

CPU_BOOLEAN   HTTPcSock_ConnDataTx           (HTTPc_CONN  *p_conn,
                                              HTTPc_ERR   *p_err);

void          HTTPcSock_Close                (HTTPc_CONN  *p_conn,
                                              HTTPc_ERR   *p_err);

void          HTTPcSock_Sel                  (HTTPc_CONN  *p_conn,
                                              HTTPc_ERR   *p_err);

CPU_BOOLEAN   HTTPcSock_IsRxClosed           (HTTPc_CONN  *p_conn,
                                              HTTPc_ERR   *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* HTTPc_SOCK_MODULE_PRESENT  */
