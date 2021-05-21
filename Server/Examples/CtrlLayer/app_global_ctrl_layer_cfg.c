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
*                            HTTP APPLICATION CONTROL LAYER CONFIGURATION
*
* Filename : app_global_ctrl_layer_cfg.c
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

#include "app_global.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                 Create a list of all authentications a user has to go through
*********************************************************************************************************
*/

HTTPs_CTRL_LAYER_AUTH_INST  *AppGlobal_AuthFilters[] = {
        &AppGlobal_AuthInst
};


/*
*********************************************************************************************************
*              Create a list of all the application an logged user can use.
*********************************************************************************************************
*/

HTTPs_CTRL_LAYER_APP_INST  *AppGlobal_AuthentifiedServices[] = {
        &AppGlobal_AppInst_AuthProtected,
        &AppGlobal_AppInst_REST,
        &AppGlobal_AppInst_Basic
};


/*
*********************************************************************************************************
*              Create a list of all the application an unlogged user can use.
*********************************************************************************************************
*/

HTTPs_CTRL_LAYER_APP_INST *AppGlobal_AuthApps[] = {
        &AppGlobal_AppInst_AuthUnprotected
};


/*
*********************************************************************************************************
*                    Create the configuration for the control layer.
*********************************************************************************************************
*/

HTTPs_CTRL_LAYER_CFG AppGlobal_ProtectedServices = {
        AppGlobal_AuthFilters,
        sizeof(AppGlobal_AuthFilters) / sizeof(HTTPs_CTRL_LAYER_AUTH_INST *),
        AppGlobal_AuthentifiedServices,
        sizeof(AppGlobal_AuthentifiedServices) / sizeof(HTTPs_CTRL_LAYER_AUTH_INST *),
};


/*
*********************************************************************************************************
*                    Create the configuration for the control layer
*********************************************************************************************************
*/

HTTPs_CTRL_LAYER_CFG AppGlobal_UnprotectedServices = {
        DEF_NULL,
        0,
        AppGlobal_AuthApps,
        sizeof(AppGlobal_AuthApps) / sizeof(HTTPs_CTRL_LAYER_APP_INST *),
};


/*
*********************************************************************************************************
*                    List the configurations in a priority order.
*********************************************************************************************************
*/

HTTPs_CTRL_LAYER_CFG  *AppGlobal_CtrlLayerCfgs[] = {
        &AppGlobal_ProtectedServices,
        &AppGlobal_UnprotectedServices
};


/*
*********************************************************************************************************
*                      Create the control layer configuration list.
*********************************************************************************************************
*/

HTTPs_CTRL_LAYER_CFG_LIST AppGlobal_CtrlLayerCfgList = {
        AppGlobal_CtrlLayerCfgs,
        sizeof(AppGlobal_CtrlLayerCfgs) / sizeof(HTTPs_CTRL_LAYER_CFG *)
};

