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
*                                   HTTP CLIENT HOOK FUNCTIONS FILE
*
* Filename : http-c_hooks.c
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

#include  <stdio.h>

#include  "http-c_hooks.h"
#include  "http-c_app.h"
#include  "static_files.h"



/*
*********************************************************************************************************
*                                      HTTPcApp_ReqQueryStrHook()
*
* Description : Add a Query String field to a specific HTTP Request.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection object.
*
*               p_req       Pointer to current HTTPc Request object.
*
*               p_key_val   Pointer to key-value pair to recover from application.
*
* Return(s)   : DEF_YES,    if all the fields of the Query String have been added.
*               DEF_NO,     otherwise.
*
* Caller(s)   : HTTPcReq_QueryStrHook() vis 'p_req->OnQueryStrTx()'.
*
* Note(s)     : (1) In this example function, the query string will be added to all the requests on any
*                   connections.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcApp_ReqQueryStrHook (HTTPc_CONN_OBJ     *p_conn,
                                       HTTPc_REQ_OBJ      *p_req,
                                       HTTPc_KEY_VAL     **p_key_val)
{
    HTTPc_APP_REQ_DATA  *p_app_data;
    HTTPc_KEY_VAL       *p_kvp;
    CPU_INT08U           index;

#if (HTTPc_CFG_USER_DATA_EN == DEF_ENABLED)
    p_app_data = (HTTPc_APP_REQ_DATA *)p_req->UserDataPtr;

    index = p_app_data->QueryStrIx;

    switch(index) {
        case 0:
             p_kvp         = &HTTPcApp_ReqQueryStrTbl[0];
             p_kvp->KeyPtr = &HTTPcApp_ReqQueryStrKeyTbl[0][0];
             p_kvp->ValPtr = &HTTPcApp_ReqQueryStrValTbl[0][0];

            (void)Str_Copy_N(p_kvp->KeyPtr, "Name", HTTPc_APP_CFG_QUERY_STR_KEY_LEN_MAX);
            (void)Str_Copy_N(p_kvp->ValPtr, "John", HTTPc_APP_CFG_QUERY_STR_VAL_LEN_MAX);

             p_kvp->KeyLen = Str_Len_N(p_kvp->KeyPtr, HTTPc_APP_CFG_QUERY_STR_KEY_LEN_MAX);
             p_kvp->ValLen = Str_Len_N(p_kvp->ValPtr, HTTPc_APP_CFG_QUERY_STR_VAL_LEN_MAX);

            *p_key_val = p_kvp;
             p_app_data->QueryStrIx++;
             return (DEF_NO);


        case 1:
             p_kvp         = &HTTPcApp_ReqQueryStrTbl[1];
             p_kvp->KeyPtr = &HTTPcApp_ReqQueryStrKeyTbl[1][0];
             p_kvp->ValPtr =  DEF_NULL;

            (void)Str_Copy_N(p_kvp->KeyPtr, "active", HTTPc_APP_CFG_QUERY_STR_KEY_LEN_MAX);

             p_kvp->KeyLen = Str_Len_N(p_kvp->KeyPtr, HTTPc_APP_CFG_QUERY_STR_KEY_LEN_MAX);

            *p_key_val = p_kvp;
             p_app_data->QueryStrIx = 0;
             return (DEF_YES);


        default:
            *p_key_val = DEF_NULL;
             p_app_data->QueryStrIx = 0;
             return (DEF_YES);
    }
#else
   (void)p_kvp;
   (void)index;
   (void)p_app_data;
   *p_key_val = DEF_NULL;
    return (DEF_YES);
#endif
}


/*
*********************************************************************************************************
*                                          HTTPcApp_ReqHdrHook()
*
* Description : Add an header field to a specific HTTP Request.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection object.
*
*               p_req       Pointer to current HTTPc Request object.
*
*               p_hdr       Pointer to header field to recover from application.
*
* Return(s)   : DEF_YES,    if all header fields have been added.
*               DEF_NO,     otherwise.
*
* Caller(s)   : HTTPcReq_HdrExtHook() via 'p_req->OnHdrTx()'
*
* Note(s)     : (1) In this example function, the header field will be added to all the requests on any
*                   connections.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcApp_ReqHdrHook (HTTPc_CONN_OBJ     *p_conn,
                                  HTTPc_REQ_OBJ      *p_req,
                                  HTTPc_HDR         **p_hdr)
{
    HTTPc_HDR  *p_hdr_tmp;


    p_hdr_tmp         = &HTTPcApp_ReqHdrTbl[0];
    p_hdr_tmp->ValPtr = &HTTPcApp_ReqHdrValStrTbl[0][0];

    p_hdr_tmp->HdrField = HTTP_HDR_FIELD_COOKIE;
    Str_Copy_N(p_hdr_tmp->ValPtr, "ID=234668", HTTPc_APP_CFG_HDR_VAL_LEN_MAX);
    p_hdr_tmp->ValLen   = Str_Len_N(p_hdr_tmp->ValPtr, HTTPc_APP_CFG_HDR_VAL_LEN_MAX);

   *p_hdr = p_hdr_tmp;

    return (DEF_YES);
}


/*
*********************************************************************************************************
*                                          HTTPcApp_ReqBodyHook()
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

CPU_BOOLEAN  HTTPcApp_ReqBodyHook (HTTPc_CONN_OBJ     *p_conn,
                                   HTTPc_REQ_OBJ      *p_req,
                                   void              **p_data,
                                   CPU_CHAR           *p_buf,
                                   CPU_INT16U          buf_len,
                                   CPU_INT16U         *p_data_len)
{

    switch (p_req->Method_reserved) {
        case HTTP_METHOD_POST:
            *p_data     = &HTTPcApp_FormBuf[0];
            *p_data_len =  Str_Len(&HTTPcApp_FormBuf[0]);
             return (DEF_YES);


        case HTTP_METHOD_PUT:
            *p_data     = (void *)&logo_gif[0];
            *p_data_len =  STATIC_LOGO_GIF_LEN;
             break;


        default:
            *p_data_len = 0;
             return (DEF_YES);

    }

    return (DEF_YES);
}


/*
*********************************************************************************************************
*                                        HTTPcApp_RespHdrHook()
*
* Description : Retrieve header fields in the HTTP response received.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection object.
*
*               p_req       Pointer to current HTTPc Request object.
*
*               hdr_field   HTTP header type of the header field received in the HTTP response.
*
*               p_hdr_val   Pointer to the value string received in the Response header field.
*
*               val_len     Length of the value string.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcResp_ParseHdr() via 'p_req->OnHdrRx()'
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPcApp_RespHdrHook (HTTPc_CONN_OBJ    *p_conn,
                            HTTPc_REQ_OBJ     *p_req,
                            HTTP_HDR_FIELD     hdr_field,
                            CPU_CHAR          *p_hdr_val,
                            CPU_INT16U         val_len)
{
    HTTPc_HDR  *p_hdr;


    p_hdr         = &HTTPcApp_RespHdrTbl[0];
    p_hdr->ValPtr = &HTTPcApp_RespHdrValStrTbl[0][0];

    switch (hdr_field) {
        case HTTP_HDR_FIELD_COOKIE:
             p_hdr->HdrField = hdr_field;
             Str_Copy_N(p_hdr->ValPtr, p_hdr_val, val_len);
             p_hdr->ValLen   = val_len;
             break;

        default:
             break;
    }
}


/*
*********************************************************************************************************
*                                       HTTPcApp_RespBodyHook()
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

CPU_INT32U HTTPcApp_RespBodyHook (HTTPc_CONN_OBJ     *p_conn,
                                  HTTPc_REQ_OBJ      *p_req,
                                  HTTP_CONTENT_TYPE   content_type,
                                  void               *p_data,
                                  CPU_INT16U          data_len,
                                  CPU_BOOLEAN         last_chunk)
{
    CPU_CHAR  *p_data_str;


    p_data_str = &HTTPcApp_Buf[0];

    Mem_Copy(p_data_str, p_data, data_len);

    p_data_str +=  data_len;
   *p_data_str  = '\0';
    p_data_str  = &HTTPcApp_Buf[0];

    HTTPc_APP_TRACE_INFO(("%s", p_data_str));

    if (last_chunk == DEF_YES) {
        HTTPc_APP_TRACE_INFO(("\n\r"));
    }

    return data_len;
}


/*
*********************************************************************************************************
*                                     HTTPcApp_FormMultipartHook()
*
* Description : Retrieve the value data for an Key-Val Extended object.
*
* Argument(s) : p_conn          Pointer to current HTTPc Connection object.
*
*               p_req           Pointer to current HTTPc Request object.
*
*               p_key_val_obj   Pointer to current Key-Value Extended object.
*
*               p_buf           Pointer to HTTP transmit buffer.
*
*               buf_len         Size remaining in HTTP buffer.
*
*               p_len_wr        Variable that will received the size of the data copied in the buffer.
*
* Return(s)   : DEF_YES, if all the data was transmitted.
*               DEF_NO,  otherwise.
*
* Caller(s)   : HTTPcApp_ReqPrepareFormMultipart().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcApp_FormMultipartHook (HTTPc_CONN_OBJ     *p_conn,
                                         HTTPc_REQ_OBJ      *p_req,
                                         HTTPc_KEY_VAL_EXT  *p_key_val_obj,
                                         CPU_CHAR           *p_buf,
                                         CPU_INT16U          buf_len,
                                         CPU_INT16U         *p_len_wr)
{
    HTTPc_APP_REQ_DATA  *p_req_data;
    CPU_CHAR            *p_data;
    CPU_INT16U           data_ix;
    CPU_INT16U           min_len;


#if (HTTPc_CFG_USER_DATA_EN == DEF_ENABLED)
    p_req_data = (HTTPc_APP_REQ_DATA *)p_req->UserDataPtr;

    data_ix    = p_req_data->FormIx;

    p_data     = &HTTPcApp_FormValStrTbl[1][0];

    min_len    = DEF_MIN(buf_len, (p_key_val_obj->ValLen - data_ix));

    if (min_len <= 0) {
        p_req_data->FormIx = 0;
       *p_len_wr = min_len;
        return (DEF_YES);
    }

    Str_Copy_N(p_buf, p_data, min_len);

   *p_len_wr  = min_len;
    data_ix  += min_len;

    p_req_data->FormIx = data_ix;

    return (DEF_NO);
#else
   (void)p_req_data;
   (void)p_data;
   (void)data_ix;
   (void)min_len;

    return (DEF_YES);
#endif
}


/*
*********************************************************************************************************
*                                   HTTPcApp_FormMultipartFileHook()
*
* Description : Retrieve the file data for a Multipart File object.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection object.
*
*               p_req       Pointer to current HTTPc Request object.
*
*               p_file_obj  Pointer to current Multipart File object.
*
*               p_buf       Pointer to HTTP transmit buffer.
*
*               buf_len     Size remaining in HTTP buffer.
*
*               p_len_wr    Variable that will received the size of the data copied in the buffer.
*
* Return(s)   : DEF_YES, if all the data was transmitted.
*               DEF_NO,  otherwise.
*
* Caller(s)   : HTTPcApp_ReqPrepareFormMultipart().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (HTTPc_APP_FS_MODULE_PRESENT == DEF_YES)
CPU_BOOLEAN  HTTPcApp_FormMultipartFileHook (HTTPc_CONN_OBJ        *p_conn,
                                             HTTPc_REQ_OBJ         *p_req,
                                             HTTPc_MULTIPART_FILE  *p_file_obj,
                                             CPU_CHAR              *p_buf,
                                             CPU_INT16U             buf_len,
                                             CPU_INT16U            *p_len_wr)
{
    HTTPc_APP_REQ_DATA  *p_req_data;
    FS_FILE             *p_file;
    FS_FLAGS             fs_flags;
    CPU_SIZE_T           file_rem;
    CPU_SIZE_T           size;
    CPU_SIZE_T           size_rd;
    CPU_BOOLEAN          is_open;
    CPU_BOOLEAN          finish;
    FS_ERR               err_fs;


    p_req_data = (HTTPc_APP_REQ_DATA *)p_req->UserDataPtr;

    fs_flags = 0;
    is_open = FSFile_IsOpen("\\index.html",
                            &fs_flags,
                            &err_fs);
    if (err_fs != FS_ERR_NONE) {
        finish  = DEF_YES;
        goto exit;
    }

    if (is_open == DEF_NO) {
        fs_flags = 0;
        DEF_BIT_SET(fs_flags, FS_FILE_ACCESS_MODE_RD);
        DEF_BIT_SET(fs_flags, FS_FILE_ACCESS_MODE_CREATE);

        p_file = FSFile_Open("\\index.html",
                              fs_flags,
                             &err_fs);
        if (err_fs != FS_ERR_NONE) {
            finish  = DEF_YES;
            goto exit;
        }

        p_req_data->FilePtr = p_file;

    } else {
        p_file = p_req_data->FilePtr;
    }

    file_rem = p_file->Size - p_file->Pos;
    if (file_rem <= 0) {
       *p_len_wr = 0;
        finish   = DEF_YES;
        goto exit_close;
    }

    size = DEF_MIN(file_rem, buf_len);

    size_rd = FSFile_Rd(p_file, p_buf, size, &err_fs);
    if (err_fs != FS_ERR_NONE) {
       *p_len_wr = 0;
        goto exit_close;
    }

   *p_len_wr = size_rd;

    finish   = DEF_NO;

    goto exit;


exit_close:
    FSFile_Close(p_file, &err_fs);

exit:
    return (finish);
}
#endif


/*
*********************************************************************************************************
*                                      HTTPcApp_ConnConnectCallback()
*
* Description : Callback to notify application that an HTTP connection connect process was completed.
*
* Argument(s) : p_conn          Pointer to current HTTPc Connection.
*
*               open_status     Status of the connection:
*
*                               DEF_OK,   if the connection with the server was successful.
*                               DEF_FAIL, otherwise.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcTask_Handler() via 'p_conn->OnConnect()'.
*
* Note(s)     : None.
*********************************************************************************************************
*/
#ifdef HTTPc_TASK_MODULE_EN
void  HTTPcApp_ConnConnectCallback (HTTPc_CONN_OBJ  *p_conn,
                                    CPU_BOOLEAN      open_status)
{
    if (open_status == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("Connection to server succeeded.\n\r"));
    } else {
        HTTPc_APP_TRACE_INFO(("Connection to server failed.\n\r"));
    }
}
#endif


/*
*********************************************************************************************************
*                                     HTTPcApp_ConnCloseCallback()
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
void  HTTPcApp_ConnCloseCallback (HTTPc_CONN_OBJ           *p_conn,
                                  HTTPc_CONN_CLOSE_STATUS   close_status,
                                  HTTPc_ERR                 err)
{
    HTTPc_APP_TRACE_INFO(("Connection closed.\n\r"));
}
#endif


/*
*********************************************************************************************************
*                                     HTTPcApp_TransDoneCallback()
*
* Description : Callback to notify application that an HTTP transaction was completed.
*
* Argument(s) : p_conn  Pointer to current HTTPc Connection object.
*
*               p_req   Pointer to current HTTPc Request object.
*
*               p_resp  Pointer to current HTTPc Response object.
*
*               status  Status of the transaction:
*
*                           DEF_OK,   transaction was successful.
*                           DEF_FAIL, otherwise.
*
* Return(s)   : None.
*
* Caller(s)   : Various, via 'p_req->OnTransComplete()'.
*
* Note(s)     : None.
*********************************************************************************************************
*/
#ifdef HTTPc_TASK_MODULE_EN
void  HTTPcApp_TransDoneCallback (HTTPc_CONN_OBJ  *p_conn,
                                  HTTPc_REQ_OBJ   *p_req,
                                  HTTPc_RESP_OBJ  *p_resp,
                                  CPU_BOOLEAN      status)
{
    HTTPc_APP_REQ_DATA  *p_data;
    HTTPc_REQ_OBJ       *p_req1;
    HTTPc_REQ_OBJ       *p_req2;
    CPU_SR_ALLOC();


    p_req1 = &HTTPcApp_ReqTbl[0];
    p_req2 = &HTTPcApp_ReqTbl[1];

    if (status == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("Transaction Status Code: %s\n\r", p_resp->ReasonPhrasePtr));
    } else {
        HTTPc_APP_TRACE_INFO(("Transaction failed\n\r"));
    }

    if (p_req == p_req1) {
        p_data = (HTTPc_APP_REQ_DATA *)p_req->UserDataPtr;
        CPU_CRITICAL_ENTER();
        p_data->Done = DEF_YES;
        CPU_CRITICAL_EXIT();
    }

    if (p_req == p_req2) {
        p_data = (HTTPc_APP_REQ_DATA *)p_req->UserDataPtr;
        CPU_CRITICAL_ENTER();
        p_data->Done = DEF_YES;
        CPU_CRITICAL_EXIT();
    }
}
#endif


/*
*********************************************************************************************************
*                                      HTTPcApp_TransErrCallback()
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
void HTTPcApp_TransErrCallback (HTTPc_CONN_OBJ  *p_conn,
                                HTTPc_REQ_OBJ   *p_req,
                                HTTPc_ERR        err_code)
{
    HTTPc_APP_TRACE_INFO(("Transaction error: %i\n\r", err_code));
}
#endif
