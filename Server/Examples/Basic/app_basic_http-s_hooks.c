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
*                                  HTTP INSTANCE SERVER HOOKS FILE
*
* Filename : app_basic_http-s_hooks.c
* Version  : V3.01.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "app_basic.h"
#include  "app_basic_http-s_instance_cfg.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  ERR_404_STR_FILE           "404.html"

#define  INDEX_PAGE_URL             "/index.html"
#define  REST_PAGE_URL              "/list.html"
#define  FORM_SUBMIT_URL            "/form_submit"

#define  FORM_LOGOUT_FIELD_NAME     "Log out"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        FUNCTIONS PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_InstanceInitHook      (const  HTTPs_INSTANCE         *p_instance,
                                                  const  void                   *p_hook_cfg);

static  CPU_BOOLEAN  HTTPs_ReqHdrRxHook          (const  HTTPs_INSTANCE         *p_instance,
                                                  const  HTTPs_CONN             *p_conn,
                                                  const  void                   *p_hook_cfg,
                                                         HTTP_HDR_FIELD          hdr_field);

static  CPU_BOOLEAN  HTTPs_ReqHook               (const  HTTPs_INSTANCE         *p_instance,
                                                         HTTPs_CONN             *p_conn,
                                                  const  void                   *p_hook_cfg);

static  CPU_BOOLEAN  HTTPs_ReqBodyRxHook         (const  HTTPs_INSTANCE         *p_instance,
                                                         HTTPs_CONN             *p_conn,
                                                  const  void                   *p_hook_cfg,
                                                         void                   *p_buf,
                                                  const  CPU_SIZE_T              buf_size,
                                                         CPU_SIZE_T             *p_buf_size_used);

static  CPU_BOOLEAN  HTTPs_ReqRdySignalHook      (const  HTTPs_INSTANCE         *p_instance,
                                                         HTTPs_CONN             *p_conn,
                                                  const  void                   *p_hook_cfg,
                                                  const  HTTPs_KEY_VAL         *p_data);

static  CPU_BOOLEAN  HTTPs_ReqRdyPollHook        (const  HTTPs_INSTANCE         *p_instance,
                                                         HTTPs_CONN             *p_conn,
                                                  const  void                   *p_hook_cfg);

static  CPU_BOOLEAN  HTTPs_RespHdrTxHook         (const  HTTPs_INSTANCE         *p_instance,
                                                         HTTPs_CONN             *p_conn,
                                                  const  void                   *p_hook_cfg);

static  CPU_BOOLEAN  HTTPs_RespTokenValGetHook   (const  HTTPs_INSTANCE         *p_instance,
                                                         HTTPs_CONN             *p_conn,
                                                  const  void                   *p_hook_cfg,
                                                  const  CPU_CHAR               *p_token,
                                                         CPU_INT16U              token_len,
                                                         CPU_CHAR               *p_val,
                                                         CPU_INT16U              val_len_max);

static  CPU_BOOLEAN  HTTPs_RespChunkDataGetHook  (const  HTTPs_INSTANCE         *p_instance,
                                                         HTTPs_CONN             *p_conn,
                                                  const  void                   *p_hook_cfg,
                                                         void                   *p_buf,
                                                         CPU_SIZE_T              buf_len_max,
                                                         CPU_SIZE_T             *p_tx_len);

static  void         HTTPs_TransCompleteHook     (const  HTTPs_INSTANCE         *p_instance,
                                                         HTTPs_CONN             *p_conn,
                                                  const  void                   *p_hook_cfg);

static void          HTTPs_ErrFileGetHook        (const  void                   *p_hook_cfg,
                                                         HTTP_STATUS_CODE        status_code,
                                                         CPU_CHAR               *p_file_str,
                                                         CPU_INT32U              file_len_max,
                                                         HTTPs_BODY_DATA_TYPE   *p_file_type,
                                                         HTTP_CONTENT_TYPE      *p_content_type,
                                                         void                  **p_data,
                                                         CPU_INT32U             *p_data_len);

static  void         HTTPs_ErrHook               (const  HTTPs_INSTANCE         *p_instance,
                                                         HTTPs_CONN             *p_conn,
                                                  const  void                   *p_hook_cfg,
                                                         HTTPs_ERR               err);

static  void         HTTPs_ConnCloseHook         (const  HTTPs_INSTANCE         *p_instance,
                                                         HTTPs_CONN             *p_conn,
                                                  const  void                   *p_hook_cfg);


/*
*********************************************************************************************************
*                                      HTTP SERVER HOOK CONFIGURATION
*
* Note(s): (1)  When the instance is created, an hook function can be called to initialize connection objects used by the instance.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_InstanceInitHook() function for further details.
*
*          (2)  Each time a header field other than the default one is received, a hook function is called
*               allowing to choose which header field(s) to keep for further processing.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_ReqHdrRxHook() function for further details.
*
*          (3)  For each new incoming connection request a hook function can be called by the web server to authenticate
*               the remote connection to accept or reject it. This function can have access to allow stored request header
*               field.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_ReqHook() function for further details.
*
*          (4)  If the upper application want to parse the data received in the request body itself, a hook function is available.
*               It will be called each time new data are received. The exception is when a POST request with a form is
*               received. In that case, the HTTP server core will parse the body and saved the data into Key-Value blocks.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_ReqBodyRxHook() function for further details.
*
*          (5)  The Signal hook function occurs after the HTTP request has been completely received.
*               The hook function SHOULD NOT be blocking and SHOULD return quickly. A time consuming function will
*               block the processing of the other connections and reduce the HTTP server performance.
*               In case the request processing is time consuming, the Poll hook function SHOULD be enabled to
*               allow the server to periodically verify if the upper application has finished the request processing.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_ReqRdySignalHook() function for further details.
*
*          (6)  The Poll hook function SHOULD be enable in case the request processing require lots of time. It
*               allows the HTTP server to periodically poll the upper application and verify if the request processing
*               has finished.
*               If the Poll feature is not required, this field SHOULD be set as DEF_NULL.
*               See HTTPs_ReqRdyPollHook() function for further details.
*
*          (7)  Before an HTTP response message is transmitted, a hook function is called to enable adding header field(s) to
*               the message before it is sent.
*               The Header Module must be enabled for this hook to be called. See HTTPs_CFG_HDR in http-s_cfg.h.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_RespHdrTxHook() function for further details.
*
*          (8)  The hook function is called by the web server when a token is found. This means the hook
*               function must fill a buffer with the value of the instance token to be sent.
*               If the feature is not enabled, this field is not used and can be set as DEF_NULL.
*               See 'HTTPs_RespTokenValGetHook' for further information.
*
*          (9)  To allow the upper application to transmit data with the Chunked Transfer Encoding, a hook function is
*               available. If defined, it will be called at the moment of the Response body transfer, and it will be called
*               until the application has transfer all its data.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_RespChunkDataGetHook() function for further details.
*
*          (10) Once an HTTP transaction is completed, a hook function can be called to notify the upper application that the
*               transaction is done. This hook function could be used to free some previously allocated memory.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_TransCompleteHook() function for further details.
*
*          (11) When an internal error occurs during the processing of a connection a hook function can be called to
*               notify the application of the error and to change the behavior such as the status code and the page returned.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_ErrHook() function for further details.
*
*          (12) Get error file hook can be called every time an error has occurred when processing a connection.
*               This function can set the web page that should be transmit instead of the default error page defined
*               in http-s_cfg.h.
*               If set to DEF_NULL the default error page will be used for every error.
*               See HTTPs_ErrFileGetHook() function for further details.
*
*          (13) Once a connection is closed a hook function can be called to notify the upper application that a connection
*               is no more active. This hook function could be used to free some previously allocated memory.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_ConnCloseHook() function for further details.
*********************************************************************************************************
*/

const  HTTPs_HOOK_CFG  HTTPs_Hooks_AppBasic = {
        HTTPs_InstanceInitHook,                                 /* .OnInstanceInitHook    See Note #1.     */
        HTTPs_ReqHdrRxHook,                                     /* .OnReqHdrRxHook        See Note #2.     */
        HTTPs_ReqHook,                                          /* .OnReqHook             See Note #3.     */
        HTTPs_ReqBodyRxHook,                                    /* .OnReqBodyRxHook       See Note #4.     */
        HTTPs_ReqRdySignalHook,                                 /* .OnReqRdySignalHook    See Note #5.     */
        HTTPs_ReqRdyPollHook,                                   /* .OnReqRdyPollHook      See Note #6.     */
        HTTPs_RespHdrTxHook,                                    /* .OnRespHdrTxHook       See Note #7.     */
        HTTPs_RespTokenValGetHook,                              /* .OnRespTokenHook       See Note #8.     */
        HTTPs_RespChunkDataGetHook,                             /* .OnRespChunkHook       See Note #9.     */
        HTTPs_TransCompleteHook,                                /* .OnTransCompleteHook   See Note #10.    */
        HTTPs_ErrHook,                                          /* .OnErrHook             See Note #11.    */
        HTTPs_ErrFileGetHook,                                   /* .OnErrFileGetHook      See Note #12.    */
        HTTPs_ConnCloseHook                                     /* .OnConnCloseHook       See Note #13.    */
};


/*
*********************************************************************************************************
*                                        HTTPs_InstanceInitHook()
*
* Description : Called to initialized the instance connection objects;
*               Examples of behaviors that could be implemented :
*
*               (a) Session connections handling initialization:
*
*                   (1) Initialize the memory pool and chained list for session connection objects.
*                   (2) Initialize a periodic timer which check for expired session and release them if
*                       it is the case.
*
*               (b) Back-end Application Request processing task initialization.
*
*
* Argument(s) : p_instance  Pointer to the HTTPs instance object.
*
*               p_hook_cfg  Pointer to hook configuration object.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_InstanceInit() via 'p_cfg->HooksPtr->OnInstanceInitHook()'.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_InstanceInitHook (const  HTTPs_INSTANCE  *p_instance,
                                             const  void            *p_hook_cfg)
{
    /* Nothing to do for this example. */
    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       HTTPs_ReqHdrRxHook()
*
* Description : Called each time a header field is parsed in a request message. Allows to choose which
*               additional header field(s) need to be processed by the upper application.
*
* Argument(s) : p_instance  Pointer to the HTTPs instance object.
*
*               p_conn      Pointer to the HTTPs connection object.
*
*               p_hook_cfg  Pointer to hook configuration object.
*
*               hdr_field   Type of the header field received.
*                               See the HTTPs_HDR_FIELD declaration in http-s.h file for all the header types supported.
*
* Return(s)   : DEF_YES,   If the header field needs to be process.
*               DEF_NO,    Otherwise.
*
* Caller(s)   : HTTPs_ReqHdrParse() via 'p_cfg->HooksPtr->OnReqHdrRxHook()'.
*
* Note(s)     : (1) The instance structure is for read-only. It MUST NOT be modified.
*
*               (2) The connection structure SHOULD NOT be modified. It should be only read to determine if the header
*                   type must be stored.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_ReqHdrRxHook (const  HTTPs_INSTANCE   *p_instance,
                                         const  HTTPs_CONN       *p_conn,
                                         const  void             *p_hook_cfg,
                                                HTTP_HDR_FIELD    hdr_field)
{
#if (HTTPs_CFG_HDR_RX_EN == DEF_ENABLED)
    switch (hdr_field) {
        case HTTP_HDR_FIELD_COOKIE:
        case HTTP_HDR_FIELD_COOKIE2:
             return(DEF_YES);

        default:
             return(DEF_NO);
    }
#else
    return (DEF_NO);
#endif
}


/*
*********************************************************************************************************
*                                          HTTPs_ReqHook()
*
* Description : Called after the parsing of an HTTP request message's first line and header(s).
*               Allows the application to process the information received in the request message.
*               Examples of behaviors that could be implemented :
*
*               (a) Analyze the Request-URI and validate that the client has the permission to access
*                   the resource. If not, change the Response Status Code to 403 (Forbidden) or 401
*                   (Unauthorized) if an Authentication technique is implemented. In case of a 401
*                   Status, a "WWW-Authenticate" header needs to be added to the response message
*                   (See HTTPs_InstanceRespHdrTx() function)
*
*               (b) Depending on whether the header feature is enabled and which header fields have been
*                   chosen for use (see HTTPs_ReqHdrRxHook() function), different behaviors
*                   are possible. Here are some examples :
*
*                       (1) A "Cookie" header is received. The default html page is modified to include
*                           personalized features for the client.
*
*                       (2) An "Authorization" header is received. This validates that the client login is good and
*                           changes permanently its' access to the folder/file.
*
*                       (3) An "If-Modified-Since" header is received. It then validates whether or not the resource
*                           has been modified since the 'HTTP-date' received with the header. If it was, continue
*                           with the request processing normally, else change the Status Code to 304 (Not Modified).
*
*
* Argument(s) : p_instance  Pointer to the HTTPs instance object.
*
*               p_conn      Pointer to the HTTPs connection object.
*
*               p_hook_cfg  Pointer to hook configuration object.
*
* Return(s)   : DEF_OK    if the application allows the request to be continue.
*               DEF_FAIL  otherwise.
*                               Status code will be set automatically to HTTPs_STATUS_UNAUTHORIZED
*
* Caller(s)   : HTTPs_Req() via 'p_cfg->HooksPtr->OnReqHook'.
*
* Note(s)     : (1) The instance structure is for read-only. It must not be modified at any point in this hook function.
*
*               (2) The following connection attributes can be accessed to analyze the connection:
*
*                   (a) 'ClientAddr'
*                           This connection parameter contains the IP address and port used by the remote client to access the
*                           server instance.
*
*                   (b) 'Method'
*                           HTTPs_METHOD_GET        Get  request
*                           HTTPs_METHOD_POST       Post request
*                           HTTPs_METHOD_HEAD       Head request
*
*                   (c) 'PathPtr'
*                           This is a pointer to the string that contains the name of the file requested.
*
*                   (d) 'HdrCtr'
*                           This parameter is a counter of the number of header field that has been stored.
*
*                   (e) 'HdrListPtr'
*                           This parameter is a pointer to the first header field stored. A linked list is created with
*                           all header field stored.
*
*               (3) In this hook function, only the under-mentioned connection parameters are allowed
*                   to be modified :
*
*                   (a) 'StatusCode'
*
*                           See HTTPs_STATUS_CODE declaration in http-s.h for all the status code supported.
*
*                   (b) 'PathPtr'
*
*                           This is a pointer to the string that contains the name of the file requested. You can change
*                           the name of the requested file to send another file instead without error.
*
*                   (c) 'DataPtr'
*
*                           This is a pointer to the data file or data to transmit. This parameter should be null when calling
*                           this function. If data from memory has to be sent instead of a file, this pointer must be set
*                           to the location of the data.
*
*                   (d) 'RespBodyDataType'
*
*                           HTTPs_BODY_DATA_TYPE_FILE              Open and transmit a file. Value by default.
*                           HTTPs_BODY_DATA_TYPE_STATIC_DATA       Transmit data from the memory. Must be set by the hook function.
*                           HTTPs_BODY_DATA_TYPE_NONE              No body in response.
*
*                   (e) 'DataLen'
*
*                           0,                              Default value, will be set when the file is opened.
*                           Data length,                    Must be set by the data length when transmitting data from
*                                                           the memory
*
*                   (f) 'ConnDataPtr'
*
*                           This is a pointer available for the upper application when memory block must be allocated
*                           to process the connection request. If memory is allocated by the upper application, the memory
*                           space can be deallocated into another hook function.
*
*               (4) When the Location of the requested file has changed, besides the Status Code to change (3xx),
*                   the 'PathPtr' parameter needs to be updated. A "Location" header will be added automatically in
*                   the response by uC/HTTPs core with the new location.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_ReqHook (const  HTTPs_INSTANCE  *p_instance,
                                           HTTPs_CONN      *p_conn,
                                    const  void            *p_hook_cfg)
{
    CPU_INT16S  str_cmp;
    HTTPs_ERR   err;


                                                                /* Redirect the REST page example to the default page. */
    str_cmp = Str_Cmp_N(p_conn->PathPtr, REST_PAGE_URL, p_conn->PathLenMax);
    if (str_cmp == 0) {

        p_conn->StatusCode = HTTP_STATUS_SEE_OTHER;

        HTTPs_RespBodySetParamNoBody(p_instance, p_conn, &err);
        if (err != HTTPs_ERR_NONE) {
            return (DEF_FAIL);
        }

#if (HTTPs_CFG_ABSOLUTE_URI_EN == DEF_ENABLED)
        Str_Copy(p_conn->HostPtr, "");
        Str_Copy(p_conn->PathPtr, INDEX_PAGE_URL);
#else
        Str_Copy(p_conn->PathPtr, INDEX_PAGE_URL);
#endif
    }

    str_cmp = Str_Cmp_N(p_conn->PathPtr, FORM_SUBMIT_URL, p_conn->PathLenMax);
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
*                                         HTTPs_ReqBodyRxHook()
*
* Description : Called when body data is received by the HTTPs core. Allows the application to retrieve
*               body data.
*
* Argument(s) : p_instance       Pointer to the HTTPs instance object.
*
*               p_conn           Pointer to the HTTPs connection object.
*
*               p_hook_cfg       Pointer to hook configuration object.
*
*               p_buf            Pointer to the data buffer.
*
*               buf_size         Size of the data rx available inside the buffer.
*
*               p_buf_size_used  Pointer to the variable that will received the length of the data consumed by the app.
*
* Return(s)   : DEF_YES    To continue with the data reception.
*               DEF_NO     If the application doesn't want to rx data anymore.
*
* Caller(s)   : HTTPs_Body() via p_cfg->HooksPtr->OnReqBodyRxHook().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_ReqBodyRxHook (const  HTTPs_INSTANCE  *p_instance,
                                                 HTTPs_CONN      *p_conn,
                                          const  void            *p_hook_cfg,
                                                 void            *p_buf,
                                          const  CPU_SIZE_T       buf_size,
                                                 CPU_SIZE_T      *p_buf_size_used)
{
    /* Nothing to do for this example. */
    return (DEF_NO);
}


/*
*********************************************************************************************************
*                                       HTTPs_ReqRdySignalHook()
*
* Description : If defined, this hook function is called after the request has been completely received and
*               parse by the HTTP server core.
*
* Argument(s) : p_instance  Pointer to the HTTPs instance object.
*
*               p_conn      Pointer to the HTTPs connection object.
*
*               p_hook_cfg  Pointer to hook configuration object.
*
*               p_data      Pointer to the first key-value pair received in a form if any.
*                           DEF_NULL otherwise.
*
* Return(s)   : DEF_YES, if the response can be sent.
*
*               DEF_NO,  if the response cannot be sent after this call and the Poll function MUST be called before
*                        sending the response (see note #3).
*
* Caller(s)   : HTTPs_MethodPost() via 'p_cfg->HooksPtr->OnReqRdySignalHook()'.
*
* Note(s)     : (1) This callback function SHOULD NOT be blocking and SHOULD return quickly. A time consuming
*                   function will block the processing of other connections and reduce the HTTP server performance.
*
*               (2) If the request data received take a while to be processed:
*
*                   (a) the processing SHOULD be done in a separate task and not in this callback function to avoid
*                       blocking other connections.
*
*                   (b) the poll callback function SHOULD be used to allow the connection to poll periodically the
*                       upper application and verify if the request data processing has been completed.
*
*                       The 'ConnDataPtr' attribute inside HTTPs_CONN structure can be used to store a
*                       semaphore pointer related to the completion of the request data processing.
*
*                       See 'HTTPs_ReqRdyPollHook()' for more details on poll function.
*
*               (3) The following connection attributes can be accessed to analyze the connection:
*
*                   (a) 'ClientAddr'
*                           This connection parameter contains the IP address and port used by the remote client to access the
*                           server instance.
*
*                   (b) 'Method'
*                           HTTPs_METHOD_GET        Get  request
*                           HTTPs_METHOD_POST       Post request
*                           HTTPs_METHOD_HEAD       Head request
*
*                   (c) 'PathPtr'
*                           This is a pointer to the string that contains the name of the file requested.
*
*                   (d) 'HdrCtr'
*                           This parameter is a counter of the number of header field that has been stored.
*
*                   (e) 'HdrListPtr'
*                           This parameter is a pointer to the first header field stored. A linked list is created with
*                           all header field stored.
*
*                   (f) 'ConnDataPtr'
*                           This is a pointer available for the upper application when memory block must be allocated
*                           to process the connection request. If memory is allocated by the upper application, the memory
*                           space can be deallocated into another hook function.
*
*               (4) In this hook function, only the under-mentioned connection parameters are allowed
*                   to be modified :
*
*                   (a) 'StatusCode'
*                           See HTTPs_STATUS_CODE declaration in http-s.h for all the status code supported.
*
*                   (b) 'PathPtr'
*                           This is a pointer to the string that contains the name of the file requested. You can change
*                           the name of the requested file to send another file instead without error.
*
*                   (c) 'DataPtr'
*                           This is a pointer to the data file or data to transmit. This parameter should be null when calling
*                           this function. If data from memory has to be sent instead of a file, this pointer must be set
*                           to the location of the data.
*
*                   (d) 'RespBodyDataType'
*                           HTTPs_BODY_DATA_TYPE_FILE              Open and transmit a file. Value by default.
*                           HTTPs_BODY_DATA_TYPE_STATIC_DATA       Transmit data from the memory. Must be set by the hook function.
*                           HTTPs_BODY_DATA_TYPE_NONE              No body in response.
*
*                   (e) 'DataLen'
*                           0,                              Default value, will be set when the file is opened.
*                           Data length,                    Must be set to the data length when transmitting data from
*                                                           the memory
*
*               (5) When the Location of the requested file has change, besides the Status Code to change (3xx),
*                   the 'PathPtr' parameter needs to be update. A "Location" header will be added automatically in
*                   the response by uC/HTTPs core with the new location.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_ReqRdySignalHook (const  HTTPs_INSTANCE  *p_instance,
                                                    HTTPs_CONN      *p_conn,
                                             const  void            *p_hook_cfg,
                                             const  HTTPs_KEY_VAL  *p_data)
{
    HTTPs_KEY_VAL  *p_ctrl_var;
    CPU_INT16S      str_cmp;
    HTTPs_ERR       err;


    p_ctrl_var = (HTTPs_KEY_VAL *)p_data;
    while (p_ctrl_var != DEF_NULL) {

                                                                /* -------------- RECEIVED KEY-VALUE TYPE ------------- */
        if (p_ctrl_var->DataType == HTTPs_KEY_VAL_TYPE_PAIR) {

            str_cmp = Str_Cmp_N(p_ctrl_var->KeyPtr, FORM_LOGOUT_FIELD_NAME, p_ctrl_var->KeyLen);
            if (str_cmp == 0) {

                p_conn->StatusCode = HTTP_STATUS_SEE_OTHER;     /* Redirect the page...                               */

                HTTPs_RespBodySetParamNoBody(p_instance, p_conn, &err);
                if (err != HTTPs_ERR_NONE) {
                    return (DEF_YES);
                }

#if (HTTPs_CFG_ABSOLUTE_URI_EN == DEF_ENABLED)
                Str_Copy(p_conn->HostPtr, "");
                Str_Copy(p_conn->PathPtr, INDEX_PAGE_URL);
#else
                Str_Copy(p_conn->PathPtr, INDEX_PAGE_URL);
#endif
            }

        } else if (p_ctrl_var->DataType == HTTPs_KEY_VAL_TYPE_FILE) {
                                                                /* Send back in response last file received in post.    */
            HTTPs_RespBodySetParamFile(p_instance,
                                       p_conn,
                                       p_ctrl_var->ValPtr,
                                       HTTP_CONTENT_TYPE_UNKNOWN,
                                       DEF_NO,
                                      &err);
            if (err != HTTPs_ERR_NONE) {
                return (DEF_YES);
            }
        }

        p_ctrl_var = p_ctrl_var->NextPtr;
    }

    (void)p_instance;                                           /* Prevent 'variable unused' compiler warning.          */


    return (DEF_YES);
}


/*
*********************************************************************************************************
*                                         HTTPs_ReqRdyPollHook()
*
* Description : Called periodically by a connection waiting for the upper application to complete the
*               request processing.
*
* Argument(s) : p_instance  Pointer to the HTTPs instance object.
*
*               p_conn      Pointer to the HTTPs connection object.
*
*               p_hook_cfg  Pointer to hook configuration object.
*
* Return(s)   : DEF_YES, if the response can be sent.
*
*               DEF_NO,  if the response cannot be sent after this call and the Poll function MUST be called again
*                        before sending the response (see note #2).
*
* Caller(s)   : HTTPs_MethodPost() via 'p_cfg->HooksPtr->OnReqRdyPollHook()'.
*
* Note(s)     : (1) This callback function SHOULD NOT be blocking and SHOULD return quickly. A time consuming
*                   function will block the processing of other connections and reduce the HTTP server performance.
*
*                   This function will be called periodically by the connection until DEF_YES is returned.
*
*               (2) The poll callback function SHOULD be used when the request processing takes a while to
*                   be completed. It will allow the server to periodically poll the upper application to verify
*                   if the request processing has finished.
*
*                   The 'ConnDataPtr' attribute inside the HTTP_CONN structure can be used to store a
*                   semaphore pointer related to the completion of the request processing.
*
*                   See 'HTTPs_InstanceReqRdySignal()' for more details on post/poll functionality.
*
*               (3) The following connection attributes can be accessed to analyze the connection:
*
*                   (a) 'ClientAddr'
*                           This connection parameter contains the IP address and port used by the remote client to access the
*                           server instance.
*
*                   (b) 'Method'
*                           HTTPs_METHOD_GET        Get  request
*                           HTTPs_METHOD_POST       Post request
*                           HTTPs_METHOD_HEAD       Head request
*
*                   (c) 'PathPtr'
*                           This is a pointer to the string that contains the name of the file requested.
*
*                   (d) 'HdrCtr'
*                           This parameter is a counter of the number of header field that has been stored.
*
*                   (e) 'HdrListPtr'
*                           This parameter is a pointer to the first header field stored. A linked list is created with
*                           all header field stored.
*
*                   (f) 'ConnDataPtr'
*                           This is a pointer available for the upper application when memory block must be allocated
*                           to process the connection request. If memory is allocated by the upper application, the memory
*                           space can be deallocated into another hook function.
*
*               (4) In this hook function, only the under-mentioned connection parameters are allowed
*                   to be modified :
*
*                   (a) 'StatusCode'
*                           See HTTPs_STATUS_CODE declaration in http-s.h for all the status code supported.
*
*                   (b) 'PathPtr'
*                           This is a pointer to the string that contains the name of the file requested. You can change
*                           the name of the requested file to send another file instead without error.
*
*                   (c) 'DataPtr'
*                           This is a pointer to the data file or data to transmit. This parameter should be null when calling
*                           this function. If data from memory has to be sent instead of a file, this pointer must be set
*                           to the location of the data.
*
*                   (d) 'RespBodyDataType'
*                           HTTPs_BODY_DATA_TYPE_FILE              Open and transmit a file. Value by default.
*                           HTTPs_BODY_DATA_TYPE_STATIC_DATA       Transmit data from the memory. Must be set by the hook function.
*                           HTTPs_BODY_DATA_TYPE_NONE              No body in response.
*
*                   (e) 'DataLen'
*                           0,                              Default value, will be set when the file is opened.
*                           Data length,                    Must be set to the data length when transmitting data from
*                                                           the memory
*
*               (5) When the Location of the requested file has change, besides the Status Code to change (3xx),
*                   the 'PathPtr' parameter needs to be update. A "Location" header will be added automatically in
*                   the response by uC/HTTPs core with the new location.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_ReqRdyPollHook (const  HTTPs_INSTANCE  *p_instance,
                                                  HTTPs_CONN      *p_conn,
                                           const  void            *p_hook_cfg)
{
    /* Nothing to do for this example. */
    return (DEF_YES);
}


/*
*********************************************************************************************************
*                                        HTTPs_RespHdrTxHook()
*
* Description : Called each time the HTTP server is building a response message. Allows for adding header
*               fields to the response message according to the application needs.
*
* Argument(s) : p_instance  Pointer to the HTTPs instance object.
*
*               p_conn      Pointer to the HTTPs connection object.
*
*               p_hook_cfg  Pointer to hook configuration object.
*
* Return(s)   : DEF_YES,    if the header fields are added without running into a error.
*
*               DEF_NO,     otherwise.
*
* Caller(s)   : HTTPsResp_Hdr() via 'p_cfg->OnRespHdrTxHook()'.
*
* Note(s)     : (1) The instance structure MUST NOT be modified.
*
*               (2) The connection structure MUST NOT be modified manually since the response is about to be
*                   transmitted at this point. The only change to the connection structure should be the
*                   addition of header fields for the response message through the function HTTPs_RespHdrGet().
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_RespHdrTxHook (const  HTTPs_INSTANCE  *p_instance,
                                                 HTTPs_CONN      *p_conn,
                                          const  void            *p_hook_cfg)
{
#if (HTTPs_CFG_HDR_TX_EN == DEF_ENABLED)

           HTTP_HDR_BLK  *p_resp_hdr_blk;
    const  HTTPs_CFG     *p_cfg;
           CPU_CHAR      *str_data;
           CPU_SIZE_T     str_len;
           HTTPs_ERR      http_err;


    p_cfg = p_instance->CfgPtr;

    if (p_cfg->HdrTxCfgPtr == DEF_NULL) {
        return (DEF_NO);
    }

    switch (p_conn->StatusCode) {
           case HTTP_STATUS_OK:

                if (p_conn->ReqContentType == HTTP_CONTENT_TYPE_HTML) {

                                                                /* --------------- ADD SERVER HDR FIELD --------------- */
                                                                /* Get and add header block to the connection.          */
                    p_resp_hdr_blk = HTTPs_RespHdrGet(p_instance,
                                                      p_conn,
                                                      HTTP_HDR_FIELD_SERVER,
                                                      HTTP_HDR_VAL_TYPE_STR_DYN,
                                                     &http_err);
                    if (p_resp_hdr_blk == DEF_NULL) {
                        return(DEF_FAIL);
                    }

                    str_data = "uC-HTTP-server";                /* Build Server string value.                           */

                    str_len = Str_Len_N(str_data, p_cfg->HdrTxCfgPtr->DataLenMax);

                                                                /* update hdr blk parameter.                            */
                    Str_Copy_N(p_resp_hdr_blk->ValPtr,
                               str_data,
                               str_len);

                    p_resp_hdr_blk->ValLen = str_len;
                }
                break;


           default:
                break;
    }
#endif

    return (DEF_YES);
}


/*
*********************************************************************************************************
*                                      HTTPs_RespTokenValGetHook()
*
* Description : Called for each ${TEXT_STRING} embedded variable found in a HTML document.
*
* Argument(s) : p_instance   Pointer to the HTTPs instance object.
*
*               p_conn       Pointer to the HTTPs connection object.
*
*               p_hook_cfg   Pointer to hook configuration object.
*
*               p_token      Pointer to the string that contains the value of the HTML embedded token.
*
*               token_len    Length of the embedded token.
*
*               p_val        Pointer to which buffer token value is copied to.
*
*               val_len_max  Maximum buffer length.
*
* Return(s)   : DEF_OK,   if token value copied successfully.
*               DEF_FAIL, otherwise (see Note #3).
*
* Caller(s)   : HTTPs_TokenValGet() via 'p_cfg->HooksPtr->OnRespTokenHook()'.
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

static  CPU_BOOLEAN  HTTPs_RespTokenValGetHook (const  HTTPs_INSTANCE  *p_instance,
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
*                                     HTTPs_RespChunkDataGetHook()
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
* Caller(s)   : HTTPs_RespDataTransferChunked via p_cfg->HooksPtr->OnRespChunkHook.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_RespChunkDataGetHook (const  HTTPs_INSTANCE  *p_instance,
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


    str_cmp = Str_Cmp_N(p_conn->PathPtr, FORM_SUBMIT_URL, p_conn->PathLenMax);
    if (str_cmp == 0) {
                                                                /* Construct JSON for user                              */
        Str_Copy(p_buf, "{\"user\": {\"first name\": \"");      /* Add First Name field.                                */
        p_key_val = p_conn->FormDataListPtr;
        while (p_key_val != DEF_NULL) {
            str_cmp = Str_Cmp_N(p_key_val->KeyPtr, "firstname", p_cfg->FormCfgPtr->KeyLenMax);
            if (str_cmp == 0) {
                Str_Cat_N(p_buf, p_key_val->ValPtr, p_key_val->ValLen);
                break;
            }
            p_key_val = p_key_val->NextPtr;
        }

        Str_Cat(p_buf, "\", \"last name\":\"");                 /* Add Last Name field.                                 */
        p_key_val = p_conn->FormDataListPtr;
        while (p_key_val != DEF_NULL) {
            str_cmp = Str_Cmp_N(p_key_val->KeyPtr, "lastname", p_cfg->FormCfgPtr->KeyLenMax);
            if (str_cmp == 0) {
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
*                                       HTTPs_TransCompleteHook()
*
* Description : Called each time an HTTP Transaction has been completed. Allows the upper application
*               to free some previously allocated memory associated with a request.
*
* Argument(s) : p_instance  Pointer to the HTTPs instance object.
*
*               p_conn      Pointer to the HTTPs connection object.
*
*               p_hook_cfg  Pointer to hook configuration object.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPsConn_Process() via 'p_cfg->HooksPtr->OnTransCompleteHook().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  HTTPs_TransCompleteHook (const  HTTPs_INSTANCE  *p_instance,
                                              HTTPs_CONN      *p_conn,
                                       const  void            *p_hook_cfg)
{
    /* Nothing to do for this example. */
}


/*
*********************************************************************************************************
*                                      HTTPs_InstanceErrFileGet()
*
* Description : (1) Called when the response status code has been changed to an error status code. The
*                   change could be the result of the request processing in the HTTPs_ReqHook()
*                   callback function or the result of an internal error in the uC/HTTPs core.
*
*               (2) This function is intended to set the name of the file which will be sent with the response message.
*                   If no file is set, a default status page will be sent including the status code number and the
*                   reason phrase.
*
* Argument(s) : p_hook_cfg      Pointer to hook configuration object.
*
*               status_code     Status code, number of the response message.
*
*               p_file_str      Pointer to the buffer where the filename string must be copied.
*
*               file_len_max    Maximum length of the filename.
*
*               p_file_type     Pointer to the variable where the file type must be copied:
*                                   HTTPs_FILE_TYPE_FS,           when file is include in a File System.
*                                   HTTPs_FILE_TYPE_STATIC_DATA,  when file is a simple data stream inside a memory
*                                                                 block.
*
*               p_content_type  Content type of the body.
*                               If the data is a File. the content type doesn't need to be set. It will be
*                               set according to the file extension.
*                               If the data is Static Data, the parameter MUST be set.
*
*               p_data          Pointer to the data memory block, if file type is HTTPs_FILE_TYPE_STATIC_DATA.
*                               DEF_NULL,                         otherwise
*
*               p_data_len      Pointer to variable holding
*                               the length of the data,           if file type is HTTPs_FILE_TYPE_STATIC_DATA.
*                               DEF_NULL,                         otherwise
*
* Return(s)   : none.
*
* Caller(s)   : HTTPsResp_PrepareStatusCode() via 'p_cfg->HooksPtr->OnErrFileGetHook().
*
* Note(s)     : (1) If the configured file doesn't exist the instance will transmit the default web page instead,
*                   defined by HTTPs_CFG_HTML_DFLT_ERR_PAGE in http-s_cfg.h
*********************************************************************************************************
*/

static  void  HTTPs_ErrFileGetHook (const  void                   *p_hook_cfg,
                                           HTTP_STATUS_CODE        status_code,
                                           CPU_CHAR               *p_file_str,
                                           CPU_INT32U              file_len_max,
                                           HTTPs_BODY_DATA_TYPE   *p_file_type,
                                           HTTP_CONTENT_TYPE      *p_content_type,
                                           void                  **p_data,
                                           CPU_INT32U             *p_data_len)
{
    switch (status_code) {
        case HTTP_STATUS_NOT_FOUND:
             Str_Copy_N(p_file_str, ERR_404_STR_FILE, file_len_max);
            *p_file_type = HTTPs_BODY_DATA_TYPE_FILE;
             return;


        default:
             Str_Copy_N(p_file_str, HTTPs_HTML_DLFT_ERR_STR_NAME, file_len_max);
            *p_data         = HTTPs_CFG_HTML_DFLT_ERR_PAGE;
            *p_data_len     = HTTPs_HTML_DFLT_ERR_LEN;
            *p_file_type    = HTTPs_BODY_DATA_TYPE_STATIC_DATA;
            *p_content_type = HTTP_CONTENT_TYPE_HTML;
             return;
    }
}


/*
*********************************************************************************************************
*                                           HTTPs_ErrHook()
*
* Description : Called each time an internal error occurs.
*
* Argument(s) : p_instance  Pointer to the HTTPs instance object.
*
*               p_conn      Pointer to the HTTPs connection object.
*
*               p_hook_cfg  Pointer to hook configuration object.
*
*               err         Internal error that occurred:
*                               See HTTPs_ERR declaration in http-s.h for all the error codes possible.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPsConn_ErrInternal() via 'p_cfg->HooksPtr->OnErrHook()'.
*
* Note(s)     : (1) The instance structure is for read-only. It must not be modified at any point in this hook function.
*
*               (2) The following connection attributes can be accessed to analyze the connection:
*
*                   (a) 'ClientAddr'
*                           This connection parameter contains the IP address and port used by the remote client to access the
*                           server instance.
*
*                   (b) 'Method'
*                           HTTPs_METHOD_GET        Get  request
*                           HTTPs_METHOD_POST       Post request
*                           HTTPs_METHOD_HEAD       Head request
*
*                   (c) 'PathPtr'
*                           This is a pointer to the string that contains the name of the file requested.
*
*                   (d) 'HdrCtr'
*                           This parameter is a counter of the number of header field that has been stored.
*
*                   (e) 'HdrListPtr'
*                           This parameter is a pointer to the first header field stored. A linked list is created with
*                           all header field stored.
*
*               (3) In this hook function, only the under-mentioned connection parameters are allowed
*                   to be modified :
*
*                   (a) 'StatusCode'
*                           See HTTPs_STATUS_CODE declaration in http-s.h for all the status code supported.
*
*                   (b) 'PathPtr'
*                           This is a pointer to the string that contains the name of the file requested. You can change
*                           the name of the requested file to send another file instead without error.
*
*                   (c) 'DataPtr'
*                           This is a pointer to the data file or data to transmit. This parameter should be null when calling
*                           this function. If data from memory has to be sent instead of a file, this pointer must be set
*                           to the location of the data.
*
*                   (d) 'RespBodyDataType'
*                           HTTPs_BODY_DATA_TYPE_FILE              Open and transmit a file. Value by default.
*                           HTTPs_BODY_DATA_TYPE_STATIC_DATA       Transmit data from the memory. Must be set by the hook function.
*                           HTTPs_BODY_DATA_TYPE_NONE              No body in response.
*
*                   (e) 'DataLen'
*                           0,                              Default value, will be set when the file is opened.
*                           Data length,                    Must be set to the data length when transmitting data from
*                                                           the memory
*
*                   (f) 'ConnDataPtr'
*                           This is a pointer available for the upper application when memory block must be allocated
*                           to process the connection request. If memory is allocated by the upper application, the memory
*                           space can be deallocated into another hook function.
*********************************************************************************************************
*/

static  void  HTTPs_ErrHook (const  HTTPs_INSTANCE  *p_instance,
                                    HTTPs_CONN      *p_conn,
                             const  void            *p_hook_cfg,
                                    HTTPs_ERR        err)
{
    AppBasic_ErrCtr++;
}


/*
*********************************************************************************************************
*                                         HTTPs_ConnCloseHook()
*
* Description : Called each time a connection is being closed. Allows the upper application to free some
*               previously allocated memory.
*
*
* Argument(s) : p_instance  Pointer to the HTTPs instance object.
*
*               p_conn      Pointer to the HTTPs connection object.
*
*               p_hook_cfg  Pointer to hook configuration object.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPsConn_Close() via 'p_cfg->HooksPtr->OnConnCloseHook()'
*
* Note(s)     : (1) The instance structure is for read-only. It MUST NOT be modified.
*********************************************************************************************************
*/

static  void  HTTPs_ConnCloseHook (const  HTTPs_INSTANCE  *p_instance,
                                          HTTPs_CONN      *p_conn,
                                   const  void            *p_hook_cfg)
{
    /* Nothing to do for this example. */
}

