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
*                                    HTTPs AUTHENTIFICATION MODULE
*
* Filename : http-s_auth.h
* Version  : V3.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef HTTPS_AUTH_MODULE_PRESENT
#define HTTPS_AUTH_MODULE_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <Auth/auth.h>
#include  "../CtrlLayer/http-s_ctrl_layer.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  struct  https_auth_result {
    CPU_CHAR   *RedirectPathOnValidCredPtr;
    CPU_CHAR   *RedirectPathOnInvalidCredPtr;
    CPU_CHAR   *RedirectPathOnNoCredPtr;
    CPU_CHAR   *UsernamePtr;
    CPU_CHAR   *PasswordPtr;
}  HTTPs_AUTH_RESULT;


typedef enum https_auth_state {
    HTTPs_AUTH_STATE_REQ_URL,
    HTTPs_AUTH_STATE_REQ_COMPLETE
} HTTPs_AUTH_STATE;


typedef  CPU_BOOLEAN        (*HTTPs_AUTH_PARSE_LOGIN_FNCT)            (const  HTTPs_INSTANCE     *p_inst,
                                                                       const  HTTPs_CONN         *p_conn,
                                                                              HTTPs_AUTH_STATE    state,
                                                                              HTTPs_AUTH_RESULT  *p_result);

typedef  CPU_BOOLEAN        (*HTTPs_AUTH_PARSE_LOGOUT_FNCT)           (const  HTTPs_INSTANCE     *p_inst,
                                                                       const  HTTPs_CONN         *p_conn,
                                                                              HTTPs_AUTH_STATE    state);

typedef  AUTH_RIGHT         (*HTTPs_AUTH_GET_REQUIRED_RIGHT_FNCT)     (const  HTTPs_INSTANCE     *p_inst,
                                                                       const  HTTPs_CONN         *p_conn);


typedef  struct  HTTPs_Authentication_Cfg {
    HTTPs_AUTH_PARSE_LOGIN_FNCT             ParseLogin;
    HTTPs_AUTH_PARSE_LOGOUT_FNCT            ParseLogout;
} HTTPs_AUTH_CFG;


typedef  struct  HTTPs_Authorization_Cfg {
    HTTPs_AUTH_GET_REQUIRED_RIGHT_FNCT      GetRequiredRights;
} HTTPs_AUTHORIZATION_CFG;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

extern  HTTPs_CTRL_LAYER_AUTH_HOOKS  HTTPsAuth_CookieHooksCfg;
extern  HTTPs_CTRL_LAYER_APP_HOOKS   HTTPsAuth_AppUnprotectedCookieHooksCfg;
extern  HTTPs_CTRL_LAYER_APP_HOOKS   HTTPsAuth_AppProtectedCookieHooksCfg;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

CPU_BOOLEAN     HTTPsAuth_InitSessionPool     (const  HTTPs_INSTANCE     *p_instance,
                                               const  void               *p_hook_cfg);

CPU_BOOLEAN     HTTPsAuth_OnHdrRx             (const  HTTPs_INSTANCE     *p_instance,
                                               const  HTTPs_CONN         *p_conn,
                                               const  void               *p_hook_cfg,
                                                      HTTP_HDR_FIELD      hdr_field);

CPU_BOOLEAN     HTTPsAuth_OnHdrTx             (const  HTTPs_INSTANCE     *p_instance,
                                                      HTTPs_CONN         *p_conn,
                                               const  void               *p_cfg);

CPU_BOOLEAN     HTTPsAuth_OnAuth              (const  HTTPs_INSTANCE     *p_instance,
                                                      HTTPs_CONN         *p_conn,
                                               const  void               *p_hook_cfg);

void            HTTPsAuth_OnTransComplete     (const  HTTPs_INSTANCE     *p_instance,
                                                      HTTPs_CONN         *p_conn,
                                               const  void               *p_hook_cfg);

void            HTTPsAuth_OnConnClose         (const  HTTPs_INSTANCE     *p_instance,
                                                      HTTPs_CONN         *p_conn,
                                               const  void               *p_hook_cfg);

CPU_BOOLEAN     HTTPsAuth_ProcessSession      (const  HTTPs_INSTANCE     *p_instance,
                                                      HTTPs_CONN         *p_conn,
                                               const  void               *p_hook_cfg,
                                               const  HTTPs_KEY_VAL      *p_data);

CPU_BOOLEAN     HTTPsAuth_ReqUnprotected      (const  HTTPs_INSTANCE     *p_instance,
                                                      HTTPs_CONN         *p_conn,
                                               const  void               *p_hook_cfg);

CPU_BOOLEAN     HTTPsAuth_ReqProtected        (const  HTTPs_INSTANCE     *p_instance,
                                                      HTTPs_CONN         *p_conn,
                                               const  void               *p_hook_cfg);

CPU_BOOLEAN     HTTPsAuth_ReqRdyUnprotected   (const  HTTPs_INSTANCE     *p_instance,
                                                      HTTPs_CONN         *p_conn,
                                               const  void               *p_hook_cfg,
                                               const  HTTPs_KEY_VAL      *p_data);

CPU_BOOLEAN     HTTPsAuth_ReqRdyProtected     (const  HTTPs_INSTANCE     *p_instance,
                                                      HTTPs_CONN         *p_conn,
                                               const  void               *p_hook_cfg,
                                               const  HTTPs_KEY_VAL      *p_data);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif /* HTTPS_AUTH_MODULE_PRESENT */
