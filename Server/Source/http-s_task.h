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
*                                       HTTP SERVER TASK MODULE
*
* Filename : http-s_task.h
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

#ifndef  HTTPs_TASK_MODULE_PRESENT                          /* See Note #1.                                         */
#define  HTTPs_TASK_MODULE_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <KAL/kal.h>
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

KAL_LOCK_HANDLE      HTTPsTask_LockCreate                   (HTTPs_ERR                 *p_err);

void                 HTTPsTask_LockAcquire                  (KAL_LOCK_HANDLE            os_lock_obj,
                                                             HTTPs_ERR                 *p_err);

void                 HTTPsTask_LockRelease                  (KAL_LOCK_HANDLE            os_lock_obj);

void                 HTTPsTask_InstanceObjInit              (HTTPs_INSTANCE            *p_instance,
                                                             HTTPs_ERR                 *p_err);

void                 HTTPsTask_InstanceTaskCreate           (HTTPs_INSTANCE            *p_instance,
                                                             HTTPs_ERR                 *p_err);

void                 HTTPsTask_InstanceTaskDel              (HTTPs_INSTANCE            *p_instance);

void                 HTTPsTask_InstanceStopReqSignal        (HTTPs_INSTANCE            *p_instance,
                                                             HTTPs_ERR                 *p_err);

CPU_BOOLEAN          HTTPsTask_InstanceStopReqPending       (HTTPs_INSTANCE            *p_instance);

void                 HTTPsTask_InstanceStopCompletedSignal  (HTTPs_INSTANCE            *p_instance);

void                 HTTPsTask_InstanceStopCompletedPending (HTTPs_INSTANCE            *p_instance,
                                                             HTTPs_ERR                 *p_err);

void                 HTTPsTask_TimeDly_ms                   (CPU_INT32U                 time_dly_ms);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* HTTPs_TASK_MODULE_PRESENT  */
