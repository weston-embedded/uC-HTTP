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
* Filename : http-c_app.h
* Version  : V3.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  HTTPc_APP_MODULE_PRESENT
#define  HTTPc_APP_MODULE_PRESENT

/*
*********************************************************************************************************
*                                             FS MODULE
*
* Note(s): If the uC/FS is present in the project, you can enable it for the example application.
*********************************************************************************************************
*/

#define  HTTPc_APP_FS_MODULE_PRESENT   DEF_NO


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <Client/Source/http-c.h>

#if (HTTPc_APP_FS_MODULE_PRESENT == DEF_YES)
#include  <fs_file.h>
#include  <fs_type.h>
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef HTTPc_APP_MODULE
#define  HTTPc_APP_EXT
#else
#define  HTTPc_APP_EXT  extern
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTP_SERVER_HOSTNAME                            "httpbin.org"

#define  HTTPc_APP_CFG_CONN_NBR_MAX                       5u
#define  HTTPc_APP_CFG_REQ_NBR_MAX                        5u
#define  HTTPc_APP_CFG_CONN_BUF_SIZE                    512u

#define  HTTPc_APP_CFG_QUERY_STR_NBR_MAX                  6u
#define  HTTPc_APP_CFG_QUERY_STR_KEY_LEN_MAX             20u
#define  HTTPc_APP_CFG_QUERY_STR_VAL_LEN_MAX             50u

#define  HTTPc_APP_CFG_HDR_NBR_MAX                        6u
#define  HTTPc_APP_CFG_HDR_VAL_LEN_MAX                  100u

#define  HTTPc_APP_CFG_FORM_BUF_SIZE                    256u
#define  HTTPc_APP_CFG_FORM_FIELD_NBR_MAX                10u
#define  HTTPc_APP_CFG_FORM_FIELD_KEY_LEN_MAX           100u
#define  HTTPc_APP_CFG_FORM_FIELD_VAL_LEN_MAX           200u
#define  HTTPc_APP_CFG_FORM_MULTIPART_NAME_LEN_MAX      100u
#define  HTTPc_APP_CFG_FORM_MULTIPART_FILENAME_LEN_MAX  100u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          CONNECTION DATA TYPE
*********************************************************************************************************
*/

typedef  struct  httpc_app_conn_data {
    CPU_BOOLEAN   Close;
} HTTPc_APP_CONN_DATA;


/*
*********************************************************************************************************
*                                           REQUEST DATA TYPE
*********************************************************************************************************
*/

typedef  struct  httpc_app_req_data {
    CPU_BOOLEAN   Done;
    CPU_INT08U    QueryStrIx;
    CPU_INT16U    FormIx;
#if (HTTPc_APP_FS_MODULE_PRESENT == DEF_YES)
    FS_FILE      *FilePtr;
#endif
} HTTPc_APP_REQ_DATA;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

HTTPc_APP_EXT  HTTPc_APP_REQ_DATA    HTTPcApp_Data[HTTPc_APP_CFG_REQ_NBR_MAX];

HTTPc_APP_EXT  CPU_CHAR              HTTPcApp_Buf[65535];

HTTPc_APP_EXT  HTTPc_CONN_OBJ        HTTPcApp_ConnTbl[HTTPc_APP_CFG_CONN_NBR_MAX];
HTTPc_APP_EXT  HTTPc_REQ_OBJ         HTTPcApp_ReqTbl[HTTPc_APP_CFG_REQ_NBR_MAX];
HTTPc_APP_EXT  HTTPc_RESP_OBJ        HTTPcApp_RespTbl[HTTPc_APP_CFG_REQ_NBR_MAX];

HTTPc_APP_EXT  CPU_CHAR              HTTPcApp_ConnBufTbl[HTTPc_APP_CFG_CONN_NBR_MAX][HTTPc_APP_CFG_CONN_BUF_SIZE];

HTTPc_APP_EXT  HTTPc_KEY_VAL         HTTPcApp_ReqQueryStrTbl[HTTPc_APP_CFG_QUERY_STR_NBR_MAX];
HTTPc_APP_EXT  CPU_CHAR              HTTPcApp_ReqQueryStrKeyTbl[HTTPc_APP_CFG_QUERY_STR_NBR_MAX][HTTPc_APP_CFG_QUERY_STR_KEY_LEN_MAX];
HTTPc_APP_EXT  CPU_CHAR              HTTPcApp_ReqQueryStrValTbl[HTTPc_APP_CFG_QUERY_STR_NBR_MAX][HTTPc_APP_CFG_QUERY_STR_VAL_LEN_MAX];

HTTPc_APP_EXT  HTTPc_HDR             HTTPcApp_ReqHdrTbl[HTTPc_APP_CFG_HDR_NBR_MAX];
HTTPc_APP_EXT  CPU_CHAR              HTTPcApp_ReqHdrValStrTbl[HTTPc_APP_CFG_HDR_NBR_MAX][HTTPc_APP_CFG_HDR_VAL_LEN_MAX];
HTTPc_APP_EXT  HTTPc_HDR             HTTPcApp_RespHdrTbl[HTTPc_APP_CFG_HDR_NBR_MAX];
HTTPc_APP_EXT  CPU_CHAR              HTTPcApp_RespHdrValStrTbl[HTTPc_APP_CFG_HDR_NBR_MAX][HTTPc_APP_CFG_HDR_VAL_LEN_MAX];

HTTPc_APP_EXT  CPU_CHAR              HTTPcApp_FormBuf[HTTPc_APP_CFG_FORM_BUF_SIZE];
HTTPc_APP_EXT  HTTPc_FORM_TBL_FIELD  HTTPcApp_FormTbl[HTTPc_APP_CFG_FORM_FIELD_NBR_MAX];

HTTPc_APP_EXT  HTTPc_KEY_VAL         HTTPcApp_FormKeyValTbl[HTTPc_APP_CFG_FORM_FIELD_NBR_MAX];
HTTPc_APP_EXT  HTTPc_KEY_VAL_EXT     HTTPcApp_FormKeyValExtTbl[HTTPc_APP_CFG_FORM_FIELD_NBR_MAX];
HTTPc_APP_EXT  HTTPc_MULTIPART_FILE  HTTPcApp_FormMultipartFileTbl[HTTPc_APP_CFG_FORM_FIELD_NBR_MAX];
HTTPc_APP_EXT  CPU_CHAR              HTTPcApp_FormKeyStrTbl[2*HTTPc_APP_CFG_FORM_FIELD_NBR_MAX][HTTPc_APP_CFG_FORM_FIELD_KEY_LEN_MAX];
HTTPc_APP_EXT  CPU_CHAR              HTTPcApp_FormValStrTbl[2*HTTPc_APP_CFG_FORM_FIELD_NBR_MAX][HTTPc_APP_CFG_FORM_FIELD_VAL_LEN_MAX];
HTTPc_APP_EXT  CPU_CHAR              HTTPcApp_FormMultipartNameStrTbl[HTTPc_APP_CFG_FORM_FIELD_NBR_MAX][HTTPc_APP_CFG_FORM_MULTIPART_NAME_LEN_MAX];
HTTPc_APP_EXT  CPU_CHAR              HTTPcApp_FormMultipartFileNameStrTbl[HTTPc_APP_CFG_FORM_FIELD_NBR_MAX][HTTPc_APP_CFG_FORM_MULTIPART_FILENAME_LEN_MAX];


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcApp_Init                 (void);

CPU_BOOLEAN  HTTPcApp_ReqSendGet           (void);

#if (HTTPc_CFG_FORM_EN == DEF_ENABLED)
CPU_BOOLEAN  HTTPcApp_ReqSendPost          (void);

CPU_BOOLEAN  HTTPcApp_ReqSendAppForm       (void);

CPU_BOOLEAN  HTTPcApp_ReqSendMultipartForm (void);
#endif

CPU_BOOLEAN  HTTPcAPP_ReqSendPut           (void);

CPU_BOOLEAN  HTTPcApp_PersistentConn       (void);

CPU_BOOLEAN  HTTPcApp_MultiConn            (void);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* HTTPc_APP_MODULE_PRESENT  */

