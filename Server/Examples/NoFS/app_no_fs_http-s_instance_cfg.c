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
* Filename : app_no_fs_http-s_instance_cfg.c
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
*
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <app_cfg.h>                                           /* See Note #1.                                         */

#include  "app_no_fs_http-s_instance_cfg.h"
#include  "app_no_fs.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   FILES & FOLDERS STRING DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPs_CFG_INSTANCE_STR_FILE_DEFAULT              "/hello_world"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

extern  const  HTTPs_HOOK_CFG  HTTPs_Hooks_NoFS;


/*
*********************************************************************************************************
*********************************************************************************************************
*                             HTTP SERVER INSTANCE TASK CONFIGURATION
*
* Note(s): See Template file http-s_instance_cfg.c for details on structure parameters.
*********************************************************************************************************
*********************************************************************************************************
*/

const  NET_TASK_CFG  HTTPs_TaskCfgInstance_NoFS = {

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

const  HTTPs_CFG_FS_NONE  HTTPs_CfgFS_NoFS = {

    256,                                                        /* .PathLenMax : Maximum path length.                   */
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

const  HTTPs_HDR_TX_CFG  HTTPs_HdrTxCfg_NoFS = {

    2,                                                          /* .NbrPerConnMax                                       */

    128,                                                        /* .DataLenMax                                          */
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

const  HTTPs_CFG  HTTPs_CfgInstance_NoFS = {


/*
*--------------------------------------------------------------------------------------------------------
*                                      INSTANCE OS CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    2,                                                          /* .OS_TaskDly_ms : Configure instance task delay.      */


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

    DEF_ENABLED,                                                /* .ConnPersistentEn : Persistent conn feature.         */


/*
*--------------------------------------------------------------------------------------------------------
*                                  INSTANCE FILE SYSTEM CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    HTTPs_FS_TYPE_NONE,                                          /* .FS_Type : File System Type.                         */


   &HTTPs_CfgFS_NoFS,                                           /* .FS_CfgPtr : File System Configuration pointer.      */

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

   &HTTPs_Hooks_NoFS,                                           /* .HooksPtr : Pointer to Hooks' Object.                */

    DEF_NULL,                                                   /* .Hooks_CfgPtr : Pointer to Application Data Hook.    */


/*
*--------------------------------------------------------------------------------------------------------
*                                     HEADER FIELD CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    DEF_NULL,                                                   /* .HdrRxCfgPtr : Pointer to Request Hdr Cfg Object.    */

   &HTTPs_HdrTxCfg_NoFS,                                        /* .HdrTxCfgPtr : Pointer to Response Hdr Cfg Object.   */


/*
*--------------------------------------------------------------------------------------------------------
*                                  INSTANCE QUERY STRING CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    DEF_NULL,                                                   /* .QueryStrCfgPtr : Pointer to Query String Cfg Object.*/


/*
*--------------------------------------------------------------------------------------------------------
*                                      INSTANCE FORM CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    DEF_NULL,                                                   /* .FormCfgPtr : Pointer to Form Cfg Object.            */


/*
*--------------------------------------------------------------------------------------------------------
*                                DYNAMIC TOKEN REPLACEMENT CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/

    DEF_NULL,                                                   /* .TokenCfgPtr : Pointer to Token Cfg Ojbect.          */

};                                                              /* End of configuration structure.                      */

