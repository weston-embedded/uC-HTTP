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
* Filename : app_basic_http-s_instance_cfg.c
* Version  : V3.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*
* Note(s) : (1) All values that are used in this file and are defined in other header files should be
*               included in this file. Some values could be located in the same file such as task priority
*               and stack size. This template file assume that the following values are defined in app_cfg.h:
*
*                   HTTPs_OS_CFG_INSTANCE_TASK_PRIO
*                   HTTPs_OS_CFG_INSTANCE_TASK_STK_SIZE
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <app_cfg.h>                                           /* See Note #1.                                         */

#include  "app_basic_http-s_instance_cfg.h"
#include  "app_basic.h"

#if (APP_BASIC_FS_DYN_EN == DEF_ENABLED)
#include  <FS/uC-FS-V4/net_fs_v4.h>
#else
#include  <Server/FS/Static/http-s_fs_static.h>
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   FILES & FOLDERS STRING DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPs_CFG_INSTANCE_STR_FOLDER_ROOT                "\\"

#define  HTTPs_CFG_INSTANCE_STR_FILE_DEFAULT              "index.html"
#define  HTTPs_CFG_INSTANCE_STR_FILE_ERR_404              "404.html"

#define  HTTPs_CFG_INSTANCE_STR_FOLDER_UPLOAD             "\\"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

extern  const  HTTPs_HOOK_CFG  HTTPs_Hooks_AppBasic;


/*
*********************************************************************************************************
*********************************************************************************************************
*                             HTTP SERVER INSTANCE TASK CONFIGURATION
*
* Note(s): See Template file http-s_instance_cfg.c for details on structure parameters.
*********************************************************************************************************
*********************************************************************************************************
*/

const  NET_TASK_CFG  HTTPs_TaskCfgInstance_AppBasic = {

    HTTPs_OS_CFG_INSTANCE_TASK_PRIO,                            /* .Prio : Configure Instance Task priority.            */

    HTTPs_OS_CFG_INSTANCE_TASK_STK_SIZE,                        /* .StkSizeBytes : Configure instance task size.        */

    DEF_NULL                                                    /* .StkPtr : Configure pointer to base of the stack.    */

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

#if (APP_BASIC_FS_DYN_EN == DEF_ENABLED)
const  HTTPs_CFG_FS_DYN  HTTPs_CfgFS_AppBasic = {

   &NetFS_API_FS_V4,                                            /* .FS_API_Ptr : Pointer to FS API.                     */

    HTTPs_CFG_INSTANCE_STR_FOLDER_ROOT                          /* .WorkingFolderNamePtr : FS working folder.           */
};
#else
const  HTTPs_CFG_FS_STATIC  HTTPs_CfgFS_AppBasic = {

   &HTTPs_FS_API_Static,                                        /* .FS_API_Ptr : Pointer to FS API.                     */
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

const  HTTPs_HDR_RX_CFG  HTTPs_HdrRxCfg_AppBasic = {

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

const  HTTPs_HDR_TX_CFG  HTTPs_HdrTxCfg_AppBasic = {

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

const  HTTPs_QUERY_STR_CFG    HTTPs_QueryStrCfg_AppBasic = {

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

const  HTTPs_FORM_CFG     HTTPs_FormCfg_AppBasic = {


    15,                                                         /* .NbrPerConnMax                                       */

    10,                                                         /* .KeyLenMax                                           */

    48,                                                         /* .ValLenMax                                           */

    DEF_ENABLED,                                                /* .MultipartEn                                         */

#if (APP_INIT_FS_DYN_EN == DEF_ENABLED)
    DEF_ENABLED,                                                /* .MultipartFileUploadEn                               */
#else
    DEF_DISABLED,
#endif

    DEF_ENABLED,                                                /* .MultipartFileUploadOverWrEn                         */

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

const  HTTPs_TOKEN_CFG   HTTPs_TokenCfg_AppBasic = {

    5,                                                          /* .NbrPerConnMax                                       */

    12,                                                         /* .ValLenMax                                           */

};


/*
*********************************************************************************************************
*********************************************************************************************************
*                               HTTP SERVER INSTANCE CONFIGURATION
*
* Note(s): See Template file http-s_instance_cfg.c for details on structure parameters.
*********************************************************************************************************
*********************************************************************************************************
*/

const  HTTPs_CFG  HTTPs_CfgInstance_AppBasic = {


/*
*--------------------------------------------------------------------------------------------------------
*                                      INSTANCE OS CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

        1u,                                                     /* .OS_TaskDly_ms : Configure instance task delay.      */


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

    15,                                                         /* .ConnNbrMax : Maximum number of simultaneous.        */

    15,                                                         /* .ConnInactivityTimeout_s : Conn inactivity timeout.  */

    1460,                                                       /* .BufLen : Connection buffer length.                  */

    DEF_ENABLED,                                               /* .ConnPersistentEn : Persistent conn feature.         */


/*
*--------------------------------------------------------------------------------------------------------
*                                  INSTANCE FILE SYSTEM CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/
#if (APP_BASIC_FS_DYN_EN == DEF_ENABLED)
    HTTPs_FS_TYPE_DYN,                                          /* .FS_Type : File System Type.                         */
#else
    HTTPs_FS_TYPE_STATIC,                                       /* .FS_Type : File System Type.                         */
#endif

   &HTTPs_CfgFS_AppBasic,                                       /* .FS_CfgPtr : File System Configuration pointer.      */

    HTTPs_CFG_INSTANCE_STR_FILE_DEFAULT,                        /* .DfltResourceNamePtr : Default page.                 */

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

   &HTTPs_Hooks_AppBasic,                                       /* .HooksPtr : Pointer to Hooks' Object.                */

    DEF_NULL,                                                   /* .Hooks_CfgPtr : Pointer to Application Data Hook.    */


/*
*--------------------------------------------------------------------------------------------------------
*                                     HEADER FIELD CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

   &HTTPs_HdrRxCfg_AppBasic,                                    /* .HdrRxCfgPtr : Pointer to Request Hdr Cfg Object.    */

   &HTTPs_HdrTxCfg_AppBasic,                                    /* .HdrTxCfgPtr : Pointer to Response Hdr Cfg Object.   */


/*
*--------------------------------------------------------------------------------------------------------
*                                  INSTANCE QUERY STRING CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

   &HTTPs_QueryStrCfg_AppBasic,                                 /* .QueryStrCfgPtr : Pointer to Query String Cfg Object.*/


/*
*--------------------------------------------------------------------------------------------------------
*                                      INSTANCE FORM CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

   &HTTPs_FormCfg_AppBasic,                                     /* .FormCfgPtr : Pointer to Form Cfg Object.            */


/*
*--------------------------------------------------------------------------------------------------------
*                                DYNAMIC TOKEN REPLACEMENT CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

   &HTTPs_TokenCfg_AppBasic,                                    /* .TokenCfgPtr : Pointer to Token Cfg Ojbect.          */

};                                                              /* End of configuration structure.                      */

