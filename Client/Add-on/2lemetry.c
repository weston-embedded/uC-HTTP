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
*                            HTTP CLIENT 2LEMETRY APPLICATION SOURCE CODE
*
* Filename : 2lemetry.c
* Version  : V3.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/
#define  MICRIUM_SOURCE
#define  TWOLEMETRY_MODULE


#include  <lib_def.h>

#include  <Common/http.h>
#include  <Client/Source/http-c.h>
#include  <http-c_cfg.h>

#include  "2lemetry.h"

#include  <Source/net_util.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef struct twolemetry_conn {
        TWOLEMETRY_RX_DATA  RxDataFnct;
        void               *UserCtxPtr;
}TWOLEMETRY_CONN;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/
static  CPU_BOOLEAN Twolemetry_ReqSend                (CPU_CHAR                   *p_url,
                                                       HTTP_METHOD                 method,
                                                       HTTPc_CONN_OBJ             *p_conn,
                                                       HTTPc_REQ_OBJ              *p_req,
                                                       CPU_CHAR                   *p_credentials);
#if 0
static  CPU_INT08U  Twolemetry_ReqPrepareQueryStr     (HTTPc_KEY_VAL             **p_tbl,
                                                       CPU_CHAR                   *p_client_id,
                                                       CPU_CHAR                   *p_topic,
                                                       CPU_CHAR                   *p_qos);

static  CPU_INT08U  Twolemetry_ReqPrepareQueryStr_SMS (HTTPc_KEY_VAL             **p_tbl,
                                                       CPU_CHAR                   *p_message,
                                                       CPU_CHAR                   *p_to_phone_number,
                                                       CPU_CHAR                   *p_topic);
#endif
static  void        Twolemetry_RespBodyHook           (HTTPc_CONN_OBJ             *p_conn,
                                                       HTTPc_REQ_OBJ              *p_req,
                                                       HTTP_CONTENT_TYPE           content_type,
                                                       void                       *p_data,
                                                       CPU_INT16U                  data_len,
                                                       CPU_BOOLEAN                 last_chunk);

static  CPU_BOOLEAN Twolemetry_Buf_ReqBodyHook        (HTTPc_CONN_OBJ             *p_conn,
                                                       HTTPc_REQ_OBJ              *p_req,
                                                       void                      **p_data,
                                                       CPU_CHAR                   *p_buf,
                                                       CPU_INT16U                  buf_len,
                                                       CPU_INT16U                 *p_data_len);

#ifdef HTTPc_TASK_MODULE_EN

static  void        Twolemetry_ConnCloseCallback      (HTTPc_CONN_OBJ             *p_conn,
                                                       HTTPc_CONN_CLOSE_STATUS     close_status,
                                                       HTTPc_ERR                   err);

static  void        Twolemetry_TransErrCallback       (HTTPc_CONN_OBJ             *p_conn,
                                                       HTTPc_REQ_OBJ              *p_req,
                                                       HTTPc_ERR                   err_code);

static  void        Twolemetry_ConnSecureCallback     (HTTPc_CONN_OBJ             *p_conn,
                                                       HTTPc_REQ_OBJ              *p_req,
                                                       HTTPc_ERR                   err_code);
#endif


/*
*********************************************************************************************************
*                                         Twolemetry_Publish()
*
* Description : Publish MQTT message on selected Topic
*
* Argument(s) : p_client_id     Pointer to the MQTT client ID
*
*               p_topic         Pointer on Topic to publish the MQTT message
*
*                               If DEF_NULL, default value is : {domain}/{stuff}/{thing}
*
*               p_payload       Pointer to JSON formatted MQTT message to publish
*
*               p_qos           Quality of service (values from 0 to 3)
*
*                               If DEF_NULL, default value is 0
*
*               p_credentials   Pointer to Twolemetry credentials ( Key (Username) : Secret(Password) )
*
*
* Return(s)   : DEF_OK,   if HTTPc transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN Twolemetry_Publish (CPU_CHAR    *p_client_id,
                                CPU_CHAR    *p_topic,
                                CPU_CHAR    *p_payload,
                                CPU_CHAR    *p_qos,
                                CPU_CHAR    *p_credentials)

{
    CPU_CHAR               url[TWOLEMETRY_URL_LEN_MAX];
    HTTPc_CONN_OBJ         http_conn;
    HTTPc_REQ_OBJ          http_req;
    HTTPc_ERR              err;
    CPU_BOOLEAN            success;
    HTTP_CONTENT_TYPE      content_type;
    CPU_SIZE_T             content_len;
#if 0
    HTTPc_PARAM_TBL        tbl_obj;
    HTTPc_KEY_VAL         *p_query_str_tbl;
    CPU_INT08U             query_nbr;
#endif

    Mem_Clr(url, TWOLEMETRY_URL_LEN_MAX);
    Mem_Clr(&http_conn, sizeof(http_conn));
    Mem_Clr(&http_req,  sizeof(http_req));


    Str_Copy_N(&url[0], TWOLEMETRY_URL_MESSAGES , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N( &url[0], "?clientid="            , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N( &url[0], p_client_id              , TWOLEMETRY_URL_LEN_MAX);
    if (p_topic != DEF_NULL) {
        Str_Cat_N( &url[0], "&topic="           , TWOLEMETRY_URL_LEN_MAX);
        Str_Cat_N( &url[0], p_topic             , TWOLEMETRY_URL_LEN_MAX);
    }
    if (p_qos != DEF_NULL) {
        Str_Cat_N( &url[0], "&qos="             , TWOLEMETRY_URL_LEN_MAX);
        Str_Cat_N( &url[0], p_qos               , TWOLEMETRY_URL_LEN_MAX);
    }


                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(&http_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(&http_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    http_req.UserDataPtr = p_payload;
#if 0

                                                                /* -------------- SET STRING QUERY DATA --------------- */
    query_nbr = Twolemetry_ReqPrepareQueryStr(&p_query_str_tbl,
                                               p_client_id,
                                               p_topic,
                                               p_qos);
    if (query_nbr <= 0) {
        return (DEF_FAIL);
    }

                                                                  /* ------------ SET STRING QUERY PARAMETERS ----------- */
    tbl_obj.EntryNbr = query_nbr;
    tbl_obj.TblPtr   = (void *)p_query_str_tbl;
    HTTPc_ReqSetParam(&http_req,
                       HTTPc_PARAM_TYPE_REQ_QUERY_STR_TBL,
                      &tbl_obj,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

#endif                                               /* ----------- SET REQUEST BODY PARAMETERS ------------ */
    content_type = HTTP_CONTENT_TYPE_JSON;
    HTTPc_ReqSetParam(&http_req,
                       HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_TYPE,
                      &content_type,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    content_len = Str_Len(p_payload);
    HTTPc_ReqSetParam(&http_req,
                       HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_LEN,
                      &content_len,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }


    HTTPc_ReqSetParam(        &http_req,
                               HTTPc_PARAM_TYPE_REQ_BODY_HOOK,
                      (void *)&Twolemetry_Buf_ReqBodyHook,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    success = Twolemetry_ReqSend( url,
                                  HTTP_METHOD_POST,
                                 &http_conn,
                                 &http_req,
                                  p_credentials);
    if (success != DEF_OK) {
        return (DEF_FAIL);
    }


    return (DEF_OK);
}

/*
*********************************************************************************************************
*                                         Twolemetry_GetData()
*
* Description : Get present data about a thing
*
* Argument(s) : p_domain        Pointer to domain of your application
*
*               p_stuff         Pointer to the group of your thing
*
*               p_thing         Pointer to the thing
*
*               p_whatever      Pointer to whatevers (see note #1)
*
*               p_credentials   Pointer to Twolemetry credentials ( Key (Username) : Secret(Password) )
*
*               fnct            Pointer to the user defined receive data function
*
*               p_ctx           Pointer to user context
*
* Return(s)   : DEF_OK,   if HTTPc transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application
*
* Note(s)     : (1) Two or more values can be passed as comma separated. An '_' can be passed to retrieve all.
*
*********************************************************************************************************
*/

CPU_BOOLEAN Twolemetry_GetData       (CPU_CHAR             *p_domain,
                                      CPU_CHAR             *p_stuff,
                                      CPU_CHAR             *p_thing,
                                      CPU_CHAR             *p_whatever,
                                      CPU_CHAR             *p_credentials,
                                      TWOLEMETRY_RX_DATA    fnct,
                                      void                 *p_ctx)
{
    CPU_CHAR               url[TWOLEMETRY_URL_LEN_MAX];
    HTTPc_CONN_OBJ         http_conn;
    HTTPc_REQ_OBJ          http_req;
    HTTPc_ERR              err;
    CPU_BOOLEAN            success;
    TWOLEMETRY_CONN        conn;


    conn.RxDataFnct = fnct;
    conn.UserCtxPtr = p_ctx;

    Mem_Clr(url, TWOLEMETRY_URL_LEN_MAX);
    Mem_Clr(&http_conn, sizeof(http_conn));
    Mem_Clr(&http_req,  sizeof(http_req));


    Str_Copy_N(&url[0], TWOLEMETRY_URL_DOMAIN , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], p_domain              , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], "/stuff/"             , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], p_stuff               , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], "/thing/"             , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], p_thing               , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], "/present"            , TWOLEMETRY_URL_LEN_MAX);
    if (p_whatever != DEF_NULL) {
        Str_Cat_N (&url[0], "/?whatever="     , TWOLEMETRY_URL_LEN_MAX);
        Str_Cat_N (&url[0], p_whatever        , TWOLEMETRY_URL_LEN_MAX);
    }


                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(&http_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(&http_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }


    http_req.UserDataPtr = &conn;

                                                                /* ------------ SET REQ/RESP HOOK FUNCTIONS ----------- */

    HTTPc_ReqSetParam(        &http_req,
                               HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      (void *)&Twolemetry_RespBodyHook,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
    success = Twolemetry_ReqSend( url,
                                  HTTP_METHOD_GET,
                                 &http_conn,
                                 &http_req,
                                  p_credentials);
    if (success != DEF_OK) {
        return (DEF_FAIL);
    }

    return(DEF_OK);
}
/*
*********************************************************************************************************
*                                         Twolemetry_SendSMS()
*
* Description : Send a SMS message
*
* Argument(s) : p_message           Pointer to the body of the message to be sent
*
*               p_to_phone_number   Pointer to the destination phone number
*
*               p_topic             MQTT topic to publish the SMS result status
*
*                                   If DEF_NULL, no result status is published
*
*               p_domain            Pointer to domain of your application
*
*               p_credentials       Pointer to Twolemetry credentials ( Key (Username) : Secret(Password) )
*
* Return(s)   : DEF_OK,   if HTTPc transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

CPU_BOOLEAN Twolemetry_SendSMS (CPU_CHAR    *p_message,
                                CPU_CHAR    *p_to_phone_number,
                                CPU_CHAR    *p_topic,
                                CPU_CHAR    *p_domain,
                                CPU_CHAR    *p_credentials)
{
    CPU_CHAR               url[TWOLEMETRY_URL_LEN_MAX];
    HTTPc_CONN_OBJ         http_conn;
    HTTPc_REQ_OBJ          http_req;
    HTTPc_ERR              err;
    CPU_BOOLEAN            success;
    CPU_CHAR               message[160];
    CPU_SIZE_T             message_len;

#if 0
    HTTPc_PARAM_TBL        tbl_obj;
    HTTPc_KEY_VAL         *p_query_str_tbl;
    CPU_INT08U             query_nbr;
#endif


    Mem_Clr(url, TWOLEMETRY_URL_LEN_MAX);
    Mem_Clr(&http_conn, sizeof(http_conn));
    Mem_Clr(&http_req,  sizeof(http_req));

    message_len = Str_Len(p_message);
    success = HTTP_URL_EncodeStr ( p_message,
                                   message,
                                  &message_len,
                                   160);
    if (success != DEF_OK) {
        return (DEF_FAIL);
    }

    Str_Copy_N(&url[0], TWOLEMETRY_URL_DOMAIN , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], p_domain              , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], "/sms?message="       , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], message               , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], "&to="                , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], p_to_phone_number     , TWOLEMETRY_URL_LEN_MAX);
    if (p_topic != DEF_NULL) {
        Str_Cat_N (&url[0], "&topic="         , TWOLEMETRY_URL_LEN_MAX);
        Str_Cat_N (&url[0], p_topic           , TWOLEMETRY_URL_LEN_MAX);
    }

                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(&http_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(&http_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#if 0
                                                                /* -------------- SET STRING QUERY DATA --------------- */
    query_nbr = Twolemetry_ReqPrepareQueryStr_SMS(&p_query_str_tbl,
                                                   p_message,
                                                   p_to_phone_number,
                                                   p_topic);
    if (query_nbr <= 0) {
        return (DEF_FAIL);
    }

                                                                  /* ------------ SET STRING QUERY PARAMETERS ----------- */
    tbl_obj.EntryNbr = query_nbr;
    tbl_obj.TblPtr   = (void *)p_query_str_tbl;
    HTTPc_ReqSetParam(&http_req,
                       HTTPc_PARAM_TYPE_REQ_QUERY_STR_TBL,
                      &tbl_obj,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif
    success = Twolemetry_ReqSend( url,
                                  HTTP_METHOD_POST,
                                 &http_conn,
                                 &http_req,
                                  p_credentials);
    if (success != DEF_OK) {
        return (DEF_FAIL);
    }


    return (DEF_OK);
}

/*
*********************************************************************************************************
*                                       Twolemetry_PushStoreKey()
*
* Description : Save a Key-Value pair in store
*
* Argument(s) : p_domain        Pointer to domain of your application
*
*               p_key           Pointer to key (RowKey or ID)
*
*               p_kvp           Pointer to Key-Value pair to save
*
*               protect         Protected User Store
*                               DEF_TRUE
*                               DEF_FALSE
*
*               p_credentials   Pointer to Twolemetry credentials ( Key (Username) : Secret(Password) )
*
* Return(s)   : DEF_OK,   if HTTPc transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

CPU_BOOLEAN Twolemetry_PushStoreKey  (CPU_CHAR         *p_domain,
                                      CPU_CHAR         *p_key,
                                      CPU_CHAR         *p_kvp,
                                      CPU_BOOLEAN       protect,
                                      CPU_CHAR         *p_credentials)
{
    CPU_CHAR               url[TWOLEMETRY_URL_LEN_MAX];
    HTTPc_CONN_OBJ         http_conn;
    HTTPc_REQ_OBJ          http_req;
    HTTPc_ERR              err;
    CPU_BOOLEAN            success;
    CPU_CHAR               prot_user_store[5];
    CPU_CHAR               kvp[100];
    CPU_SIZE_T             kvp_len;

    Mem_Clr(url, TWOLEMETRY_URL_LEN_MAX);
    Mem_Clr(kvp, 50);
    Mem_Clr(&http_conn, sizeof(http_conn));
    Mem_Clr(&http_req,  sizeof(http_req));

    if (protect == DEF_TRUE) {
        Str_Copy(prot_user_store, "true");
    } else {
        Str_Copy(prot_user_store, "false");
    }

    kvp_len = Str_Len(p_kvp);
    success = HTTP_URL_EncodeStr ( p_kvp,
                                   kvp,
                                  &kvp_len,
                                   100);
    if (success != DEF_OK) {
        return (DEF_FAIL);
    }

    Str_Copy_N(&url[0], TWOLEMETRY_URL_DOMAIN , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], p_domain              , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], "/store/"             , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], p_key                 , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], "?protect="           , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], prot_user_store       , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], "&keyValue="          , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], kvp                   , TWOLEMETRY_URL_LEN_MAX);


                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(&http_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(&http_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    success = Twolemetry_ReqSend( url,
                                  HTTP_METHOD_POST,
                                 &http_conn,
                                 &http_req,
                                  p_credentials);
    if (success != DEF_OK) {
        return (DEF_FAIL);
    }


    return (DEF_OK);

}

/*
*********************************************************************************************************
*                                       Twolemetry_GetStoreKey()
*
* Description : Retrieve Key-Value pair from store
*
* Argument(s) : p_domain        Pointer to domain of your application
*
*               p_key           Pointer to key (RowKey or ID)
*
*               protect         Protected User Store
*                               DEF_TRUE
*                               DEF_FALSE
*
*               p_credentials   Pointer to Twolemetry credentials ( Key (Username) : Secret(Password) )
*
*               fnct            Pointer to the user defined receive data function
*
*               p_ctx           Pointer to user context
*
* Return(s)   : DEF_OK,   if HTTPc transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

CPU_BOOLEAN Twolemetry_GetStoreKey   (CPU_CHAR            *p_domain,
                                      CPU_CHAR            *p_key,
                                      CPU_BOOLEAN          protect,
                                      CPU_CHAR            *p_credentials,
                                      TWOLEMETRY_RX_DATA   fnct,
                                      void                *p_ctx)
{
    CPU_CHAR               url[TWOLEMETRY_URL_LEN_MAX];
    HTTPc_CONN_OBJ         http_conn;
    HTTPc_REQ_OBJ          http_req;
    HTTPc_ERR              err;
    CPU_BOOLEAN            success;
    CPU_CHAR               prot_user_store[5];
    TWOLEMETRY_CONN        conn;


    conn.RxDataFnct = fnct;
    conn.UserCtxPtr = p_ctx;

    Mem_Clr(url, TWOLEMETRY_URL_LEN_MAX);
    Mem_Clr(&http_conn, sizeof(http_conn));
    Mem_Clr(&http_req,  sizeof(http_req));

    if (protect == DEF_TRUE) {
        Str_Copy(prot_user_store, "true");
    } else {
        Str_Copy(prot_user_store, "false");
    }


    Str_Copy_N(&url[0], TWOLEMETRY_URL_DOMAIN , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], p_domain              , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], "/store/"             , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], p_key                 , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], "?protect="           , TWOLEMETRY_URL_LEN_MAX);
    Str_Cat_N (&url[0], prot_user_store       , TWOLEMETRY_URL_LEN_MAX);


                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(&http_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(&http_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }


    http_req.UserDataPtr = &conn;

                                                                /* ------------ SET REQ/RESP HOOK FUNCTIONS ----------- */

    HTTPc_ReqSetParam(        &http_req,
                               HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      (void *)&Twolemetry_RespBodyHook,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
    success = Twolemetry_ReqSend( url,
                                  HTTP_METHOD_GET,
                                 &http_conn,
                                 &http_req,
                                  p_credentials);
    if (success != DEF_OK) {
        return (DEF_FAIL);
    }

    return(DEF_OK);
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
*                                         Twolemetry_ReqSend()
*
* Description : Send a request.
*
* Argument(s) : p_url           Pointer to complete URI (or only resource path) of the request.
*
*               method          HTTP method of the request.
*
*               p_conn          Pointer to valid HTTPc Connection on which request will occurred.
*
*               p_req           Pointer to request to send.
*
*               p_credentials   Pointer to Twolemetry credentials ( Key (Username) : Secret(Password) )
*
* Return(s)   : DEF_OK,   if HTTP transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Twolemetry_SendSMS()
*
*               Twolemetry_Publish()
*
* Note(s)     : None.
*********************************************************************************************************
*/

static CPU_BOOLEAN  Twolemetry_ReqSend (CPU_CHAR          *p_url,
                                        HTTP_METHOD        method,
                                        HTTPc_CONN_OBJ    *p_conn,
                                        HTTPc_REQ_OBJ     *p_req,
                                        CPU_CHAR          *p_credentials)
{
    HTTPc_HDR           hdr;
    CPU_CHAR            hdr_val[TWOLEMETRY_CFG_HDR_VAL_LEN_MAX];
    CPU_CHAR            buf[TWOLEMETRY_CFG_CONN_BUF_SIZE];
    HTTPc_RESP_OBJ      http_resp;
    HTTPc_PARAM_TBL     tbl_obj;
    CPU_SIZE_T          str_len;
    CPU_BOOLEAN         result;
    HTTPc_ERR           err;
    CPU_CHAR            credentials_b64[105];
    NET_ERR             err_net;


    NetUtil_Base64Encode (p_credentials,
                          Str_Len(p_credentials),
                          &credentials_b64[0],
                          105,
                          &err_net);
    if (err_net != NET_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* Prepare the Authorization header                     */
    Mem_Clr  (hdr_val, TWOLEMETRY_CFG_HDR_VAL_LEN_MAX);
    Str_Cat_N(hdr_val, "Basic "           ,TWOLEMETRY_CFG_HDR_VAL_LEN_MAX);
    Str_Cat_N(hdr_val, &credentials_b64[0],TWOLEMETRY_CFG_HDR_VAL_LEN_MAX);

    hdr.ValPtr   = hdr_val;
    hdr.ValLen   = Str_Len_N(hdr_val, TWOLEMETRY_CFG_HDR_VAL_LEN_MAX);
    hdr.HdrField = HTTP_HDR_FIELD_AUTHORIZATION;

                                                                /* ------------- SET CONNECTION CALLBACKS ------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ConnSetParam(         p_conn,
                                HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       (void *)&Twolemetry_ConnCloseCallback,
                               &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ConnSetParam(         p_conn,
                                HTTPc_PARAM_TYPE_SECURE_TRUST_CALLBACK,
                       (void *)&Twolemetry_ConnSecureCallback,
                               &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif



                                                                /* ------------ SET REQ/HDR PARAM          ----------- */
    tbl_obj.EntryNbr =  1;
    tbl_obj.TblPtr   = &hdr;
    HTTPc_ReqSetParam( p_req,
                       HTTPc_PARAM_TYPE_REQ_HDR_TBL,
                      &tbl_obj,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
                                                                /* --------- SET REQ/RESP CALLBACK FUNCTIONS ---------- */

#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ReqSetParam(       p_req,
                             HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                    (void *)&Twolemetry_TransErrCallback,
                            &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------------- OPEN CONNECTION ------------------ */
    str_len = Str_Len(TWOLEMETRY_API_HOSTNAME);
    result = HTTPc_ConnOpen(p_conn,
                            buf,
                            TWOLEMETRY_CFG_CONN_BUF_SIZE,
                            TWOLEMETRY_API_HOSTNAME,
                            str_len,
                            HTTPc_FLAG_NONE,
                           &err);
    if (err != HTTPc_ERR_NONE ||
        result != DEF_OK        ) {
        return (DEF_FAIL);
    }
                                                                /* ---------------- SEND HTTP REQUEST ----------------- */

    Mem_Clr(&http_resp, sizeof(http_resp));

    str_len = Str_Len(p_url);
    result = HTTPc_ReqSend(p_conn,
                           p_req,
                          &http_resp,
                           method,
                           p_url,
                           str_len,
                           HTTPc_FLAG_NONE,
                          &err);
    if (err != HTTPc_ERR_NONE ||
        result != DEF_OK        ) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}

#if 0
/*
*********************************************************************************************************
*                                    Twolemetry_ReqPrepareQueryStr()
*
* Description : Prepare query string table for MQTT publish.
*
* Argument(s) : p_tbl           Variable that will received the pointer to the Query String table.
*
*               p_client_id     Pointer to the MQTT client ID
*
*               p_topic         Pointer on Topic to publish the MQTT message
*
*                               If DEF_NULL, default value is : {domain}/{stuff}/{thing}
*
*               p_qos           Quality of service (values from 0 to 3)
*
*                               If DEF_NULL, default value is 0
*
* Return(s)   : Number of fields in the Query String table.
*
* Caller(s)   : Twolemetry_Publish().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  CPU_INT08U  Twolemetry_ReqPrepareQueryStr (HTTPc_KEY_VAL  **p_tbl,
                                                   CPU_CHAR        *p_client_id,
                                                   CPU_CHAR        *p_topic,
                                                   CPU_CHAR        *p_qos)
{
    HTTPc_KEY_VAL  *p_key_val;
    CPU_INT08U      query_nbr;

                                                                /* ----------------- SET FIRST QUERY ------------------ */
    p_key_val         = &Twolemetry_ReqQueryStrTbl[0];
    p_key_val->KeyPtr = &Twolemetry_ReqQueryStrKeyTbl[0][0];
    p_key_val->ValPtr = &Twolemetry_ReqQueryStrValTbl[0][0];

    (void)Str_Copy_N(p_key_val->KeyPtr, "clientid", TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX);
    (void)Str_Copy_N(p_key_val->ValPtr, p_client_id,TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX);

    p_key_val->KeyLen = Str_Len_N(p_key_val->KeyPtr, TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX);
    p_key_val->ValLen = Str_Len_N(p_key_val->ValPtr, TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX);

    query_nbr = 1;
    if (p_topic != DEF_NULL) {
                                                                /* ----------------- SET SECOND QUERY ----------------- */
        p_key_val         = &Twolemetry_ReqQueryStrTbl[1];
        p_key_val->KeyPtr = &Twolemetry_ReqQueryStrKeyTbl[1][0];
        p_key_val->ValPtr = &Twolemetry_ReqQueryStrValTbl[1][0];

        (void)Str_Copy_N(p_key_val->KeyPtr, "topic", TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX);
        (void)Str_Copy_N(p_key_val->ValPtr, p_topic, TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX);

        p_key_val->KeyLen = Str_Len_N(p_key_val->KeyPtr, TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX);
        p_key_val->ValLen = Str_Len_N(p_key_val->ValPtr, TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX);
        query_nbr++;
    }
    if (p_qos != DEF_NULL) {
                                                                /* ----------------- SET THIRD QUERY ------------------ */
        p_key_val         = &Twolemetry_ReqQueryStrTbl[1];
        p_key_val->KeyPtr = &Twolemetry_ReqQueryStrKeyTbl[1][0];
        p_key_val->ValPtr = &Twolemetry_ReqQueryStrValTbl[1][0];

        (void)Str_Copy_N(p_key_val->KeyPtr, "qos", TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX);
        (void)Str_Copy_N(p_key_val->ValPtr, p_qos, TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX);

        p_key_val->KeyLen = Str_Len_N(p_key_val->KeyPtr, TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX);
        p_key_val->ValLen = Str_Len_N(p_key_val->ValPtr, TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX);

        query_nbr++;
    }


   *p_tbl = &Twolemetry_ReqQueryStrTbl[0];

    return (query_nbr);

}


/*
*********************************************************************************************************
*                                  Twolemetry_ReqPrepareQueryStr_SMS()
*
* Description : Prepare query string table for sms send.
*
* Argument(s) : p_tbl               Variable that will received the pointer to the Query String table.
*
*               p_message           Pointer to the body of the message to be sent
*
*               p_to_phone_number   Pointer to the destination phone number
*
*               p_topic             MQTT topic to publish the SMS result status
*
*                                   If DEF_NULL, no result status is published
*
* Return(s)   : Number of fields in the Query String table.
*
* Caller(s)   : Twolemetry_SendSMS().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  CPU_INT08U  Twolemetry_ReqPrepareQueryStr_SMS (HTTPc_KEY_VAL  **p_tbl,
                                                       CPU_CHAR        *p_message,
                                                       CPU_CHAR        *p_to_phone_number,
                                                       CPU_CHAR        *p_topic)
{

    HTTPc_KEY_VAL  *p_key_val;
    CPU_INT08U      query_nbr;

                                                                /* ----------------- SET FIRST QUERY ------------------ */
    p_key_val         = &Twolemetry_ReqQueryStrTbl[0];
    p_key_val->KeyPtr = &Twolemetry_ReqQueryStrKeyTbl[0][0];
    p_key_val->ValPtr = &Twolemetry_ReqQueryStrValTbl[0][0];

   (void)Str_Copy_N(p_key_val->KeyPtr, "message", TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX);
   (void)Str_Copy_N(p_key_val->ValPtr, p_message,TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX);

    p_key_val->KeyLen = Str_Len_N(p_key_val->KeyPtr, TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX);
    p_key_val->ValLen = Str_Len_N(p_key_val->ValPtr, TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX);

                                                                /* ----------------- SET SECOND QUERY ----------------- */
    p_key_val         = &Twolemetry_ReqQueryStrTbl[1];
    p_key_val->KeyPtr = &Twolemetry_ReqQueryStrKeyTbl[1][0];
    p_key_val->ValPtr = &Twolemetry_ReqQueryStrValTbl[1][0];

   (void)Str_Copy_N(p_key_val->KeyPtr, "to", TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX);
   (void)Str_Copy_N(p_key_val->ValPtr, p_to_phone_number, TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX);

    p_key_val->KeyLen = Str_Len_N(p_key_val->KeyPtr, TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX);
    p_key_val->ValLen = Str_Len_N(p_key_val->ValPtr, TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX);

    query_nbr = 2;
    if (p_topic != DEF_NULL) {
                                                                /* ----------------- SET THIRD QUERY ------------------ */
        p_key_val         = &Twolemetry_ReqQueryStrTbl[1];
        p_key_val->KeyPtr = &Twolemetry_ReqQueryStrKeyTbl[1][0];
        p_key_val->ValPtr = &Twolemetry_ReqQueryStrValTbl[1][0];

        (void)Str_Copy_N(p_key_val->KeyPtr, "topic", TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX);
        (void)Str_Copy_N(p_key_val->ValPtr, p_topic, TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX);

        p_key_val->KeyLen = Str_Len_N(p_key_val->KeyPtr, TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX);
        p_key_val->ValLen = Str_Len_N(p_key_val->ValPtr, TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX);

        query_nbr++;
    }


   *p_tbl = &Twolemetry_ReqQueryStrTbl[0];
    return(query_nbr);
}
#endif

/*
*********************************************************************************************************
*                                       Twolemetry_RespBodyHook()
*
* Description : Retrieve data in HTTP Response received.
*
* Argument(s) : p_conn          Pointer to current HTTPc Connection object.
*
*               p_req           Pointer to current HTTPc Request object.
*
*               content_type    HTTP Content Type of the HTTP Response body's data.
*
*               p_data          Pointer to a data piece of the HTTP Response body.
*
*               data_len        Length of the data piece received.
*
*               last_chunk      DEF_YES, if this is the last piece of data.
*
*                               DEF_NO,  if more data is up coming.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcResp_BodyStd()   via 'p_req->OnBodyRx()'
*               HTTPcResp_BodyChunk() via 'p_req->OnBodyRx()'
*
* Note(s)     : None.
*********************************************************************************************************
*/

static void  Twolemetry_RespBodyHook (HTTPc_CONN_OBJ     *p_conn,
                                      HTTPc_REQ_OBJ      *p_req,
                                      HTTP_CONTENT_TYPE   content_type,
                                      void               *p_data,
                                      CPU_INT16U          data_len,
                                      CPU_BOOLEAN         last_chunk)
{
    TWOLEMETRY_CONN *p_twolemetry_conn;


    p_twolemetry_conn = (TWOLEMETRY_CONN *)p_req->UserDataPtr;
    if (p_twolemetry_conn != DEF_NULL &&
        p_twolemetry_conn->RxDataFnct != DEF_NULL) {
        p_twolemetry_conn->RxDataFnct(p_data,
                                      data_len,
                                      last_chunk,
                                      p_twolemetry_conn->UserCtxPtr);
    }
}

/*
*********************************************************************************************************
*                                          Twolemetry_Buf_ReqBodyHook()
*
* Description : Specify the data to be sent in the Request body.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection object.
*
*               p_req       Pointer to current HTTPc Request object.
*
*               p_data      Variable that will received the pointer to the data to include in the HTTP request.
*
*               p_buf       Pointer to HTTP transmit buffer.
*
*               buf_len     Length of space remaining in  the HTTP transmit buffer.
*
*               p_data_len  Length of the data.
*
* Return(s)   : DEF_YES, if all data to transmit was passed by the application
*               DEF_NO,  if data still remaining to be sent.
*
* Caller(s)   : HTTPcReq_BodyData() via 'p_req->OnBodyTx()'
*
* Note(s)     : None.
*********************************************************************************************************
*/

static CPU_BOOLEAN  Twolemetry_Buf_ReqBodyHook  (HTTPc_CONN_OBJ     *p_conn,
                                                 HTTPc_REQ_OBJ      *p_req,
                                                 void              **p_data,
                                                 CPU_CHAR           *p_buf,
                                                 CPU_INT16U          buf_len,
                                                 CPU_INT16U         *p_data_len)
{
    CPU_SIZE_T str_len;

    *p_data = p_req->UserDataPtr;

    str_len = Str_Len(*p_data);
    *p_data_len = str_len;


    Str_Copy_N(p_buf, *p_data, str_len);


    return (DEF_YES);
}


/*
*********************************************************************************************************
*                                     Twolemetry_ConnCloseCallback()
*
* Description : Callback to notify application that an HTTP connection was closed.
*
* Argument(s) : p_conn          Pointer to current HTTPc Connection.
*
*               close_status    Status of the connection closing:
*                                   HTTPc_CONN_CLOSE_STATUS_ERR_INTERNAL
*                                   HTTPc_CONN_CLOSE_STATUS_SERVER
*                                   HTTPc_CONN_CLOSE_STATUS_NO_PERSISTENT
*                                   HTTPc_CONN_CLOSE_STATUS_APP
*
*               err             Error Code when connection was closed.
*
* Return(s)   : None.
*
* Caller(s)   : Various, via 'p_conn->OnClose()'
*
* Note(s)     : none.
*********************************************************************************************************
*/

#ifdef HTTPc_TASK_MODULE_EN
static void  Twolemetry_ConnCloseCallback (HTTPc_CONN_OBJ           *p_conn,
                                           HTTPc_CONN_CLOSE_STATUS   close_status,
                                           HTTPc_ERR                 err)
{

}
#endif




/*
*********************************************************************************************************
*                                      Twolemetry_TransErrCallback()
*
* Description : Callback to notify application that an error occurred during an HTTP transaction.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection object.
*
*               p_req       Pointer to current HTTPc Request object.
*
*               err_code    Error Code.
*
* Return(s)   : None.
*
* Caller(s)   : Various, via 'p_req->OnErr()'.
*
* Note(s)     : None.
*********************************************************************************************************
*/

#ifdef HTTPc_TASK_MODULE_EN
static void Twolemetry_TransErrCallback (HTTPc_CONN_OBJ  *p_conn,
                                         HTTPc_REQ_OBJ   *p_req,
                                         HTTPc_ERR        err_code)
{

}
#endif


/*
*********************************************************************************************************
*                                      Twolemetry_ConnSecureCallback()
*
* Description :
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection object.
*
*               p_req       Pointer to current HTTPc Request object.
*
*               err_code    Error Code.
*
* Return(s)   : None.
*
* Caller(s)   : Various, via 'p_req->OnErr()'.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static void Twolemetry_ConnSecureCallback (HTTPc_CONN_OBJ  *p_conn,
                                           HTTPc_REQ_OBJ   *p_req,
                                           HTTPc_ERR        err_code)
{

}
