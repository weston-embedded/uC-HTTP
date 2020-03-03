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
*                                 HTTP APPLICATION WITH CONTROL LAYER
*
* Filename : app_global.h
* Version  : V3.01.00
*********************************************************************************************************
* Note(s)  : (1) This application regroup with the control layer a login layer, a default page app layer
*                and a REST app layer.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef APP_GLOBAL_MODULE_PRESENT
#define APP_GLOBAL_MODULE_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include <Auth/auth.h>
#include <Server/Add-on/Auth/http-s_auth.h>

#include "app_global_ctrl_layer_cfg.h"
#include "app_global_http-s_instance_cfg.h"

#include  <Server/Add-on/REST/http-s_rest.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

extern  HTTPs_CTRL_LAYER_AUTH_INST  AppGlobal_AuthInst;

extern  HTTPs_CTRL_LAYER_APP_INST   AppGlobal_AppInst_AuthUnprotected;
extern  HTTPs_CTRL_LAYER_APP_INST   AppGlobal_AppInst_AuthProtected;

extern  HTTPs_CTRL_LAYER_APP_INST   AppGlobal_AppInst_REST;

extern  HTTPs_CTRL_LAYER_APP_INST   AppGlobal_AppInst_Basic;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

CPU_BOOLEAN            AppGlobal_Init                       (void);

CPU_BOOLEAN            AppGlobal_REST_ResourcesInit         (void);

AUTH_RIGHT             AppGlobal_Auth_GetRequiredRightsHook (const  HTTPs_INSTANCE     *p_instance,
                                                             const  HTTPs_CONN         *p_conn);

CPU_BOOLEAN            AppGlobal_Auth_ParseLoginHook        (const  HTTPs_INSTANCE     *p_instance,
                                                             const  HTTPs_CONN         *p_conn,
                                                                    HTTPs_AUTH_STATE    state,
                                                                    HTTPs_AUTH_RESULT  *p_result);

CPU_BOOLEAN            AppGlobal_Auth_ParseLogoutHook       (const  HTTPs_INSTANCE     *p_instance,
                                                             const  HTTPs_CONN         *p_conn,
                                                                    HTTPs_AUTH_STATE    state);

CPU_BOOLEAN            AppGlobal_Basic_ReqHook              (const  HTTPs_INSTANCE     *p_instance,
                                                                    HTTPs_CONN         *p_conn,
                                                             const  void               *p_hook_cfg);

CPU_BOOLEAN            AppGlobal_Basic_TokenValGetHook      (const  HTTPs_INSTANCE     *p_instance,
                                                                    HTTPs_CONN         *p_conn,
                                                             const  void               *p_hook_cfg,
                                                             const  CPU_CHAR           *p_token,
                                                                    CPU_INT16U          token_len,
                                                                    CPU_CHAR           *p_val,
                                                                    CPU_INT16U          val_len_max);

CPU_BOOLEAN            AppGlobal_Basic_RespChunkDataGetHook (const  HTTPs_INSTANCE     *p_instance,
                                                                    HTTPs_CONN         *p_conn,
                                                             const  void               *p_hook_cfg,
                                                                    void               *p_buf,
                                                                    CPU_SIZE_T          buf_len_max,
                                                                    CPU_SIZE_T         *p_tx_len);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif /* APP_GLOBAL_MODULE_PRESENT */
