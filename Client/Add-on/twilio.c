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
*                             HTTP CLIENT TWILIO APPLICATION SOURCE CODE
*
* Filename : twilio.c
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
#define  TWILIO_MODULE


#include  <lib_def.h>

#include  <Common/http.h>
#include  <Client/Source/http-c.h>
#include  <http-c_cfg.h>

#include  <Source/net_util.h>

#include  "twilio.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/
static  CPU_INT08U   Twilio_ReqPrepareFormApp       (HTTPc_FORM_TBL_FIELD     **p_form_tbl,
                                                     CPU_CHAR                  *p_from_phone_number,
                                                     CPU_CHAR                  *p_to_phone_number,
                                                     CPU_CHAR                  *p_message);

#ifdef HTTPc_TASK_MODULE_EN
static  void         Twilio_ConnCloseCallback       (HTTPc_CONN_OBJ            *p_conn,
                                                     HTTPc_CONN_CLOSE_STATUS    close_status,
                                                     HTTPc_ERR                  err);


static  void         Twilio_TransErrCallback        (HTTPc_CONN_OBJ            *p_conn,
                                                     HTTPc_REQ_OBJ             *p_req,
                                                     HTTPc_ERR                  err_code);

static  void         Twilio_ConnSecureCallback      (HTTPc_CONN_OBJ            *p_conn,
                                                     HTTPc_REQ_OBJ             *p_req,
                                                     HTTPc_ERR                  err_code);
#endif


/*
*********************************************************************************************************
*                                           Twilio_SendSMS()
*
* Description : Send a SMS message
*
* Argument(s) : p_from_phone_number     Pointer to the sender phone number
*
*               p_to_phone_number       Pointer to the destination phone number
*
*               p_message               Pointer to the body of the message to be sent
*
*               p_account_id            Pointer to the account SID
*
*               p_auth_token            Pointer to the authentication token
*
* Return(s)   : DEF_OK,   if HTTPc transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : None.
*********************************************************************************************************
*/
CPU_BOOLEAN Twilio_SendSMS    (CPU_CHAR         *p_from_phone_number,
                               CPU_CHAR         *p_to_phone_number,
                               CPU_CHAR         *p_message,
                               CPU_CHAR         *p_account_id,
                               CPU_CHAR         *p_auth_token)

{
    CPU_CHAR               url[TWILIO_URL_LEN_MAX];
    HTTPc_CONN_OBJ         http_conn;
    HTTPc_REQ_OBJ          http_req;
    HTTPc_RESP_OBJ         http_resp;
    CPU_INT08U             form_nbr;
    HTTPc_FORM_TBL_FIELD  *p_form_tbl;
    HTTP_CONTENT_TYPE      content_type;
    HTTPc_PARAM_TBL        tbl_obj;
    CPU_CHAR               buf[TWILIO_CFG_CONN_BUF_SIZE];
    CPU_SIZE_T             str_len;
    CPU_BOOLEAN            result;
    CPU_CHAR               credentials[100];
    CPU_CHAR               credentials_b64[100];
    NET_ERR                err_net;
    HTTPc_ERR              err;
    HTTPc_HDR              hdr;
    CPU_CHAR               hdr_val[TWILIO_CFG_HDR_VAL_LEN_MAX];


    Mem_Clr(url, TWILIO_URL_LEN_MAX);
    Mem_Clr(&http_conn, sizeof(http_conn));
    Mem_Clr(&http_req,  sizeof(http_req));
    Mem_Clr(&http_resp, sizeof(http_resp));
    Mem_Clr(hdr_val,    TWILIO_CFG_HDR_VAL_LEN_MAX);
                                                                /* ------------------- SET THE URL -------------------- */
    Str_Copy_N(&url[0],TWILIO_URL_MESSAGES,TWILIO_URL_LEN_MAX);
    Str_Cat_N( &url[0],p_account_id       , TWILIO_URL_LEN_MAX);
    Str_Cat_N( &url[0],"/Messages"        , TWILIO_URL_LEN_MAX);


                                                                /* --------- PREPARE THE AUTHORIZATION HEADER --------- */
    Str_Copy(&credentials[0], p_account_id);
    Str_Cat( &credentials[0], ":");
    Str_Cat( &credentials[0], p_auth_token);

    NetUtil_Base64Encode (&credentials[0],
                          Str_Len(&credentials[0]),
                          &credentials_b64[0],
                          100,
                          &err_net);

    Str_Cat_N(hdr_val, "Basic ",TWILIO_CFG_HDR_VAL_LEN_MAX);
    Str_Cat_N(hdr_val, &credentials_b64[0], TWILIO_CFG_HDR_VAL_LEN_MAX);

    hdr.ValPtr   = hdr_val;
    hdr.ValLen   = Str_Len_N(hdr_val, TWILIO_CFG_HDR_VAL_LEN_MAX);
    hdr.HdrField = HTTP_HDR_FIELD_AUTHORIZATION;

                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(&http_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(&http_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------------- PREPARE FORM ------------------- */
    form_nbr = Twilio_ReqPrepareFormApp(&p_form_tbl,
                                         p_from_phone_number,
                                         p_to_phone_number,
                                         p_message);
    if (form_nbr <= 0) {
    return (DEF_FAIL);
    }


                                                                /* ------------- SET CONNECTION CALLBACKS ------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ConnSetParam(        &http_conn,
                                HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       (void *)&Twilio_ConnCloseCallback,
                               &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

    HTTPc_ConnSetParam(        &http_conn,
                                HTTPc_PARAM_TYPE_SECURE_TRUST_CALLBACK,
                       (void *)&Twilio_ConnSecureCallback,
                               &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }


                                                                /* ----------- SET REQUEST BODY PARAMETERS ------------ */
    content_type = HTTP_CONTENT_TYPE_APP_FORM;
    HTTPc_ReqSetParam(&http_req,
                       HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_TYPE,
                      &content_type,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    tbl_obj.EntryNbr = form_nbr;
    tbl_obj.TblPtr   = (void *)p_form_tbl;
    HTTPc_ReqSetParam(&http_req,
                       HTTPc_PARAM_TYPE_REQ_FORM_TBL,
                      &tbl_obj,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------ SET REQ/HDR PARAM          ----------- */
    tbl_obj.EntryNbr = 1;
    tbl_obj.TblPtr = &hdr;
    HTTPc_ReqSetParam(&http_req,
                       HTTPc_PARAM_TYPE_REQ_HDR_TBL,
                      &tbl_obj,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#if 0

                                                                /* ------------ SET REQ/RESP HOOK FUNCTIONS ----------- */
    HTTPc_ReqSetParam(        &http_req,
                               HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      (void *)&Twilio_RespBodyHook,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif
                                                                /* --------- SET REQ/RESP CALLBACK FUNCTIONS ---------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ReqSetParam(        &http_req,
                               HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      (void *)&Twilio_TransErrCallback,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------------- OPEN CONNECTION ------------------ */
    str_len = Str_Len(TWILIO_API_HOSTNAME);
    result  = HTTPc_ConnOpen(&http_conn,
                              buf,
                              TWILIO_CFG_CONN_BUF_SIZE,
                              TWILIO_API_HOSTNAME,
                              str_len,
                              HTTPc_FLAG_NONE,
                             &err);
    if (err != HTTPc_ERR_NONE ||
        result != DEF_OK        ) {
        return (DEF_FAIL);
    }

                                                                /* ---------------- SEND HTTP REQUEST ----------------- */
    str_len = Str_Len(&url[0]);
    result  = HTTPc_ReqSend(&http_conn,
                            &http_req,
                            &http_resp,
                             HTTP_METHOD_POST,
                            &url[0],
                             str_len,
                             HTTPc_FLAG_NONE,
                            &err);
    if (err != HTTPc_ERR_NONE ||
        result != DEF_OK        ) {
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
*                                      Twilio_ReqPrepareFormApp()
*
* Description : Configure the application type form table.
*
* Argument(s) : p_form_tbl              Variable that will received the pointer to the form table.
*
*               p_from_phone_number     Pointer to the sender phone number
*
*               p_to_phone_number       Pointer to the destination phone number
*
*               p_message               Pointer to the body of the message to be sent
*
* Return(s)   : Number of fields in the form table.
*
* Caller(s)   : Twilio_SendSMS().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
static  CPU_INT08U  Twilio_ReqPrepareFormApp (HTTPc_FORM_TBL_FIELD  **p_form_tbl,
                                              CPU_CHAR               *p_from_phone_number,
                                              CPU_CHAR               *p_to_phone_number,
                                              CPU_CHAR               *p_message)
{
    HTTPc_FORM_TBL_FIELD  *p_tbl;
    HTTPc_KEY_VAL         *p_kvp;
    HTTPc_ERR              httpc_err;


    p_tbl = &Twilio_FormTbl[0];

                                                                /* --------------- ADD FIRST FORM FIELD --------------- */
    p_kvp          = &Twilio_FormKeyValTbl[0];
    p_kvp->KeyPtr  = &Twilio_FormKeyStrTbl[0][0];
    p_kvp->ValPtr  = &Twilio_FormValStrTbl[0][0];

    p_kvp->KeyLen = Str_Len("From");
    p_kvp->ValLen = Str_Len(p_from_phone_number);

    (void)Str_Copy_N(p_kvp->KeyPtr, "From",               p_kvp->KeyLen);
    (void)Str_Copy_N(p_kvp->ValPtr, p_from_phone_number,  p_kvp->ValLen);


    HTTPc_FormAddKeyVal(p_tbl, p_kvp, &httpc_err);

                                                                /* -------------- ADD SECOND FORM FIELD --------------- */
    p_tbl++;
    p_kvp++;

    p_kvp->KeyPtr    = &Twilio_FormKeyStrTbl[1][0];
    p_kvp->ValPtr    = &Twilio_FormValStrTbl[1][0];

    p_kvp->KeyLen = Str_Len("To");
    p_kvp->ValLen = Str_Len(p_to_phone_number);

   (void)Str_Copy_N(p_kvp->KeyPtr, "To",              p_kvp->KeyLen);
   (void)Str_Copy_N(p_kvp->ValPtr, p_to_phone_number, p_kvp->ValLen);

    HTTPc_FormAddKeyVal(p_tbl, p_kvp, &httpc_err);

                                                                /* --------------- ADD THIRD FORM FIELD --------------- */
    p_tbl++;
    p_kvp++;

    p_kvp->KeyPtr    = &Twilio_FormKeyStrTbl[2][0];
    p_kvp->ValPtr    = &Twilio_FormValStrTbl[2][0];

    p_kvp->KeyLen = Str_Len("Body");
    p_kvp->ValLen = Str_Len(p_message);

    (void)Str_Copy_N(p_kvp->KeyPtr, "Body",    p_kvp->KeyLen);
    (void)Str_Copy_N(p_kvp->ValPtr, p_message, p_kvp->ValLen);

    HTTPc_FormAddKeyVal(p_tbl, p_kvp, &httpc_err);

   *p_form_tbl = &Twilio_FormTbl[0];

    return (3);
}
#endif

#if 0
/*
*********************************************************************************************************
*                                       Twilio_RespBodyHook()
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

void  Twilio_RespBodyHook   (HTTPc_CONN_OBJ     *p_conn,
                             HTTPc_REQ_OBJ      *p_req,
                             HTTP_CONTENT_TYPE   content_type,
                             void               *p_data,
                             CPU_INT16U          data_len,
                             CPU_BOOLEAN         last_chunk)
{
    CPU_CHAR  *p_data_str;


    p_data_str = &Twilio_Buf[0];

    Mem_Copy(p_data_str, p_data, data_len);

    p_data_str +=  data_len;
   *p_data_str  = '\0';
    p_data_str  = &Twilio_Buf[0];

    TWILIO_TRACE_INFO(("%s", p_data_str));


    if (last_chunk == DEF_YES) {

        TWILIO_TRACE_INFO(("\n\r"));
    }
}
#endif

/*
*********************************************************************************************************
*                                     Twilio_ConnCloseCallback()
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
static void  Twilio_ConnCloseCallback (HTTPc_CONN_OBJ             *p_conn,
                                       HTTPc_CONN_CLOSE_STATUS   close_status,
                                       HTTPc_ERR                 err)
{

}
#endif


/*
*********************************************************************************************************
*                                      Twilio_TransErrCallback()
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
static void Twilio_TransErrCallback (HTTPc_CONN_OBJ  *p_conn,
                                     HTTPc_REQ_OBJ   *p_req,
                                     HTTPc_ERR        err_code)
{

}
#endif


/*
*********************************************************************************************************
*                                      Twilio_ConnSecureCallback()
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

static void Twilio_ConnSecureCallback (HTTPc_CONN_OBJ  *p_conn,
                                       HTTPc_REQ_OBJ   *p_req,
                                       HTTPc_ERR        err_code)
{

}
