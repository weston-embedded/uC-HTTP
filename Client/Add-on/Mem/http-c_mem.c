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
*                                  HTTP CLIENT MEMORY LIBRARY MODULE
*
* Filename : http-c_mem.c
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
#define    HTTPs_MEM_MODULE

#include  <http-c_cfg.h>
#include  "http-c_mem.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef  HTTPc_MEM_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

MEM_DYN_POOL  HTTPcMem_PoolConn;
MEM_DYN_POOL  HTTPcMem_PoolBuf;
MEM_DYN_POOL  HTTPcMem_PoolReq;
MEM_DYN_POOL  HTTPcMem_PoolResp;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       HTTPcMem_PoolInit()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_err       $$$$ Add description for 'p_err'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPcMem_PoolInit(const  HTTPc_MEM_CFG  *p_cfg,
                               HTTPc_MEM_ERR  *p_err)
{
    LIB_ERR      err_lib;


                                                                /* ----------------- CREATE CONN POOL ----------------- */
    Mem_DynPoolCreate(DEF_NULL,
                     &HTTPcMem_PoolConn,
                      DEF_NULL,
                      sizeof(HTTPc_CONN_OBJ),
                      sizeof(CPU_SIZE_T),
                      1,
                      p_cfg->ConnNbrMax,
                     &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = HTTPc_MEM_ERR_POOL_INIT_CONN;
        goto exit;
    }

                                                                /* ---------------- CREATE BUFFER POOL ---------------- */
    Mem_DynPoolCreate(DEF_NULL,
                     &HTTPcMem_PoolBuf,
                      DEF_NULL,
                      p_cfg->BufSize,
                      sizeof(CPU_SIZE_T),
                      1,
                      p_cfg->ConnNbrMax,
                     &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = HTTPc_MEM_ERR_POOL_INIT_BUF;
        goto exit;
    }

                                                                /* ----------------- CREATE REQ POOL ------------------ */
    Mem_DynPoolCreate(DEF_NULL,
                     &HTTPcMem_PoolReq,
                      DEF_NULL,
                      sizeof(HTTPc_REQ_OBJ),
                      sizeof(CPU_SIZE_T),
                      1,
                      p_cfg->ReqNbrMax,
                     &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = HTTPc_MEM_ERR_POOL_INIT_REQ;
        goto exit;
    }

                                                                /* ----------------- CREATE RESP POOL ----------------- */
    Mem_DynPoolCreate(DEF_NULL,
                     &HTTPcMem_PoolResp,
                      DEF_NULL,
                      sizeof(HTTPc_RESP_OBJ),
                      sizeof(CPU_SIZE_T),
                      1,
                      p_cfg->ReqNbrMax,
                     &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = HTTPc_MEM_ERR_POOL_INIT_RESP;
        goto exit;
    }

   *p_err = HTTPc_MEM_ERR_NONE;


exit:
   return;
}


/*
*********************************************************************************************************
*                                          HTTPcMem_ConnGet()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_err       $$$$ Add description for 'p_err'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

HTTPc_CONN_OBJ  *HTTPcMem_ConnGet (HTTPc_MEM_ERR  *p_err)
{
    HTTPc_CONN_OBJ  *p_conn;
    LIB_ERR          err_lib;


    p_conn = (HTTPc_CONN_OBJ *)Mem_DynPoolBlkGet(&HTTPcMem_PoolConn,
                                                 &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;


        case LIB_MEM_ERR_POOL_EMPTY:
             p_conn = DEF_NULL;
            *p_err  = HTTPc_MEM_ERR_POOL_CONN_EMPTY;
             goto exit;


        default:
            p_conn = DEF_NULL;
           *p_err  = HTTPc_MEM_ERR_POOL_CONN_FAULT;
            goto exit;
    }

   *p_err = HTTPc_MEM_ERR_NONE;


exit:
    return (p_conn);
}


/*
*********************************************************************************************************
*                                        HTTPcMem_ConnRelease()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_err       $$$$ Add description for 'p_err'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPcMem_ConnRelease (HTTPc_CONN_OBJ  *p_conn,
                            HTTPc_MEM_ERR   *p_err)
{
    LIB_ERR  err_lib;


    Mem_DynPoolBlkFree(&HTTPcMem_PoolConn,
                        p_conn,
                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = HTTPc_MEM_ERR_POOL_CONN_FREE;
        goto exit;
    }

   *p_err = HTTPc_MEM_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                          HTTPcMem_BufGet()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_err       $$$$ Add description for 'p_err'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_CHAR  *HTTPcMem_BufGet (HTTPc_MEM_ERR  *p_err)
{
    CPU_CHAR  *p_buf;
    LIB_ERR    err_lib;


    p_buf = (CPU_CHAR *)Mem_DynPoolBlkGet(&HTTPcMem_PoolBuf,
                                          &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;


        case LIB_MEM_ERR_POOL_EMPTY:
             p_buf = DEF_NULL;
            *p_err = HTTPc_MEM_ERR_POOL_BUF_EMPTY;
             goto exit;


        default:
            p_buf = DEF_NULL;
           *p_err = HTTPc_MEM_ERR_POOL_BUF_FAULT;
            goto exit;
    }

   *p_err = HTTPc_MEM_ERR_NONE;


exit:
    return (p_buf);
}


/*
*********************************************************************************************************
*                                        HTTPcMem_BufRelease()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_buf
*
*               p_err       $$$$ Add description for 'p_err'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPcMem_BufRelease (CPU_CHAR       *p_buf,
                           HTTPc_MEM_ERR  *p_err)
{
    LIB_ERR  err_lib;


    Mem_DynPoolBlkFree(&HTTPcMem_PoolBuf,
                        p_buf,
                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = HTTPc_MEM_ERR_POOL_BUF_FREE;
        goto exit;
    }

   *p_err = HTTPc_MEM_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                          HTTPcMem_ReqGet()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_err       $$$$ Add description for 'p_err'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

HTTPc_REQ_OBJ  *HTTPcMem_ReqGet (HTTPc_MEM_ERR  *p_err)
{
    HTTPc_REQ_OBJ  *p_req;
    LIB_ERR         err_lib;


    p_req = (HTTPc_REQ_OBJ *)Mem_DynPoolBlkGet(&HTTPcMem_PoolReq,
                                               &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;


        case LIB_MEM_ERR_POOL_EMPTY:
             p_req = DEF_NULL;
            *p_err = HTTPc_MEM_ERR_POOL_REQ_EMPTY;
             goto exit;


        default:
            p_req = DEF_NULL;
           *p_err = HTTPc_MEM_ERR_POOL_REQ_FAULT;
            goto exit;
    }

   *p_err = HTTPc_MEM_ERR_NONE;


exit:
    return (p_req);
}


/*
*********************************************************************************************************
*                                        HTTPcMem_ReqRelease()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_buf
*
*               p_err       $$$$ Add description for 'p_err'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPcMem_ReqRelease (HTTPc_REQ_OBJ  *p_req,
                           HTTPc_MEM_ERR  *p_err)
{
    LIB_ERR  err_lib;


    Mem_DynPoolBlkFree(&HTTPcMem_PoolReq,
                        p_req,
                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = HTTPc_MEM_ERR_POOL_BUF_FREE;
        goto exit;
    }

   *p_err = HTTPc_MEM_ERR_NONE;


exit:
    return;
}

/*
*********************************************************************************************************
*                                          HTTPcMem_RespGet()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_err       $$$$ Add description for 'p_err'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

HTTPc_RESP_OBJ  *HTTPcMem_RespGet (HTTPc_MEM_ERR  *p_err)
{
    HTTPc_RESP_OBJ  *p_resp;
    LIB_ERR          err_lib;


    p_resp = (HTTPc_RESP_OBJ *)Mem_DynPoolBlkGet(&HTTPcMem_PoolResp,
                                                 &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;


        case LIB_MEM_ERR_POOL_EMPTY:
             p_resp = DEF_NULL;
            *p_err  = HTTPc_MEM_ERR_POOL_REQ_EMPTY;
             goto exit;


        default:
            p_resp = DEF_NULL;
           *p_err  = HTTPc_MEM_ERR_POOL_REQ_FAULT;
            goto exit;
    }

   *p_err = HTTPc_MEM_ERR_NONE;


exit:
    return (p_resp);
}


/*
*********************************************************************************************************
*                                        HTTPcMem_RespRelease()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_resp
*
*               p_err       $$$$ Add description for 'p_err'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPcMem_RespRelease (HTTPc_RESP_OBJ  *p_resp,
                            HTTPc_MEM_ERR   *p_err)
{
    LIB_ERR  err_lib;


    Mem_DynPoolBlkFree(&HTTPcMem_PoolResp,
                        p_resp,
                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = HTTPc_MEM_ERR_POOL_BUF_FREE;
        goto exit;
    }

   *p_err = HTTPc_MEM_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* HTTPc_MEM_MODULE_EN       */
