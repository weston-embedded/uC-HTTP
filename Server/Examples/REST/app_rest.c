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
*                                        HTTP REST APPLICATION
*
* Filename : app_rest.c
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

#define    MICRIUM_SOURCE
#define    APP_REST_MODULE
#include  "app_rest.h"

#include  <Server/Add-on/REST/http-s_rest.h>

#include  <FS/net_fs.h>
#include  <Server/FS/Static/http-s_fs_static.h>
#include  "../Common/StaticFiles/generated_fs.h"

#include  "app_rest_http-s_instance_cfg.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        APP CONFIGURATION DEFINES - CAN BE MODIFIED
*********************************************************************************************************
*/

#define  APP_REST_USER_MAX_NBR                       10u

#define  APP_REST_LINK_STR_MAX_LEN                  100u
#define  APP_REST_FIRST_NAME_STR_MAX_LEN             50u
#define  APP_REST_LAST_NAME_STR_MAX_LEN              50u
#define  APP_REST_JOB_TITLE_STR_MAX_LEN             100u


/*
*********************************************************************************************************
*                                             OTHER DEFINES
*********************************************************************************************************
*/

#define  APP_REST_GENDER_STR_MAX_LEN             7u
#define  APP_REST_AGE_STR_MAX_LEN                4u


/*
*********************************************************************************************************
*                                            JSON DEFINES
*********************************************************************************************************
*/

#define  APP_REST_JSON_OBJ_CHARS_NBR                2u
#define  APP_REST_JSON_TBL_CHARS_NBR                2u
#define  APP_REST_JSON_FIELD_MIN_CHARS_NBR          6u
#define  APP_REST_JSON_FIELD_MAX_CHARS_NBR          8u

#define  APP_REST_JSON_LIST_USER_START_STR          "{\"Users\":["
#define  APP_REST_JSON_LIST_USER_END_STR            "]}"

#define  APP_REST_JSON_KEY_ID_STR_NAME              "User ID"
#define  APP_REST_JSON_KEY_LINK_STR_NAME            "Link"
#define  APP_REST_JSON_KEY_FIRST_NAME_STR_NAME      "First Name"
#define  APP_REST_JSON_KEY_LAST_NAME_STR_NAME       "Last Name"
#define  APP_REST_JSON_KEY_GENDER_STR_NAME          "Gender"
#define  APP_REST_JSON_KEY_AGE_STR_NAME             "Age"
#define  APP_REST_JSON_KEY_JOB_STR_NAME             "Job Title"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       USER INFO FIELD DATA TYPE
*********************************************************************************************************
*/

typedef  enum  AppREST_UserField {
    APP_REST_USER_FIELD_ID,
    APP_REST_USER_FIELD_LINK,
    APP_REST_USER_FIELD_FIRST_NAME,
    APP_REST_USER_FIELD_LAST_NAME,
    APP_REST_USER_FIELD_GENDER,
    APP_REST_USER_FIELD_AGE,
    APP_REST_USER_FIELD_JOB
} APP_REST_USER_FIELD;


/*
*********************************************************************************************************
*                                           USER DATA TYPE
*
* Note(s): (1) The user data type will be use to create a double chain list of users.
*********************************************************************************************************
*/

typedef  struct  AppREST_User APP_REST_USER;

struct  AppREST_User {
    CPU_INT32U      ID;
    CPU_CHAR        Link[APP_REST_LINK_STR_MAX_LEN];
    CPU_CHAR        FirstName[APP_REST_FIRST_NAME_STR_MAX_LEN];
    CPU_CHAR        LastName[APP_REST_LAST_NAME_STR_MAX_LEN];
    CPU_CHAR        Gender[APP_REST_GENDER_STR_MAX_LEN];
    CPU_INT08U      Age;
    CPU_CHAR        JobTitle[APP_REST_JOB_TITLE_STR_MAX_LEN];
    CPU_INT32U      RdRefCtr;
    CPU_INT32U      WrRefCtr;
    APP_REST_USER  *PrevPtr;
    APP_REST_USER  *NextPtr;
};


/*
*********************************************************************************************************
*                                       FREE USER ID DATA TYPE
*
* Note(s): (1) The free user ID data type will be use to create a simple chain list of Free ID.
*
*          (2) Free ID are ID that have already been used but are now free because the user was deleted.
*
*          (3) This allows to reuse ID from deleted user.
*********************************************************************************************************
*/

typedef  struct  AppREST_FreeUserID  APP_REST_FREE_USER_ID;

struct  AppREST_FreeUserID {
    CPU_INT32U              ID;
    APP_REST_FREE_USER_ID  *NextPtr;
};


/*
*********************************************************************************************************
*                                     APPLICATION DATA DATA TYPE
*
* Notes(s): (1) The application data structure is used to store some information relative to one HTTP
*               transaction that must be accessible through all the calls to the same hook function.
*
*               For example, when a request is received to get a specific user infos, in the Init state,
*               we will try to found that user and store a the user pointer in the app data so that when
*               we fall in the TX state we can retrive the user pointer.
*********************************************************************************************************
*/

typedef  struct  AppREST_Data  APP_REST_DATA;

struct  AppREST_Data {
    APP_REST_USER        *UserPtr;          /* Current user being process by the request.                   */
    APP_REST_USER_FIELD   FieldType;        /* Store the current user info field that must be transmitted.  */
    APP_REST_DATA        *NextPtr;
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE   AppREST_GetPage               (const  HTTPs_REST_RESOURCE      *p_resource,
                                                               const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                               const  HTTPs_REST_STATE          state,
                                                                      void                    **p_data,
                                                               const  HTTPs_INSTANCE           *p_instance,
                                                                      HTTPs_CONN               *p_conn,
                                                                      void                     *p_buf,
                                                               const  CPU_SIZE_T                buf_len,
                                                                      CPU_SIZE_T               *p_buf_len_used);

static  HTTPs_REST_HOOK_STATE   AppREST_GetUserListHook       (const  HTTPs_REST_RESOURCE      *p_resource,
                                                               const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                               const  HTTPs_REST_STATE          state,
                                                                      void                    **p_data,
                                                               const  HTTPs_INSTANCE           *p_instance,
                                                                      HTTPs_CONN               *p_conn,
                                                                      void                     *p_buf,
                                                               const  CPU_SIZE_T                buf_len,
                                                                      CPU_SIZE_T               *p_buf_len_used);

static  HTTPs_REST_HOOK_STATE   AppREST_GetUserInfoHook       (const  HTTPs_REST_RESOURCE      *p_resource,
                                                               const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                               const  HTTPs_REST_STATE          state,
                                                                      void                    **p_data,
                                                               const  HTTPs_INSTANCE           *p_instance,
                                                                      HTTPs_CONN               *p_conn,
                                                                      void                     *p_buf,
                                                               const  CPU_SIZE_T                buf_len,
                                                                      CPU_SIZE_T               *p_buf_len_used);

static  HTTPs_REST_HOOK_STATE   AppREST_SetUserInfoHook       (const  HTTPs_REST_RESOURCE      *p_resource,
                                                               const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                               const  HTTPs_REST_STATE          state,
                                                                      void                    **p_data,
                                                               const  HTTPs_INSTANCE           *p_instance,
                                                                      HTTPs_CONN               *p_conn,
                                                                      void                     *p_buf,
                                                               const  CPU_SIZE_T                buf_len,
                                                                      CPU_SIZE_T               *p_buf_len_used);

static  HTTPs_REST_HOOK_STATE   AppREST_CreateUserHook        (const  HTTPs_REST_RESOURCE      *p_resource,
                                                               const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                               const  HTTPs_REST_STATE          state,
                                                                      void                    **p_data,
                                                               const  HTTPs_INSTANCE           *p_instance,
                                                                      HTTPs_CONN               *p_conn,
                                                                      void                     *p_buf,
                                                               const  CPU_SIZE_T                buf_len,
                                                                      CPU_SIZE_T               *p_buf_len_used);

static  HTTPs_REST_HOOK_STATE   AppREST_DeleteUserHook        (const  HTTPs_REST_RESOURCE      *p_resource,
                                                               const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                               const  HTTPs_REST_STATE          state,
                                                                      void                    **p_data,
                                                               const  HTTPs_INSTANCE           *p_instance,
                                                                      HTTPs_CONN               *p_conn,
                                                                      void                     *p_buf,
                                                               const  CPU_SIZE_T                buf_len,
                                                                      CPU_SIZE_T               *p_buf_len_used);

static  CPU_SIZE_T              AppREST_JSON_GetUserFieldSize (       APP_REST_USER            *p_user);

static  CPU_SIZE_T              AppREST_JSON_GetUserInfoSize  (       APP_REST_USER            *p_user);

static  HTTPs_REST_HOOK_STATE   AppREST_JSON_WrUserToBuf      (       APP_REST_USER            *p_user,
                                                                      void                     *p_buf,
                                                               const  CPU_SIZE_T                buf_len,
                                                               const  CPU_SIZE_T                buf_len_max,
                                                                      CPU_SIZE_T               *p_buf_len_used);

static  HTTPs_REST_HOOK_STATE   AppREST_JSON_WrUserListToBuf  (       APP_REST_DATA            *p_app_data,
                                                                      void                     *p_buf,
                                                               const  CPU_SIZE_T                buf_len,
                                                               const  CPU_SIZE_T                buf_len_max,
                                                                      CPU_SIZE_T               *p_buf_len_used);

static  HTTPs_REST_HOOK_STATE   AppREST_JSON_WrUserInfoToBuf  (       APP_REST_USER            *p_user,
                                                                      void                     *p_buf,
                                                               const  CPU_SIZE_T                buf_len,
                                                               const  CPU_SIZE_T                buf_len_max,
                                                                      CPU_SIZE_T               *p_buf_len_used);

static  HTTPs_REST_HOOK_STATE   AppREST_JSON_ParseUser        (       APP_REST_DATA            *p_app_data,
                                                                      APP_REST_USER            *p_user,
                                                               const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                                      void                     *p_buf,
                                                               const  CPU_SIZE_T                buf_len,
                                                               const  CPU_SIZE_T                buf_len_max,
                                                                      CPU_SIZE_T               *p_buf_len_used);

static  HTTPs_REST_HOOK_STATE   AppREST_JSON_ParseUserInfo    (       APP_REST_DATA            *p_app_data,
                                                                      APP_REST_USER            *p_user,
                                                                      void                     *p_buf,
                                                               const  CPU_SIZE_T                data_len,
                                                               const  CPU_SIZE_T                buf_len_max,
                                                                      CPU_SIZE_T               *p_buf_len_used);


static  HTTPs_REST_HOOK_STATE   AppREST_JSON_ParseField       (       APP_REST_USER_FIELD       field_type,
                                                                      CPU_CHAR                **p_val_found,
                                                                      CPU_SIZE_T               *p_val_len,
                                                                      void                     *p_buf,
                                                               const  CPU_SIZE_T                data_len,
                                                               const  CPU_SIZE_T                buf_len_max,
                                                                      CPU_SIZE_T               *p_buf_len_used);

static  APP_REST_USER          *AppREST_FindUser             (       CPU_INT08U                user_id);

static  APP_REST_USER          *AppREST_GetUser               (void);

static  void                    AppREST_RemoveUser            (       APP_REST_USER            *p_user);

static  CPU_INT08U              AppREST_GetUserID             (void);

static  APP_REST_FREE_USER_ID  *AppREST_GetFreeUserID         (       CPU_INT08U                user_id);

static  void                    AppREST_RemoveFreeUserID      (void);

static  APP_REST_DATA          *AppREST_GetDataBlk            (void);

static  void                    AppREST_RemoveDataBlk         (       APP_REST_DATA            *p_app_data);

static  CPU_SIZE_T              AppREST_GetStrLenOfIntDec     (       CPU_INT32U                i);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      REST RESOURCE VARIABLES
*********************************************************************************************************
*/

                                            /* Resource associated with the User List.      */
HTTPs_REST_RESOURCE AppREST_List_Resource = {
        "/users",                           /* Resource URI                 */
        DEF_NULL,                           /* No HTTP headers to keep.     */
        0,                                  /* No HTTP headers to keep.     */
        {
                DEF_NULL,                   /* Method Delete                */
               &AppREST_GetUserListHook,    /* Method Get                   */
                DEF_NULL,                   /* Method Head, Not supported   */
               &AppREST_CreateUserHook,     /* Method Post                  */
                DEF_NULL                    /* Method Put                   */
        }
};

                                            /* Resource associated with a specific user.    */
HTTPs_REST_RESOURCE AppREST_User_Resource = {
        "/users/{user_id}",
        DEF_NULL,
        0,
        {
               &AppREST_DeleteUserHook,     /* Method Delete                */
               &AppREST_GetUserInfoHook,    /* Method Get                   */
                DEF_NULL,                   /* Method Head, Not supported   */
                DEF_NULL,                   /* Method Post                  */
               &AppREST_SetUserInfoHook     /* Method Put                   */
        }
};


                                            /* Resource associated with files.              */
HTTPs_REST_RESOURCE AppREST_File_Resource = {
        "/{file}",
        DEF_NULL,
        0,
        {
                DEF_NULL,                   /* Method Delete                */
               &AppREST_GetPage,            /* Method Get                   */
                DEF_NULL,                   /* Method Head, Not supported   */
                DEF_NULL,                   /* Method Post                  */
                DEF_NULL                    /* Method Put                   */
        }
};


/*
*********************************************************************************************************
*                                       APP DATA VARIABLES
*********************************************************************************************************
*/

static  MEM_DYN_POOL            AppREST_UserPool;               /* Pool of user objects.                                */
static  MEM_DYN_POOL            AppREST_FreeUserID_Pool;        /* Pool of Free User ID objects.                        */
static  MEM_DYN_POOL            AppREST_DataPool;               /* Pool of app data objects.                            */

static  CPU_INT32U              AppREST_UserCtr;                /* Counter of the current number of users.              */

static  APP_REST_USER          *AppREST_UserFirstPtr;           /* Pointer to the Head of the user list.                */
static  APP_REST_USER          *AppREST_UserLastPtr;            /* Pointer to the End of the user list.                 */
static  APP_REST_FREE_USER_ID  *AppREST_FreeUserID_ListPtr;     /* Pointer to the Head of the Free user ID list.        */
static  APP_REST_DATA          *AppREST_DataListPtr;            /* Pointer to the Head of the app data object list.     */

static  CPU_CHAR               *AppREST_UserID_Str;             /* Pointer to the mem seg to temp copy the ID to str.   */

static  CPU_INT32U              AppREST_ListRdRefCtr;           /* Counter for the nbr of req accessing the list in rd. */

static  CPU_INT32U              AppREST_ListWrRefCtr;           /* Counter for the nbr of req accessing the list in wr. */


/*
*********************************************************************************************************
*                                            AppREST_Init()
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

CPU_BOOLEAN  AppREST_Init (void)
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


                                                                /* Initialize HTTPs Instance.                           */
    p_instance = HTTPs_InstanceInit(&HTTPs_CfgInstance_REST,
                                    &HTTPs_TaskCfgInstance_REST,
                                    &err_https);
    if (err_https != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

    result = AppREST_MemInit();
    if (result != DEF_YES) {
        return (DEF_FAIL);
    }

    result = AppREST_ResourcesInit();
    if (result != DEF_YES) {
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
*                                           AppREST_MemInit()
*
* Description : Allocate the necessary memory for the REST application.
*
* Argument(s) : none.
*
* Return(s)   : DEF_OK,   if initialization was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : AppREST_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  AppREST_MemInit (void)
{
    CPU_SIZE_T      char_nbr;
    LIB_ERR         err_lib;


    AppREST_UserCtr            = 0;
    AppREST_UserFirstPtr       = DEF_NULL;
    AppREST_UserLastPtr        = DEF_NULL;
    AppREST_FreeUserID_ListPtr = DEF_NULL;
    AppREST_DataListPtr        = DEF_NULL;

    char_nbr           = AppREST_GetStrLenOfIntDec(APP_REST_USER_MAX_NBR);
    AppREST_UserID_Str = Mem_SegAlloc("User ID String", DEF_NULL, char_nbr, &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (DEF_FAIL);
    }

    Mem_DynPoolCreate("User Pool",
                      &AppREST_UserPool,
                       DEF_NULL,
                       sizeof(APP_REST_USER),
                       sizeof(CPU_SIZE_T),
                       1,
                       APP_REST_USER_MAX_NBR,
                      &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (DEF_FAIL);
    }

    Mem_DynPoolCreate("Free User ID Pool",
                      &AppREST_FreeUserID_Pool,
                       DEF_NULL,
                       sizeof(APP_REST_FREE_USER_ID),
                       sizeof(CPU_SIZE_T),
                       1,
                       APP_REST_USER_MAX_NBR,
                      &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (DEF_FAIL);
    }

    Mem_DynPoolCreate("App Data Pool",
                      &AppREST_DataPool,
                       DEF_NULL,
                       sizeof(APP_REST_DATA),
                       sizeof(CPU_SIZE_T),
                       1,
                       APP_REST_HTTPs_CONN_NBR_MAX,
                      &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                          AppREST_ResourcesInit()
*
* Description : Initialize the REST application resources.
*
* Argument(s) : none.
*
* Return(s)   : DEF_OK,   if initialization was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : AppREST_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  AppREST_ResourcesInit (void)
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

    HTTPsREST_Publish(&AppREST_File_Resource, 0, &err);
    if (err != HTTPs_REST_ERR_NONE) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
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
*                                           AppREST_GetPage()
*
* Description : Hook function that will be called when GET request is received with the any URI that is not
*               "/users" or /users/{user_id}.
*               Therefore, this hook function is to catch all the HTTP requests that are not specific to REST
*               but are related to the web page browsing, for example get html/js/image files for the view in
*               the web browser.
*
* Argument(s) : p_resource      Pointer to REST resource.
*
*               p_uri           Pointer to the REST URI of the resource.
*
*               state           REST State :
*                                   HTTPs_REST_STATE_INIT:  Bound to the initialize hook of the HTTP server (OnInstanceInitHook).
*                                   HTTPs_REST_STATE_RX:    Bound to the RX hook of the HTTP server (OnReqBodyRxHook).
*                                   HTTPs_REST_STATE_TX:    Bound to the ready hook (OnReqRdySignalHook) and the TX chunk hook (OnRespChunkHook).
*                                   HTTPs_REST_STATE_CLOSE: Bound to the 'OnTransCompleteHook' and 'OnConnCloseHook'.
*                                   HTTPs_REST_STATE_ERROR: Should not occur.
*
*               p_data          Pointer to the connection application data pointer.
*
*               p_instance      Pointer to the HTTP server instance object.
*
*               p_conn          Pointer to the HTTP connection object.
*
*               p_buf           Pointer to the connection buffer to read/write data.
*
*               buf_len         RX state: Data length available to be read.
*                               TX state: Buffer length available to write.
*
*               p_buf_len_used  RX state: Must be updated by the hook to indicate the length of the data read by the app.
*                               TX state: Must be updated by the hook to indicate the length of the data written by the app.
*
* Return(s)   : REST hook state:
*                   HTTPs_REST_HOOK_STATE_CONTINUE, to continue the transaction processing.
*                   HTTPs_REST_HOOK_STATE_STAY,     to fall back at the same stage the next time the hook is called.
*                   HTTPs_REST_HOOK_STATE_ERROR,    when an error occurred in the hook processing.
*
* Caller(s)   : HTTP server core.
*
* Note(s)     : (1) Since the default behavior of the HTTP core is to serve web pages, this hook function
*                   does not do anything and let the server core process the request.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE  AppREST_GetPage (const  HTTPs_REST_RESOURCE      *p_resource,
                                                const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                const  HTTPs_REST_STATE          state,
                                                       void                    **p_data,
                                                const  HTTPs_INSTANCE           *p_instance,
                                                       HTTPs_CONN               *p_conn,
                                                       void                     *p_buf,
                                                const  CPU_SIZE_T                buf_len,
                                                       CPU_SIZE_T               *p_buf_len_used)
{
    return (HTTPs_REST_HOOK_STATE_CONTINUE);                    /* See note #1.                                         */
}


/*
*********************************************************************************************************
*                                       AppREST_GetUserListHook()
*
* Description : Hook function that will be called when GET request is received with the "/users" URI.
*               This function, will set the response body data to a JSON with the user list.
*
* Argument(s) : p_resource      Pointer to REST resource.
*
*               p_uri           Pointer to the REST URI of the resource.
*
*               state           REST State :
*                                   HTTPs_REST_STATE_INIT:  Bound to the initialize hook of the HTTP server (OnInstanceInitHook).
*                                   HTTPs_REST_STATE_RX:    Bound to the RX hook of the HTTP server (OnReqBodyRxHook).
*                                   HTTPs_REST_STATE_TX:    Bound to the ready hook (OnReqRdySignalHook) and the TX chunk hook (OnRespChunkHook).
*                                   HTTPs_REST_STATE_CLOSE: Bound to the 'OnTransCompleteHook' and 'OnConnCloseHook'.
*                                   HTTPs_REST_STATE_ERROR: Should not occur.
*
*               p_data          Pointer to the connection application data pointer.
*
*               p_instance      Pointer to the HTTP server instance object.
*
*               p_conn          Pointer to the HTTP connection object.
*
*               p_buf           Pointer to the connection buffer to read/write data.
*
*               buf_len         RX state: Data length available to be read.
*                               TX state: Buffer length available to write.
*
*               p_buf_len_used  RX state: Must be updated by the hook to indicate the length of the data read by the app.
*                               TX state: Must be updated by the hook to indicate the length of the data written by the app.
*
* Return(s)   : REST hook state:
*                   HTTPs_REST_HOOK_STATE_CONTINUE, to continue the transaction processing.
*                   HTTPs_REST_HOOK_STATE_STAY,     to fall back at the same stage the next time the hook is called.
*                   HTTPs_REST_HOOK_STATE_ERROR,    when an error occurred in the hook processing.
*
* Caller(s)   : HTTP server core.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE  AppREST_GetUserListHook (const  HTTPs_REST_RESOURCE      *p_resource,
                                                        const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                        const  HTTPs_REST_STATE          state,
                                                               void                    **p_data,
                                                        const  HTTPs_INSTANCE           *p_instance,
                                                               HTTPs_CONN               *p_conn,
                                                               void                     *p_buf,
                                                        const  CPU_SIZE_T                buf_len,
                                                               CPU_SIZE_T               *p_buf_len_used)
{
    APP_REST_DATA          *p_app_data;
    HTTPs_REST_HOOK_STATE   result;
    HTTPs_ERR               err_https;


    switch (state) {
        case HTTPs_REST_STATE_INIT:
                                                                /* Increment read reference counter of list.            */
             if (AppREST_ListWrRefCtr == 0) {
                 AppREST_ListRdRefCtr++;
             } else {
                 return (HTTPs_REST_HOOK_STATE_STAY);
             }
                                                                /* Get application data blk for the transaction.        */
             p_app_data = AppREST_GetDataBlk();
             if (p_app_data == DEF_NULL) {
                 return (HTTPs_REST_HOOK_STATE_ERROR);
             }
            *p_data = (void *)p_app_data;
                                                                /* Set Response body parameters to use Chunk hook.      */
             HTTPs_RespBodySetParamStaticData(p_instance,
                                              p_conn,
                                              HTTP_CONTENT_TYPE_JSON,
                                              DEF_NULL,
                                              0,
                                              DEF_NO,
                                             &err_https);
             if (err_https != HTTPs_ERR_NONE) {
                 return (HTTPs_REST_HOOK_STATE_ERROR);
             }
             break;


        case HTTPs_REST_STATE_RX:
             /* Nothing to do. It's a GET request, so no body.  */
             break;


        case HTTPs_REST_STATE_TX:
             p_app_data = (APP_REST_DATA *)*p_data;             /* Retrieve application data blk.                       */
                                                                /* Write the user list to the connection buffer.        */
             result     = AppREST_JSON_WrUserListToBuf(p_app_data, p_buf, buf_len, p_conn->BufLen, p_buf_len_used);
             if (result != HTTPs_REST_HOOK_STATE_CONTINUE) {
                 return (result);
             }
             break;


        case HTTPs_REST_STATE_CLOSE:
                                                                /* Free the application data blk.                       */
             p_app_data = (APP_REST_DATA *)*p_data;
             AppREST_RemoveDataBlk(p_app_data);
            *p_data = DEF_NULL;
             AppREST_ListRdRefCtr--;
             break;


        default:
             break;
    }

    return (HTTPs_REST_HOOK_STATE_CONTINUE);
}


/*
*********************************************************************************************************
*                                      AppREST_GetUserInfoHook()
*
* Description : Hook function that will be called when GET request is received with the "/users/{user_id}" URI.
*               This function, will set the response body data to a JSON with the user info specified by the
*               user ID.
*
* Argument(s) : p_resource      Pointer to REST resource.
*
*               p_uri           Pointer to the REST URI of the resource.
*
*               state           REST State :
*                                   HTTPs_REST_STATE_INIT:  Bound to the initialize hook of the HTTP server (OnInstanceInitHook).
*                                   HTTPs_REST_STATE_RX:    Bound to the RX hook of the HTTP server (OnReqBodyRxHook).
*                                   HTTPs_REST_STATE_TX:    Bound to the ready hook (OnReqRdySignalHook) and the TX chunk hook (OnRespChunkHook).
*                                   HTTPs_REST_STATE_CLOSE: Bound to the 'OnTransCompleteHook' and 'OnConnCloseHook'.
*                                   HTTPs_REST_STATE_ERROR: Should not occur.
*
*               p_data          Pointer to the connection application data pointer.
*
*               p_instance      Pointer to the HTTP server instance object.
*
*               p_conn          Pointer to the HTTP connection object.
*
*               p_buf           Pointer to the connection buffer to read/write data.
*
*               buf_len         RX state: Data length available to be read.
*                               TX state: Buffer length available to write.
*
*               p_buf_len_used  RX state: Must be updated by the hook to indicate the length of the data read by the app.
*                               TX state: Must be updated by the hook to indicate the length of the data written by the app.
*
* Return(s)   : REST hook state:
*                   HTTPs_REST_HOOK_STATE_CONTINUE, to continue the transaction processing.
*                   HTTPs_REST_HOOK_STATE_STAY,     to fall back at the same stage the next time the hook is called.
*                   HTTPs_REST_HOOK_STATE_ERROR,    when an error occurred in the hook processing.
*
* Caller(s)   : HTTP server core.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE  AppREST_GetUserInfoHook (const  HTTPs_REST_RESOURCE      *p_resource,
                                                        const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                        const  HTTPs_REST_STATE          state,
                                                               void                    **p_data,
                                                        const  HTTPs_INSTANCE           *p_instance,
                                                               HTTPs_CONN               *p_conn,
                                                               void                     *p_buf,
                                                        const  CPU_SIZE_T                buf_len,
                                                               CPU_SIZE_T               *p_buf_len_used)
{
    APP_REST_DATA          *p_app_data;
    APP_REST_USER          *p_user;
    CPU_INT08U              user_id = DEF_INT_08U_MAX_VAL;
    HTTPs_REST_HOOK_STATE   result;
    HTTPs_ERR               err_https;


    switch (state) {
        case HTTPs_REST_STATE_INIT:
                                                                /* Get application data blk for the transaction.        */
             p_app_data = AppREST_GetDataBlk();
             if (p_app_data == DEF_NULL) {
                 return (HTTPs_REST_HOOK_STATE_ERROR);
             }
            *p_data = (void *)p_app_data;
                                                                /* Retrieve User ID from Rx URI.                        */
             if (p_uri->WildCardsNbr > 0) {
                 user_id = Str_ParseNbr_Int32U(p_uri->WildCards[0].ValPtr,
                                               DEF_NULL,
                                               10);
             }
                                                                /* Retrieve User with user ID.                          */
             p_user = AppREST_FindUser(user_id);
             if (p_user == DEF_NULL) {
                 p_conn->StatusCode = HTTP_STATUS_NOT_FOUND;
                 HTTPs_RespBodySetParamNoBody(p_instance, p_conn, &err_https);
                 if (err_https != HTTPs_ERR_NONE) {
                     return (HTTPs_REST_HOOK_STATE_ERROR);
                 }
                 return (HTTPs_REST_HOOK_STATE_CONTINUE);
             }
                                                                /* Check if user is reference by a written transaction. */
             if (p_user->WrRefCtr != 0) {
                 p_conn->StatusCode = HTTP_STATUS_CONFLICT;
                 HTTPs_RespBodySetParamNoBody(p_instance, p_conn, &err_https);
                 if (err_https != HTTPs_ERR_NONE) {
                     return (HTTPs_REST_HOOK_STATE_ERROR);
                 }
                 return (HTTPs_REST_HOOK_STATE_CONTINUE);
             } else {
                 p_user->RdRefCtr++;                            /* Increment read reference counter of user.            */
             }

             p_app_data->UserPtr = p_user;

                                                                /* Set Response body parameters to use Chunk hook.      */
             HTTPs_RespBodySetParamStaticData(p_instance,
                                              p_conn,
                                              HTTP_CONTENT_TYPE_JSON,
                                              DEF_NULL,
                                              0,
                                              DEF_NO,
                                             &err_https);
             if (err_https != HTTPs_ERR_NONE) {
                 return (HTTPs_REST_HOOK_STATE_ERROR);
             }
             break;


        case HTTPs_REST_STATE_RX:
             /* Nothing to do. It's a GET request, so no body.  */
             break;


        case HTTPs_REST_STATE_TX:
             p_app_data = (APP_REST_DATA *)*p_data;             /* Retrieve application data blk.                       */
             p_user     = p_app_data->UserPtr;
                                                                /* Write User Info to response body.                    */
             result = AppREST_JSON_WrUserInfoToBuf(p_user, p_buf, buf_len, p_conn->BufLen, p_buf_len_used);
             if (result != HTTPs_REST_HOOK_STATE_CONTINUE) {
                 return (result);
             }
             break;


        case HTTPs_REST_STATE_CLOSE:
                                                                /* Decrement the Read Reference Counter of the user.    */
             p_app_data = (APP_REST_DATA *)*p_data;
             p_user = p_app_data->UserPtr;
             if (p_user != DEF_NULL) {
                 p_user->RdRefCtr--;
             }
             p_app_data->UserPtr = DEF_NULL;
                                                                /* Free the application data blk.                       */
             AppREST_RemoveDataBlk(p_app_data);
            *p_data = DEF_NULL;
             break;


        default:
             break;
    }

    return (HTTPs_REST_HOOK_STATE_CONTINUE);
}


/*
*********************************************************************************************************
*                                       AppREST_SetUserInfoHook()
*
* Description : Hook function that will be called when PUT request is received with the "/users/{user_id}" URI.
*               This function, will parse the JSON with the new user info received in the request body and set
*               the new user info.
*               The hook will also set the response body data to a JSON with the user first name and last name
*               to update the list preview.
*
* Argument(s) : p_resource      Pointer to REST resource.
*
*               p_uri           Pointer to the REST URI of the resource.
*
*               state           REST State :
*                                   HTTPs_REST_STATE_INIT:  Bound to the initialize hook of the HTTP server (OnInstanceInitHook).
*                                   HTTPs_REST_STATE_RX:    Bound to the RX hook of the HTTP server (OnReqBodyRxHook).
*                                   HTTPs_REST_STATE_TX:    Bound to the ready hook (OnReqRdySignalHook) and the TX chunk hook (OnRespChunkHook).
*                                   HTTPs_REST_STATE_CLOSE: Bound to the 'OnTransCompleteHook' and 'OnConnCloseHook'.
*                                   HTTPs_REST_STATE_ERROR: Should not occur.
*
*               p_data          Pointer to the connection application data pointer.
*
*               p_instance      Pointer to the HTTP server instance object.
*
*               p_conn          Pointer to the HTTP connection object.
*
*               p_buf           Pointer to the connection buffer to read/write data.
*
*               buf_len         RX state: Data length available to be read.
*                               TX state: Buffer length available to write.
*
*               p_buf_len_used  RX state: Must be updated by the hook to indicate the length of the data read by the app.
*                               TX state: Must be updated by the hook to indicate the length of the data written by the app.
*
* Return(s)   : REST hook state:
*
*                   HTTPs_REST_HOOK_STATE_CONTINUE, to continue the transaction processing.
*                   HTTPs_REST_HOOK_STATE_STAY,     to fall back at the same stage the next time the hook is called.
*                   HTTPs_REST_HOOK_STATE_ERROR,    when an error occurred in the hook processing.
*
* Caller(s)   : HTTP server core.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE  AppREST_SetUserInfoHook (const  HTTPs_REST_RESOURCE      *p_resource,
                                                        const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                        const  HTTPs_REST_STATE          state,
                                                               void                    **p_data,
                                                        const  HTTPs_INSTANCE           *p_instance,
                                                               HTTPs_CONN               *p_conn,
                                                               void                     *p_buf,
                                                        const  CPU_SIZE_T                buf_len,
                                                               CPU_SIZE_T               *p_buf_len_used)
{
    APP_REST_DATA          *p_app_data;
    APP_REST_USER          *p_user;
    CPU_INT08U              user_id = DEF_INT_08U_MAX_VAL;
    HTTPs_REST_HOOK_STATE   result;
    HTTPs_ERR               err_https;


    switch (state) {
        case HTTPs_REST_STATE_INIT:
                                                                /* Get application data blk for the transaction.        */
             p_app_data = AppREST_GetDataBlk();
             if (p_app_data == DEF_NULL) {
                 return (HTTPs_REST_HOOK_STATE_ERROR);
             }
            *p_data = (void *)p_app_data;
                                                                /* Retrieve User ID from Rx URI.                        */
             if (p_uri->WildCardsNbr > 0) {
                 user_id = Str_ParseNbr_Int32U(p_uri->WildCards[0].ValPtr,
                                               DEF_NULL,
                                               10);
             }
                                                                /* Retrieve User with user ID.                          */
             p_user = AppREST_FindUser(user_id);
             if (p_user == DEF_NULL) {
                 p_conn->StatusCode  = HTTP_STATUS_NOT_FOUND;
                 HTTPs_RespBodySetParamNoBody(p_instance, p_conn, &err_https);
                 if (err_https != HTTPs_ERR_NONE) {
                     return (HTTPs_REST_HOOK_STATE_ERROR);
                 }
                 return (HTTPs_REST_HOOK_STATE_CONTINUE);
             }
                                                                /* Check if user is already reference elsewhere.        */
             if ((AppREST_ListRdRefCtr != 0) ||
                 (p_user->WrRefCtr     != 0) ||
                 (p_user->RdRefCtr     != 0)) {
                 p_conn->StatusCode  = HTTP_STATUS_CONFLICT;
                 HTTPs_RespBodySetParamNoBody(p_instance, p_conn, &err_https);
                 if (err_https != HTTPs_ERR_NONE) {
                     return (HTTPs_REST_HOOK_STATE_ERROR);
                 }
                 return (HTTPs_REST_HOOK_STATE_CONTINUE);
             } else {
                 p_user->WrRefCtr++;                            /* Increment write reference counter of user.           */
                 AppREST_ListWrRefCtr++;                        /* Increment write reference counter of list.           */
             }

             p_app_data->UserPtr   = p_user;
             p_app_data->FieldType = APP_REST_USER_FIELD_FIRST_NAME;
                                                                /* Set Response body parameters to use Chunk hook.      */
             HTTPs_RespBodySetParamStaticData(p_instance,
                                              p_conn,
                                              HTTP_CONTENT_TYPE_JSON,
                                              DEF_NULL,
                                              0,
                                              DEF_NO,
                                             &err_https);
             if (err_https != HTTPs_ERR_NONE) {
                 return (HTTPs_REST_HOOK_STATE_ERROR);
             }
             break;


        case HTTPs_REST_STATE_RX:
             p_app_data = (APP_REST_DATA *)*p_data;             /* Retrieve application data blk.                       */
             p_user     = p_app_data->UserPtr;
                                                                /* If no user was found, just continue to send a 404.   */
             if (p_user == DEF_NULL) {
                 return (HTTPs_REST_HOOK_STATE_CONTINUE);
             }
                                                                /* If no data available to read yet, just continue.     */
             if (buf_len == 0) {
                 return (HTTPs_REST_HOOK_STATE_CONTINUE);
             }
                                                                /* Parse the user info contain in the request body.     */
             result = AppREST_JSON_ParseUserInfo(p_app_data, p_user, p_buf, buf_len, p_conn->BufLen, p_buf_len_used);
             if (result != HTTPs_REST_HOOK_STATE_CONTINUE) {
                 return (result);
             }
            *p_buf_len_used = buf_len;
             break;


        case HTTPs_REST_STATE_TX:
             p_app_data = (APP_REST_DATA *)*p_data;             /* Retrieve application data blk.                       */
             p_user     = p_app_data->UserPtr;
                                                                /*  Write User name to response body.                   */
             result     = AppREST_JSON_WrUserToBuf(p_user, p_buf, buf_len, p_conn->BufLen, p_buf_len_used);
             if (result != HTTPs_REST_HOOK_STATE_CONTINUE) {
                 return (result);
             }
             break;


        case HTTPs_REST_STATE_CLOSE:
             p_app_data = (APP_REST_DATA *)*p_data;             /* Retrieve application data blk.                       */
             p_user = p_app_data->UserPtr;
             if (p_user != DEF_NULL) {
                 p_user->WrRefCtr--;                            /* Decrement the Write Reference Counter of the user.   */
                 AppREST_ListWrRefCtr--;                        /* Decrement the Write Reference Counter of the list.   */
             }
             p_app_data->UserPtr = DEF_NULL;
                                                                /* Free the application data blk.                       */
             AppREST_RemoveDataBlk(p_app_data);
            *p_data = DEF_NULL;
             break;


        default:
             break;
    }

    return (HTTPs_REST_HOOK_STATE_CONTINUE);
}


/*
*********************************************************************************************************
*                                       AppREST_CreateUserHook()
*
* Description : Hook function that will be called when POST request is received with the "/users" URI.
*               This function, will parse the JSON with the new user name received in the request body and create
*               a new user to add to the list.
*               The hook will also set the response body data to a JSON with the user first name and last name
*               to update the list preview.
*
* Argument(s) : p_resource      Pointer to REST resource.
*
*               p_uri           Pointer to the REST URI of the resource.
*
*               state           REST State :
*                                   HTTPs_REST_STATE_INIT:  Bound to the initialize hook of the HTTP server (OnInstanceInitHook).
*                                   HTTPs_REST_STATE_RX:    Bound to the RX hook of the HTTP server (OnReqBodyRxHook).
*                                   HTTPs_REST_STATE_TX:    Bound to the ready hook (OnReqRdySignalHook) and the TX chunk hook (OnRespChunkHook).
*                                   HTTPs_REST_STATE_CLOSE: Bound to the 'OnTransCompleteHook' and 'OnConnCloseHook'.
*                                   HTTPs_REST_STATE_ERROR: Should not occur.
*
*               p_data          Pointer to the connection application data pointer.
*
*               p_instance      Pointer to the HTTP server instance object.
*
*               p_conn          Pointer to the HTTP connection object.
*
*               p_buf           Pointer to the connection buffer to read/write data.
*
*               buf_len         RX state: Data length available to be read.
*                               TX state: Buffer length available to write.
*
*               p_buf_len_used  RX state: Must be updated by the hook to indicate the length of the data read by the app.
*                               TX state: Must be updated by the hook to indicate the length of the data written by the app.
*
* Return(s)   : REST hook state:
*
*                   HTTPs_REST_HOOK_STATE_CONTINUE, to continue the transaction processing.
*                   HTTPs_REST_HOOK_STATE_STAY,     to fall back at the same stage the next time the hook is called.
*                   HTTPs_REST_HOOK_STATE_ERROR,    when an error occurred in the hook processing.
*
* Caller(s)   : HTTP server core.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE  AppREST_CreateUserHook (const  HTTPs_REST_RESOURCE      *p_resource,
                                                       const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                       const  HTTPs_REST_STATE          state,
                                                              void                    **p_data,
                                                       const  HTTPs_INSTANCE           *p_instance,
                                                              HTTPs_CONN               *p_conn,
                                                              void                     *p_buf,
                                                       const  CPU_SIZE_T                buf_len,
                                                              CPU_SIZE_T               *p_buf_len_used)
{
    APP_REST_DATA          *p_app_data;
    APP_REST_USER          *p_user;
    HTTPs_REST_HOOK_STATE   result;
    HTTPs_ERR               err_https;


    switch (state) {
        case HTTPs_REST_STATE_INIT:
                                                                /* Get application data blk for the transaction.        */
             p_app_data = AppREST_GetDataBlk();
             if (p_app_data == DEF_NULL) {
                 return (HTTPs_REST_HOOK_STATE_ERROR);
             }
            *p_data = (void *)p_app_data;
                                                                /* Check if the user list is reference elsewhere.       */
             if (AppREST_ListRdRefCtr != 0) {
                 p_conn->StatusCode  = HTTP_STATUS_CONFLICT;
                 HTTPs_RespBodySetParamNoBody(p_instance, p_conn, &err_https);
                 if (err_https != HTTPs_ERR_NONE) {
                     return (HTTPs_REST_HOOK_STATE_ERROR);
                 }
                 return (HTTPs_REST_HOOK_STATE_CONTINUE);
             }
                                                                /* Create and add new user.                             */
             p_user = AppREST_GetUser();
             if (p_user == DEF_NULL) {
                 p_conn->StatusCode = HTTP_STATUS_FORBIDDEN;
                 HTTPs_RespBodySetParamNoBody(p_instance, p_conn, &err_https);
                 if (err_https != HTTPs_ERR_NONE) {
                     return (HTTPs_REST_HOOK_STATE_ERROR);
                 }
                 return(HTTPs_REST_HOOK_STATE_CONTINUE);
             }

             AppREST_UserCtr++;
             p_user->WrRefCtr++;
             AppREST_ListWrRefCtr++;

             p_app_data->UserPtr   = p_user;
             p_app_data->FieldType = APP_REST_USER_FIELD_FIRST_NAME;

                                                                /* Set Response body parameters to use Chunk hook. */
             HTTPs_RespBodySetParamStaticData(p_instance,
                                              p_conn,
                                              HTTP_CONTENT_TYPE_JSON,
                                              DEF_NULL,
                                              0,
                                              DEF_NO,
                                             &err_https);
             if (err_https != HTTPs_ERR_NONE) {
                 return (HTTPs_REST_HOOK_STATE_ERROR);
             }
             break;


        case HTTPs_REST_STATE_RX:
             p_app_data = (APP_REST_DATA *)*p_data;             /* Retrieve application data blk.                       */
             p_user     = p_app_data->UserPtr;
                                                                /* If no user was created, just continue.               */
             if (p_user == DEF_NULL) {
                 return (HTTPs_REST_HOOK_STATE_CONTINUE);
             }
                                                                /* If no data available to read yet, just continue.     */
             if (buf_len == 0) {
                 return (HTTPs_REST_HOOK_STATE_CONTINUE);
             }
                                                                /* Parse the user name contain in the request body.     */
             result     = AppREST_JSON_ParseUser(p_app_data, p_user, p_uri, p_buf, buf_len, p_conn->BufLen, p_buf_len_used);
             if (result != HTTPs_REST_HOOK_STATE_CONTINUE) {
                 return (result);
             }
            *p_buf_len_used      = buf_len;
             p_conn->StatusCode  = HTTP_STATUS_CREATED;
             break;


        case HTTPs_REST_STATE_TX:
             p_app_data = (APP_REST_DATA *)*p_data;             /* Retrieve application data blk.                       */
             p_user     = p_app_data->UserPtr;
                                                                /*  Write User name to response body.                   */
             result     = AppREST_JSON_WrUserToBuf(p_user, p_buf, buf_len, p_conn->BufLen, p_buf_len_used);
             if (result != HTTPs_REST_HOOK_STATE_CONTINUE) {
                 return (result);
             }
             break;


        case HTTPs_REST_STATE_CLOSE:
             p_app_data = (APP_REST_DATA *)*p_data;             /* Retrieve application data blk.                       */
             p_user     = p_app_data->UserPtr;
             if (p_user != DEF_NULL) {
                 p_user->WrRefCtr--;                            /* Decrement the Write Reference Counter of the user.   */
                 AppREST_ListWrRefCtr--;                        /* Decrement the Write Reference Counter of the list.   */
             }
             p_app_data->UserPtr = DEF_NULL;
                                                                /* Free the application data blk.                       */
             AppREST_RemoveDataBlk(p_app_data);
            *p_data = DEF_NULL;
             break;


        default:
             break;
    }

    return (HTTPs_REST_HOOK_STATE_CONTINUE);
}


/*
*********************************************************************************************************
*                                       AppREST_DeleteUserHook()
*
* Description : Hook function that will be called when DELETE request is received with the "/users/{user_id}" URI.
*               This function, will simply remove the user specified by the user id of the URI from the list.
*
* Argument(s) : p_resource      Pointer to REST resource.
*
*               p_uri           Pointer to the REST URI of the resource.
*
*               state           REST State :
*                                   HTTPs_REST_STATE_INIT:  Bound to the initialize hook of the HTTP server (OnInstanceInitHook).
*                                   HTTPs_REST_STATE_RX:    Bound to the RX hook of the HTTP server (OnReqBodyRxHook).
*                                   HTTPs_REST_STATE_TX:    Bound to the ready hook (OnReqRdySignalHook) and the TX chunk hook (OnRespChunkHook).
*                                   HTTPs_REST_STATE_CLOSE: Bound to the 'OnTransCompleteHook' and 'OnConnCloseHook'.
*                                   HTTPs_REST_STATE_ERROR: Should not occur.
*
*               p_data          Pointer to the connection application data pointer.
*
*               p_instance      Pointer to the HTTP server instance object.
*
*               p_conn          Pointer to the HTTP connection object.
*
*               p_buf           Pointer to the connection buffer to read/write data.
*
*               buf_len         RX state: Data length available to be read.
*                               TX state: Buffer length available to write.
*
*               p_buf_len_used  RX state: Must be updated by the hook to indicate the length of the data read by the app.
*                               TX state: Must be updated by the hook to indicate the length of the data written by the app.
*
* Return(s)   : REST hook state:
*
*                   HTTPs_REST_HOOK_STATE_CONTINUE, to continue the transaction processing.
*                   HTTPs_REST_HOOK_STATE_STAY,     to fall back at the same stage the next time the hook is called.
*                   HTTPs_REST_HOOK_STATE_ERROR,    when an error occurred in the hook processing.
*
* Caller(s)   : HTTP server core.
*
* Note(s)     : (1) If the user specify by the user ID of the URL is not found a 404 (Not Found) status code
*                   will be send back.
*
*               (2) If the user is already used by another transaction, a 409 (Conflict) status code will
*                   be send back.
*
*               (3) If the delete is successful, a 204 (No Content) status code will be send back.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE  AppREST_DeleteUserHook (const  HTTPs_REST_RESOURCE      *p_resource,
                                                       const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                       const  HTTPs_REST_STATE          state,
                                                              void                    **p_data,
                                                       const  HTTPs_INSTANCE           *p_instance,
                                                              HTTPs_CONN               *p_conn,
                                                              void                     *p_buf,
                                                       const  CPU_SIZE_T                buf_len,
                                                              CPU_SIZE_T               *p_buf_len_used)
{
    APP_REST_DATA  *p_app_data;
    APP_REST_USER  *p_user;
    CPU_INT08U      user_id = DEF_INT_08U_MAX_VAL;
    HTTPs_ERR       err_https;


    switch (state) {
        case HTTPs_REST_STATE_INIT:
                                                                /* Get application data blk for the transaction.        */
             p_app_data = AppREST_GetDataBlk();
             if (p_app_data == DEF_NULL) {
                 return (HTTPs_REST_HOOK_STATE_ERROR);
             }
            *p_data = (void *)p_app_data;
                                                                /* Retrieve User ID from Rx URI.                        */
             if (p_uri->WildCardsNbr > 0) {
                 user_id = Str_ParseNbr_Int32U(p_uri->WildCards[0].ValPtr,
                                               DEF_NULL,
                                               10);
             }
                                                                /* Retrieve User with user ID.                          */
             p_user = AppREST_FindUser(user_id);
             if (p_user == DEF_NULL) {
                 p_conn->StatusCode = HTTP_STATUS_NOT_FOUND;
                 HTTPs_RespBodySetParamNoBody(p_instance, p_conn, &err_https);
                 if (err_https != HTTPs_ERR_NONE) {
                     return (HTTPs_REST_HOOK_STATE_ERROR);
                 }
                 return(HTTPs_REST_HOOK_STATE_CONTINUE);
             }
                                                                /* Check if the user is already reference elsewhere.    */
             if ((AppREST_ListRdRefCtr != 0) ||
                 (p_user->WrRefCtr     != 0) ||
                 (p_user->RdRefCtr     != 0)) {
                 p_conn->StatusCode  = HTTP_STATUS_CONFLICT;
                 HTTPs_RespBodySetParamNoBody(p_instance, p_conn, &err_https);
                 if (err_https != HTTPs_ERR_NONE) {
                     return (HTTPs_REST_HOOK_STATE_ERROR);
                 }
                 return (HTTPs_REST_HOOK_STATE_CONTINUE);
             }
                                                                /* Remove User from list.                               */
             AppREST_RemoveUser(p_user);
             AppREST_UserCtr--;
                                                                /* Set Response body parameters to no body.             */
             HTTPs_RespBodySetParamNoBody(p_instance, p_conn, &err_https);
             if (err_https != HTTPs_ERR_NONE) {
                 return (HTTPs_REST_HOOK_STATE_ERROR);
             }
             if (err_https != HTTPs_ERR_NONE) {
                 return (HTTPs_REST_HOOK_STATE_ERROR);
             }
             p_conn->StatusCode = HTTP_STATUS_NO_CONTENT;
             break;


        case HTTPs_REST_STATE_RX:
             /* Nothing to do. It's a DELETE request with no body.  */
             break;


        case HTTPs_REST_STATE_TX:
             /* Nothing to do. The response will not have a body.   */
             break;


        case HTTPs_REST_STATE_CLOSE:
                                                                /* Free the application data blk.                       */
             p_app_data          = (APP_REST_DATA *)*p_data;
             p_app_data->UserPtr = DEF_NULL;
             AppREST_RemoveDataBlk(p_app_data);
            *p_data = DEF_NULL;
             break;


        default:
             break;
    }

    return (HTTPs_REST_HOOK_STATE_CONTINUE);
}


/*
*********************************************************************************************************
*                                    AppREST_JSON_GetUserFieldSize()
*
* Description : Calculate the necessary buffer size to write a User Field from the user list for a
*               specific user.
*
* Argument(s) : p_user     Pointer to the user object.
*
* Return(s)   : size of data to write.
*
* Caller(s)   : AppREST_GetUserListHook().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_SIZE_T  AppREST_JSON_GetUserFieldSize (APP_REST_USER  *p_user)
{
    CPU_SIZE_T  char_nbr;
    CPU_SIZE_T  tot_size = 0;


                                                                /* Size of all JSON specific characters { } , : "       */
    tot_size +=   APP_REST_JSON_OBJ_CHARS_NBR
                + APP_REST_JSON_FIELD_MAX_CHARS_NBR * 3
                + APP_REST_JSON_FIELD_MIN_CHARS_NBR;

                                                                /* Size of all the field keys.                          */
    tot_size += sizeof(APP_REST_JSON_KEY_ID_STR_NAME)         +
                sizeof(APP_REST_JSON_KEY_FIRST_NAME_STR_NAME) +
                sizeof(APP_REST_JSON_KEY_LAST_NAME_STR_NAME)  +
                sizeof(APP_REST_JSON_KEY_LINK_STR_NAME);

                                                                /* Size of all the field values.                        */
    tot_size +=   Str_Len_N(p_user->FirstName, APP_REST_FIRST_NAME_STR_MAX_LEN)
                + Str_Len_N(p_user->LastName, APP_REST_LAST_NAME_STR_MAX_LEN)
                + Str_Len_N(p_user->Link, APP_REST_LINK_STR_MAX_LEN);

    char_nbr = AppREST_GetStrLenOfIntDec(p_user->ID);
    Str_FmtNbr_Int32U(p_user->ID, char_nbr, 10, '\0', DEF_NO, DEF_YES, AppREST_UserID_Str);
    tot_size += Str_Len_N(AppREST_UserID_Str, char_nbr);

    return (tot_size);
}


/*
*********************************************************************************************************
*                                    AppREST_JSON_GetUserInfoSize()
*
* Description : Calculate the necessary buffer size to write user info fields for a specific user.
*
* Argument(s) : p_user     Pointer to the user object.
*
* Return(s)   : size of data to write.
*
* Caller(s)   : AppREST_GetUserInfoHook().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_SIZE_T  AppREST_JSON_GetUserInfoSize (APP_REST_USER  *p_user)
{
    CPU_CHAR    str[APP_REST_AGE_STR_MAX_LEN];
    CPU_SIZE_T  char_nbr;
    CPU_SIZE_T  tot_size = 0;


                                                                /* Size of all JSON specific characters { } , : "       */
    tot_size += APP_REST_JSON_OBJ_CHARS_NBR
              + APP_REST_JSON_FIELD_MAX_CHARS_NBR * 5
              + APP_REST_JSON_FIELD_MIN_CHARS_NBR;

                                                                /* Size of all the field keys.                          */
    tot_size += sizeof(APP_REST_JSON_KEY_ID_STR_NAME)
              + sizeof(APP_REST_JSON_KEY_FIRST_NAME_STR_NAME)
              + sizeof(APP_REST_JSON_KEY_LAST_NAME_STR_NAME)
              + sizeof(APP_REST_JSON_KEY_GENDER_STR_NAME)
              + sizeof(APP_REST_JSON_KEY_AGE_STR_NAME)
              + sizeof(APP_REST_JSON_KEY_JOB_STR_NAME);

                                                                /* Size of all the field values.                        */
    tot_size += Str_Len_N(p_user->FirstName, APP_REST_FIRST_NAME_STR_MAX_LEN)
              + Str_Len_N(p_user->LastName, APP_REST_LAST_NAME_STR_MAX_LEN)
              + Str_Len_N(p_user->Gender, APP_REST_GENDER_STR_MAX_LEN)
              + Str_Len_N(p_user->JobTitle, APP_REST_JOB_TITLE_STR_MAX_LEN);

    char_nbr = AppREST_GetStrLenOfIntDec(p_user->ID);
    Str_FmtNbr_Int32U(p_user->ID, char_nbr, 10, '\0', DEF_NO, DEF_YES, AppREST_UserID_Str);
    tot_size += Str_Len_N(AppREST_UserID_Str, char_nbr);

    Str_FmtNbr_Int32U(p_user->Age, APP_REST_AGE_STR_MAX_LEN, 10, '\0', DEF_NO, DEF_YES, &str[0]);
    tot_size += Str_Len_N(&str[0], APP_REST_AGE_STR_MAX_LEN);

    return (tot_size);
}


/*
*********************************************************************************************************
*                                      AppREST_JSON_WrUserToBuf()
*
* Description : Write JSON response with the following user fields : user ID, first name, last name and link.
*
* Argument(s) : p_user          Pointer to user object.
*
*               p_buf           Pointer to connection buffer where to write data.
*
*               buf_len         Length available in buffer to write data.
*
*               buf_len_max     The maximum size of the connection buffer.
*
*               p_buf_len_used  Variable that will received the length of data written to the buffer.
*
* Return(s)   : REST hook state:
*
*                   HTTPs_REST_HOOK_STATE_CONTINUE, to continue the transaction processing.
*                   HTTPs_REST_HOOK_STATE_STAY,     to fall back at the same stage the next time the hook is called.
*                   HTTPs_REST_HOOK_STATE_ERROR,    when an error occurred in the hook processing.
*
* Caller(s)   : AppREST_CreateUserHook(),
*               AppREST_JSON_WrUserListToBuf(),
*               AppREST_SetUserInfoHook().
*
* Note(s)     : (1) The connection buffer must be enough big to fit all the fields of one user.
*                   Else the process will fail.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE  AppREST_JSON_WrUserToBuf (       APP_REST_USER  *p_user,
                                                                void           *p_buf,
                                                         const  CPU_SIZE_T      buf_len,
                                                         const  CPU_SIZE_T      buf_len_max,
                                                                CPU_SIZE_T     *p_buf_len_used)
{
    CPU_CHAR       *p_str;
    CPU_SIZE_T      char_nbr;
    CPU_SIZE_T      data_len = 0;


    p_str = p_buf;

                                                                /* Calculate the data length to write.                  */
    data_len = AppREST_JSON_GetUserFieldSize(p_user);

    if (data_len > buf_len_max) {
        return (HTTPs_REST_HOOK_STATE_ERROR);                   /* The configured buffer size is not enough.            */
    }
    if (data_len > buf_len) {
        return (HTTPs_REST_HOOK_STATE_STAY);                    /* not enough space in the buffer, so try next time.    */
    }

                                                                /* Write JSON data to buffer.                           */
    char_nbr = AppREST_GetStrLenOfIntDec(p_user->ID);
    Str_FmtNbr_Int32U(p_user->ID, char_nbr, 10, '\0', DEF_NO, DEF_YES, AppREST_UserID_Str);
    Str_Copy(p_str, "{\"User ID\": \"");
    Str_Cat(p_str, AppREST_UserID_Str);
    Str_Cat(p_str, "\", \"First Name\": \"");
    Str_Cat(p_str, p_user->FirstName);
    Str_Cat(p_str, "\", \"Last Name\": \"");
    Str_Cat(p_str, p_user->LastName);
    Str_Cat(p_str, "\", \"Link\":\"");
    Str_Cat(p_str, p_user->Link);
    Str_Cat(p_str, "\"}");

   *p_buf_len_used = Str_Len_N(p_str, buf_len);

    return (HTTPs_REST_HOOK_STATE_CONTINUE);
}

/*
*********************************************************************************************************
*                                    AppREST_JSON_WrUserListToBuf()
*
* Description : Write the JSON user list to the connection buffer.
*
* Argument(s) : p_app_data      Pointer to application data object for the current transaction.
*
*               p_buf           Pointer to connection buffer where to write data.
*
*               buf_len         Length available in buffer to write data.
*
*               buf_len_max     The maximum size of the connection buffer.
*
*               p_buf_len_used  Variable that will received the length of data written to the buffer.
*
* Return(s)   : REST hook state:
*
*                   HTTPs_REST_HOOK_STATE_CONTINUE, to continue the transaction processing.
*                   HTTPs_REST_HOOK_STATE_STAY,     to fall back at the same stage the next time the hook is called.
*                   HTTPs_REST_HOOK_STATE_ERROR,    when an error occurred in the hook processing.
*
* Caller(s)   : AppREST_GetUserListHook().
*
* Note(s)     : (1) The connection buffer must be enough big to fit all the fields of one user.
*                   Else the process will fail.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE  AppREST_JSON_WrUserListToBuf (       APP_REST_DATA  *p_app_data,
                                                                    void           *p_buf,
                                                             const  CPU_SIZE_T      buf_len,
                                                             const  CPU_SIZE_T      buf_len_max,
                                                                    CPU_SIZE_T     *p_buf_len_used)
{
    APP_REST_USER          *p_user;
    CPU_CHAR               *p_str;
    HTTPs_REST_HOOK_STATE   state;
    CPU_SIZE_T              buf_len_rem;


    p_str       = p_buf;
    buf_len_rem = buf_len;

                                                                /* Write start of JSON list if not already done.        */
    if (p_app_data->UserPtr == DEF_NULL) {
        if (sizeof(APP_REST_JSON_LIST_USER_START_STR) > buf_len) {
            return (HTTPs_REST_HOOK_STATE_STAY);
        }
        Str_Copy(p_str, APP_REST_JSON_LIST_USER_START_STR);
        p_str  += Str_Len(p_str);
        p_user  = AppREST_UserFirstPtr;
    } else {
        p_user = p_app_data->UserPtr;
    }

                                                                /* Write all users of list until buffer is full.        */
    while (p_user != DEF_NULL) {

                                                                /* Write user fields to buffer.                         */
        state = AppREST_JSON_WrUserToBuf(p_user, p_str, buf_len_rem, buf_len_max, p_buf_len_used);
        switch (state) {
            case HTTPs_REST_HOOK_STATE_CONTINUE:
                 break;

            case HTTPs_REST_HOOK_STATE_STAY:                    /* Buffer is full, save the index user for next time.   */
                 p_app_data->UserPtr = p_user;
                *p_buf_len_used      = p_str - (CPU_CHAR* )p_buf;
                 return (state);

            case HTTPs_REST_HOOK_STATE_ERROR:
            default:
                 return (state);
        }

        if (p_user != AppREST_UserLastPtr) {                    /* Write comma if user is not the last of the list.     */
            Str_Cat(p_str, ",");
            p_str++;
        }

        p_str       += *p_buf_len_used;
        buf_len_rem -= *p_buf_len_used;

        p_user = p_user->NextPtr;
    }

    Str_Cat(p_str, "]}");                                       /* Write end of table list in JSON.                     */
    p_str += 2;
    p_app_data->UserPtr = DEF_NULL;
   *p_buf_len_used      = (p_str - (CPU_CHAR* )p_buf);

    return (HTTPs_REST_HOOK_STATE_CONTINUE);
}


/*
*********************************************************************************************************
*                                    AppREST_JSON_WrUserInfoToBuf()
*
* Description : Write the JSON with the user info fields.
*
* Argument(s) : p_user          Pointer to user object.
*
*               p_buf           Pointer to connection buffer where to write data.
*
*               buf_len         Length available in buffer to write data.
*
*               buf_len_max     The maximum size of the connection buffer.
*
*               p_buf_len_used  Variable that will received the length of data written to the buffer.
*
* Return(s)   : REST hook state:
*
*                   HTTPs_REST_HOOK_STATE_CONTINUE, to continue the transaction processing.
*                   HTTPs_REST_HOOK_STATE_STAY,     to fall back at the same stage the next time the hook is called.
*                   HTTPs_REST_HOOK_STATE_ERROR,    when an error occurred in the hook processing.
*
* Caller(s)   : AppREST_GetUserInfoHook().
*
* Note(s)     : (1) The connection buffer must be enough big to fit all the info fields of the user.
*                   Else the process will fail.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE  AppREST_JSON_WrUserInfoToBuf (       APP_REST_USER  *p_user,
                                                                    void           *p_buf,
                                                             const  CPU_SIZE_T      buf_len,
                                                             const  CPU_SIZE_T      buf_len_max,
                                                                    CPU_SIZE_T     *p_buf_len_used)
{
    CPU_CHAR    *p_str;
    CPU_CHAR     str[3];
    CPU_SIZE_T   data_len = 0;
    CPU_SIZE_T   char_nbr;


    p_str = p_buf;

                                                                /* Calculate data length to write.                      */
    data_len = AppREST_JSON_GetUserInfoSize(p_user);
    if (data_len > buf_len_max) {
        return (HTTPs_REST_HOOK_STATE_ERROR);
    }
    if (data_len > buf_len) {
       *p_buf_len_used = 0;
        return (HTTPs_REST_HOOK_STATE_STAY);
    }

                                                                /* Write JSON data with User Info to the buffer.        */
    Str_Copy(p_str, "{\"User ID\": \"");
    char_nbr = AppREST_GetStrLenOfIntDec(p_user->ID);
    Str_FmtNbr_Int32U(p_user->ID, char_nbr, 10, '\0', DEF_NO, DEF_YES, AppREST_UserID_Str);
    Str_Cat(p_str, AppREST_UserID_Str);
    Str_Cat(p_str, "\", \"First Name\": \"");
    Str_Cat(p_str, p_user->FirstName);
    Str_Cat(p_str, "\", \"Last Name\": \"");
    Str_Cat(p_str, p_user->LastName);
    Str_Cat(p_str, "\", \"Gender\": \"");
    Str_Cat(p_str, p_user->Gender);
    Str_Cat(p_str, "\", \"Age\": \"");
    if (p_user->Age != 0) {
        Str_FmtNbr_Int32U(p_user->Age, APP_REST_AGE_STR_MAX_LEN, 10, '\0', DEF_NO, DEF_YES, &str[0]);
        Str_Cat(p_str, &str[0]);
    }
    Str_Cat(p_str, "\", \"Job Title\": \"");
    Str_Cat(p_str, p_user->JobTitle);
    Str_Cat(p_str, "\"}");

   *p_buf_len_used = Str_Len(p_str);

    return (HTTPs_REST_HOOK_STATE_CONTINUE);
}


/*
*********************************************************************************************************
*                                       AppREST_JSON_ParseUser()
*
* Description : Parse JSON data to retrieve the new user first name and last name.
*
* Argument(s) : p_app_data      Pointer to application data block.
*
*               p_user          Pointer to user object.
*
*               p_uri           Pointer to REST URI received in the request.
*
*               p_buf           Pointer to the connection buffer with the received data.
*
*               data_len        Length of the data received inside the buffer.
*
*               buf_len_max     Maximum size of the connection buffer.
*
*               p_buf_len_used  Variable that will received the length of the data read.
*
* Return(s)   : REST hook state:
*
*                   HTTPs_REST_HOOK_STATE_CONTINUE, to continue the transaction processing.
*                   HTTPs_REST_HOOK_STATE_STAY,     to fall back at the same stage the next time the hook is called.
*                   HTTPs_REST_HOOK_STATE_ERROR,    when an error occurred in the hook processing.
*
* Caller(s)   : AppREST_CreateUserHook().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE  AppREST_JSON_ParseUser (       APP_REST_DATA            *p_app_data,
                                                              APP_REST_USER            *p_user,
                                                       const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                              void                     *p_buf,
                                                       const  CPU_SIZE_T                data_len,
                                                       const  CPU_SIZE_T                buf_len_max,
                                                              CPU_SIZE_T               *p_buf_len_used)
{
    CPU_CHAR              *p_str_val;
    APP_REST_USER_FIELD    field_type;
    HTTPs_REST_HOOK_STATE  state;
    CPU_SIZE_T             str_len;
    CPU_SIZE_T             val_len;
    CPU_SIZE_T             min_len;
    CPU_SIZE_T             char_nbr;
    CPU_SIZE_T             data_rd_len = 0;


   *p_buf_len_used = 0;
                                                                /* ------- PARSE RECEIVED JSON TO RETRIEVE USER ------- */
    field_type = p_app_data->FieldType;
    while (field_type <= APP_REST_USER_FIELD_LAST_NAME) {
        state = AppREST_JSON_ParseField(field_type,
                                       &p_str_val,
                                       &val_len,
                                        p_buf,
                                        data_len,
                                        buf_len_max,
                                       &data_rd_len);
        if (state != HTTPs_REST_HOOK_STATE_CONTINUE) {
            p_app_data->FieldType = field_type;
            return (state);
        }

        switch (field_type) {
            case APP_REST_USER_FIELD_FIRST_NAME:
                 min_len = DEF_MIN(val_len, (APP_REST_FIRST_NAME_STR_MAX_LEN - 1));
                 Str_Copy_N(p_user->FirstName, p_str_val, min_len);
                 Str_Copy(p_user->FirstName + min_len, "\0");
                 break;

            case APP_REST_USER_FIELD_LAST_NAME:
                 min_len = DEF_MIN(val_len, (APP_REST_LAST_NAME_STR_MAX_LEN - 1));
                 Str_Copy_N(p_user->LastName, p_str_val, min_len);
                 Str_Copy(p_user->LastName + min_len, "\0");
                 break;

            case APP_REST_USER_FIELD_GENDER:
            case APP_REST_USER_FIELD_AGE:
            case APP_REST_USER_FIELD_JOB:
            default:
                 break;
        }

       *p_buf_len_used += data_rd_len;

        field_type++;
    }

    Str_Copy_N(p_user->Link, p_uri->ParsedURI.PathPtr, APP_REST_LINK_STR_MAX_LEN);
    Str_Cat_N(p_user->Link, "/", APP_REST_LINK_STR_MAX_LEN);
    str_len = Str_Len_N(p_user->Link, APP_REST_LINK_STR_MAX_LEN);
    char_nbr = AppREST_GetStrLenOfIntDec(p_user->ID);
    Str_FmtNbr_Int32U(p_user->ID, char_nbr, 10, '\0', DEF_NO, DEF_YES, p_user->Link + str_len);

    return (HTTPs_REST_HOOK_STATE_CONTINUE);
}


/*
*********************************************************************************************************
*                                     AppREST_JSON_ParseUserInfo()
*
* Description : Parse JSON data to retrieve the user new info.
*
* Argument(s) : p_app_data      Pointer to application data block.
*
*               p_user          Pointer to user object.
*
*               p_buf           Pointer to the connection buffer with the received data.
*
*               data_len        Length of the data received inside the buffer.
*
*               buf_len_max     Maximum size of the connection buffer.
*
*               p_buf_len_used  Variable that will received the length of the data read.
*
* Return(s)   : REST hook state:
*
*                   HTTPs_REST_HOOK_STATE_CONTINUE, to continue the transaction processing.
*                   HTTPs_REST_HOOK_STATE_STAY,     to fall back at the same stage the next time the hook is called.
*                   HTTPs_REST_HOOK_STATE_ERROR,    when an error occurred in the hook processing.
*
* Caller(s)   : AppREST_SetUserInfoHook().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE  AppREST_JSON_ParseUserInfo (       APP_REST_DATA  *p_app_data,
                                                                  APP_REST_USER  *p_user,
                                                                  void           *p_buf,
                                                           const  CPU_SIZE_T      data_len,
                                                           const  CPU_SIZE_T      buf_len_max,
                                                                  CPU_SIZE_T     *p_buf_len_used)
{
    CPU_CHAR              *p_str_val;
    APP_REST_USER_FIELD    field_type;
    HTTPs_REST_HOOK_STATE  state;
    CPU_SIZE_T             val_len;
    CPU_SIZE_T             min_len;
    CPU_SIZE_T             data_rd_len = 0;


   *p_buf_len_used = 0;
                                                                /* ----- PARSE RECEIVED JSON TO RETRIEVE USER INFO ---- */
    field_type = p_app_data->FieldType;
    while (field_type <= APP_REST_USER_FIELD_JOB) {
        state = AppREST_JSON_ParseField(field_type,
                                       &p_str_val,
                                       &val_len,
                                        p_buf,
                                        data_len,
                                        buf_len_max,
                                       &data_rd_len);
        if (state != HTTPs_REST_HOOK_STATE_CONTINUE) {
            p_app_data->FieldType = field_type;
            return (state);
        }

        switch (field_type) {
            case APP_REST_USER_FIELD_FIRST_NAME:
                 min_len = DEF_MIN(val_len, (APP_REST_FIRST_NAME_STR_MAX_LEN - 1));
                 Str_Copy_N(p_user->FirstName, p_str_val, min_len);
                 Str_Copy(p_user->FirstName + min_len, "\0");
                 break;

            case APP_REST_USER_FIELD_LAST_NAME:
                 min_len = DEF_MIN(val_len, (APP_REST_LAST_NAME_STR_MAX_LEN - 1));
                 Str_Copy_N(p_user->LastName, p_str_val, min_len);
                 Str_Copy(p_user->LastName + min_len, "\0");
                 break;

            case APP_REST_USER_FIELD_GENDER:
                 min_len = DEF_MIN(val_len, (APP_REST_GENDER_STR_MAX_LEN - 1));
                 Str_Copy_N(p_user->Gender, p_str_val, min_len);
                 Str_Copy(p_user->Gender + min_len, "\0");
                 break;

            case APP_REST_USER_FIELD_AGE:
                 p_user->Age = Str_ParseNbr_Int32U(p_str_val, DEF_NULL, 10);
                 break;

            case APP_REST_USER_FIELD_JOB:
                 min_len = DEF_MIN(val_len, (APP_REST_JOB_TITLE_STR_MAX_LEN - 1));
                 Str_Copy_N(p_user->JobTitle, p_str_val, min_len);
                 Str_Copy(p_user->JobTitle + min_len, "\0");
                 break;

            default:
                 break;
        }

       *p_buf_len_used = data_rd_len;
        field_type++;
    }

    return (HTTPs_REST_HOOK_STATE_CONTINUE);
}


/*
*********************************************************************************************************
*                                       AppREST_JSON_ParseField()
*
* Description : Parse a JSON to retrieve the value of a given user field.
*
* Argument(s) : field_type      User field type to look for in the JSON:
*                                   APP_REST_USER_FIELD_ID
*                                   APP_REST_USER_FIELD_LINK
*                                   APP_REST_USER_FIELD_FIRST_NAME
*                                   APP_REST_USER_FIELD_lAST_NAME
*                                   APP_REST_USER_FIELD_GENDER
*                                   APP_REST_USER_FIELD_AGE
*                                   APP_REST_USER_FIELD_JOB
*
*               p_val_found     Variable that will received the pointer to the value found.
*
*               p_val_len       Length of the value found.
*
*               p_buf           Pointer to the connection buffer with the received data.
*
*               data_len        Length of the data received inside the buffer.
*
*               buf_len_max     Maximum size of the connection buffer.
*
*               p_buf_len_used  Variable that will received the length of the data read.
*
* Return(s)   : REST hook state:
*
*                   HTTPs_REST_HOOK_STATE_CONTINUE, to continue the transaction processing.
*                   HTTPs_REST_HOOK_STATE_STAY,     to fall back at the same stage the next time the hook is called.
*                   HTTPs_REST_HOOK_STATE_ERROR,    when an error occurred in the hook processing.
*
* Caller(s)   : AppREST_JSON_ParseUser(),
*               AppREST_JSON_ParseUserInfo().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_STATE  AppREST_JSON_ParseField (       APP_REST_USER_FIELD    field_type,
                                                               CPU_CHAR             **p_val_found,
                                                               CPU_SIZE_T            *p_val_len,
                                                               void                  *p_buf,
                                                        const  CPU_SIZE_T             data_len,
                                                        const  CPU_SIZE_T             buf_len_max,
                                                               CPU_SIZE_T            *p_buf_len_used)
{
    CPU_CHAR    *p_str;
    CPU_CHAR    *p_str_tmp;
    CPU_CHAR    *p_field_name;
    CPU_SIZE_T   field_max_len;
    CPU_SIZE_T   data_len_max;
    CPU_SIZE_T   str_len;


    switch (field_type) {
        case APP_REST_USER_FIELD_ID:
             p_field_name  = APP_REST_JSON_KEY_ID_STR_NAME;
             field_max_len = 3;
             break;

        case APP_REST_USER_FIELD_LINK:
             p_field_name  = APP_REST_JSON_KEY_LINK_STR_NAME;
             field_max_len = APP_REST_LINK_STR_MAX_LEN;
             break;

        case APP_REST_USER_FIELD_FIRST_NAME:
             p_field_name  = APP_REST_JSON_KEY_FIRST_NAME_STR_NAME;
             field_max_len = APP_REST_FIRST_NAME_STR_MAX_LEN;
             break;

        case APP_REST_USER_FIELD_LAST_NAME:
             p_field_name  = APP_REST_JSON_KEY_LAST_NAME_STR_NAME;
             field_max_len = APP_REST_LAST_NAME_STR_MAX_LEN;
             break;

        case APP_REST_USER_FIELD_GENDER:
             p_field_name  = APP_REST_JSON_KEY_GENDER_STR_NAME;
             field_max_len = 7;
             break;

        case APP_REST_USER_FIELD_AGE:
             p_field_name  = APP_REST_JSON_KEY_AGE_STR_NAME;
             field_max_len = 3;
             break;

        case APP_REST_USER_FIELD_JOB:
             p_field_name  = APP_REST_JSON_KEY_JOB_STR_NAME;
             field_max_len = APP_REST_JOB_TITLE_STR_MAX_LEN;
             break;

        default:
            goto exit_error;
    }

    data_len_max = APP_REST_JSON_FIELD_MAX_CHARS_NBR +
                   sizeof(p_field_name)              +
                   field_max_len;

    if (data_len_max > buf_len_max) {
        goto exit_error;
    }

    p_str   = p_buf;
    str_len = Str_Len_N(p_str, data_len);

                                                                /* Found field key.                                     */
    p_str_tmp = Str_Str_N(p_str, p_field_name, str_len);
    if (p_str_tmp == DEF_NULL) {
        goto exit_stay;
    }
    str_len -= p_str_tmp - p_str;
    p_str    = p_str_tmp;
                                                                /* Found " char to found end of field key.              */
    p_str_tmp = Str_Char_N(p_str, str_len, ASCII_CHAR_QUOTATION_MARK);
    if (p_str_tmp == DEF_NULL) {
        goto exit_stay;
    }
    str_len -= p_str_tmp - p_str;
    if (str_len <= 0) {
        goto exit_stay;
    }
    p_str_tmp++;
    str_len--;
    p_str = p_str_tmp;
                                                                /* Found " char to found start of field value.          */
    p_str_tmp = Str_Char_N(p_str, str_len, ASCII_CHAR_QUOTATION_MARK);
    if (p_str_tmp == DEF_NULL) {
        goto exit_stay;
    }
    str_len -= p_str_tmp - p_str;
    if (str_len <= 0) {
        goto exit_stay;
    }
    p_str_tmp++;
    str_len--;
    p_str = p_str_tmp;
                                                                /* Found " char to found end of field value.            */
    p_str_tmp = Str_Char_N(p_str, str_len, ASCII_CHAR_QUOTATION_MARK);
    if (p_str_tmp == DEF_NULL) {
        goto exit_stay;
    }
    str_len -= p_str_tmp - p_str;

   *p_val_found = p_str;
   *p_val_len = p_str_tmp - p_str;

    goto exit_continue;


exit_error:
    return (HTTPs_REST_HOOK_STATE_ERROR);

exit_stay:
   *p_buf_len_used = 0;
    return (HTTPs_REST_HOOK_STATE_STAY);

exit_continue:
   *p_buf_len_used = data_len - str_len;
    return (HTTPs_REST_HOOK_STATE_CONTINUE);
}


/*
*********************************************************************************************************
*                                          AppREST_FindUser()
*
* Description : Find an already existing user in the user list based on the given user ID.
*
* Argument(s) : user_id     User ID of the user to look for.
*
* Return(s)   : Pointer to the user object found.
*               DEF_NULL, if no user is found.
*
* Caller(s)   : AppREST_DeleteUserHook(),
*               AppREST_GetUserInfoHook(),
*               AppREST_SetUserInfoHook().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  APP_REST_USER  *AppREST_FindUser (CPU_INT08U  user_id)
{
    APP_REST_USER  *p_user;


    p_user = AppREST_UserFirstPtr;
    while (p_user != DEF_NULL) {

        if (p_user->ID == user_id) {
            break;
        }
        p_user = p_user->NextPtr;
    }

    return (p_user);
}

/*
*********************************************************************************************************
*                                           AppREST_GetUser()
*
* Description : Get a user object from the user pool and add it to the user list.
*
* Argument(s) : none.
*
* Return(s)   : Pointer to the user object retrieved.
*               DEF_NULL, if no block available or in case of an error.
*
* Caller(s)   : AppREST_CreateUserHook().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  APP_REST_USER  *AppREST_GetUser (void)
{
    APP_REST_USER  *p_user;
    LIB_ERR         err_lib;


    p_user = (APP_REST_USER *)Mem_DynPoolBlkGet(&AppREST_UserPool,
                                                &err_lib);
    if (p_user == DEF_NULL) {
        return (DEF_NULL);
    }

    if (AppREST_UserFirstPtr == DEF_NULL) {
        p_user->PrevPtr = DEF_NULL;
        p_user->NextPtr = DEF_NULL;
        AppREST_UserFirstPtr = p_user;
        AppREST_UserLastPtr = p_user;
    } else {
        p_user->NextPtr = DEF_NULL;
        p_user->PrevPtr = AppREST_UserLastPtr;
        AppREST_UserLastPtr->NextPtr = p_user;
        AppREST_UserLastPtr = p_user;
    }

    p_user->ID       = AppREST_GetUserID();
    p_user->Age      = 0;
    Str_Copy_N(p_user->FirstName, "", APP_REST_FIRST_NAME_STR_MAX_LEN);
    Str_Copy_N(p_user->LastName, "", APP_REST_LAST_NAME_STR_MAX_LEN);
    Str_Copy_N(p_user->Gender, "", APP_REST_GENDER_STR_MAX_LEN);
    Str_Copy_N(p_user->JobTitle, "", APP_REST_JOB_TITLE_STR_MAX_LEN);
    p_user->RdRefCtr = 0;
    p_user->WrRefCtr = 0;

    (void)err_lib;

    return (p_user);
}


/*
*********************************************************************************************************
*                                         AppREST_RemoveUser()
*
* Description : Remove a user object from the user list and free it to the user pool.
*
* Argument(s) : p_user  Pointer to the user object to remove and free.
*
* Return(s)   : none.
*
* Caller(s)   : AppREST_DeleteUserHook().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AppREST_RemoveUser (APP_REST_USER  *p_user)
{
    LIB_ERR  err_lib;


    if (AppREST_UserFirstPtr == DEF_NULL) {
        return;
    }

    if (p_user == AppREST_UserFirstPtr) {

        if (p_user->NextPtr != DEF_NULL) {
            AppREST_UserFirstPtr          = p_user->NextPtr;
            AppREST_UserFirstPtr->PrevPtr = DEF_NULL;
            p_user->NextPtr               = DEF_NULL;
        } else {
            AppREST_UserFirstPtr = DEF_NULL;
            AppREST_UserLastPtr  = DEF_NULL;
        }
    } else if (p_user == AppREST_UserLastPtr) {
        AppREST_UserLastPtr          = p_user->PrevPtr;
        AppREST_UserLastPtr->NextPtr = DEF_NULL;
        p_user->PrevPtr              = DEF_NULL;
    } else {
        (p_user->PrevPtr)->NextPtr = p_user->NextPtr;
        (p_user->NextPtr)->PrevPtr = p_user->PrevPtr;
         p_user->NextPtr           = DEF_NULL;
         p_user->PrevPtr           = DEF_NULL;
    }

    (void)AppREST_GetFreeUserID(p_user->ID);

    Mem_DynPoolBlkFree(&AppREST_UserPool,
                        p_user,
                       &err_lib);

    (void)err_lib;
}


/*
*********************************************************************************************************
*                                          AppREST_GetUserID()
*
* Description : Get a new user ID from the free user ID list or create a new ID.
*
* Argument(s) : none.
*
* Return(s)   : The new user ID number.
*
* Caller(s)   : AppREST_GetUser().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_INT08U  AppREST_GetUserID (void)
{
    CPU_INT08U  user_id;

    if (AppREST_FreeUserID_ListPtr != DEF_NULL) {
        user_id = AppREST_FreeUserID_ListPtr->ID;
        AppREST_RemoveFreeUserID();
    } else {
        user_id = AppREST_UserCtr + 1;
    }

    return (user_id);
}


/*
*********************************************************************************************************
*                                        AppREST_GetFreeUserID()
*
* Description : Get from the pool a free user ID object.
*
* Argument(s) : user_id     User ID number.
*
* Return(s)   : Pointer to the free user ID object.
*
* Caller(s)   : AppREST_RemoveUser().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  APP_REST_FREE_USER_ID  *AppREST_GetFreeUserID (CPU_INT08U  user_id)
{
    APP_REST_FREE_USER_ID  *p_free_user_id;
    LIB_ERR                 err_lib;


    p_free_user_id = (APP_REST_FREE_USER_ID *)Mem_DynPoolBlkGet(&AppREST_FreeUserID_Pool,
                                                                &err_lib);
    if (p_free_user_id == DEF_NULL) {
        return (DEF_NULL);
    }

    p_free_user_id->ID = user_id;

    if (AppREST_FreeUserID_ListPtr == DEF_NULL) {
        AppREST_FreeUserID_ListPtr = p_free_user_id;
        p_free_user_id->NextPtr    = DEF_NULL;
    } else {
        p_free_user_id->NextPtr    = AppREST_FreeUserID_ListPtr;
        AppREST_FreeUserID_ListPtr = p_free_user_id;
    }

    (void)err_lib;

    return (p_free_user_id);
}


/*
*********************************************************************************************************
*                                      AppREST_RemoveFreeUserID()
*
* Description : Remove from the free user ID list a block to put it back in the pool.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : AppREST_GetUserID().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AppREST_RemoveFreeUserID (void)
{
    APP_REST_FREE_USER_ID  *p_free_user_id;
    LIB_ERR                 err_lib;


    p_free_user_id             = AppREST_FreeUserID_ListPtr;
    AppREST_FreeUserID_ListPtr = p_free_user_id->NextPtr;
    p_free_user_id->NextPtr    = DEF_NULL;

    Mem_DynPoolBlkFree(&AppREST_FreeUserID_Pool,
                        p_free_user_id,
                       &err_lib);

    (void)err_lib;
}


/*
*********************************************************************************************************
*                                         AppREST_GetDataBlk()
*
* Description : Get an application data block from the pool.
*
* Argument(s) : none.
*
* Return(s)   : Pointer to the retrieved application data block.
*               DEF_NULL if no block available or in case of an error.
*
* Caller(s)   : AppREST_CreateUserHook(),
*               AppREST_DeleteUserHook(),
*               AppREST_GetUserInfoHook(),
*               AppREST_GetUserListHook(),
*               AppREST_SetUserInfoHook().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  APP_REST_DATA  *AppREST_GetDataBlk (void)
{
    APP_REST_DATA  *p_app_data;
    LIB_ERR         err_lib;


    p_app_data = (APP_REST_DATA *)Mem_DynPoolBlkGet(&AppREST_DataPool,
                                                    &err_lib);
    if (p_app_data == DEF_NULL) {
        return (DEF_NULL);
    }

    if (AppREST_DataListPtr == DEF_NULL) {
        AppREST_DataListPtr = p_app_data;
        p_app_data->NextPtr = DEF_NULL;
    } else {
        p_app_data->NextPtr = AppREST_DataListPtr;
        AppREST_DataListPtr = p_app_data;
    }

    p_app_data->UserPtr   = DEF_NULL;
    p_app_data->FieldType = APP_REST_USER_FIELD_FIRST_NAME;

    (void)err_lib;

    return (p_app_data);
}


/*
*********************************************************************************************************
*                                        AppREST_RemoveDataBlk()
*
* Description : Remove an application data block from the list and put it back in the free pool.
*
* Argument(s) : p_app_data  Pointer to the application data block to free.
*
* Return(s)   : none.
*
* Caller(s)   : AppREST_CreateUserHook(),
*               AppREST_DeleteUserHook(),
*               AppREST_GetUserInfoHook(),
*               AppREST_GetUserListHook(),
*               AppREST_SetUserInfoHook().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AppREST_RemoveDataBlk (APP_REST_DATA  *p_app_data)
{
    APP_REST_DATA  *p_item;
    APP_REST_DATA  *p_item_prev;
    LIB_ERR         err_lib;


    p_item      = AppREST_DataListPtr;
    p_item_prev = DEF_NULL;
    while (p_item != DEF_NULL) {

        if (p_app_data == AppREST_DataListPtr) {
            AppREST_DataListPtr = p_app_data->NextPtr;
        }

        if (p_item == p_app_data) {
            if (p_item_prev != DEF_NULL) {
                p_item_prev->NextPtr = p_item->NextPtr;
            }
            p_item->NextPtr = DEF_NULL;
            break;
        }

        p_item_prev = p_item;
        p_item      = p_item->NextPtr;
    }

    Mem_DynPoolBlkFree(&AppREST_DataPool,
                        p_app_data,
                       &err_lib);

    (void)err_lib;
}


/*
*********************************************************************************************************
*                                      AppREST_GetStrLenOfIntDec()
*
* Description : Get the number of decimals of a integer number.
*
* Argument(s) : i       Integer number.
*
* Return(s)   : Number of decimals.
*
* Caller(s)   : AppREST_JSON_GetUserFieldSize(),
*               AppREST_JSON_GetUserInfoSize(),
*               AppREST_JSON_ParseUser(),
*               AppREST_JSON_WrUserInfoToBuf(),
*               AppREST_JSON_WrUserToBuf(),
*               AppREST_ResourcesInit().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_SIZE_T  AppREST_GetStrLenOfIntDec (CPU_INT32U  i)
{
    if(i < 10) {
        return 1;
    } else if(i < 100) {
        return 2;
    } else if(i < 1000) {
        return 3;
    } else if(i < 10000) {
        return 4;
    } else if(i < 100000) {
        return 5;
    } else if(i < 1000000) {
        return 6;
    } else if(i < 10000000) {
        return 7;
    } else if(i < 100000000) {
        return 8;
    } else {
        return 9;
    }
}
