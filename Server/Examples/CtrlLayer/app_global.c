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
* Filename : app_global.c
* Version  : V3.01.01
*********************************************************************************************************
* Note(s)  : (1) This application regroup with the control layer a login layer, a default page app layer
*                and a REST app layer.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    APP_GLOBAL_MODULE

#include "app_global_auth_rights_cfg.h"    /* This include defines new rights, it must be before the authentication includes. */

#include  "app_global.h"
#include  "../REST/app_rest.h"
#include  "../Common/StaticFiles/generated_fs.h"

#include  <FS/net_fs.h>
#include  <Server/FS/Static/http-s_fs_static.h>

#include  <Server/Add-on/CtrlLayer/http-s_ctrl_layer_rest_cfg.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  LOGIN_PAGE_URL             "/login.html"
#define  MICRIUM_LOGO_URL           "/logo.gif"
#define  MICRIUM_CSS_URL            "/uc_style.css"
#define  INDEX_PAGE_URL             "/index.html"

#define  LOGIN_PAGE_CMD             "/logme"
#define  LOGOUT_PAGE_CMD            "/logout"
#define  FORM_PAGE_CMD              "/form_submit"

#define  FORM_USERNAME_FIELD_NAME   "username"
#define  FORM_PASSWORD_FIELD_NAME   "password"

#define  FORM_LOGOUT_FIELD_NAME     "Log out"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      CONTROL LAYER VARIABLES
*********************************************************************************************************
*/

                                                                /* ----------- AUTHENTICATION CONFIGURATION ----------- */
                                                                /* Set Authorization Configuration (Access Right).      */
HTTPs_AUTHORIZATION_CFG  AppGlobal_AuthInst_Cfg = {
        AppGlobal_Auth_GetRequiredRightsHook                    /* .GetRequiredRights: Authorization check function.    */
};

                                                                /* Set the Authentication Configuration.                */
HTTPs_AUTH_CFG  AppGlobal_AppInst_AuthCfg = {
        AppGlobal_Auth_ParseLoginHook,                          /* .ParseLogin: Fnct to parse rx credentials in login.  */
        AppGlobal_Auth_ParseLogoutHook                          /* .ParseLogout: Fnct to check for rx logout.           */
};

                                                                /* ------------- CTRL LAYER AUTH INSTANCE ------------- */
                                                                /* Set Authentication Instance.                         */
HTTPs_CTRL_LAYER_AUTH_INST  AppGlobal_AuthInst = {
        &HTTPsAuth_CookieHooksCfg,                              /* .HooksPtr: Authentication with Cookie Hooks Cfg.     */
        &AppGlobal_AuthInst_Cfg                                 /* .HooksCfgPtr: Pass the Authorization function.       */
};

                                                                /* --- CTRL LAYER APP INSTANCE FOR AUTH UNPROTECTED --- */
                                                                /* Set the App Instance for the Login part (Unprotected).*/
HTTPs_CTRL_LAYER_APP_INST  AppGlobal_AppInst_AuthUnprotected = {
        &HTTPsAuth_AppUnprotectedCookieHooksCfg,                /* .HooksPtr: Login App with Cookie Hooks Cfg.          */
        &AppGlobal_AppInst_AuthCfg                              /* .HooksCfgPtr: Pass the Application Authentication Cfg*/
};

                                                                /* ---- CTRL LAYER APP INSTANCE FOR AUTH PROTECTED ---- */
                                                                /* Set the App Instance for the Login part (Protected). */
HTTPs_CTRL_LAYER_APP_INST  AppGlobal_AppInst_AuthProtected = {
       &HTTPsAuth_AppProtectedCookieHooksCfg,                   /* .HooksPtr: Login App with Cookie Hooks Cfg.          */
       &AppGlobal_AppInst_AuthCfg                               /* .HooksCfgPtr: Pass the Application Authentication Cfg*/
};

                                                                /* --------- CTRL LAYER APP INSTANCE FOR REST --------- */
HTTPs_REST_CFG  RestCfg = { 0 };                                /* Simply the list index REST is going to use.          */

                                                                /* Set the Application Instance for the REST part.      */
HTTPs_CTRL_LAYER_APP_INST  AppGlobal_AppInst_REST = {
       (HTTPs_CTRL_LAYER_APP_HOOKS *)&HTTPsCtrlLayer_REST_App,  /* .HooksPtr: REST Application Hooks Cfg.               */
        &RestCfg                                                /* .HooksCfgPtr: Pass the REST  Cfg.                    */
};

                                                                /* --------- CTRL LAYER APP INSTANCE FOR BASIC -------- */
                                                                /* Set Hooks for Basic Application.                     */
HTTPs_CTRL_LAYER_APP_HOOKS  AppGlobal_BasicHooks = {
        DEF_NULL,
        DEF_NULL,
        AppGlobal_Basic_ReqHook,
        DEF_NULL,
        DEF_NULL,
        DEF_NULL,
        DEF_NULL,
        AppGlobal_Basic_TokenValGetHook,
        AppGlobal_Basic_RespChunkDataGetHook,
        DEF_NULL,
        DEF_NULL,
        DEF_NULL
};

                                                                /* Set the Application Instance for the other features. */
HTTPs_CTRL_LAYER_APP_INST  AppGlobal_AppInst_Basic = {
       &AppGlobal_BasicHooks,
        DEF_NULL
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  CPU_BOOLEAN  AppGlobal_UsersInit  (void);


/*
*********************************************************************************************************
*                                           AppGlobal_Init()
*
* Description : Initialize HTTPs REST Example application.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   if initialization was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  AppGlobal_Init (void)
{
    HTTPs_INSTANCE  *p_instance;
    CPU_BOOLEAN      result;
    HTTPs_ERR        err_https;


                                                                /* Initialize HTTPs Static File System.                 */
    HTTPs_FS_Init();

                                                                /* Add required files to the Static FS.                 */
    result = GeneratedFS_FileAdd();
    if (result != DEF_YES) {
        return (DEF_FAIL);
    }

    HTTPs_Init(DEF_NULL, &err_https);                           /* Initialize HTTPs suite.                              */
    if (err_https != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* Initialize HTTP Server users.                        */
    result = AppGlobal_UsersInit();
    if (result != DEF_YES) {
        return (DEF_FAIL);
    }

                                                                /* Initialize HTTPs Instance.                           */
    p_instance = HTTPs_InstanceInit(&HTTPs_CfgInstance_AppGlobal,
                                    &HTTPs_TaskCfgInstance_AppGlobal,
                                    &err_https);
    if (err_https != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

    result = AppREST_MemInit();
    if (result != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

    result = AppGlobal_REST_ResourcesInit();
    if (result != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* Start HTTPs Instance.                                */
    HTTPs_InstanceStart(p_instance, &err_https);
    if (err_https != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                           AppGlobal_REST_ResourcesInit()
*
* Description : Initialize HTTPs REST resources for the global application.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   if initialization was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : AppGlobal_Init().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  AppGlobal_REST_ResourcesInit (void)
{
    HTTPs_REST_ERR  err;


    HTTPsREST_Publish(&AppREST_List_Resource, 0, &err);
    if (err != HTTPs_REST_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPsREST_Publish(&AppREST_User_Resource, 0, &err);
    if (err != HTTPs_REST_ERR_NONE) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    AppGlobal_Auth_GetRequiredRightsHook()
*
* Description : Returns the rights required to fulfill the needs of a given request.
*
* Argument(s) : p_instance  Pointer to the HTTP server instance object.
*
*               p_conn      Pointer to the HTTP connection object.
*
* Return(s)   : returns the authorization right level for this example application.
*
* Caller(s)   : HTTPsAuth_OnAuth() via p_auth_cfg->GetRequiredRights()
*
* Note(s)     : none.
*********************************************************************************************************
*/

AUTH_RIGHT  AppGlobal_Auth_GetRequiredRightsHook (const  HTTPs_INSTANCE  *p_instance,
                                                  const  HTTPs_CONN      *p_conn)
{
    return (HTTP_USER_ACCESS);
}


/*
*********************************************************************************************************
*                                     AppGlobal_Auth_ParseLoginHook()
*
* Description : (1) Check all the HTTP requests received to see if they are related to resources of the login page.
*                   (a) Check if the POST login is received.
*                   (b) For each request set the redirect paths on no, invalid & valid credentials.
*                   (c) Parse the form fields received in the body for the user and password.
*
*
* Argument(s) : p_instance  Pointer to the HTTP server instance object.
*
*               p_conn      Pointer to the HTTP connection object.
*
*               state       State of the Authentication module:
*                               HTTPs_AUTH_STATE_REQ_URL:      The url and the headers were received and parse.
*                               HTTPs_AUTH_STATE_REQ_COMPLETE: All the request (url + headers + body) was received and parse.
*
*               p_result    Pointer to the authentication result structure to fill.
*
* Return(s)   : DEF_YES, if the request is the POST login.
*               DEF_NO,  otherwise.
*
* Caller(s)   : AppGlobal_AppInst_AuthCfg.
*
* Note(s)     : (2) This hook will be called twice for a request processed by the Authentication module:
*                   (a) When the Start line of the request (with the url) and the headers have been
*                       received and parse -> HTTPs_AUTH_STATE_REQ_URL state.
*                   (b) When all the request has been completely received and parse including the body
*                       -> HTTPs_AUTH_STATE_REQ_COMPLETE state.
*
*               (3) for each request received the redirect paths is set as follow:
*                   (a) RedirectPath_OnValidCred    INDEX_PAGE_URL
*                   (b) RedirectPath_OnInvalidCred  LOGIN_PAGE_URL
*                   (c) RedirectPath_OnNoCred
*                       (1) if the path is an unprotected path, let it go. (DEF_NULL) (i.e. the logo)
*                       (2) otherwise               LOGIN_PAGE_URL
*********************************************************************************************************
*/

CPU_BOOLEAN  AppGlobal_Auth_ParseLoginHook (const  HTTPs_INSTANCE     *p_instance,
                                            const  HTTPs_CONN         *p_conn,
                                                   HTTPs_AUTH_STATE    state,
                                                   HTTPs_AUTH_RESULT  *p_result)
{
    CPU_INT16S      cmp_val;
    CPU_BOOLEAN     is_login = DEF_NO;
#if (HTTPs_CFG_FORM_EN == DEF_ENABLED)
    HTTPs_KEY_VAL  *p_current;
    CPU_INT16S      cmp_val_username;
    CPU_INT16S      cmp_val_password;
#endif


    p_result->UsernamePtr = DEF_NULL;
    p_result->PasswordPtr = DEF_NULL;
                                                                /* Set redirect paths for each requests.                */
    p_result->RedirectPathOnInvalidCredPtr = LOGIN_PAGE_URL;
    p_result->RedirectPathOnValidCredPtr   = INDEX_PAGE_URL;

    switch (state) {
                                                                /* --------------- REQUEST URL RECEIVED --------------- */
        case HTTPs_AUTH_STATE_REQ_URL:
                                                                /* Set redirect paths for each requests.                */
             cmp_val = Str_Cmp(p_conn->PathPtr, LOGIN_PAGE_URL);
             if (cmp_val != 0) {
                 cmp_val = Str_Cmp(p_conn->PathPtr, MICRIUM_LOGO_URL);
                 if (cmp_val != 0) {
                     cmp_val = Str_Cmp(p_conn->PathPtr, MICRIUM_CSS_URL);
                 }
             }
             p_result->RedirectPathOnNoCredPtr = (cmp_val == 0) ? DEF_NULL : LOGIN_PAGE_URL;
                                                                /* Check if POST login received.                        */
             cmp_val = Str_Cmp(p_conn->PathPtr, LOGIN_PAGE_CMD);
             if (cmp_val == 0) {
                 is_login = DEF_YES;
             }
             break;

                                                                /* -------------- REQUEST BODY RECEIVED --------------- */
        case HTTPs_AUTH_STATE_REQ_COMPLETE:
#if (HTTPs_CFG_FORM_EN == DEF_ENABLED)
                                                                /* Parse form fields received for user/password.        */
             p_current = p_conn->FormDataListPtr;

             while ((p_current              != DEF_NULL)  &&
                    ((p_result->UsernamePtr == DEF_NULL)  ||
                     (p_result->PasswordPtr == DEF_NULL))) {

                 if (p_current->DataType == HTTPs_KEY_VAL_TYPE_PAIR) {
                     cmp_val_username = Str_CmpIgnoreCase_N(p_current->KeyPtr,
                                                            FORM_USERNAME_FIELD_NAME,
                                                            p_current->KeyLen);

                     cmp_val_password = Str_CmpIgnoreCase_N(p_current->KeyPtr,
                                                            FORM_PASSWORD_FIELD_NAME,
                                                            p_current->KeyLen);

                     if (cmp_val_username == 0) {
                         p_result->UsernamePtr = p_current->ValPtr;
                         is_login = DEF_YES;
                     } else if (cmp_val_password == 0) {
                         p_result->PasswordPtr = p_current->ValPtr;
                     }
                 }

                 p_current = p_current->NextPtr;
             }
#endif
             break;


        default:
             break;
    }

    return (is_login);
}


/*
*********************************************************************************************************
*                                     AppGlobal_Auth_ParseLogoutHook()
*
* Description : Parse requests received for logout URL and form data logout info.
*
* Argument(s) : p_instance  Pointer to HTTPs instance object.
*
*               p_conn      Pointer to HTTPs connection object.
*
*               state       State of the Authentication module:
*                               HTTPs_AUTH_STATE_REQ_URL:      The url and the headers were received and parse.
*                               HTTPs_AUTH_STATE_REQ_COMPLETE: All the request (url + headers + body) was received and parse.
*
* Return(s)   : DEF_YES, if Logout received.
*               DEF_NO,  otherwise.
*
* Caller(s)   : AppGlobal_AppInst_AuthCfg.
*
* Note(s)     : (1) This hook will be called twice for a request processed by the Authentication module:
*                   (a) When the Start line of the request (with the url) and the headers have been
*                       received and parse -> HTTPs_AUTH_STATE_REQ_URL state.
*                   (b) When all the request has been completely received and parse including the body
*                       -> HTTPs_AUTH_STATE_REQ_COMPLETE state.
*********************************************************************************************************
*/

CPU_BOOLEAN  AppGlobal_Auth_ParseLogoutHook (const  HTTPs_INSTANCE    *p_instance,
                                             const  HTTPs_CONN        *p_conn,
                                                    HTTPs_AUTH_STATE   state)
{
    CPU_BOOLEAN     is_logout = DEF_NO;
#if (HTTPs_CFG_FORM_EN == DEF_ENABLED)
    HTTPs_KEY_VAL  *p_current;
    CPU_INT16S       cmp_val;
#endif


    switch (state) {
                                                                /* --------------- REQUEST URL RECEIVED --------------- */
        case HTTPs_AUTH_STATE_REQ_URL:
                                                                /* Check if POST logout received.                       */
             cmp_val = Str_Cmp(p_conn->PathPtr, LOGOUT_PAGE_CMD);
             if (cmp_val == 0) {
                 is_logout = DEF_YES;
             }
             break;

                                                                /* -------------- REQUEST BODY RECEIVED --------------- */
        case HTTPs_AUTH_STATE_REQ_COMPLETE:
#if (HTTPs_CFG_FORM_EN == DEF_ENABLED)
                                                                /* Parse form fields received for logout.               */
             p_current = p_conn->FormDataListPtr;

             while (p_current != DEF_NULL) {
                 if (p_current->DataType == HTTPs_KEY_VAL_TYPE_PAIR) {
                     cmp_val = Str_CmpIgnoreCase_N(p_current->KeyPtr,
                                                   FORM_LOGOUT_FIELD_NAME,
                                                   p_current->KeyLen);
                     if (cmp_val == 0) {
                         is_logout = DEF_YES;
                         break;
                     }
                 }

                 p_current = p_current->NextPtr;
             }
#endif
             break;


        default:
             break;
    }

    return (is_logout);
}


/*
*********************************************************************************************************
*                                     AppGlobal_Basic_ReqHook()
*
* Description : Called when a HTTP request is received.
*               Allows to authorized/denied the request based on the URL.
*
* Argument(s) : p_instance  Pointer to the HTTPs instance object.
*
*               p_conn      Pointer to the HTTPs connection object.
*
*               p_hook_cfg  Pointer to hook configuration object.
*
* Return(s)   : DEF_OK,   if the request is allowed.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : AppGlobal_BasicHooks.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  AppGlobal_Basic_ReqHook (const  HTTPs_INSTANCE  *p_instance,
                                             HTTPs_CONN      *p_conn,
                                      const  void            *p_hook_cfg)
{
    CPU_INT16S  str_cmp;
    HTTPs_ERR   err;


    str_cmp = Str_Cmp_N(p_conn->PathPtr, FORM_PAGE_CMD, p_conn->PathLenMax);
    if (str_cmp == 0) {
                                                                /* Set Parameters to tx response body in chunk.         */
        HTTPs_RespBodySetParamStaticData(p_instance,
                                         p_conn,
                                         HTTP_CONTENT_TYPE_JSON,
                                         DEF_NULL,
                                         0,
                                         DEF_NO,
                                        &err);
        if (err != HTTPs_ERR_NONE) {
            return (DEF_FAIL);
        }
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   AppGlobal_Basic_TokenValGetHook()
*
* Description : Called for each ${TEXT_STRING} embedded variable found in a HTML document.
*
* Argument(s) : p_instance      Pointer to the HTTPs instance object.
*
*               p_conn          Pointer to the HTTPs connection object.
*
*               p_hook_cfg      Pointer to hook configuration object.
*
*               p_token         Pointer to the string that contains the value of the HTML embedded token.
*
*               token_len       Length of the embedded token.
*
*               p_val           Pointer to which buffer token value is copied to.
*
*               val_len_max     Maximum buffer length.
*
* Return(s)   : DEF_OK,   if token value copied successfully.
*               DEF_FAIL, otherwise (see Note #3).
*
* Caller(s)   : AppGlobal_BasicHooks.
*
* Note(s)     : (1) The instance structure MUST NOT be modified.
*
*               (2) The connection structure MUST NOT be modified manually since the response is about to be
*                   transmitted at this point. The only change to the connection structure should be the
*                   addition of header fields for the response message through the function HTTPs_RespHdrGet().
*
*               (3) If the token replacement failed, the token will be replaced by a line of tilde (~) of
*                   length equal to val_len_max.
*********************************************************************************************************
*/

CPU_BOOLEAN  AppGlobal_Basic_TokenValGetHook (const  HTTPs_INSTANCE  *p_instance,
                                                     HTTPs_CONN      *p_conn,
                                              const  void            *p_hook_cfg,
                                              const  CPU_CHAR        *p_token,
                                                     CPU_INT16U       token_len,
                                                     CPU_CHAR        *p_val,
                                                     CPU_INT16U       val_len_max)
{
    static  CPU_CHAR    buf[20];
            CPU_INT32U  ver;


    if (Str_Cmp_N(p_token, "NET_VERSION", 11) == 0) {
#if (NET_VERSION >  205u)
        ver =  NET_VERSION / 10000;
       (void)Str_FmtNbr_Int32U(ver,  2, DEF_NBR_BASE_DEC, ' ', DEF_NO, DEF_NO,  &buf[0]);
        buf[2] = '.';

        ver = (NET_VERSION /   100) % 100;
       (void)Str_FmtNbr_Int32U(ver,  2, DEF_NBR_BASE_DEC, '0', DEF_NO, DEF_NO,  &buf[3]);
        buf[5] = '.';

        ver = (NET_VERSION /     1) % 100;
       (void)Str_FmtNbr_Int32U(ver,  2, DEF_NBR_BASE_DEC, '0', DEF_NO, DEF_YES, &buf[6]);
        buf[8] = '\0';

#else
        ver =  NET_VERSION /   100;
       (void)Str_FmtNbr_Int32U(ver,  2, DEF_NBR_BASE_DEC, ' ', DEF_NO, DEF_NO,  &buf[0]);
        buf[2] = '.';

        ver = (NET_VERSION /     1) % 100;
       (void)Str_FmtNbr_Int32U(ver,  2, DEF_NBR_BASE_DEC, '0', DEF_NO, DEF_YES, &buf[3]);
        buf[5] = '\0';
#endif

    } else if (Str_Cmp_N(p_token, "HTTPs_VERSION", 13) == 0) {
        ver =  HTTPs_VERSION / 10000;
       (void)Str_FmtNbr_Int32U(ver,  2, DEF_NBR_BASE_DEC, ' ', DEF_NO, DEF_NO,  &buf[0]);
        buf[2] = '.';

        ver = (HTTPs_VERSION /   100) % 100;
       (void)Str_FmtNbr_Int32U(ver,  2, DEF_NBR_BASE_DEC, '0', DEF_NO, DEF_NO,  &buf[3]);
        buf[5] = '.';

        ver = (HTTPs_VERSION /     1) % 100;
       (void)Str_FmtNbr_Int32U(ver,  2, DEF_NBR_BASE_DEC, '0', DEF_NO, DEF_YES, &buf[6]);
        buf[8] = '\0';
    }

    Str_Copy_N(p_val, &buf[0], val_len_max);


    (void)p_instance;                                           /* Prevent 'variable unused' compiler warning.          */
    (void)p_conn;
    (void)token_len;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                  AppGlobal_Basic_RespChunkDataGetHook()
*
* Description : Called to get the application data to put in the body when transferring in chunk.
*
* Argument(s) : p_instance      Pointer to the HTTPs instance object.
*
*               p_conn          Pointer to the HTTPs connection object.
*
*               p_hook_cfg      Pointer to hook configuration object.
*
*               p_buf           Pointer to the buffer to fill.
*
*               buf_len_max     Maximum length the buffer can contain.
*
*               p_tx_len        Variable that will received the length written in the buffer.
*
* Return(s)   : DEF_YES      if there is no more data to send.
*               DEF_NO       otherwise.
*
* Caller(s)   : AppGlobal_BasicHooks.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  AppGlobal_Basic_RespChunkDataGetHook (const  HTTPs_INSTANCE  *p_instance,
                                                          HTTPs_CONN      *p_conn,
                                                   const  void            *p_hook_cfg,
                                                          void            *p_buf,
                                                          CPU_SIZE_T       buf_len_max,
                                                          CPU_SIZE_T      *p_tx_len)
{
#if (HTTPs_CFG_FORM_EN == DEF_ENABLED)
    const  HTTPs_CFG      *p_cfg = p_instance->CfgPtr;
           HTTPs_KEY_VAL  *p_key_val;
           CPU_INT16S      str_cmp;


    str_cmp = Str_Cmp_N(p_conn->PathPtr, FORM_PAGE_CMD, p_conn->PathLenMax);
    if (str_cmp == 0) {
                                                                /* Construct JSON for user                              */
        Str_Copy(p_buf, "{\"user\":{\"first name\":\"");        /* Add First Name field.                                */
        p_key_val = p_conn->FormDataListPtr;
        while (p_key_val != DEF_NULL) {
            str_cmp = Str_Cmp_N(p_key_val->KeyPtr, "firstname", p_cfg->FormCfgPtr->KeyLenMax);
            if(str_cmp == 0) {
                Str_Cat_N(p_buf, p_key_val->ValPtr, p_key_val->ValLen);
                break;
            }
            p_key_val = p_key_val->NextPtr;
        }

        Str_Cat(p_buf, "\", \"last name\":\"");                 /* Add Last Name field.                                 */
        p_key_val = p_conn->FormDataListPtr;
        while (p_key_val != DEF_NULL) {
            str_cmp = Str_Cmp_N(p_key_val->KeyPtr, "lastname", p_cfg->FormCfgPtr->KeyLenMax);
            if(str_cmp == 0) {
                Str_Cat_N(p_buf, p_key_val->ValPtr, p_key_val->ValLen);
                break;
            }
            p_key_val = p_key_val->NextPtr;
        }
        Str_Cat(p_buf, "\"}}");
    }

   *p_tx_len = Str_Len_N(p_buf, p_cfg->BufLen);

#else
    CPU_SW_EXCEPTION(;);
#endif

    return (DEF_YES);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         AppGlobal_UsersInit()
*
* Description : Adds three users to the authentication system.
*               (1) Username:   admin
*                   Password:   password
*                   Rights:     Manager of HTTP user access right
*
*               (2) Username:   user0
*                   Password:
*                   Rights:     HTTP user
*
*               (3) Username:   user1
*                   Password:   user1
*                   Rights:     HTTP user
*
* Argument(s) : none.
*
* Return(s)   : DEF_OK,   if users initialization was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  AppGlobal_UsersInit (void)
{
    AUTH_USER    admin;
    AUTH_USER    user;
    AUTH_RIGHT   right;
    RTOS_ERR     err_auth;
    CPU_BOOLEAN  result;


    result = Auth_Init(&err_auth);
    if (result != DEF_OK) {
        return (DEF_FAIL);
    }

    result = Auth_CreateUser("admin",
                             "password",
                             &admin,
                             &err_auth);
    if (result != DEF_OK) {
        return (DEF_FAIL);
    }

    right = (HTTP_USER_ACCESS | AUTH_RIGHT_MNG);
    result = Auth_GrantRight(right,
                            &admin,
                            &Auth_RootUser,
                            &err_auth);
    if (result != DEF_OK) {
        return (DEF_FAIL);
    }

    result = Auth_CreateUser("user0",
                             "",
                             &user,
                             &err_auth);
    if (result != DEF_OK) {
        return (DEF_FAIL);
    }

    result = Auth_GrantRight(HTTP_USER_ACCESS,
                            &user,
                            &admin,
                            &err_auth);
    if (result != DEF_OK) {
        return (DEF_FAIL);
    }

    result = Auth_CreateUser("user1",
                             "user1",
                             &user,
                             &err_auth);
    if (result != DEF_OK) {
        return (DEF_FAIL);
    }

    result = Auth_GrantRight(HTTP_USER_ACCESS,
                            &user,
                            &admin,
                            &err_auth);
    if (result != DEF_OK) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}
