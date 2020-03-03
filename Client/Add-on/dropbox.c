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
*                             HTTP CLIENT DROPBOX APPLICATION SOURCE CODE
*
* Filename : dropbox.c
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
#define  DROPBOX_MODULE


#include  <lib_def.h>

#include  <Common/http.h>
#include  <Client/Source/http-c.h>
#include  <http-c_cfg.h>

#include  <FS/net_fs.h>
#include  <FS/uC-FS-V4/net_fs_v4.h>

#include  "dropbox.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  struct  dropbox_conn {
    DROPBOX_RX_DATA        RxDataFnct;
    CPU_CHAR              *RemoteFileName;
    void                  *UserCtxPtr;
} DROPBOX_CONN;

typedef struct dropbox_file {
    CPU_CHAR              *LocalFilename;
    FS_FILE               *FilePtr;
} DROPBOX_FILE;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/



static  CPU_BOOLEAN  Dropbox_ReqSend           (CPU_CHAR                  *p_url,
                                                HTTP_METHOD                method,
                                                HTTPc_CONN_OBJ            *p_conn,
                                                HTTPc_REQ_OBJ             *p_req,
                                                CPU_CHAR                  *p_access_token,
                                                CPU_CHAR                  *p_dropbox_api_url);

static  CPU_BOOLEAN  Dropbox_FS_ReqBodyHook    (HTTPc_CONN_OBJ            *p_conn,
                                                HTTPc_REQ_OBJ             *p_req,
                                                void                     **p_data,
                                                CPU_CHAR                  *p_buf,
                                                CPU_INT16U                 buf_len,
                                                CPU_INT16U                *p_data_len);

static  CPU_BOOLEAN  Dropbox_Buf_ReqBodyHook   (HTTPc_CONN_OBJ            *p_conn,
                                                HTTPc_REQ_OBJ             *p_req,
                                                void                     **p_data,
                                                CPU_CHAR                  *p_buf,
                                                CPU_INT16U                 buf_len,
                                                CPU_INT16U                *p_data_len);

static  void         Dropbox_RespBodyHook      (HTTPc_CONN_OBJ            *p_conn,
                                                HTTPc_REQ_OBJ             *p_req,
                                                HTTP_CONTENT_TYPE          content_type,
                                                void                      *p_data,
                                                CPU_INT16U                 data_len,
                                                CPU_BOOLEAN                last_chunk);

static void          Dropbox_RxFS               (CPU_CHAR                  *remote_file_name,
                                                 void                      *p_data,
                                                 CPU_INT16U                 data_len,
                                                 CPU_BOOLEAN                last_chunk,
                                                 void                      *p_user_ctx);

#ifdef HTTPc_TASK_MODULE_EN
static  void         Dropbox_ConnCloseCallback (HTTPc_CONN_OBJ            *p_conn,
                                                HTTPc_CONN_CLOSE_STATUS    close_status,
                                                HTTPc_ERR                  err);

static  void         Dropbox_TransErrCallback  (HTTPc_CONN_OBJ            *p_conn,
                                                HTTPc_REQ_OBJ             *p_req,
                                                HTTPc_ERR                  err_code);

static  void         Dropbox_ConnSecureCallback(HTTPc_CONN_OBJ            *p_conn,
                                                HTTPc_REQ_OBJ             *p_req,
                                                HTTPc_ERR                  err_code);
#endif


/*
*********************************************************************************************************
*                                           Dropbox_Upload_File()
*
* Description : Upload file on dropbox using uC/FS
*
* Argument(s) : remote_file_name    Pointer to name of the file in server.
*
*               local_file_name     Pointer to name of the file in file system.
*
*               content_type        HTTP_CONTENT_TYPE :
*
*                                        HTTP_CONTENT_TYPE_UNKOWN
*                                        HTTP_CONTENT_TYPE_NONE
*                                        HTTP_CONTENT_TYPE_HTML
*                                        HTTP_CONTENT_TYPE_OCTET_STREAM
*                                        HTTP_CONTENT_TYPE_PDF
*                                        HTTP_CONTENT_TYPE_ZIP
*                                        HTTP_CONTENT_TYPE_GIF
*                                        HTTP_CONTENT_TYPE_JPEG
*                                        HTTP_CONTENT_TYPE_PNG
*                                        HTTP_CONTENT_TYPE_JS
*                                        HTTP_CONTENT_TYPE_PLAIN
*                                        HTTP_CONTENT_TYPE_CSS
*                                        HTTP_CONTENT_TYPE_JSON
*                                        HTTP_CONTENT_TYPE_APP_FORM
*                                        HTTP_CONTENT_TYPE_MULTIPART_FORM
*
*               content_len         Length of the file to upload.
*
*               p_access_token      Pointer to the access token used for Authentication
*
* Return(s)   : DEF_OK,   if HTTPc transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN Dropbox_Upload_File (CPU_CHAR         *p_remote_file_name,
                                 CPU_CHAR         *p_local_file_name,
                                 HTTP_CONTENT_TYPE content_type,
                                 CPU_SIZE_T        content_len,
                                 CPU_CHAR         *p_access_token)
{

    CPU_CHAR            url[DROPBOX_URL_LEN_MAX];
    HTTPc_CONN_OBJ      http_conn;
    HTTPc_REQ_OBJ       http_req;
    HTTPc_ERR           err;
    CPU_BOOLEAN         success;
    DROPBOX_FILE        tx_file;


    Mem_Clr(url, DROPBOX_URL_LEN_MAX);
    Mem_Clr(&http_conn, sizeof(http_conn));
    Mem_Clr(&http_req,  sizeof(http_req));

    Str_Cat_N(&url[0], DROPBOX_URL_FILES_PUT, DROPBOX_URL_LEN_MAX);
    Str_Cat_N(&url[0], p_remote_file_name   , DROPBOX_URL_LEN_MAX);


                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(&http_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(&http_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    tx_file.LocalFilename = p_local_file_name;

    http_req.UserDataPtr = &tx_file;

    HTTPc_ReqSetParam(        &http_req,
                               HTTPc_PARAM_TYPE_REQ_BODY_HOOK,
                      (void *)&Dropbox_FS_ReqBodyHook,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }


    HTTPc_ReqSetParam(&http_req,
                       HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_TYPE,
                      &content_type,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }


    HTTPc_ReqSetParam(&http_req,
                       HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_LEN,
                      &content_len,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    success = Dropbox_ReqSend( url,
                               HTTP_METHOD_PUT,
                              &http_conn,
                              &http_req,
                               p_access_token,
                               DROPBOX_API_CONTENT_HOSTNAME);
    if (success != DEF_OK) {
        return (DEF_FAIL);
    }


    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                           Dropbox_Upload_Buf()
*
* Description : Upload file on dropbox using memory buffer.
*
* Argument(s) : remote_file_name    Pointer to name of the file in server.
*
*               p_buf               Pointer to memory buffer to send.
*
*               p_access_token      Pointer to the access token used for Authentication
*
*               content_type        HTTP_CONTENT_TYPE :
*
*                                        HTTP_CONTENT_TYPE_UNKOWN
*                                        HTTP_CONTENT_TYPE_NONE
*                                        HTTP_CONTENT_TYPE_HTML
*                                        HTTP_CONTENT_TYPE_OCTET_STREAM
*                                        HTTP_CONTENT_TYPE_PDF
*                                        HTTP_CONTENT_TYPE_ZIP
*                                        HTTP_CONTENT_TYPE_GIF
*                                        HTTP_CONTENT_TYPE_JPEG
*                                        HTTP_CONTENT_TYPE_PNG
*                                        HTTP_CONTENT_TYPE_JS
*                                        HTTP_CONTENT_TYPE_PLAIN
*                                        HTTP_CONTENT_TYPE_CSS
*                                        HTTP_CONTENT_TYPE_JSON
*                                        HTTP_CONTENT_TYPE_APP_FORM
*                                        HTTP_CONTENT_TYPE_MULTIPART_FORM
*
*               content_len         Length of the file to upload.
*
* Return(s)   : DEF_OK,   if HTTPc transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN Dropbox_Upload_Buf (CPU_CHAR         *p_remote_file_name,
                                CPU_INT08U       *p_buf,
                                HTTP_CONTENT_TYPE content_type,
                                CPU_SIZE_T        content_len,
                                CPU_CHAR         *p_access_token)
{
    HTTPc_CONN_OBJ      http_conn;
    HTTPc_REQ_OBJ       http_req;
    CPU_CHAR            url[DROPBOX_URL_LEN_MAX];
    HTTPc_ERR           err;
    CPU_BOOLEAN         success;


    Mem_Clr(url, DROPBOX_URL_LEN_MAX);
    Mem_Clr(&http_conn, sizeof(http_conn));
    Mem_Clr(&http_req,  sizeof(http_req));

    Str_Cat_N(&url[0], DROPBOX_URL_FILES_PUT, DROPBOX_URL_LEN_MAX);
    Str_Cat_N(&url[0], p_remote_file_name   , DROPBOX_URL_LEN_MAX);
                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(&http_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(&http_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }


    http_req.UserDataPtr = p_buf;

    HTTPc_ReqSetParam(         &http_req,
                                HTTPc_PARAM_TYPE_REQ_BODY_HOOK,
                      (void *) &Dropbox_Buf_ReqBodyHook,
                               &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }


    HTTPc_ReqSetParam(&http_req,
                       HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_TYPE,
                      &content_type,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }


    HTTPc_ReqSetParam(&http_req,
                       HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_LEN,
                      &content_len,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    success = Dropbox_ReqSend( url,
                               HTTP_METHOD_PUT,
                              &http_conn,
                              &http_req,
                               p_access_token,
                               DROPBOX_API_CONTENT_HOSTNAME);
    if (success != DEF_OK) {
        return (DEF_FAIL);
    }


    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                           Dropbox_Download_Buf()
*
* Description : Download file from dropbox using memory buffer.
*
* Argument(s) : remote_file_name    Pointer to name of the file in server.
*
*               access_token        Pointer to the access token used for Authentication
*
*               fnct                Pointer to the user defined receive data function
*
*               ctx                 Pointer to user context
*
* Return(s)   : DEF_OK,   if HTTPc transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  Dropbox_Download_Buf (CPU_CHAR              *p_remote_file_name,
                                   CPU_CHAR              *p_access_token,
                                   DROPBOX_RX_DATA        fnct,
                                   void                  *p_ctx)
{
    DROPBOX_CONN        conn;
    CPU_CHAR            url[DROPBOX_URL_LEN_MAX];
    HTTPc_CONN_OBJ      http_conn;
    HTTPc_REQ_OBJ       http_req;
    HTTPc_ERR           err;
    CPU_BOOLEAN         success;


    conn.RemoteFileName = p_remote_file_name;
    conn.RxDataFnct     = fnct;
    conn.UserCtxPtr     = p_ctx;

    Mem_Clr(url, DROPBOX_URL_LEN_MAX);
    Mem_Clr(&http_conn, sizeof(http_conn));
    Mem_Clr(&http_req,  sizeof(http_req));

    Str_Cat_N(&url[0], DROPBOX_URL_FILES_GET, DROPBOX_URL_LEN_MAX);
    Str_Cat_N(&url[0], p_remote_file_name,    DROPBOX_URL_LEN_MAX);

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
                      (void *)&Dropbox_RespBodyHook,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    success = Dropbox_ReqSend( url,
                               HTTP_METHOD_GET,
                              &http_conn,
                              &http_req,
                               p_access_token,
                               DROPBOX_API_CONTENT_HOSTNAME);
    if (success != DEF_OK) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}

/*
*********************************************************************************************************
*                                        Dropbox_Download_File()
*
* Description : Download a file from Dropbox.
*
* Argument(s) : p_remote_file_name  Pointer to name of the file in server.
*
*               p_local_file_name   Pointer to name of the file in file system
*
*               p_access_token      Pointer to the access token used for Authentication
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

CPU_BOOLEAN  Dropbox_Download_File (CPU_CHAR              *p_remote_file_name,
                                    CPU_CHAR              *p_local_file_name,
                                    CPU_CHAR              *p_access_token)
{
    DROPBOX_CONN        conn;
    CPU_CHAR            url[DROPBOX_URL_LEN_MAX];
    HTTPc_CONN_OBJ      http_conn;
    HTTPc_REQ_OBJ       http_req;
    HTTPc_ERR           err;
    CPU_BOOLEAN         success;
    DROPBOX_FILE        rx_file;


    rx_file.LocalFilename = p_local_file_name;

    conn.RemoteFileName   =  p_remote_file_name;
    conn.RxDataFnct       = &Dropbox_RxFS;
    conn.UserCtxPtr       = &rx_file;


    Mem_Clr(url, DROPBOX_URL_LEN_MAX);
    Mem_Clr(&http_conn, sizeof(http_conn));
    Mem_Clr(&http_req,  sizeof(http_req));

    Str_Cat_N(&url[0], DROPBOX_URL_FILES_GET, DROPBOX_URL_LEN_MAX);
    Str_Cat_N(&url[0], p_remote_file_name,    DROPBOX_URL_LEN_MAX);

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
                      (void *)&Dropbox_RespBodyHook,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    success = Dropbox_ReqSend( url,
                               HTTP_METHOD_GET,
                              &http_conn,
                              &http_req,
                               p_access_token,
                               DROPBOX_API_CONTENT_HOSTNAME);
    if (success != DEF_OK) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                         Dropbox_Delete_File()
*
* Description : Delete a file on Dropbox server.
*
* Argument(s) : p_remote_file_name  Pointer to name of the file in server.
*
*               p_access_token      Pointer to the access token used for Authentication
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

CPU_BOOLEAN  Dropbox_Delete_File (CPU_CHAR              *p_remote_file_name,
                                  CPU_CHAR              *p_access_token)
{
    CPU_CHAR            url[DROPBOX_URL_LEN_MAX];
    HTTPc_CONN_OBJ      http_conn;
    HTTPc_REQ_OBJ       http_req;
    HTTPc_ERR           err;
    CPU_BOOLEAN         success;


    Mem_Clr(url, DROPBOX_URL_LEN_MAX);
    Mem_Clr(&http_conn, sizeof(http_conn));
    Mem_Clr(&http_req,  sizeof(http_req));


    Str_Cat_N(&url[0], DROPBOX_URL_FILES_OPS   , DROPBOX_URL_LEN_MAX);
    Str_Cat_N(&url[0], "delete?root=auto&path=", DROPBOX_URL_LEN_MAX);
    Str_Cat_N(&url[0], p_remote_file_name      , DROPBOX_URL_LEN_MAX);

                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(&http_conn, &err);
    if (err != HTTPc_ERR_NONE) {
    return (DEF_FAIL);
    }

    HTTPc_ReqClr(&http_req, &err);
    if (err != HTTPc_ERR_NONE) {
    return (DEF_FAIL);
    }

    success = Dropbox_ReqSend( url,
                               HTTP_METHOD_POST,
                              &http_conn,
                              &http_req,
                               p_access_token,
                               DROPBOX_API_HOSTAME);
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
*                                         Dropbox_ReqSend()
*
* Description : Send a request.
*
* Argument(s) : p_url    Pointer to complete URI (or only resource path) of the request.
*
*               method   HTTP method of the request.
*
*               p_conn   Pointer to valid HTTPc Connection on which request will occurred.
*
*               p_req    Pointer to request to send.
*
* Return(s)   : DEF_OK,   if HTTP transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static CPU_BOOLEAN  Dropbox_ReqSend  (CPU_CHAR       *p_url,
                                      HTTP_METHOD     method,
                                      HTTPc_CONN_OBJ *p_conn,
                                      HTTPc_REQ_OBJ  *p_req,
                                      CPU_CHAR       *p_access_token,
                                      CPU_CHAR       *p_dropbox_api_url)
{
    HTTPc_HDR           hdr;
    CPU_CHAR            hdr_val[DROPBOX_CFG_HDR_VAL_LEN_MAX];
    CPU_CHAR            buf[DROPBOX_CFG_CONN_BUF_SIZE];
    HTTPc_RESP_OBJ      http_resp;
    HTTPc_PARAM_TBL     tbl_obj;
    CPU_SIZE_T          str_len;
    CPU_BOOLEAN         result;
    HTTPc_ERR           err;


    Mem_Clr(hdr_val, 255);
                                                                /* Prepare the "Signature" required by Oauth ...*/
                                                                /* ... sent through "Authorization" header      */

    Str_Cat_N(hdr_val, "Bearer ",       DROPBOX_CFG_HDR_VAL_LEN_MAX);
    Str_Cat_N(hdr_val,  p_access_token, DROPBOX_CFG_HDR_VAL_LEN_MAX);

    hdr.ValPtr  = hdr_val;
    hdr.ValLen  = Str_Len_N(hdr_val, DROPBOX_CFG_HDR_VAL_LEN_MAX);
    hdr.HdrField = HTTP_HDR_FIELD_AUTHORIZATION;

                                                                /* ------------- SET CONNECTION CALLBACKS ------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)
    HTTPc_ConnSetParam(         p_conn,
                                HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       (void *)&Dropbox_ConnCloseCallback,
                               &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ConnSetParam(         p_conn,
                                HTTPc_PARAM_TYPE_SECURE_TRUST_CALLBACK,
                       (void *)&Dropbox_ConnSecureCallback,
                               &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif


                                                                /* ------------ SET REQ/HDR PARAM          ----------- */
    tbl_obj.EntryNbr = 1;
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
    HTTPc_ReqSetParam(         p_req,
                               HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      (void *)&Dropbox_TransErrCallback,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
#endif

                                                                /* ----------------- OPEN CONNECTION ------------------ */
    str_len = Str_Len(p_dropbox_api_url);
    result  = HTTPc_ConnOpen(p_conn,
                             buf,
                             DROPBOX_CFG_CONN_BUF_SIZE,
                             p_dropbox_api_url,
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
    result  = HTTPc_ReqSend(p_conn,
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


/*
*********************************************************************************************************
*                                            Dropbox_RxFS()
*
* Description : Save data received in file system.
*
* Argument(s) : remote_file_name    Pointer to name of the file in server.
*
*               p_data              Pointer to a data piece of the HTTP Response body.
*
*               data_len            Length of the data piece received.
*
*               last_chunk          DEF_YES, if this is the last piece of data.
*
*                                   DEF_NO,  if more data is up coming.
*
*               p_user_ctx          Pointer to context.
*
* Return(s)   : none.
*
* Caller(s)   : Dropbox_Download_File().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

void  Dropbox_RxFS  (CPU_CHAR          *remote_file_name,
                     void              *p_data,
                     CPU_INT16U         data_len,
                     CPU_BOOLEAN        last_chunk,
                     void              *p_user_ctx)
{
    FS_ERR           err_fs;
    CPU_SIZE_T       len_wr;
    FS_FILE         *p_file;
    CPU_BOOLEAN      is_open;
    FS_FLAGS         fs_flags;
    CPU_CHAR        *p_filename;
    DROPBOX_FILE    *rx_file;

    rx_file = p_user_ctx;
    p_filename = rx_file->LocalFilename;


    is_open = FSFile_IsOpen(p_filename,
                           &fs_flags,
                           &err_fs);
    if (err_fs != FS_ERR_NONE) {
        goto exit;
    }

    if (is_open == DEF_NO) {
        fs_flags = 0;
        DEF_BIT_SET(fs_flags, FS_FILE_ACCESS_MODE_WR);
        DEF_BIT_SET(fs_flags, FS_FILE_ACCESS_MODE_APPEND);
        DEF_BIT_SET(fs_flags, FS_FILE_ACCESS_MODE_CREATE);
        p_file = FSFile_Open(p_filename,
                             fs_flags,
                            &err_fs);
        if (err_fs != FS_ERR_NONE) {
             goto exit;
        }
        rx_file->FilePtr = p_file;
    } else {
        p_file = rx_file->FilePtr;
    }


    len_wr = FSFile_Wr(p_file,
                       p_data,
                       data_len,
                      &err_fs);
    if (err_fs != FS_ERR_NONE) {
        goto exit_close;
    }

    if (last_chunk == DEF_YES) {
        goto exit_close;
    }

    goto exit;

exit_close:
    FSFile_Close(p_file, &err_fs);
exit:
    return;


}


/*
*********************************************************************************************************
*                                          Dropbox_FS_ReqBodyHook()
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

static  CPU_BOOLEAN  Dropbox_FS_ReqBodyHook (HTTPc_CONN_OBJ     *p_conn,
                                             HTTPc_REQ_OBJ      *p_req,
                                             void              **p_data,
                                             CPU_CHAR           *p_buf,
                                             CPU_INT16U          buf_len,
                                             CPU_INT16U         *p_data_len)
{
    FS_ERR           err_fs;
    CPU_SIZE_T       len_rd;
    CPU_SIZE_T       file_rem;
    CPU_SIZE_T       size;
    FS_FILE         *p_file;
    CPU_BOOLEAN      is_open;
    CPU_BOOLEAN      finish;
    FS_FLAGS         fs_flags;
    CPU_CHAR        *p_filename;
    DROPBOX_FILE    *tx_file;

    tx_file = p_req->UserDataPtr;
    p_filename = tx_file->LocalFilename;
   *p_data     = DEF_NULL;

    is_open = FSFile_IsOpen(p_filename,
                           &fs_flags,
                           &err_fs);
    if (err_fs != FS_ERR_NONE) {
       *p_data_len = 0;
        finish = DEF_YES;
        goto exit;
    }

    if (is_open == DEF_NO) {
        fs_flags = 0;
        DEF_BIT_SET(fs_flags, FS_FILE_ACCESS_MODE_RD);
        DEF_BIT_SET(fs_flags, FS_FILE_ACCESS_MODE_CREATE);
        p_file = FSFile_Open(p_filename,
                             fs_flags,
                            &err_fs);
        if (err_fs != FS_ERR_NONE) {
            *p_data_len = 0;
             finish = DEF_YES;
             goto exit;
        }
        tx_file->FilePtr = p_file;
    } else {
        p_file = tx_file->FilePtr;
    }

    file_rem = p_file->Size - p_file->Pos;
    if (file_rem <= 0) {
        *p_data_len = 0;
        finish = DEF_YES;
        goto exit_close;
    }

    size = DEF_MIN(file_rem, buf_len);

    len_rd = FSFile_Rd(p_file,
                       p_buf,
                       size,
                      &err_fs);
    if (err_fs != FS_ERR_NONE) {
       *p_data_len = 0;
        finish = DEF_YES;
        goto exit_close;
    }

   *p_data_len = len_rd;
    finish = DEF_NO;
    if (len_rd == file_rem) {
        finish = DEF_YES;
        goto exit_close;
    }
    goto exit;

exit_close:
    FSFile_Close(p_file, &err_fs);
    p_req->UserDataPtr = DEF_NULL;
exit:
    return (finish);

}


/*
*********************************************************************************************************
*                                          Dropbox_Buf_ReqBodyHook()
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

static CPU_BOOLEAN  Dropbox_Buf_ReqBodyHook  (HTTPc_CONN_OBJ     *p_conn,
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

    p_req->UserDataPtr = DEF_NULL;

    return (DEF_YES);
}


/*
*********************************************************************************************************
*                                       Dropbox_RespBodyHook()
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

static  void  Dropbox_RespBodyHook (HTTPc_CONN_OBJ     *p_conn,
                                    HTTPc_REQ_OBJ      *p_req,
                                    HTTP_CONTENT_TYPE   content_type,
                                    void               *p_data,
                                    CPU_INT16U          data_len,
                                    CPU_BOOLEAN         last_chunk)
{
    DROPBOX_CONN  *p_dropbox_conn;


    p_dropbox_conn = (DROPBOX_CONN *)p_req->UserDataPtr;
        if (p_dropbox_conn->RxDataFnct != DEF_NULL) {
            p_dropbox_conn->RxDataFnct(p_dropbox_conn->RemoteFileName,
                                       p_data,
                                       data_len,
                                       last_chunk,
                                       p_dropbox_conn->UserCtxPtr);
    }
}


/*
*********************************************************************************************************
*                                     Dropbox_ConnCloseCallback()
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
static  void  Dropbox_ConnCloseCallback (HTTPc_CONN_OBJ           *p_conn,
                                         HTTPc_CONN_CLOSE_STATUS   close_status,
                                         HTTPc_ERR                 err)
{

}
#endif


/*
*********************************************************************************************************
*                                      Dropbox_TransErrCallback()
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
static  void Dropbox_TransErrCallback (HTTPc_CONN_OBJ  *p_conn,
                                       HTTPc_REQ_OBJ   *p_req,
                                       HTTPc_ERR        err_code)
{

}
#endif


/*
*********************************************************************************************************
*                                      Dropbox_ConnSecureCallback()
*
* Description : TODO
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

static  void Dropbox_ConnSecureCallback (HTTPc_CONN_OBJ  *p_conn,
                                         HTTPc_REQ_OBJ   *p_req,
                                         HTTPc_ERR        err_code)
{

}
