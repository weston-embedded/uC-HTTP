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
* Filename : app_global_http-s_instance_cfg.c
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

#include "app_global_ctrl_layer_cfg.h"


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

#define  HTTPs_CFG_INSTANCE_STR_FILE_DEFAULT              "index.html"


/*
*********************************************************************************************************
*********************************************************************************************************
*                             HTTP SERVER INSTANCE TASK CONFIGURATION
*
* Note(s): See Template file http-s_instance_cfg.c for details on structure parameters.
*********************************************************************************************************
*********************************************************************************************************
*/

const  NET_TASK_CFG  HTTPs_TaskCfgInstance_AppGlobal = {

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
const  HTTPs_CFG_FS_NONE  HTTPs_CfgFS_None_AppGlobal = {

    256,                                                        /* .PathLenMax : Maximum path length.                   */
};
#endif

#if 1
const  HTTPs_CFG_FS_STATIC  HTTPs_CfgFS_Static_AppGlobal = {

   &HTTPs_FS_API_Static,                                        /* .FS_API_Ptr : Pointer to FS API.                     */
};
#endif

#if 0
const  HTTPs_CFG_FS_DYN  HTTPs_CfgFS_Dyn_AppGlobal = {

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

const  HTTPs_HDR_RX_CFG  HTTPs_HdrRxCfg_AppGlobal = {

    15,                                                         /* .NbrPerConnMax                                       */

    128,                                                        /* .DataLenMax                                          */
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                             HTTP SERVER INSTANCE HEADER TX CONFIGURATION
*
* Note(s): See Template file http-s_instance_cfg.c for details on structure parameters.
*********************************************************************************************************
*********************************************************************************************************
*/

const  HTTPs_HDR_TX_CFG  HTTPs_HdrTxCfg_AppGlobal = {

    15,                                                         /* .NbrPerConnMax                                       */

    128,                                                        /* .DataLenMax                                          */
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                           HTTP SERVER INSTANCE QUERY STRING CONFIGURATION
*
* Note(s) : See Template file http-s_instance_cfg.c for details on structure parameters.
*********************************************************************************************************
*********************************************************************************************************
*/

const  HTTPs_QUERY_STR_CFG    HTTPs_QueryStrCfg_AppGlobal = {

    5,                                                          /* .NbrPerConnMax                                       */

    15,                                                         /* .KeyLenMax                                           */

    20,                                                         /* .ValLenMax                                           */
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                             HTTP SERVER INSTANCE FORM CONFIGURATION
*
* Note(s): See Template file http-s_instance_cfg.c for details on structure parameters.
*********************************************************************************************************
*********************************************************************************************************
*/

const  HTTPs_FORM_CFG     HTTPs_FormCfg_AppGlobal = {


    15,                                                         /* .NbrPerConnMax                                       */

    10,                                                         /* .KeyLenMax                                           */

    48,                                                         /* .ValLenMax                                           */

    DEF_ENABLED,                                                /* .MultipartEn                                         */

    DEF_DISABLED,                                               /* .MultipartFileUploadEn                               */

    DEF_DISABLED,                                               /* .MultipartFileUploadOverWrEn                         */

    HTTPs_CFG_INSTANCE_STR_FOLDER_UPLOAD,                       /* .MultipartFileUploadFolderPtr                        */
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                             HTTP SERVER INSTANCE TOKEN CONFIGURATION
*
* Note(s): See Template file http-s_instance_cfg.c for details on structure parameters.
*********************************************************************************************************
*********************************************************************************************************
*/

const  HTTPs_TOKEN_CFG   HTTPs_TokenCfg_AppGlobal = {

    5,                                                          /* .NbrPerConnMax                                       */

    12,                                                         /* .ValLenMax                                           */

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

const  HTTPs_CFG  HTTPs_CfgInstance_AppGlobal = {

/*
*--------------------------------------------------------------------------------------------------------
*                                      INSTANCE TASK CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    0u,                                                         /* .OS_TaskDly_ms : Configure instance task delay.      */


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

    6,                                                          /* .ConnNbrMax : Maximum number of simultaneous.        */

    10,                                                         /* .ConnInactivityTimeout_s : Conn inactivity timeout.  */

    1460,                                                       /* .BufLen : Connection buffer length.                  */

    DEF_ENABLED,                                                /* .ConnPersistentEn : Persistent conn feature.         */


/*
*--------------------------------------------------------------------------------------------------------
*                                  INSTANCE FILE SYSTEM CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    HTTPs_FS_TYPE_STATIC,                                       /* .FS_Type : File System Type.                         */

   &HTTPs_CfgFS_Static_AppGlobal,                               /* .FS_CfgPtr : File System Configuration pointer.      */

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

    &HTTPsCtrlLayer_HookCfg,                                    /* .HooksPtr : Pointer to Hooks' Object.                */

    &AppGlobal_CtrlLayerCfgList,                                /* .Hooks_CfgPtr : Pointer to Application Data Hook.    */


/*
*--------------------------------------------------------------------------------------------------------
*                                     HEADER FIELD CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

   &HTTPs_HdrRxCfg_AppGlobal,                                   /* .HdrRxCfgPtr : Pointer to Request Hdr Cfg Object.    */

   &HTTPs_HdrTxCfg_AppGlobal,                                   /* .HdrTxCfgPtr : Pointer to Response Hdr Cfg Object.   */


/*
*--------------------------------------------------------------------------------------------------------
*                                  INSTANCE QUERY STRING CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

   &HTTPs_QueryStrCfg_AppGlobal,                                /* .QueryStrCfgPtr : Pointer to Query String Cfg Object.*/

/*
*--------------------------------------------------------------------------------------------------------
*                                    INSTANCE FORM CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

   &HTTPs_FormCfg_AppGlobal,                                    /* .FormCfgPtr : Pointer to Form Cfg Object.            */


/*
*--------------------------------------------------------------------------------------------------------
*                               DYNAMIC TOKEN REPLACEMENT CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

   &HTTPs_TokenCfg_AppGlobal,                                   /* .TokenCfgPtr : Pointer to Token Cfg Ojbect.          */

};                                                              /* End of configuration structure.                      */

