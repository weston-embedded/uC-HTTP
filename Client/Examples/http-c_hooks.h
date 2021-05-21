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
* Filename : http-c_hooks.h
* Version  : V3.01.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  HTTPc_HOOKS_MODULE_PRESENT
#define  HTTPc_HOOKS_MODULE_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <Client/Source/http-c.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcApp_ReqQueryStrHook       (HTTPc_CONN_OBJ            *p_conn,
                                             HTTPc_REQ_OBJ             *p_req,
                                             HTTPc_KEY_VAL            **p_key_val);

CPU_BOOLEAN  HTTPcApp_ReqHdrHook            (HTTPc_CONN_OBJ            *p_conn,
                                             HTTPc_REQ_OBJ             *p_req,
                                             HTTPc_HDR                **p_hdr);

CPU_BOOLEAN  HTTPcApp_ReqBodyHook           (HTTPc_CONN_OBJ            *p_conn,
                                             HTTPc_REQ_OBJ             *p_req,
                                             void                     **p_data,
                                             CPU_CHAR                  *p_buf,
                                             CPU_INT16U                 buf_len,
                                             CPU_INT16U                *p_data_len);

void         HTTPcApp_RespHdrHook           (HTTPc_CONN_OBJ            *p_conn,
                                             HTTPc_REQ_OBJ             *p_req,
                                             HTTP_HDR_FIELD             hdr_field,
                                             CPU_CHAR                  *p_hdr_val,
                                             CPU_INT16U                 val_len);

CPU_INT32U   HTTPcApp_RespBodyHook          (HTTPc_CONN_OBJ            *p_conn,
                                             HTTPc_REQ_OBJ             *p_req,
                                             HTTP_CONTENT_TYPE          content_type,
                                             void                      *p_data,
                                             CPU_INT16U                 data_len,
                                             CPU_BOOLEAN                last_chunk);

CPU_BOOLEAN  HTTPcApp_FormMultipartHook     (HTTPc_CONN_OBJ            *p_conn,
                                             HTTPc_REQ_OBJ             *p_req,
                                             HTTPc_KEY_VAL_EXT         *p_key_val_obj,
                                             CPU_CHAR                  *p_buf,
                                             CPU_INT16U                 buf_len,
                                             CPU_INT16U                *p_len_wr);

#if (HTTPc_APP_FS_MODULE_PRESENT == DEF_YES)
CPU_BOOLEAN  HTTPcApp_FormMultipartFileHook (HTTPc_CONN_OBJ            *p_conn,
                                             HTTPc_REQ_OBJ             *p_req,
                                             HTTPc_MULTIPART_FILE      *p_file_obj,
                                             CPU_CHAR                  *p_buf,
                                             CPU_INT16U                 buf_len,
                                             CPU_INT16U                *p_len_wr);
#endif

#ifdef HTTPc_TASK_MODULE_EN
void         HTTPcApp_ConnConnectCallback   (HTTPc_CONN_OBJ            *p_conn,
                                             CPU_BOOLEAN                open_status);

void         HTTPcApp_ConnCloseCallback     (HTTPc_CONN_OBJ            *p_conn,
                                             HTTPc_CONN_CLOSE_STATUS    close_status,
                                             HTTPc_ERR                  err);

void         HTTPcApp_TransDoneCallback     (HTTPc_CONN_OBJ            *p_conn,
                                             HTTPc_REQ_OBJ             *p_req,
                                             HTTPc_RESP_OBJ            *p_resp,
                                             CPU_BOOLEAN                status);

void         HTTPcApp_TransErrCallback      (HTTPc_CONN_OBJ            *p_conn,
                                             HTTPc_REQ_OBJ             *p_req,
                                             HTTPc_ERR                  err_code);
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     TRACE / DEBUG CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                       0
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                      1
#endif

#ifndef  TRACE_LEVEL_DEBUG
#define  TRACE_LEVEL_DEBUG                     2
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                       2
#endif

#define  HTTPc_APP_TRACE_LEVEL                TRACE_LEVEL_INFO
#define  HTTPc_APP_TRACE                      printf

#define  HTTPc_APP_TRACE_INFO(x)              ((HTTPc_APP_TRACE_LEVEL >= TRACE_LEVEL_INFO)  ? (void)(HTTPc_APP_TRACE x) : (void)0)
#define  HTTPc_APP_TRACE_DBG(x)               ((HTTPc_APP_TRACE_LEVEL >= TRACE_LEVEL_DEBUG) ? (void)(HTTPc_APP_TRACE x) : (void)0)

#define  HTTPc_APP_TRACE_DEBUG(x)             HTTPc_APP_TRACE_DBG(x)

/*
*********************************************************************************************************
*********************************************************************************************************
*                                              MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* HTTPc_HOOKS_MODULE_PRESENT  */
