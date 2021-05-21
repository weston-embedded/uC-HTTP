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
* Filename : app_rest_http-s_instance_cfg.c
* Version  : V3.01.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*
* Note(s) : (1) All values that are used in this file and that are defined in other header files should
*               be included by this file. Some values could be located in the same file such as task priority
*               and
size. This template file assume that the following values are defined in app_cfg.h:
*
*                   HTTPs_OS_CFG_INSTANCE_TASK_PRIO
*                   HTTPs_OS_CFG_INSTANCE_TASK_STK_SIZE
*
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <Server/Source/http-s.h>
#include  <Server/FS/Static/http-s_fs_static.h>

#include  <Server/Add-on/REST/http-s_rest_hook_cfg.h>

#include  "app_rest_http-s_instance_cfg.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   FILES & FOLDERS STRING DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPs_OS_CFG_INSTANCE_TASK_PRIO                  17
#define  HTTPs_OS_CFG_INSTANCE_TASK_STK_SIZE             (4*1024u)

#define  HTTPs_CFG_INSTANCE_STR_FOLDER_ROOT               "\\"
#define  HTTPs_CFG_INSTANCE_STR_FOLDER_UPLOAD             HTTPs_CFG_INSTANCE_STR_FOLDER_ROOT

#define  HTTPs_CFG_INSTANCE_STR_FILE_DEFAULT              "/list.html"


/*
*********************************************************************************************************
*********************************************************************************************************
*                             HTTP SERVER INSTANCE TASK CONFIGURATION
*
* Note(s): See Template file http-s_instance_cfg.c for details on structure parameters.
*********************************************************************************************************
*********************************************************************************************************
*/

const  NET_TASK_CFG  HTTPs_TaskCfgInstance_REST = {

        HTTPs_OS_CFG_INSTANCE_TASK_PRIO,                        /* .Prio : Configure Instance Task priority.            */

        HTTPs_OS_CFG_INSTANCE_TASK_STK_SIZE,                    /* .StkSizeBytes : Configure instance task size.        */

        DEF_NULL                                                /* .StkPtr : Configure pointer to base of the stack.    */
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                              HTTP SERVER FILE SYSTEM CONFIGURATION
*
* Note(s) : (1) See Template file http-s_instance_cfg.c for details on structure parameters.
*********************************************************************************************************
*********************************************************************************************************
*/

#if 0
const  HTTPs_CFG_FS_NONE  HTTPs_CfgFS_None_REST = {

    256,                                                        /* .PathLenMax : Maximum path length.                   */
};
#endif

#if 1
const  HTTPs_CFG_FS_STATIC  HTTPs_CfgFS_Static_REST = {

   &HTTPs_FS_API_Static,                                        /* .FS_API_Ptr : Pointer to FS API.                     */
};
#endif

#if 0
const  HTTPs_CFG_FS_DYN  HTTPs_CfgFS_Dyn_REST = {

   &NetFS_API_FS_V4,                                            /* .FS_API_Ptr : Pointer to FS API.                     */

    HTTPs_CFG_INSTANCE_STR_FOLDER_ROOT                          /* .WorkingFolderPtr : FS working folder.               */
};
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                             HTTP SERVER INSTANCE HEADER RX CONFIGURATION
*
* Note(s): See Template file http-s_instance_cfg.c for details on structure parameters.
*********************************************************************************************************
*********************************************************************************************************
*/

const  HTTPs_HDR_RX_CFG  HTTPs_HdrRxCfg_REST = {

    15,                                                         /* .NbrPerConnMax                                       */

    128,                                                        /* .DataLenMax                                          */
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                            HTTP INSTANCE SERVER CONFIGURATION STRUCTURE
*
* Note(s): See Template file http-s_instance_cfg.c for details on structure parameters.
*********************************************************************************************************
*********************************************************************************************************
*/

const  HTTPs_CFG  HTTPs_CfgInstance_REST = {

/*
*--------------------------------------------------------------------------------------------------------
*                                      INSTANCE TASK CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    2u,                                                         /* .OS_TaskDly_ms : Configure instance task delay.      */


/*
*--------------------------------------------------------------------------------------------------------
*                                INSTANCE LISTEN SOCKET CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    HTTPs_SOCK_SEL_IPv4_IPv6,                                   /* .SockSel : Socket type.                              */

    DEF_NULL,                                                   /* .SecurePtr : Secure configuration (SSL) Pointer.     */

    HTTPs_CFG_DFLT_PORT,                                        /* .Port : Server port number.                          */


/*
*--------------------------------------------------------------------------------------------------------
*                                  INSTANCE CONNECTION CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    APP_REST_HTTPs_CONN_NBR_MAX,                                /* .ConnNbrMax : Maximum number of simultaneous.        */

    10,                                                         /* .ConnInactivityTimeout_s : Conn inactivity timeout.  */

    1460,                                                       /* .BufLen : Connection buffer length.                  */

    DEF_ENABLED,                                                /* .ConnPersistentEn : Persistent conn feature.         */


/*
*--------------------------------------------------------------------------------------------------------
*                                  INSTANCE FILE SYSTEM CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    HTTPs_FS_TYPE_STATIC,                                       /* .FS_Type : File System Type.                         */

   &HTTPs_CfgFS_Static_REST,                                    /* .FS_CfgPtr : File System Configuration pointer.      */

    HTTPs_CFG_INSTANCE_STR_FILE_DEFAULT,                        /* .DfltResourceNamePtr : default web page.             */


/*
*--------------------------------------------------------------------------------------------------------
*                                    INSTANCE PROXY CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    128,                                                        /* .HostNameLenMax : Maximum host name length.          */

/*
*--------------------------------------------------------------------------------------------------------
*                                        HOOK CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    &HTTPs_REST_HookCfg,                                        /* .HooksPtr : Pointer to Hooks' Object.                */

    &HTTPs_REST_Cfg,                                            /* .Hooks_CfgPtr : Pointer to Application Data Hook.    */


/*
*--------------------------------------------------------------------------------------------------------
*                                     HEADER FIELD CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

   &HTTPs_HdrRxCfg_REST,                                        /* .HdrRxCfgPtr : Pointer to Request Hdr Cfg Object.    */

    DEF_NULL,                                                   /* .HdrTxCfgPtr : Pointer to Response Hdr Cfg Object.   */


/*
*--------------------------------------------------------------------------------------------------------
*                                  INSTANCE QUERY STRING CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    DEF_NULL,                                                   /* .QueryStrCfgPtr : Pointer to Query String Cfg Object.*/

/*
*--------------------------------------------------------------------------------------------------------
*                                    INSTANCE FORM CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    DEF_NULL,                                                   /* .FormCfgPtr : Pointer to Form Cfg Object.            */


/*
*--------------------------------------------------------------------------------------------------------
*                               DYNAMIC TOKEN REPLACEMENT CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    DEF_NULL,                                                   /* .TokenCfgPtr : Pointer to Token Cfg Ojbect.          */

};                                                              /* End of configuration structure.                      */

