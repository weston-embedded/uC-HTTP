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
*                               HTTPs CONTROL LAYER REST CONFIGURATION
*
* Filename : http-s_ctrl_layer_rest_cfg.c
* Version  : V3.01.01
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
#define   HTTPs_CTRL_LAYER_REST_CFG_MODULE
#include "http-s_ctrl_layer_rest_cfg.h"
#include "../REST/http-s_rest.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*********************************************************************************************************
*/

const  HTTPs_CTRL_LAYER_APP_HOOKS HTTPsCtrlLayer_REST_App = {
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
        DEF_NULL,                                               /* If there is a connection error, it is most likely not recoverable. */
        HTTPsREST_OnConnClosed
};

