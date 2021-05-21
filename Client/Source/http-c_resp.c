/*
*********************************************************************************************************
*                                               uC/HTTP
*                                     Hypertext Transfer Protocol
*
*                    Copyright 2004-2021 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                     HTTP CLIENT RESPONSE MODULE
*
* Filename : http-c_resp.c
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
#define    MICRIUM_SOURCE
#define    HTTPc_RESP_MODULE

#include  <Source/net_ascii.h>
#include  <Source/net_util.h>

#include  "http-c_resp.h"
#include  "http-c_conn.h"
#include  "http-c_sock.h"
#ifdef  HTTPc_WEBSOCK_MODULE_EN
#include  "http-c_websock.h"
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static         void               HTTPcResp_ParseStatusLine    (       HTTPc_CONN        *p_conn,
                                                                       HTTPc_ERR         *p_err);

static         HTTP_PROTOCOL_VER  HTTPcResp_ParseProtocolVer   (const  CPU_CHAR          *p_resp_line,
                                                                       CPU_INT32U         str_len);

static         HTTP_STATUS_CODE   HTTPcResp_ParseStatusCode    (const  CPU_CHAR          *p_resp_line,
                                                                       CPU_INT32U         str_len);

static  const  CPU_CHAR          *HTTPcResp_ParseReasonPhrase  (const  CPU_CHAR          *p_resp_line,
                                                                       CPU_INT32U         str_len,
                                                                       HTTP_STATUS_CODE   status_code);

static         void               HTTPcResp_ParseHdr           (       HTTPc_CONN        *p_conn,
                                                                       HTTPc_ERR         *p_err);

static         void               HTTPcResp_Body               (       HTTPc_CONN        *p_conn,
                                                                       HTTPc_ERR         *p_err);

static         void               HTTPcResp_BodyStd            (       HTTPc_CONN        *p_conn,
                                                                       HTTPc_ERR         *p_err);

static         void               HTTPcResp_BodyChunk          (       HTTPc_CONN        *p_conn,
                                                                       HTTPc_ERR         *p_err);


/*
*********************************************************************************************************
*                                             HTTPcResp()
*
* Description : Main HTTP response processing function.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                          Response processing operation successful.
*                               HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED   More data required to continue processing.
*                               HTTPc_ERR_CONN_INVALID_STATE            Invalid Connection state.
*
*                               ------------ RETURNED BY HTTPcSock_ConnDataRx() ------------
*                               See HTTPcSock_ConnDataRx() for additional return error codes.
*
*                               ------------ RETURNED BY HTTPcResp_ParseStatusLine() ------------
*                               See HTTPcResp_ParseStatusLine() for additional return error codes.
*
*                               ------------ RETURNED BY HTTPcResp_ParseHdr() ------------
*                               See HTTPcResp_ParseHdr() for additional return error codes.
*
*                               ------------ RETURNED BY HTTPcResp_Body() ------------
*                               See HTTPcResp_Body() for additional return error codes
*
* Return(s)   : DEF_YES, if response processing is complete successfully.
*               DEF_NO,  otherwise.
*
* Caller(s)   : HTTPcTask_Handler().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcResp (HTTPc_CONN  *p_conn,
                        HTTPc_ERR   *p_err)
{
    CPU_BOOLEAN        rx_data;
    CPU_BOOLEAN        process;
    CPU_BOOLEAN        sock_rdy_rd;
    CPU_BOOLEAN        sock_rdy_err;
    CPU_BOOLEAN        done;


    done    = DEF_NO;
    process = DEF_NO;
    rx_data = DEF_BIT_IS_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_RX_MORE_DATA);

    if (rx_data == DEF_YES) {

        sock_rdy_rd  = DEF_BIT_IS_SET(p_conn->SockFlags, HTTPc_FLAG_SOCK_RDY_RD);
        sock_rdy_err = DEF_BIT_IS_SET(p_conn->SockFlags, HTTPc_FLAG_SOCK_RDY_ERR);

        if((sock_rdy_rd  == DEF_YES) ||
           (sock_rdy_err == DEF_YES)) {

            process = HTTPcSock_ConnDataRx(p_conn, DEF_NULL, p_err);
            switch (*p_err) {
                case HTTPc_ERR_NONE:
                case HTTPc_ERR_CONN_SOCK_RX:
                     break;


                default:
                     goto exit;
            }
        }
    }

    switch (p_conn->State) {
        case HTTPc_CONN_STATE_RESP_INIT:
             DEF_BIT_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_RX_MORE_DATA);
             if (process == DEF_YES) {
                 DEF_BIT_CLR(p_conn->RespFlags, HTTPc_FLAG_RESP_RX_MORE_DATA);
                 p_conn->State = HTTPc_CONN_STATE_RESP_STATUS_LINE;
             }
            *p_err = HTTPc_ERR_NONE;
             break;


        case HTTPc_CONN_STATE_RESP_STATUS_LINE:
             HTTPcResp_ParseStatusLine(p_conn, p_err);
             switch (*p_err) {
                 case HTTPc_ERR_NONE:
                      DEF_BIT_CLR(p_conn->RespFlags, HTTPc_FLAG_RESP_RX_MORE_DATA);
                      p_conn->State = HTTPc_CONN_STATE_RESP_HDR;
                      break;


                 case HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED:
                      DEF_BIT_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_RX_MORE_DATA);
                      break;


                 default:
                      break;
             }
             break;


        case HTTPc_CONN_STATE_RESP_HDR:
             HTTPcResp_ParseHdr(p_conn, p_err);
             switch (*p_err) {
                 case HTTPc_ERR_NONE:
                      DEF_BIT_CLR(p_conn->RespFlags, HTTPc_FLAG_RESP_RX_MORE_DATA);
                      p_conn->State = HTTPc_CONN_STATE_RESP_BODY;
                      break;


                 case HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED:
                      DEF_BIT_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_RX_MORE_DATA);
                      break;


                 default:
                      break;
             }
             break;


        case HTTPc_CONN_STATE_RESP_BODY:
        case HTTPc_CONN_STATE_RESP_BODY_CHUNK_SIZE:
        case HTTPc_CONN_STATE_RESP_BODY_CHUNK_DATA:
        case HTTPc_CONN_STATE_RESP_BODY_CHUNK_CRLF:
        case HTTPc_CONN_STATE_RESP_BODY_CHUNK_LAST:
             HTTPcResp_Body(p_conn, p_err);
             switch (*p_err) {
                case HTTPc_ERR_NONE:
                     DEF_BIT_CLR(p_conn->RespFlags, HTTPc_FLAG_RESP_RX_MORE_DATA);
                     p_conn->State = HTTPc_CONN_STATE_RESP_COMPLETED;
                     break;


                case HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED:
                     DEF_BIT_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_RX_MORE_DATA);
                     break;


                default:
                     break;
             }
             break;


        case HTTPc_CONN_STATE_RESP_COMPLETED:
             done          = DEF_YES;
             p_conn->State = HTTPc_CONN_STATE_COMPLETED;
            *p_err         = HTTPc_ERR_NONE;
             break;


        default:
            *p_err = HTTPc_ERR_CONN_INVALID_STATE;
             break;
    }


exit:
    return(done);
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
*                                      HTTPcResp_ParseStatusLine()
*
* Description : (1) HTTP Response status line processing :
*                   (a) Parse HTTP protocol version received.
*                   (b) Parse Status Code received.
*                   (c) Parse Reason phrase received.
*
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                          Status Line successfully parsed.
*                               HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED
*                               HTTPc_ERR_RESP_INVALID_FORMAT
*                               HTTPc_ERR_RESP_INVALID_PROTOCOL_VER
*                               HTTPc_ERR_RESP_INVALID_STATUS_CODE
*                               HTTPc_ERR_RESP_INVALID_REASON_PHRASE
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcResp().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  HTTPcResp_ParseStatusLine (HTTPc_CONN  *p_conn,
                                         HTTPc_ERR   *p_err)
{
    HTTPc_REQ   *p_req;
    HTTPc_RESP  *p_resp;
    CPU_CHAR    *p_resp_line_start;
    CPU_CHAR    *p_resp_line_end;
    CPU_SIZE_T   len;


    p_req  = p_conn->ReqListHeadPtr;
    p_resp = p_req->RespPtr;

                                                                /* Clean response object.                               */
    p_resp->ContentLen      = 0;
    p_resp->ContentType     = HTTP_CONTENT_TYPE_NONE;
    p_resp->ProtocolVer     = HTTP_PROTOCOL_VER_UNKNOWN;
    p_resp->ReasonPhrasePtr = DEF_NULL;
    p_resp->StatusCode      = HTTP_STATUS_UNKNOWN;

    len = p_conn->RxDataLenRem;
    if (len <= sizeof(HTTP_STR_METHOD_GET)) {
       *p_err = HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED;
        return;
    }

                                                                /* ------------ RETRIEVE RESP STATUS LINE ------------- */
                                                                /* Search beginning and remove white spaces before Resp.*/
    p_resp_line_start = HTTP_StrGraphSrchFirst(p_conn->RxBufPtr, len);
    if (p_resp_line_start == DEF_NULL) {
       *p_err = HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED;
        return;
    }

                                                                /* Find end of resp status line.                        */
    len -= (p_resp_line_start - p_conn->BufPtr);
    p_resp_line_end = Str_Str_N(p_resp_line_start, STR_CR_LF, len);
    if (p_resp_line_end == DEF_NULL) {
       *p_err = HTTPc_ERR_RESP_FORMAT_INVALID;
        return;
    }

    len = p_resp_line_end - p_resp_line_start;
                                                                /* ---------- PARSE RESPONSE PROTOCOL VERSION --------- */
    p_resp->ProtocolVer = HTTPcResp_ParseProtocolVer(p_resp_line_start, len);
    switch (p_resp->ProtocolVer) {
        case HTTP_PROTOCOL_VER_1_1:
             break;


        case HTTP_PROTOCOL_VER_1_0:
        case HTTP_PROTOCOL_VER_0_9:
        case HTTP_PROTOCOL_VER_UNKNOWN:
        default:
            *p_err = HTTPc_ERR_RESP_PROTOCOL_VER_INVALID;
             return;
    }

                                                                /* ------------ FIND SPACE IN STATUS LINE ------------- */
    p_resp_line_start = Str_Char_N(p_resp_line_start, len, ASCII_CHAR_SPACE);
    if (p_resp_line_start == DEF_NULL) {
       *p_err = HTTPc_ERR_RESP_FORMAT_INVALID;
        return;
    }
    p_resp_line_start++;
    len = p_resp_line_end - p_resp_line_start;
                                                                /* ------------ PARSE RESPONSE STATUS CODE ------------ */
    p_resp->StatusCode = HTTPcResp_ParseStatusCode(p_resp_line_start, len);
    switch (p_resp->StatusCode) {
        case HTTP_STATUS_CONTINUE:
        case HTTP_STATUS_SWITCHING_PROTOCOLS:
        case HTTP_STATUS_PROCESSING:
        case HTTP_STATUS_EARLY_HINTS:
        case HTTP_STATUS_OK:
        case HTTP_STATUS_CREATED:
        case HTTP_STATUS_ACCEPTED:
        case HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION:
        case HTTP_STATUS_NO_CONTENT:
        case HTTP_STATUS_RESET_CONTENT:
        case HTTP_STATUS_PARTIAL_CONTENT:
        case HTTP_STATUS_MULTI_STATUS:
        case HTTP_STATUS_ALREADY_REPORTED:
        case HTTP_STATUS_IM_USED:
        case HTTP_STATUS_MULTIPLE_CHOICES:
        case HTTP_STATUS_MOVED_PERMANENTLY:
        case HTTP_STATUS_FOUND:
        case HTTP_STATUS_SEE_OTHER:
        case HTTP_STATUS_NOT_MODIFIED:
        case HTTP_STATUS_USE_PROXY:
        case HTTP_STATUS_SWITCH_PROXY:
        case HTTP_STATUS_TEMPORARY_REDIRECT:
        case HTTP_STATUS_PERMANENT_REDIRECT:
        case HTTP_STATUS_BAD_REQUEST:
        case HTTP_STATUS_UNAUTHORIZED:
        case HTTP_STATUS_FORBIDDEN:
        case HTTP_STATUS_NOT_FOUND:
        case HTTP_STATUS_METHOD_NOT_ALLOWED:
        case HTTP_STATUS_NOT_ACCEPTABLE:             /* With the Accept Req Hdr */
        case HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED:
        case HTTP_STATUS_REQUEST_TIMEOUT:
        case HTTP_STATUS_CONFLICT:
        case HTTP_STATUS_GONE:
        case HTTP_STATUS_LENGTH_REQUIRED:
        case HTTP_STATUS_PRECONDITION_FAILED:
        case HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE:
        case HTTP_STATUS_REQUEST_URI_TOO_LONG:
        case HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE:
        case HTTP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE:
        case HTTP_STATUS_EXPECTATION_FAILED:
        case HTTP_STATUS_MISDIRECTED_REQUEST:
        case HTTP_STATUS_UNPROCESSABLE_ENTITY:
        case HTTP_STATUS_LOCKED:
        case HTTP_STATUS_FAILED_DEPENDENCY:
        case HTTP_STATUS_UPGRADE_REQUIRED:
        case HTTP_STATUS_PRECONDITION_REQUIRED:
        case HTTP_STATUS_TOO_MANY_REQUESTS:
        case HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE:
        case HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS:
        case HTTP_STATUS_INTERNAL_SERVER_ERR:
        case HTTP_STATUS_NOT_IMPLEMENTED:
        case HTTP_STATUS_BAD_GATEWAY:
        case HTTP_STATUS_SERVICE_UNAVAILABLE:
        case HTTP_STATUS_GATEWAY_TIMEOUT:
        case HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED:
        case HTTP_STATUS_VARIANT_ALSO_NEGOTIATES:
        case HTTP_STATUS_INSUFFICIENT_STORAGE:
        case HTTP_STATUS_LOOP_DETECTED:
        case HTTP_STATUS_NOT_EXTENDED:
        case HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED:
             break;


        case HTTP_STATUS_UNKNOWN:
        default:
            *p_err = HTTPc_ERR_RESP_STATUS_CODE_INVALID;
             return;
    }

                                                                /* ------------ FIND SPACE IN STATUS LINE ------------- */
    p_resp_line_start = Str_Char_N(p_resp_line_start, len, ASCII_CHAR_SPACE);
    if (p_resp_line_start == DEF_NULL) {
       *p_err = HTTPc_ERR_RESP_FORMAT_INVALID;
        return;
    }
    p_resp_line_start++;
    len = p_resp_line_end - p_resp_line_start;

                                                                /* ----------- PARSE RESPONSE REASON PHRASE ----------- */
    p_resp->ReasonPhrasePtr = HTTPcResp_ParseReasonPhrase(p_resp_line_start, len, p_resp->StatusCode);
    if (p_resp->ReasonPhrasePtr == DEF_NULL) {
       *p_err = HTTPc_ERR_RESP_REASON_PHRASE_INVALID;
        return;
    }

                                                                /* -------------- UPDATE CONN RX PARAMS --------------- */
    p_resp_line_end      +=  STR_CR_LF_LEN;
    p_conn->RxDataLenRem -= (p_resp_line_end - p_conn->RxBufPtr);
    p_conn->RxBufPtr      =  p_resp_line_end;

   *p_err = HTTPc_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     HTTPcResp_ParseProtocolVer()
*
* Description : Find HTTP Protocol version in the HTTP response received.
*
* Argument(s) : p_resp_line     Pointer to Response status line received.
*
*               str_len         Length of the response status line.
*
* Return(s)   : HTTP protocol version.
*
* Caller(s)   : HTTPcResp_ParseStatusLine().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  HTTP_PROTOCOL_VER  HTTPcResp_ParseProtocolVer (const  CPU_CHAR    *p_resp_line,
                                                              CPU_INT32U   str_len)
{
    HTTP_PROTOCOL_VER  ver;

    ver = (HTTP_PROTOCOL_VER)HTTP_Dict_KeyGet(HTTP_Dict_ProtocolVer,
                                              HTTP_Dict_ProtocolVerSize,
                                              p_resp_line,
                                              DEF_YES,
                                              str_len);
    switch (ver) {
        case HTTP_PROTOCOL_VER_0_9:
        case HTTP_PROTOCOL_VER_1_0:
        case HTTP_PROTOCOL_VER_1_1:
             break;

        default:
            return (HTTP_PROTOCOL_VER_UNKNOWN);
    }

    return ((HTTP_PROTOCOL_VER)ver);
}



/*
*********************************************************************************************************
*                                      HTTPcResp_ParseStatusCode()
*
* Description : Find HTTP Status Code in the HTTP Response received.
*
* Argument(s) : p_resp_line     Pointer to the HTTP Response status line received.
*
*               str_len         Length of the Response status line.
*
* Return(s)   : HTTP status code.
*
* Caller(s)   : HTTPcResp_ParseStatusLine().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  HTTP_STATUS_CODE  HTTPcResp_ParseStatusCode (const  CPU_CHAR    *p_resp_line,
                                                            CPU_INT32U   str_len)
{
    HTTP_STATUS_CODE  status_code;

    status_code = (HTTP_STATUS_CODE)HTTP_Dict_KeyGet(HTTP_Dict_StatusCode,
                                                     HTTP_Dict_StatusCodeSize,
                                                     p_resp_line,
                                                     DEF_YES,
                                                     str_len);

    if (status_code >= HTTP_STATUS_UNKNOWN) {
        return (HTTP_STATUS_UNKNOWN);
    }

    return (status_code);
}



/*
*********************************************************************************************************
*                                     HTTPcResp_ParseReasonPhrase()
*
* Description : Find HTTP Reason Phrase in the HTTP Response received.
*
* Argument(s) : p_resp_line     Pointer to the HTTP Response status line received.
*
*               str_len         Length of the Response status line.
*
*               status_code     Status code number received.
*
* Return(s)   : Pointer to the Reason phrase associated with the status code saved in the HTTP libraries.
*
* Caller(s)   : HTTPcResp_ParseStatusLine().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  const  CPU_CHAR  *HTTPcResp_ParseReasonPhrase (const  CPU_CHAR          *p_resp_line,
                                                              CPU_INT32U         str_len,
                                                              HTTP_STATUS_CODE   status_code)
{
    HTTP_DICT  *p_entry;


    p_entry = HTTP_Dict_EntryGet(            HTTP_Dict_ReasonPhrase,
                                             HTTP_Dict_ReasonPhraseSize,
                                 (CPU_INT32U)status_code);

    return (p_entry->StrPtr);
}

/*
*********************************************************************************************************
*                                         HTTPcResp_ParseHdr()
*
* Description : Parse all the headers received in the HTTP response.
*
* Argument(s) : p_conn      Pointer to the current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                          Headers parsing successful.
*                               HTTPc_ERR_RESP_HDR_MALFORMED            Malformed header received.
*                               HTTPc_ERR_RESP_HDR_INVALID              Invalid arguments with header.
*                               HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED   More data needed.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcResp().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  HTTPcResp_ParseHdr (HTTPc_CONN  *p_conn,
                                  HTTPc_ERR   *p_err)
{
    HTTPc_REQ                     *p_req;
    HTTPc_RESP                    *p_resp;
    CPU_CHAR                      *p_field_start;
    CPU_CHAR                      *p_field_end;
    CPU_CHAR                      *p_val;
    HTTP_HDR_FIELD                 hdr_field;
    HTTP_HDR_FIELD_CONN_VAL        hdr_con_val;
    HTTP_CONTENT_TYPE              content_type;
    CPU_INT32U                     content_len;
    HTTP_HDR_FIELD_TRANSFER_TYPE   transfer_type;
    CPU_INT16U                     len;
#if (HTTPc_CFG_HDR_RX_EN == DEF_ENABLED)
    HTTPc_CONN_OBJ                *p_conn_const;
    HTTPc_REQ_OBJ                 *p_req_const;
    HTTP_DICT                     *p_entry;
#endif
#ifdef  HTTPc_WEBSOCK_MODULE_EN
    HTTPc_WEBSOCK_REQ             *p_ws_req;
    CPU_BOOLEAN                    is_matched;
    CPU_INT32U                     version;
#endif

    p_req   = p_conn->ReqListHeadPtr;
    p_resp  = p_req->RespPtr;

    p_field_start = p_conn->RxBufPtr;
#ifdef  HTTPc_WEBSOCK_MODULE_EN
    p_ws_req      = p_req->WebSockPtr;
#endif
    while (DEF_TRUE) {

        p_field_end = Str_Str_N(p_field_start,                  /* Find end of header field.                            */
                                STR_CR_LF,
                                p_conn->RxDataLenRem);

        if ((p_field_end != DEF_NULL     ) &&                   /* If the field and value are present.                  */
            (p_field_end  > p_field_start)) {

            len = p_field_end - p_field_start;

            hdr_field = (HTTP_HDR_FIELD)HTTP_Dict_KeyGet(HTTP_Dict_HdrField,
                                                         HTTP_Dict_HdrFieldSize,
                                                         p_field_start,
                                                         DEF_NO,
                                                         len);

            switch (hdr_field) {
                case HTTP_HDR_FIELD_CONN:
                                                                /* Get field val beginning.                             */
                     p_val = HTTP_HdrParseFieldValueGet(p_field_start,
                                                        HTTP_STR_HDR_FIELD_CONN_LEN,
                                                        p_field_end,
                                                       &len);
                     if (p_val == DEF_NULL) {
                        *p_err = HTTPc_ERR_RESP_HDR_MALFORMED;
                         goto exit;
                     }

                     len = p_field_end - p_val;

                     hdr_con_val = (HTTP_HDR_FIELD_CONN_VAL)HTTP_Dict_KeyGet(HTTP_Dict_HdrFieldConnVal,
                                                                             HTTP_Dict_HdrFieldConnValSize,
                                                                             p_val,
                                                                             DEF_NO,
                                                                             len);
                     switch (hdr_con_val) {
                         case HTTP_HDR_FIELD_CONN_CLOSE:
                              DEF_BIT_SET(p_conn->Flags, HTTPc_FLAG_CONN_TO_CLOSE);
                              p_conn->CloseStatus = HTTPc_CONN_CLOSE_STATUS_NO_PERSISTENT;
                              break;

                         case HTTP_HDR_FIELD_CONN_PERSISTENT:
                              break;
#ifdef  HTTPc_WEBSOCK_MODULE_EN
                         case HTTP_HDR_FIELD_CONN_UPGRADE:
                              DEF_BIT_SET(p_ws_req->Flags, HTTPc_FLAG_WEBSOCK_REQ_CONN_UPGRADE);
                              break;
#endif
                         case HTTP_HDR_FIELD_CONN_UNKNOWN:
                         default:
                             *p_err = HTTPc_ERR_RESP_HDR_INVALID;
                              goto exit;
                     }
                     break;


                case HTTP_HDR_FIELD_CONTENT_TYPE:
                                                                /* Get field val beginning.                             */
                     p_val = HTTP_HdrParseFieldValueGet(p_field_start,
                                                        HTTP_STR_HDR_FIELD_CONTENT_TYPE_LEN,
                                                        p_field_end,
                                                       &len);
                     if (p_val == DEF_NULL) {
                        *p_err = HTTPc_ERR_RESP_HDR_MALFORMED;
                         goto exit;
                     }

                     len = p_field_end - p_val;

                     content_type = (HTTP_CONTENT_TYPE)HTTP_Dict_KeyGet(HTTP_Dict_ContentType,
                                                                        HTTP_Dict_ContentTypeSize,
                                                                        p_val,
                                                                        DEF_NO,
                                                                        len);
                     if (content_type == HTTP_CONTENT_TYPE_UNKNOWN) {
                        *p_err = HTTPc_ERR_RESP_CONTENT_TYPE_INVALID;
                         goto exit;
                     }

                     p_resp->ContentType = content_type;
                     break;


                case HTTP_HDR_FIELD_CONTENT_LEN:
                                                                /* Get field val beginning.                             */
                     p_val = HTTP_HdrParseFieldValueGet(p_field_start,
                                                        HTTP_STR_HDR_FIELD_CONTENT_LEN_LEN,
                                                        p_field_end,
                                                       &len);
                     if (p_val == DEF_NULL) {
                        *p_err = HTTPc_ERR_RESP_HDR_MALFORMED;
                         goto exit;
                     }

                     content_len = Str_ParseNbr_Int32U(p_val, 0, DEF_NBR_BASE_DEC);

                     if (content_len <= 0u) {
                        *p_err = HTTPc_ERR_RESP_CONTENT_LEN_INVALID;
                         goto exit;
                     }

                     p_resp->ContentLen = content_len;
                     break;


                case HTTP_HDR_FIELD_TRANSFER_ENCODING:
                                                                /* Get field val beginning.                             */
                     p_val = HTTP_HdrParseFieldValueGet(p_field_start,
                                                        HTTP_STR_HDR_FIELD_TRANSFER_ENCODING_LEN,
                                                        p_field_end,
                                                       &len);
                     if (p_val == DEF_NULL) {
                        *p_err = HTTPc_ERR_RESP_HDR_MALFORMED;
                         goto exit;
                     }

                     len = p_field_end - p_val;

                     transfer_type = (HTTP_HDR_FIELD_TRANSFER_TYPE)HTTP_Dict_KeyGet(HTTP_Dict_HdrFieldTransferTypeVal,
                                                                                    HTTP_Dict_HdrFieldTransferTypeValSize,
                                                                                    p_val,
                                                                                    DEF_NO,
                                                                                    len);
                     if (transfer_type != HTTP_HDR_FIELD_TRANSFER_TYPE_CHUNKED) {
                        *p_err = HTTPc_ERR_RESP_HDR_INVALID;
                         goto exit;
                     }
                     DEF_BIT_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_BODY_CHUNK_TRANSFER);
                     break;

#ifdef  HTTPc_WEBSOCK_MODULE_EN
                case HTTP_HDR_FIELD_UPGRADE:
                                                                /* Get field val beginning.                             */
                     p_val = HTTP_HdrParseFieldValueGet(p_field_start,
                                                        HTTP_STR_HDR_FIELD_UPGRADE_LEN,
                                                        p_field_end,
                                                       &len);
                     if (p_val == DEF_NULL) {
                        *p_err = HTTPc_ERR_RESP_HDR_MALFORMED;
                         goto exit;
                     }

                     len = p_field_end - p_val;

                     hdr_con_val = (HTTP_HDR_FIELD_CONN_VAL)HTTP_Dict_KeyGet(HTTP_Dict_HdrFieldUpgradeVal,
                                                                             HTTP_Dict_HdrFieldUpgradeValSize,
                                                                             p_val,
                                                                             DEF_NO,
                                                                             len);
                     switch (hdr_con_val) {
                         case HTTP_HDR_FIELD_UPGRADE_WEBSOCKET:
                              DEF_BIT_SET(p_ws_req->Flags, HTTPc_FLAG_WEBSOCK_REQ_UPGRADE_WEBSOCKET);
                              break;

                         default:
                             *p_err = HTTPc_ERR_RESP_HDR_INVALID;
                              goto exit;
                     }
                     break;


              case HTTP_HDR_FIELD_WEBSOCKET_ACCEPT:
                                                                 /* Get field val beginning.                            */
                   p_val = HTTP_HdrParseFieldValueGet(p_field_start,
                                                      HTTP_STR_HDR_FIELD_WEBSOCKET_ACCEPT_LEN,
                                                      p_field_end,
                                                     &len);
                   if (p_val == DEF_NULL) {
                      *p_err = HTTPc_ERR_RESP_HDR_MALFORMED;
                       goto exit;
                   }

                   len = p_field_end - p_val;
                                                                /* Validate the websocket field.                        */

                   is_matched = Mem_Cmp(p_ws_req->Accept, p_val, HTTPc_WEBSOCK_KEY_HASH_ENCODED_LEN - 1);

                   if (is_matched == DEF_YES) {
                       DEF_BIT_SET(p_ws_req->Flags, HTTPc_FLAG_WEBSOCK_REQ_ACCEPT_VALIDATED);
                   }
                   break;


              case HTTP_HDR_FIELD_WEBSOCKET_VERSION:
                                                               /* Get field val beginning.                              */
                   p_val = HTTP_HdrParseFieldValueGet( p_field_start,
                                                       HTTP_STR_HDR_FIELD_WEBSOCKET_VERSION_LEN,
                                                       p_field_end,
                                                      &len);
                   if (p_val == DEF_NULL) {
                      *p_err = HTTPc_ERR_RESP_HDR_MALFORMED;
                       goto exit;
                   }
                   version = Str_ParseNbr_Int32U (p_val,DEF_NULL,DEF_NBR_BASE_DEC);

                   if (version == HTTPc_WEBSOCK_PROTOCOL_VERSION_13) {
                       DEF_BIT_SET(p_ws_req->Flags, HTTPc_FLAG_WEBSOCK_REQ_VERSION_VALIDATED);
                   }
                   break;
#endif

                default:
#if (HTTPc_CFG_HDR_RX_EN == DEF_ENABLED)
                     p_conn_const = (HTTPc_CONN_OBJ *)p_conn;
                     p_req_const  = (HTTPc_REQ_OBJ *)p_req;

                     p_entry = HTTP_Dict_EntryGet(HTTP_Dict_HdrField,
                                                  HTTP_Dict_HdrFieldSize,
                                                  hdr_field);
                     if (p_entry != DEF_NULL) {

                                                            /* Get field val beginning.                             */
                         p_val = HTTP_HdrParseFieldValueGet(p_field_start,
                                                            p_entry->StrLen,
                                                            p_field_end,
                                                           &len);
                         if (p_val == DEF_NULL) {
                            *p_err = HTTPc_ERR_RESP_HDR_MALFORMED;
                             goto exit;
                         }

                         len = p_field_end - p_val;

                         if (p_req->OnHdrRx != DEF_NULL) {
                             p_req->OnHdrRx(p_conn_const, p_req_const, hdr_field, p_val, len);
                         }
                     }
#endif
                     break;
            }

                                                                /* --------------- UPDATE RX CONN PARAM --------------- */
            p_field_end          += STR_CR_LF_LEN;
            p_conn->RxDataLenRem -= p_field_end - p_field_start;
            p_field_start         = p_field_end;
            p_conn->RxBufPtr      = p_field_start;
           *p_err = HTTPc_ERR_NONE;

        } else if (p_field_end == p_field_start) {              /* All field processed.                                 */
            p_conn->RxBufPtr     += STR_CR_LF_LEN;
            p_conn->RxDataLenRem -= STR_CR_LF_LEN;
           *p_err = HTTPc_ERR_NONE;
            goto exit;

        } else {                                                /* More data req'd to complete processing.              */
           *p_err = HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED;
            goto exit;
        }

    }


exit:
    return;
}


/*
*********************************************************************************************************
*                                           HTTPcResp_Body()
*
* Description : Process Response body received.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                           ------------ RETURNED BY  HTTPcResp_BodyChunk() ------------
*                           See HTTPcResp_BodyChunk() for additional return error codes.
*
*                           ------------ RETURNED BY HTTPcResp_BodyStd() ------------
*                           See HTTPcResp_BodyStd() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcResp().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  HTTPcResp_Body (HTTPc_CONN  *p_conn,
                              HTTPc_ERR   *p_err)
{
    CPU_BOOLEAN   chunk_transfer;


    chunk_transfer = DEF_BIT_IS_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_BODY_CHUNK_TRANSFER);

    if (chunk_transfer == DEF_YES) {
        HTTPcResp_BodyChunk(p_conn, p_err);
    } else {
        HTTPcResp_BodyStd(p_conn, p_err);
    }
}



/*
*********************************************************************************************************
*                                          HTTPcResp_BodyStd()
*
* Description : Process HTTP response body received with the Content-Length header.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                          Body successfully rx and process.
*                               HTTPc_ERR_RESP_RX_DATA_LEN_INVALID      Invalid data length received.
*                               HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED   More data to received.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcResp_Body().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  HTTPcResp_BodyStd (HTTPc_CONN  *p_conn,
                                 HTTPc_ERR   *p_err)
{
    HTTPc_CONN_OBJ  *p_conn_const;
    HTTPc_REQ       *p_req;
    HTTPc_REQ_OBJ   *p_req_const;
    HTTPc_RESP      *p_resp;
    CPU_CHAR        *p_buf;
    CPU_INT32U       content_len;
    CPU_INT32U       content_len_rem;
    CPU_INT32U       content_len_chunk;
    CPU_INT32U       data_len_read;
    CPU_BOOLEAN      last_chunk;


    p_conn_const     = (HTTPc_CONN_OBJ *)p_conn;
    p_req            =  p_conn->ReqListHeadPtr;
    p_req_const      = (HTTPc_REQ_OBJ *)p_req;
    p_resp           =  p_req->RespPtr;
    content_len      =  p_resp->ContentLen;
    p_buf            =  p_conn->RxBufPtr;
    content_len_rem  =  content_len - p_conn->RxDataLen;

    if (content_len_rem >= p_conn->RxDataLenRem) {
        content_len_chunk = p_conn->RxDataLenRem;
    } else {
        content_len_chunk = content_len_rem;
    }
    p_conn->RxDataLen += content_len_chunk;

    last_chunk = DEF_NO;
    if (p_conn->RxDataLen == content_len) {
        last_chunk = DEF_YES;
       *p_err = HTTPc_ERR_NONE;
    } else {
       *p_err = HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED;
    }


    if (p_req->OnBodyRx != DEF_NULL) {
        data_len_read = p_req->OnBodyRx( p_conn_const,
                                         p_req_const,
                                         p_resp->ContentType,
                                         p_buf,
                                         content_len_chunk,
                                         last_chunk);
        p_conn->RxDataLen    -= content_len_chunk - data_len_read;
        p_conn->RxBufPtr     += data_len_read;
        p_conn->RxDataLenRem -= data_len_read;

    } else {
        p_conn->RxBufPtr     += content_len_chunk;
        p_conn->RxDataLenRem -= content_len_chunk;
    }
}


/*
*********************************************************************************************************
*                                         HTTPcResp_BodyChunk()
*
* Description : Process Response body received with the chunked transfer encoding.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                          Body successfully received.
*                               HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED   More data to received.
*                               HTTPc_ERR_RESP_CHUNK_INVALID            Invalid chunk received.
*                               HTTPc_ERR_CONN_INVALID_STATE            Invalid Connection state.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcResp_Body().
*
* Note(s)     : (1) Chunk Extensions located after chunk size are not supported.
*********************************************************************************************************
*/

static  void  HTTPcResp_BodyChunk (HTTPc_CONN  *p_conn,
                                   HTTPc_ERR   *p_err)
{
    HTTPc_CONN_OBJ    *p_conn_const;
    HTTPc_REQ         *p_req;
    HTTPc_REQ_OBJ     *p_req_const;
    HTTPc_RESP        *p_resp;
    CPU_CHAR          *p_size_start;
    CPU_CHAR          *p_size_end;
    CPU_CHAR          *p_chunk_start;
    CPU_CHAR          *p_str;
    CPU_INT32U         chunk_len;
    CPU_INT32U         buf_len;
    CPU_INT32U         data_len;
    CPU_INT32U         data_len_read = 0;


    p_conn_const = (HTTPc_CONN_OBJ *)p_conn;
    p_req        =  p_conn->ReqListHeadPtr;
    p_req_const  = (HTTPc_REQ_OBJ *)p_req;
    p_resp       =  p_req->RespPtr;

    buf_len      = p_conn->BufLen - p_conn->RxDataLenRem;

    while (DEF_TRUE) {
        switch (p_conn->State) {
            case HTTPc_CONN_STATE_RESP_BODY:
            case HTTPc_CONN_STATE_RESP_BODY_CHUNK_SIZE:
                 p_size_start = p_conn->RxBufPtr;

                 p_size_end = Str_Str_N(p_size_start,           /* Find end of size of chunk.                           */
                                        STR_CR_LF,
                                        p_conn->RxDataLenRem);
                 if (p_size_end == DEF_NULL) {
                    *p_err = HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED;
                     goto exit;
                 }
                                                                /* Convert the hexadecimal string to 32 bits integer.  */
                 chunk_len = Str_ParseNbr_Int32U(p_size_start, &p_str, 16);
                 if (p_str == p_size_start) {
                    *p_err = HTTPc_ERR_RESP_CHUNK_INVALID;
                     goto exit;
                 }

                 p_conn->RxDataLen = chunk_len;

                 p_size_end           += STR_CR_LF_LEN;
                 p_conn->RxDataLenRem -= p_size_end - p_size_start;
                 p_conn->RxBufPtr      = p_size_end;

                 if (chunk_len == 0) {
                     p_conn->State = HTTPc_CONN_STATE_RESP_BODY_CHUNK_LAST;
                 } else {
                     p_conn->State = HTTPc_CONN_STATE_RESP_BODY_CHUNK_DATA;
                 }
                 break;


            case HTTPc_CONN_STATE_RESP_BODY_CHUNK_DATA:
                 p_chunk_start = p_conn->RxBufPtr;
                                                                /* If data in the buffer is less than the chunk length. */
                 if (p_conn->RxDataLenRem < p_conn->RxDataLen) {
                     if (buf_len > 0) {                         /* Buffer not full, go get more data.                   */
                        *p_err = HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED;
                         goto exit;
                     } else {
                         data_len = p_conn->RxDataLenRem;       /* Part of chunk received in buffer.                    */
                     }
                 } else {
                     data_len = p_conn->RxDataLen;              /* End of chunk received in buffer.                     */
                 }

                 if (p_req->OnBodyRx != DEF_NULL) {
                                                                /* Pass data rx to application.                         */
                     data_len_read = p_req->OnBodyRx(p_conn_const,
                                                     p_req_const,
                                                     p_resp->ContentType,
                                                     p_conn->RxBufPtr,
                                                     data_len,
                                                     DEF_NO);
                 } else {
                     data_len_read = data_len;
                 }

                 p_conn->RxDataLenRem -= data_len_read;
                 p_conn->RxBufPtr      = p_chunk_start + data_len_read;
                 p_conn->RxDataLen    -= data_len_read;

                 if (p_conn->RxDataLenRem >= p_conn->RxDataLen) {
                     if (data_len_read >= data_len) {
                         p_conn->State  = HTTPc_CONN_STATE_RESP_BODY_CHUNK_CRLF;
                     }
                 } else {
                    *p_err = HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED;
                     goto exit;
                 }
                 break;


            case HTTPc_CONN_STATE_RESP_BODY_CHUNK_CRLF:
                 if (p_conn->RxDataLenRem < STR_CR_LF_LEN) {
                    *p_err = HTTPc_ERR_TRANS_RX_MORE_DATA_REQUIRED;
                     goto exit;
                 }

                 p_str = Str_Str_N(p_conn->RxBufPtr,            /* Find end of header field.                            */
                                   STR_CR_LF,
                                   p_conn->RxDataLenRem);
                 if (p_str == DEF_NULL) {
                    *p_err = HTTPc_ERR_RESP_CHUNK_INVALID;
                     goto exit;
                 }

                 p_conn->RxDataLenRem -= STR_CR_LF_LEN;
                 p_conn->RxBufPtr     += STR_CR_LF_LEN;
                 p_conn->State         = HTTPc_CONN_STATE_RESP_BODY_CHUNK_SIZE;
                 break;


            case HTTPc_CONN_STATE_RESP_BODY_CHUNK_LAST:
                 if (p_req->OnBodyRx != DEF_NULL) {
                     (void)p_req->OnBodyRx(p_conn_const,
                                           p_req_const,
                                           p_resp->ContentType,
                                           DEF_NULL,
                                           0,
                                           DEF_YES);
                 }
                 p_conn->State = HTTPc_CONN_STATE_RESP_COMPLETED;
                *p_err         = HTTPc_ERR_NONE;
                 goto exit;


            default:
                *p_err = HTTPc_ERR_CONN_INVALID_STATE;
                 goto exit;
        }
    }

exit:
    return;
}
