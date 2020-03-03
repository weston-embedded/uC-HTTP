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
*                                    HTTPs REST HOOK CONFIGURATION
*
* Filename : http-s_rest_hook_cfg.c
* Version  : V3.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define   MICRIUM_SOURCE
#define   HTTPs_REST_HOOK_CFG_MODULE
#include "http-s_rest_hook_cfg.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           REST CONFIGURATION
*
* Note(s): The REST configuration in the REST resources list ID that the HTTP server will used for the
*          REST application.
*********************************************************************************************************
*/

const  HTTPs_REST_CFG  HTTPs_REST_Cfg = {0};


/*
*********************************************************************************************************
*                                         REST HOOK CONFIGURATION
*********************************************************************************************************
*/

const  HTTPs_HOOK_CFG HTTPs_REST_HookCfg = {
        HTTPsREST_Init,
        HTTPsREST_RxHeader,
        HTTPsREST_Authenticate,
        HTTPsREST_RxBody,
        HTTPsREST_ReqRdySignal,
        DEF_NULL,                                               /* Poll not used by REST. Same mechanism replaced by GET_CHUNK and RX_BODY */
        DEF_NULL,                                               /* Headers will be added before the chunk call */
        DEF_NULL,                                               /* No token replacement for REST */
        HTTPsREST_GetChunk,
        HTTPsREST_OnTransComplete,
        DEF_NULL,
        DEF_NULL,                                               /* If there is a connection error, it is most likely not recoverable. */
        HTTPsREST_OnConnClosed
};
