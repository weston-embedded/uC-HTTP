/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                               HTTP INSTANCE SERVER CONFIGURATION FILE
*
* Filename : http-s_instance_secure_cfg.h
* Version  : V3.01.00
*********************************************************************************************************
*/

#include  <Server/Source/http-s.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   FILES & FOLDERS STRING DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPs_CFG_INSTANCE_SECURE_STR_FOLDER_ROOT               "\\"

#define  HTTPs_CFG_INSTANCE_SECURE_STR_FILE_DEFAULT              "index.html"
#define  HTTPs_CFG_INSTANCE_SECURE_STR_FILE_ERR_404              "404.html"

#define  HTTPs_CFG_INSTANCE_SECURE_STR_FOLDER_UPLOAD             "\\"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   HTTP SERVER INSTANCE CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

extern  const  NET_TASK_CFG  HTTPs_TaskCfgInstance_Secure;
extern  const  HTTPs_CFG     HTTPs_CfgInstance_Secure;



