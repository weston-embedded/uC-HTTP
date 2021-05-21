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
*                                    HTTP CLIENT CONNECTION MODULE
*
* Filename : http-c_conn.h
* Version  : V3.01.01
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

#ifndef  HTTPc_CONN_MODULE_PRESENT                              /* See Note #1.                                         */
#define  HTTPc_CONN_MODULE_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
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

void          HTTPcConn_Process         (HTTPc_CONN  *p_conn);

void          HTTPcConn_Connect         (HTTPc_CONN  *p_conn,
                                         HTTPc_ERR   *p_err);

void          HTTPcConn_Close           (HTTPc_CONN  *p_conn,
                                         HTTPc_ERR   *p_err);

void          HTTPcConn_TransProcess    (HTTPc_CONN  *p_conn);

void          HTTPcConn_TransParamReset (HTTPc_CONN  *p_conn);

CPU_BOOLEAN   HTTPcConn_TransComplete   (HTTPc_CONN  *p_conn);

void          HTTPcConn_Add             (HTTPc_CONN  *p_conn);

void          HTTPcConn_Remove          (HTTPc_CONN  *p_conn);

void          HTTPcConn_ReqAdd          (HTTPc_REQ   *p_req,
                                         HTTPc_ERR   *p_err);

void          HTTPcConn_ReqRemove       (HTTPc_CONN  *p_conn);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* HTTPc_CONN_MODULE_PRESENT  */
