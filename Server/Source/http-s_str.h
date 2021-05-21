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
*                                      HTTP SERVER STRING MODULE
*
* Filename : http-s_str.h
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

#ifndef  HTTPs_STR_MODULE_PRESENT                          /* See Note #1.                                         */
#define  HTTPs_STR_MODULE_PRESENT


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

#if ((HTTPs_CFG_FORM_EN           == DEF_ENABLED) && \
     (HTTPs_CFG_FORM_MULTIPART_EN == DEF_ENABLED))
CPU_BOOLEAN   HTTPs_StrPathFormat  (       CPU_CHAR     *p_filename,
                                           CPU_CHAR     *p_folder,
                                           CPU_CHAR     *p_path_dst,
                                           CPU_SIZE_T    path_len_max,
                                           CPU_CHAR      path_sep);
#endif

#if (HTTPs_CFG_ABSOLUTE_URI_EN == DEF_ENABLED)
CPU_CHAR     *HTTPs_StrPathGet     (       CPU_CHAR     *p_path,
                                           CPU_INT16U    path_len_max,
                                           CPU_CHAR     *p_host,
                                           CPU_INT16U    host_len_max,
                                           CPU_BOOLEAN  *p_resp_location);
#endif

#if ((HTTPs_CFG_FORM_EN           == DEF_ENABLED) && \
     (HTTPs_CFG_FORM_MULTIPART_EN == DEF_ENABLED))
CPU_CHAR     *HTTPs_StrMemSrch     (const  CPU_CHAR     *p_data,
                                           CPU_INT32U    data_len,
                                    const  CPU_CHAR     *p_str,
                                           CPU_INT32U    str_len);
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* HTTPs_STR_MODULE_PRESENT  */
