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
*                                               EXAMPLE
*
*                               HTTP CLIENT APPLICATION FUNCTIONS FILE
*
* Filename : http-c_app.c
* Version  : V3.01.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                           INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/
#define    MICRIUM_SOURCE
#define    HTTPc_APP_MODULE
#include  <stdio.h>

#include  "http-c_app.h"
#include  "http-c_hooks.h"
#include  "static_files.h"

#if (HTTPc_APP_FS_MODULE_PRESENT == DEF_YES)
#include  <fs_app.h>
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#if (HTTPc_APP_FS_MODULE_PRESENT == DEF_YES)
static  CPU_BOOLEAN  HTTPcApp_FS_Init                 (void);
#endif

static  CPU_INT08U   HTTPcApp_ReqPrepareQueryStr      (HTTPc_KEY_VAL         **p_tbl);

static  CPU_INT08U   HTTPcApp_ReqPrepareHdrs          (HTTPc_HDR             **p_hdr_tbl);

#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
static  CPU_INT08U   HTTPcApp_ReqPrepareFormApp       (HTTPc_FORM_TBL_FIELD  **p_form_tbl);

static  CPU_INT08U   HTTPcApp_ReqPrepareFormMultipart (HTTPc_FORM_TBL_FIELD  **p_form_tbl);
#endif


/*
*********************************************************************************************************
*                                            HTTPcApp_Init()
*
* Description : Initialize the uC/HTTP-client stack for the example application.
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

CPU_BOOLEAN  HTTPcApp_Init (void)
{
    CPU_BOOLEAN  success;
    HTTPc_ERR    httpc_err;

#if (HTTPc_APP_FS_MODULE_PRESENT == DEF_YES)
    success = HTTPcApp_FS_Init();
    if (success != DEF_YES) {
        return (DEF_FAIL);
    }
#endif
                                                                /* ------------- INITIALIZE CLIENT SUITE -------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_Init(&HTTPc_Cfg, &HTTPc_TaskCfg, DEF_NULL, &httpc_err);
    if (httpc_err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#else
    HTTPc_Init(&HTTPc_Cfg, DEF_NULL, DEF_NULL, &httpc_err);
    if (httpc_err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

   (void)success;

    return (DEF_OK);
}

/*
*********************************************************************************************************
*                                         HTTPcApp_ReqSendGet()
*
* Description : Send a GET request.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   if HTTP transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcApp_ReqSendGet (void)
{
    HTTPc_CONN_OBJ     *p_conn;
    HTTPc_REQ_OBJ      *p_req;
    HTTPc_RESP_OBJ     *p_resp;
    CPU_CHAR           *p_buf;
    HTTPc_KEY_VAL      *p_query_str_tbl;
    HTTPc_HDR          *p_hdr_tbl;
    HTTPc_PARAM_TBL     tbl_obj;
    HTTPc_FLAGS         flags;
    CPU_INT08U          query_nbr;
    CPU_INT08U          ext_hdr_nbr;
    CPU_SIZE_T          str_len;
    CPU_BOOLEAN         result;
    HTTPc_ERR           err;


    p_conn = &HTTPcApp_ConnTbl[0];
    p_req  = &HTTPcApp_ReqTbl[0];
    p_resp = &HTTPcApp_RespTbl[0];
    p_buf  = &HTTPcApp_ConnBufTbl[0][0];

#if (HTTPc_CFG_USER_DATA_EN == DEF_ENABLED)
    p_req->UserDataPtr = (void *)&HTTPcApp_Data[0];
#else
    return (DEF_FAIL);
#endif

                                                                /* -------------- SET STRING QUERY DATA --------------- */
    query_nbr = HTTPcApp_ReqPrepareQueryStr(&p_query_str_tbl);
    if (query_nbr == 0) {
        return (DEF_FAIL);
    }

                                                                /* -------------- SET ADDITIONAL HEADERS -------------- */
    ext_hdr_nbr = HTTPcApp_ReqPrepareHdrs(&p_hdr_tbl);
    if (ext_hdr_nbr == 0) {
        return (DEF_FAIL);
    }

                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(p_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(p_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------- SET CONNECTION CALLBACKS ------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ConnSetParam(p_conn,
                       HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       HTTPcApp_ConnCloseCallback,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ------------ SET STRING QUERY PARAMETERS ----------- */
    tbl_obj.EntryNbr = query_nbr;
    tbl_obj.TblPtr   = (void *)p_query_str_tbl;
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_QUERY_STR_TBL,
                     &tbl_obj,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ---------- SET REQUEST ADDITIONAL HEADERS ---------- */
    tbl_obj.EntryNbr = ext_hdr_nbr;
    tbl_obj.TblPtr   = (void *)p_hdr_tbl;
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_HDR_TBL,
                     &tbl_obj,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------ SET REQ/RESP HOOK FUNCTIONS ----------- */
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_RESP_HDR_HOOK,
                      HTTPcApp_RespHdrHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      HTTPcApp_RespBodyHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* --------- SET REQ/RESP CALLBACK FUNCTIONS ---------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      HTTPcApp_TransErrCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------------- OPEN CONNECTION ------------------ */
    str_len = Str_Len(HTTP_SERVER_HOSTNAME);
    flags = HTTPc_FLAG_NONE;
    result = HTTPc_ConnOpen(p_conn,
                            p_buf,
                            HTTPc_APP_CFG_CONN_BUF_SIZE,
                            HTTP_SERVER_HOSTNAME,
                            str_len,
                            flags,
                           &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("Connection to server succeeded.\n\r"));
    } else {
        HTTPc_APP_TRACE_INFO(("Connection to server failed.\n\r"));
        return (DEF_FAIL);
    }

                                                                /* ---------------- SEND HTTP REQUEST ----------------- */
    str_len = Str_Len("/get");
    flags = HTTPc_FLAG_NONE;
    result = HTTPc_ReqSend(p_conn,
                           p_req,
                           p_resp,
                           HTTP_METHOD_GET,
                          "/get",
                           str_len,
                           flags,
                          &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("%s\n\r", p_resp->ReasonPhrasePtr));
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        HTTPcApp_ReqSendPost()
*
* Description : Send a POST request with a pre-formatted form as body.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   if HTTP transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
CPU_BOOLEAN  HTTPcApp_ReqSendPost (void)
{
    HTTPc_CONN_OBJ        *p_conn;
    HTTPc_REQ_OBJ         *p_req;
    HTTPc_RESP_OBJ        *p_resp;
    CPU_CHAR              *p_buf;
    HTTPc_FLAGS            flags;
    CPU_SIZE_T             str_len;
    CPU_BOOLEAN            result;
    HTTPc_FORM_TBL_FIELD  *p_form_tbl;
    CPU_INT08U             form_nbr;
    CPU_INT32U             content_len;
    HTTP_CONTENT_TYPE      content_type;
    CPU_CHAR              *p_data;
    HTTPc_ERR              err;


    p_conn = &HTTPcApp_ConnTbl[0];
    p_req  = &HTTPcApp_ReqTbl[0];
    p_resp = &HTTPcApp_RespTbl[0];
    p_buf  = &HTTPcApp_ConnBufTbl[0][0];

#if (HTTPc_CFG_USER_DATA_EN == DEF_ENABLED)
    p_req->UserDataPtr = (void *)&HTTPcApp_Data[0];
#else
    return (DEF_FAIL);
#endif

                                                                /* ----------------- SET FORM TO SEND ----------------- */
    form_nbr = HTTPcApp_ReqPrepareFormApp(&p_form_tbl);
    if (form_nbr <= 0) {
        return (DEF_FAIL);
    }

    p_data      = &HTTPcApp_FormBuf[0];
    content_len =  HTTPc_FormAppFmt(p_data,
                                    HTTPc_APP_CFG_FORM_BUF_SIZE,
                                    p_form_tbl,
                                    form_nbr,
                                   &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(p_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(p_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------- SET CONNECTION CALLBACKS ------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ConnSetParam(p_conn,
                       HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       HTTPcApp_ConnCloseCallback,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ------------ SET STRING QUERY PARAMETERS ----------- */
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_QUERY_STR_HOOK,
                      HTTPcApp_ReqQueryStrHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ---------- SET REQUEST ADDITIONAL HEADERS ---------- */
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_HDR_HOOK,
                      HTTPcApp_ReqHdrHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ----------- SET REQUEST BODY PARAMETERS ------------ */
    content_type = HTTP_CONTENT_TYPE_APP_FORM;
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_TYPE,
                     &content_type,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_LEN,
                     &content_len,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_BODY_HOOK,
                      HTTPcApp_ReqBodyHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------ SET REQ/RESP HOOK FUNCTIONS ----------- */
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_RESP_HDR_HOOK,
                      HTTPcApp_RespHdrHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      HTTPcApp_RespBodyHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* --------- SET REQ/RESP CALLBACK FUNCTIONS ---------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      HTTPcApp_TransErrCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------------- OPEN CONNECTION ------------------ */
    str_len = Str_Len(HTTP_SERVER_HOSTNAME);
    flags   = HTTPc_FLAG_NONE;
    result  = HTTPc_ConnOpen(p_conn,
                             p_buf,
                             HTTPc_APP_CFG_CONN_BUF_SIZE,
                             HTTP_SERVER_HOSTNAME,
                             str_len,
                             flags,
                            &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("Connection to server succeeded.\n\r"));
    } else {
        HTTPc_APP_TRACE_INFO(("Connection to server failed.\n\r"));
        return (DEF_FAIL);
    }

                                                                /* ---------------- SEND HTTP REQUEST ----------------- */
    str_len = Str_Len("/post");
    flags   = HTTPc_FLAG_NONE;
    result  = HTTPc_ReqSend(p_conn,
                            p_req,
                            p_resp,
                            HTTP_METHOD_POST,
                           "/post",
                            str_len,
                            flags,
                           &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("%s\n\r", p_resp->ReasonPhrasePtr));
    }

    return (DEF_OK);
}
#endif


/*
*********************************************************************************************************
*                                       HTTPcApp_ReqSendAppForm()
*
* Description : Send a Application type form.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   if HTTP transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
CPU_BOOLEAN  HTTPcApp_ReqSendAppForm (void)
{
    HTTPc_CONN_OBJ        *p_conn;
    HTTPc_REQ_OBJ         *p_req;
    HTTPc_RESP_OBJ        *p_resp;
    CPU_CHAR              *p_buf;
    HTTPc_PARAM_TBL        tbl_obj;
    HTTPc_FLAGS            flags;
    CPU_SIZE_T             str_len;
    CPU_BOOLEAN            result;
    HTTPc_FORM_TBL_FIELD  *p_form_tbl;
    CPU_INT08U             form_nbr;
    HTTP_CONTENT_TYPE      content_type;
    HTTPc_ERR              err;


    p_conn = &HTTPcApp_ConnTbl[0];
    p_req  = &HTTPcApp_ReqTbl[0];
    p_resp = &HTTPcApp_RespTbl[0];
    p_buf  = &HTTPcApp_ConnBufTbl[0][0];

#if (HTTPc_CFG_USER_DATA_EN == DEF_ENABLED)
    p_req->UserDataPtr = (void *)&HTTPcApp_Data[0];
#else
    return (DEF_FAIL);
#endif

                                                                 /* ----------------- SET FORM TO SEND ----------------- */
    form_nbr = HTTPcApp_ReqPrepareFormApp(&p_form_tbl);
    if (form_nbr <= 0) {
        return (DEF_FAIL);
    }

                                                                 /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(p_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(p_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------- SET CONNECTION CALLBACKS ------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ConnSetParam(p_conn,
                       HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       HTTPcApp_ConnCloseCallback,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------- SET REQUEST BODY PARAMETERS ------------ */
    content_type = HTTP_CONTENT_TYPE_APP_FORM;
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_TYPE,
                     &content_type,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    tbl_obj.EntryNbr = form_nbr;
    tbl_obj.TblPtr   = (void *)p_form_tbl;
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_FORM_TBL,
                     &tbl_obj,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------ SET REQ/RESP HOOK FUNCTIONS ----------- */
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_RESP_HDR_HOOK,
                      HTTPcApp_RespHdrHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      HTTPcApp_RespBodyHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* --------- SET REQ/RESP CALLBACK FUNCTIONS ---------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      HTTPcApp_TransErrCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------------- OPEN CONNECTION ------------------ */
    str_len = Str_Len(HTTP_SERVER_HOSTNAME);
    flags   = HTTPc_FLAG_NONE;
    result  = HTTPc_ConnOpen(p_conn,
                             p_buf,
                             HTTPc_APP_CFG_CONN_BUF_SIZE,
                             HTTP_SERVER_HOSTNAME,
                             str_len,
                             flags,
                            &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("Connection to server succeeded.\n\r"));
    } else {
        HTTPc_APP_TRACE_INFO(("Connection to server failed.\n\r"));
        return (DEF_FAIL);
    }

                                                                /* ---------------- SEND HTTP REQUEST ----------------- */
    str_len = Str_Len("/post");
    flags   = HTTPc_FLAG_NONE;
    result  = HTTPc_ReqSend(p_conn,
                            p_req,
                            p_resp,
                            HTTP_METHOD_POST,
                           "/post",
                            str_len,
                            flags,
                           &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("%s\n\r", p_resp->ReasonPhrasePtr));
    }

    return (DEF_OK);
}
#endif


/*
*********************************************************************************************************
*                                    HTTPcApp_ReqSendMultipartForm()
*
* Description : Send a multipart type form.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   if HTTP transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
CPU_BOOLEAN  HTTPcApp_ReqSendMultipartForm (void)
{
    HTTPc_CONN_OBJ        *p_conn;
    HTTPc_REQ_OBJ         *p_req;
    HTTPc_RESP_OBJ        *p_resp;
    CPU_CHAR              *p_buf;
    HTTPc_PARAM_TBL        tbl_obj;
    HTTPc_FLAGS            flags;
    CPU_SIZE_T             str_len;
    CPU_BOOLEAN            result;
    HTTPc_FORM_TBL_FIELD  *p_form_tbl;
    CPU_INT08U             form_nbr;
    HTTP_CONTENT_TYPE      content_type;
    HTTPc_ERR              err;


    p_conn = &HTTPcApp_ConnTbl[0];
    p_req  = &HTTPcApp_ReqTbl[0];
    p_resp = &HTTPcApp_RespTbl[0];
    p_buf  = &HTTPcApp_ConnBufTbl[0][0];

#if (HTTPc_CFG_USER_DATA_EN == DEF_ENABLED)
    p_req->UserDataPtr = (void *)&HTTPcApp_Data[0];
#else
    return (DEF_FAIL);
#endif

                                                                 /* ----------------- SET FORM TO SEND ----------------- */
    form_nbr = HTTPcApp_ReqPrepareFormMultipart(&p_form_tbl);
    if (form_nbr <= 0) {
        return (DEF_FAIL);
    }

                                                                 /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(p_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(p_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------- SET CONNECTION CALLBACKS ------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ConnSetParam(p_conn,
                       HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       HTTPcApp_ConnCloseCallback,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------- SET REQUEST BODY PARAMETERS ------------ */
    content_type = HTTP_CONTENT_TYPE_MULTIPART_FORM;
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_TYPE,
                     &content_type,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    tbl_obj.EntryNbr = form_nbr;
    tbl_obj.TblPtr   = (void *)p_form_tbl;
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_FORM_TBL,
                     &tbl_obj,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------ SET REQ/RESP HOOK FUNCTIONS ----------- */
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_RESP_HDR_HOOK,
                      HTTPcApp_RespHdrHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      HTTPcApp_RespBodyHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* --------- SET REQ/RESP CALLBACK FUNCTIONS ---------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      HTTPcApp_TransErrCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------------- OPEN CONNECTION ------------------ */
    str_len = Str_Len(HTTP_SERVER_HOSTNAME);
    flags   = HTTPc_FLAG_NONE;
    result  = HTTPc_ConnOpen(p_conn,
                             p_buf,
                             HTTPc_APP_CFG_CONN_BUF_SIZE,
                             HTTP_SERVER_HOSTNAME,
                             str_len,
                             flags,
                            &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("Connection to server succeeded.\n\r"));
    } else {
        HTTPc_APP_TRACE_INFO(("Connection to server failed.\n\r"));
        return (DEF_FAIL);
    }

                                                                /* ---------------- SEND HTTP REQUEST ----------------- */
    str_len = Str_Len("/post");
    flags   = HTTPc_FLAG_NONE;
    result  = HTTPc_ReqSend(p_conn,
                            p_req,
                            p_resp,
                            HTTP_METHOD_POST,
                           "/post",
                            str_len,
                            flags,
                           &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("%s\n\r", p_resp->ReasonPhrasePtr));
    }

    return (DEF_OK);
}
#endif


/*
*********************************************************************************************************
*                                         HTTPcAPP_ReqSendPut()
*
* Description : Send PUT request.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   if HTTP transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcAPP_ReqSendPut (void)
{
    HTTPc_CONN_OBJ     *p_conn;
    HTTPc_REQ_OBJ      *p_req;
    HTTPc_RESP_OBJ     *p_resp;
    CPU_CHAR           *p_buf;
    HTTPc_FLAGS         flags;
    CPU_SIZE_T          str_len;
    CPU_SIZE_T          content_len;
    CPU_BOOLEAN         result;
    HTTP_CONTENT_TYPE   content_type;
    HTTPc_ERR           err;


    p_conn = &HTTPcApp_ConnTbl[0];
    p_req  = &HTTPcApp_ReqTbl[0];
    p_resp = &HTTPcApp_RespTbl[0];
    p_buf  = &HTTPcApp_ConnBufTbl[0][0];

#if (HTTPc_CFG_USER_DATA_EN == DEF_ENABLED)
    p_req->UserDataPtr = (void *)&HTTPcApp_Data[0];
#else
    return (DEF_FAIL);
#endif

                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(p_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(p_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------- SET CONNECTION CALLBACKS ------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ConnSetParam(p_conn,
                       HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       HTTPcApp_ConnCloseCallback,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------- SET REQUEST BODY PARAMETERS ------------ */
    content_type = HTTP_CONTENT_TYPE_GIF;
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_TYPE,
                     &content_type,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    content_len = STATIC_LOGO_GIF_LEN;
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_LEN,
                     &content_len,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_REQ_BODY_HOOK,
                      HTTPcApp_ReqBodyHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------ SET REQ/RESP HOOK FUNCTIONS ----------- */
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_RESP_HDR_HOOK,
                      HTTPcApp_RespHdrHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      HTTPcApp_RespBodyHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* --------- SET REQ/RESP CALLBACK FUNCTIONS ---------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ReqSetParam(p_req,
                      HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      HTTPcApp_TransErrCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------------- OPEN CONNECTION ------------------ */
    str_len = Str_Len(HTTP_SERVER_HOSTNAME);
    flags   = HTTPc_FLAG_NONE;
    result  = HTTPc_ConnOpen(p_conn,
                             p_buf,
                             HTTPc_APP_CFG_CONN_BUF_SIZE,
                             HTTP_SERVER_HOSTNAME,
                             str_len,
                             flags,
                            &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("Connection to server succeeded.\n\r"));
    } else {
        HTTPc_APP_TRACE_INFO(("Connection to server failed.\n\r"));
        return (DEF_FAIL);
    }

                                                                /* ---------------- SEND HTTP REQUEST ----------------- */
    str_len = Str_Len("/post");
    flags   = HTTPc_FLAG_NONE;
    result  = HTTPc_ReqSend(p_conn,
                            p_req,
                            p_resp,
                            HTTP_METHOD_PUT,
                           "/put",
                            str_len,
                            flags,
                           &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("%s\n\r", p_resp->ReasonPhrasePtr));
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       HTTPcApp_PersistentConn()
*
* Description : Send multiple requests on same connection.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   if persistent conn test was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcApp_PersistentConn (void)
{
    HTTPc_APP_REQ_DATA  *p_data1;
    HTTPc_APP_REQ_DATA  *p_data2;
    HTTPc_CONN_OBJ      *p_conn;
    HTTPc_REQ_OBJ       *p_req1;
    HTTPc_RESP_OBJ      *p_resp1;
    HTTPc_REQ_OBJ       *p_req2;
    HTTPc_RESP_OBJ      *p_resp2;
    CPU_CHAR            *p_buf;
    HTTPc_FLAGS          flags;
    CPU_SIZE_T           str_len;
    CPU_BOOLEAN          persistent;
    CPU_BOOLEAN          result;
    CPU_BOOLEAN          req1_done;
    CPU_BOOLEAN          req2_done;
    CPU_BOOLEAN          close;
    HTTPc_ERR            err;
    NET_ERR              err_net;
    CPU_SR_ALLOC();


    p_conn  = &HTTPcApp_ConnTbl[0];
    p_req1  = &HTTPcApp_ReqTbl[0];
    p_resp1 = &HTTPcApp_RespTbl[0];
    p_req2  = &HTTPcApp_ReqTbl[1];
    p_resp2 = &HTTPcApp_RespTbl[1];
    p_buf   = &HTTPcApp_ConnBufTbl[0][0];

#if (HTTPc_CFG_USER_DATA_EN == DEF_ENABLED)
    p_req1->UserDataPtr = (void *)&HTTPcApp_Data[0];
    p_req2->UserDataPtr = (void *)&HTTPcApp_Data[1];

    p_data1 = (HTTPc_APP_REQ_DATA *)p_req1->UserDataPtr;
    p_data2 = (HTTPc_APP_REQ_DATA *)p_req2->UserDataPtr;
#else
    return (DEF_FAIL);
#endif

    close = DEF_NO;

                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(p_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(p_req1, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(p_req2, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------- SET CONNECTION PARAMETERS ------------ */
    persistent = DEF_YES;
    HTTPc_ConnSetParam(p_conn,
                       HTTPc_PARAM_TYPE_PERSISTENT,
                      &persistent,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------- SET CONNECTION CALLBACKS ------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ConnSetParam(p_conn,
                       HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       HTTPcApp_ConnCloseCallback,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif


                                                                /* ------------ SET REQ/RESP HOOK FUNCTIONS ----------- */
    HTTPc_ReqSetParam(p_req1,
                      HTTPc_PARAM_TYPE_RESP_HDR_HOOK,
                      HTTPcApp_RespHdrHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req1,
                      HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      HTTPcApp_RespBodyHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req2,
                      HTTPc_PARAM_TYPE_RESP_HDR_HOOK,
                      HTTPcApp_RespHdrHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req2,
                      HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      HTTPcApp_RespBodyHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* --------- SET REQ/RESP CALLBACK FUNCTIONS ---------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ReqSetParam(p_req1,
                      HTTPc_PARAM_TYPE_TRANS_COMPLETE_CALLBACK,
                      HTTPcApp_TransDoneCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req2,
                      HTTPc_PARAM_TYPE_TRANS_COMPLETE_CALLBACK,
                      HTTPcApp_TransDoneCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req1,
                      HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      HTTPcApp_TransErrCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req2,
                      HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      HTTPcApp_TransErrCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------------- OPEN CONNECTION ------------------ */
    str_len = Str_Len(HTTP_SERVER_HOSTNAME);
    flags  = HTTPc_FLAG_NONE;
    result = HTTPc_ConnOpen(p_conn,
                            p_buf,
                            HTTPc_APP_CFG_CONN_BUF_SIZE,
                            HTTP_SERVER_HOSTNAME,
                            str_len,
                            flags,
                           &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("Connection to server succeeded.\n\r"));
    } else {
        HTTPc_APP_TRACE_INFO(("Connection to server failed.\n\r"));
        return (DEF_FAIL);
    }

                                                                /* ---------------- SEND HTTP REQUEST ----------------- */
    str_len = Str_Len("/get");
    flags = HTTPc_FLAG_NONE;
    DEF_BIT_SET(flags, HTTPc_FLAG_REQ_NO_BLOCK);
    (void)HTTPc_ReqSend(p_conn,
                        p_req1,
                        p_resp1,
                        HTTP_METHOD_GET,
                       "/get",
                        str_len,
                        flags,
                       &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    (void)HTTPc_ReqSend(p_conn,
                        p_req2,
                        p_resp2,
                        HTTP_METHOD_GET,
                       "/get",
                        str_len,
                        flags,
                       &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    do {
        CPU_CRITICAL_ENTER();
        req1_done = p_data1->Done;
        req2_done = p_data2->Done;
        CPU_CRITICAL_EXIT();

        if ((req1_done == DEF_YES) &&
            (req2_done == DEF_YES)) {
             close = DEF_YES;
        }

        NetApp_TimeDly_ms(10, &err_net);

    } while (close == DEF_NO);

    flags = HTTPc_FLAG_NONE;
    HTTPc_ConnClose(p_conn, flags, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                         HTTPcApp_MultiConn()
*
* Description : Open multiple Connections to send HTTP requests in parallel.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   if multi-conn test was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcApp_MultiConn (void)
{
    HTTPc_CONN_OBJ     *p_conn1;
    HTTPc_CONN_OBJ     *p_conn2;
    HTTPc_REQ_OBJ      *p_req1;
    HTTPc_RESP_OBJ     *p_resp1;
    HTTPc_REQ_OBJ      *p_req2;
    HTTPc_RESP_OBJ     *p_resp2;
    CPU_CHAR           *p_buf1;
    CPU_CHAR           *p_buf2;
    HTTPc_FLAGS         flags;
    CPU_SIZE_T          str_len;
    HTTPc_ERR           err;


    p_conn1 = &HTTPcApp_ConnTbl[0];
    p_conn2 = &HTTPcApp_ConnTbl[1];
    p_req1  = &HTTPcApp_ReqTbl[0];
    p_resp1 = &HTTPcApp_RespTbl[0];
    p_req2  = &HTTPcApp_ReqTbl[1];
    p_resp2 = &HTTPcApp_RespTbl[1];
    p_buf1  = &HTTPcApp_ConnBufTbl[0][0];
    p_buf2  = &HTTPcApp_ConnBufTbl[1][0];

#if (HTTPc_CFG_USER_DATA_EN == DEF_ENABLED)
    p_req1->UserDataPtr = (void *)&HTTPcApp_Data[0];
    p_req2->UserDataPtr = (void *)&HTTPcApp_Data[1];
#else
    return (DEF_FAIL);
#endif

                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(p_conn1, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ConnClr(p_conn2, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(p_req1, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(p_req2, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------- SET CONNECTION CALLBACKS ------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ConnSetParam(p_conn1,
                       HTTPc_PARAM_TYPE_CONN_CONNECT_CALLBACK,
                       HTTPcApp_ConnConnectCallback,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ConnSetParam(p_conn2,
                       HTTPc_PARAM_TYPE_CONN_CONNECT_CALLBACK,
                       HTTPcApp_ConnConnectCallback,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ConnSetParam(p_conn1,
                       HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       HTTPcApp_ConnCloseCallback,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ConnSetParam(p_conn2,
                       HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       HTTPcApp_ConnCloseCallback,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ------------ SET REQ/RESP HOOK FUNCTIONS ----------- */
    HTTPc_ReqSetParam(p_req1,
                      HTTPc_PARAM_TYPE_RESP_HDR_HOOK,
                      HTTPcApp_RespHdrHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req1,
                      HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      HTTPcApp_RespBodyHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req2,
                      HTTPc_PARAM_TYPE_RESP_HDR_HOOK,
                      HTTPcApp_RespHdrHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req2,
                      HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      HTTPcApp_RespBodyHook,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* --------- SET REQ/RESP CALLBACK FUNCTIONS ---------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ReqSetParam(p_req1,
                      HTTPc_PARAM_TYPE_TRANS_COMPLETE_CALLBACK,
                      HTTPcApp_TransDoneCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req2,
                      HTTPc_PARAM_TYPE_TRANS_COMPLETE_CALLBACK,
                      HTTPcApp_TransDoneCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req1,
                      HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      HTTPcApp_TransErrCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam(p_req2,
                      HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      HTTPcApp_TransErrCallback,
                     &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------------- OPEN CONNECTION ------------------ */
    str_len = Str_Len(HTTP_SERVER_HOSTNAME);
    flags  = HTTPc_FLAG_NONE;
    DEF_BIT_SET(flags, HTTPc_FLAG_CONN_NO_BLOCK);

    (void)HTTPc_ConnOpen(p_conn1,
                         p_buf1,
                         HTTPc_APP_CFG_CONN_BUF_SIZE,
                         HTTP_SERVER_HOSTNAME,
                         str_len,
                         flags,
                        &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    (void)HTTPc_ConnOpen(p_conn2,
                         p_buf2,
                         HTTPc_APP_CFG_CONN_BUF_SIZE,
                         HTTP_SERVER_HOSTNAME,
                         str_len,
                         flags,
                        &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }


                                                                /* ---------------- SEND HTTP REQUEST ----------------- */
    str_len = Str_Len("/get");
    flags = HTTPc_FLAG_NONE;
    DEF_BIT_SET(flags, HTTPc_FLAG_REQ_NO_BLOCK);
    (void)HTTPc_ReqSend(p_conn1,
                        p_req1,
                        p_resp1,
                        HTTP_METHOD_GET,
                       "/get",
                        str_len,
                        flags,
                       &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    (void)HTTPc_ReqSend(p_conn2,
                        p_req2,
                        p_resp2,
                        HTTP_METHOD_GET,
                       "/get",
                        str_len,
                        flags,
                       &err);
    if (err != HTTPc_ERR_NONE) {
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
*                                          AppHTTPc_FS_Init()
*
* Description : If uC/FS is present in the project, initialize it for the example application.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK, if FS initialization was successful.
*               DEF_FAIL, otherwise
*
* Caller(s)   : HTTPcApp_Init().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (HTTPc_APP_FS_MODULE_PRESENT == DEF_YES)
static  CPU_BOOLEAN  HTTPcApp_FS_Init (void)
{

    FS_FILE       *p_file;
    FS_FLAGS       fs_flags;
    CPU_BOOLEAN    success;
    CPU_BOOLEAN    result;
    CPU_SIZE_T     len_wr;
    CPU_SIZE_T     len_tot;
    FS_ERR         err_fs;


                                                                /* --------------- INITIALIZE FS STACK ---------------- */
    success = App_FS_Init();
    if (success == DEF_FAIL) {
        result = DEF_FAIL;
        goto exit;
    }

                                                                /* ------------ SET FS WORKING DIRECTORY -------------- */
    FS_WorkingDirSet("\\", &err_fs);
    if (err_fs != FS_ERR_NONE) {
        result = DEF_FAIL;
        goto exit;
    }

                                                                /* ----------------- COPY FILES TO FS ----------------- */
    fs_flags = 0;
    DEF_BIT_SET(fs_flags, FS_FILE_ACCESS_MODE_WR);
    DEF_BIT_SET(fs_flags, FS_FILE_ACCESS_MODE_CREATE);
    p_file = FSFile_Open("\\index.html",
                          fs_flags,
                         &err_fs);
    if (err_fs != FS_ERR_NONE) {
        result = DEF_FAIL;
        goto exit;
    }

    len_wr  = 0;
    len_tot = 0;
    while (len_tot < STATIC_INDEX_HTML_LEN) {

        len_wr = FSFile_Wr(p_file,
                          &index_html,
                           STATIC_INDEX_HTML_LEN,
                          &err_fs);
        if (err_fs != FS_ERR_NONE) {
            result = DEF_FAIL;
            goto exit_close;
        }

        len_tot += len_wr;
    }

    result = DEF_OK;


exit_close:
    FSFile_Close(p_file, &err_fs);
    if (err_fs != FS_ERR_NONE) {
        result = DEF_FAIL;
    }

exit:
    return (result);
}
#endif


/*
*********************************************************************************************************
*                                     HTTPcApp_ReqPrepareQueryStr()
*
* Description : Configure the Query String table.
*
* Argument(s) : p_tbl   Variable that will received the pointer to the Query String Table.
*
* Return(s)   : Number of fields in the table.
*
* Caller(s)   : HTTPcApp_ReqSendGet().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT08U  HTTPcApp_ReqPrepareQueryStr (HTTPc_KEY_VAL  **p_tbl)
{
    HTTPc_KEY_VAL  *p_kvp;


                                                                /* ----------------- SET FIRST QUERY ------------------ */
    p_kvp         = &HTTPcApp_ReqQueryStrTbl[0];
    p_kvp->KeyPtr = &HTTPcApp_ReqQueryStrKeyTbl[0][0];
    p_kvp->ValPtr = &HTTPcApp_ReqQueryStrValTbl[0][0];

   (void)Str_Copy_N(p_kvp->KeyPtr, "Name",       HTTPc_APP_CFG_QUERY_STR_KEY_LEN_MAX);
   (void)Str_Copy_N(p_kvp->ValPtr, "John Smith", HTTPc_APP_CFG_QUERY_STR_VAL_LEN_MAX);

    p_kvp->KeyLen = Str_Len("Name");
    p_kvp->ValLen = Str_Len("John Smith");

                                                                /* ---------------- SET SECOND QUERY ------------------ */
    p_kvp++;

    p_kvp->KeyPtr = &HTTPcApp_ReqQueryStrKeyTbl[1][0];
    p_kvp->ValPtr = DEF_NULL;

   (void)Str_Copy_N(p_kvp->KeyPtr, "Active", HTTPc_APP_CFG_QUERY_STR_KEY_LEN_MAX);

    p_kvp->KeyLen = Str_Len("Active");
    p_kvp->ValLen = 0;

   *p_tbl = &HTTPcApp_ReqQueryStrTbl[0];

    return (2);
}


/*
*********************************************************************************************************
*                                       HTTPcApp_ReqPrepareHdrs()
*
* Description : Configure the Header Fields Table
*
* Argument(s) : p_hdr_tbl   Variable that will received the pointer to the Header Fields table.
*
* Return(s)   : Number of fields in the table.
*
* Caller(s)   : HTTPcApp_ReqSendGet().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT08U  HTTPcApp_ReqPrepareHdrs (HTTPc_HDR  **p_hdr_tbl)
{
    HTTPc_HDR  *p_hdr;


                                                                /* ---------------- ADD ACCEPT HEADER ----------------- */
    p_hdr           = &HTTPcApp_ReqHdrTbl[0];
    p_hdr->ValPtr   = &HTTPcApp_ReqHdrValStrTbl[0][0];
    p_hdr->HdrField =  HTTP_HDR_FIELD_ACCEPT;

    Str_Copy_N(p_hdr->ValPtr, "text/*", HTTPc_APP_CFG_HDR_VAL_LEN_MAX);

    p_hdr->ValLen = Str_Len("text/*");

   *p_hdr_tbl = &HTTPcApp_ReqHdrTbl[0];

    return (1);
}


/*
*********************************************************************************************************
*                                     HTTPcApp_ReqPrepareFormApp()
*
* Description : Configure the application type form table.
*
* Argument(s) : p_form_tbl  Variable that will received the pointer to the form table.
*
* Return(s)   : Number of fields in the form table.
*
* Caller(s)   : HTTPcApp_ReqSendPost().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
static  CPU_INT08U  HTTPcApp_ReqPrepareFormApp (HTTPc_FORM_TBL_FIELD  **p_form_tbl)
{
    HTTPc_FORM_TBL_FIELD  *p_tbl;
    HTTPc_KEY_VAL         *p_kvp;
    HTTPc_ERR              httpc_err;


    p_tbl = &HTTPcApp_FormTbl[0];

                                                                /* -------------- ADD FIRST FORM FIELD ---------------- */
    p_kvp          = &HTTPcApp_FormKeyValTbl[0];
    p_kvp->KeyPtr  = &HTTPcApp_FormKeyStrTbl[0][0];
    p_kvp->ValPtr  = &HTTPcApp_FormValStrTbl[0][0];

    p_kvp->KeyLen = Str_Len("Age");
    p_kvp->ValLen = Str_Len("32");

   (void)Str_Copy_N(p_kvp->KeyPtr, "Age", p_kvp->KeyLen);
   (void)Str_Copy_N(p_kvp->ValPtr, "32",  p_kvp->ValLen);


    HTTPc_FormAddKeyVal(p_tbl, p_kvp, &httpc_err);

                                                                /* -------------- ADD SECOND FORM FIELD --------------- */
    p_tbl++;
    p_kvp++;

    p_kvp->KeyPtr    = &HTTPcApp_FormKeyStrTbl[1][0];
    p_kvp->ValPtr    = &HTTPcApp_FormValStrTbl[1][0];

    p_kvp->KeyLen = Str_Len("book");
    p_kvp->ValLen = Str_Len("Implementing IPv6 Second Edition");

   (void)Str_Copy_N(p_kvp->KeyPtr, "book",                             p_kvp->KeyLen);
   (void)Str_Copy_N(p_kvp->ValPtr, "Implementing IPv6 Second Edition", p_kvp->ValLen);

    HTTPc_FormAddKeyVal(p_tbl, p_kvp, &httpc_err);

   *p_form_tbl = &HTTPcApp_FormTbl[0];

    return (2);
}
#endif


/*
*********************************************************************************************************
*                                  HTTPcApp_ReqPrepareFormMultipart()
*
* Description : Configure the multipart type form table.
*
* Argument(s) : p_form_tbl  Variable that will received the pointer to the form table.
*
* Return(s)   : Number of fields in the form table.
*
* Caller(s)   : HTTPcApp_ReqSendMultipartForm().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
static  CPU_INT08U  HTTPcApp_ReqPrepareFormMultipart (HTTPc_FORM_TBL_FIELD  **p_form_tbl)
{
    HTTPc_FORM_TBL_FIELD  *p_tbl;
    HTTPc_KEY_VAL         *p_kvp;
    HTTPc_KEY_VAL_EXT     *p_kvp_big;
    HTTPc_ERR              httpc_err;
#if (HTTPc_APP_FS_MODULE_PRESENT == DEF_YES)
    HTTPc_MULTIPART_FILE  *p_form_file;
    FS_FILE               *p_file;
    FS_FLAGS               fs_flags;
    FS_ERR                 err_fs;
#endif


    p_tbl = &HTTPcApp_FormTbl[0];

                                                                /* -------------- ADD FIRST FORM FIELD ---------------- */
    p_kvp         = &HTTPcApp_FormKeyValTbl[0];
    p_kvp->KeyPtr = &HTTPcApp_FormKeyStrTbl[0][0];
    p_kvp->ValPtr = &HTTPcApp_FormValStrTbl[0][0];

    p_kvp->KeyLen =  Str_Len("LED1");
    p_kvp->ValLen =  Str_Len("ON");

   (void)Str_Copy_N(p_kvp->KeyPtr, "LED1", p_kvp->KeyLen);
   (void)Str_Copy_N(p_kvp->ValPtr, "ON",   p_kvp->ValLen);

   HTTPc_FormAddKeyVal(p_tbl, p_kvp, &httpc_err);

                                                                /* -------------- ADD SECOND FORM FIELD --------------- */
    p_tbl++;

    p_kvp_big         = &HTTPcApp_FormKeyValExtTbl[0];
    p_kvp_big->KeyPtr = &HTTPcApp_FormKeyStrTbl[1][0];

    p_kvp_big->KeyLen = Str_Len("Text");
    p_kvp_big->ValLen = Str_Len("The Hypertext Transfer Protocol (HTTP) is a stateless application-level protocol for distributed, collaborative, hypertext information systems.");

   (void)Str_Copy_N(p_kvp_big->KeyPtr, "Text", p_kvp_big->KeyLen);
   (void)Str_Copy_N(&HTTPcApp_FormValStrTbl[1][0], "The Hypertext Transfer Protocol (HTTP) is a stateless application-level protocol for distributed, collaborative, hypertext information systems.", p_kvp_big->ValLen);

    p_kvp_big->OnValTx = &HTTPcApp_FormMultipartHook;

    HTTPc_FormAddKeyValExt(p_tbl, p_kvp_big, &httpc_err);

#if (HTTPc_APP_FS_MODULE_PRESENT == DEF_YES)
                                                                /* -------------- ADD THIRD FORM FIELD ---------------- */
    p_tbl++;

    p_form_file              = &HTTPcApp_FormMultipartFileTbl[0];
    p_form_file->NamePtr     = &HTTPcApp_FormMultipartNameStrTbl[0][0];
    p_form_file->FileNamePtr = &HTTPcApp_FormMultipartFileNameStrTbl[0][0];
    p_form_file->ContentType =  HTTP_CONTENT_TYPE_HTML;

    Str_Copy(p_form_file->NamePtr, "File");
    Str_Copy(p_form_file->FileNamePtr, "index.html");

    p_form_file->NameLen = Str_Len("File");
    p_form_file->FileNameLen = Str_Len("index.html");

    fs_flags = 0;
    DEF_BIT_SET(fs_flags, FS_FILE_ACCESS_MODE_RD);
    DEF_BIT_SET(fs_flags, FS_FILE_ACCESS_MODE_CREATE);
    p_file = FSFile_Open("\\index.html",
                         fs_flags,
                         &err_fs);
    if (err_fs != FS_ERR_NONE) {
        return (0);
    }

    p_form_file->FileLen  =  p_file->Size;

    p_form_file->OnFileTx = &HTTPcApp_FormMultipartFileHook;

    HTTPc_FormAddFile(p_tbl, p_form_file, &httpc_err);

   *p_form_tbl = p_tbl;

    FSFile_Close(p_file, &err_fs);

   *p_form_tbl = &HTTPcApp_FormTbl[0];

    return (3);
#endif

   *p_form_tbl = &HTTPcApp_FormTbl[0];

    return (2);
}
#endif
