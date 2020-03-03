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
*                               HTTP INSTANCE SERVER CONFIGURATION FILE
*
*                                               TEMPLATE
*
* Filename : http-s_instance_cfg.h
* Version  : V3.01.00
*********************************************************************************************************
*/

#include  <Server/Source/http-s.h>
#include  <Source/net_type.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   FILES & FOLDERS STRING DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPs_CFG_INSTANCE_STR_FOLDER_ROOT               "\\"

#define  HTTPs_CFG_INSTANCE_STR_FILE_DEFAULT              "index.html"
#define  HTTPs_CFG_INSTANCE_STR_FILE_ERR_404              "404.html"

#define  HTTPs_CFG_INSTANCE_STR_FOLDER_UPLOAD             "\\"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   HTTP SERVER INSTANCE CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

extern  const  NET_TASK_CFG  HTTPs_TaskCfgInstance;
extern  const  HTTPs_CFG     HTTPs_CfgInstance;

