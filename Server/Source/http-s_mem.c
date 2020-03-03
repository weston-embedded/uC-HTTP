/*
*********************************************************************************************************
*                                               uC/HTTP
*                                     Hypertext Transfer Protocol
*
*                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
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
*                                     HTTP SERVER MEMORY LIBRARY
*
* Filename : http-s_mem.c
* Version  : V3.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    HTTPs_MEM_MODULE
#include  "http-s.h"
#include  "http-s_mem.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPs_CFG_POOLS_INIT_NBR       1


/*
*********************************************************************************************************
*                                             FORM DEFINES
*********************************************************************************************************
*/

#define  HTTPs_FORM_BOUNDARY_STR_LEN_MAX                     72u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  MEM_SEG         *HTTPs_MemSegPtr;
static  MEM_DYN_POOL    *HTTPs_InstancesPoolPtr;

#if (HTTPs_CFG_DBG_INFO_EN == DEF_ENABLED)
static  HTTPs_INSTANCE  *HTTPsMem_InstancesListPtr;
#endif

static  CPU_INT32U       HTTPsMem_InstanceNbrNext;

/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#if (HTTPs_CFG_HDR_RX_EN == DEF_ENABLED)
static  void  HTTPsMem_ReqHdrPoolInit  (HTTPs_INSTANCE  *p_instance,
                                        HTTPs_ERR       *p_err);
#endif

#if (HTTPs_CFG_HDR_TX_EN == DEF_ENABLED)
static  void  HTTPsMem_RespHdrPoolInit (HTTPs_INSTANCE  *p_instance,
                                        HTTPs_ERR       *p_err);
#endif


/*
*********************************************************************************************************
*                                       HTTPsMem_InstanceInit()
*
* Description : Initialize HTTP server instance list variables.
*
* Argument(s) : p_mem_seg   Pointer to the memory segment to use to allocate necessary objects.
*                           Set to DEF_NULL to allocate objects on the HEAP configured in uC/LIB.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPs_ERR_NONE
*                               HTTPs_ERR_INSTANCE_INIT_POOL_REM_MEM
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_Init().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : (1) In order for HTTP server initialization complete to be able to be verified from
*                   interrupt service routines, following variables MUST be accessed exclusively in
*                   critical sections during initialization:
*
*                   (a) HTTPs_InstanceNbrMax
*                   (b) HTTPs_InstanceNbrNext
*                   (c) HTTPs_InstancesListPtr
*********************************************************************************************************
*/

void  HTTPsMem_InstanceInit (MEM_SEG    *p_mem_seg,
                             HTTPs_ERR  *p_err)
{
    LIB_ERR  err_lib;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();                                       /* See Note #1.                                         */
    HTTPsMem_InstanceNbrNext      = 0u;
#if (HTTPs_CFG_DBG_INFO_EN == DEF_ENABLED)
    HTTPsMem_InstancesListPtr     = DEF_NULL;
#endif
    CPU_CRITICAL_EXIT();

    HTTPs_MemSegPtr = p_mem_seg;

    HTTPs_InstancesPoolPtr = (MEM_DYN_POOL *)Mem_SegAlloc("HTTPs Instances Pool Object",
                                                          p_mem_seg,
                                                          sizeof(MEM_DYN_POOL),
                                                         &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
        goto exit;
    }


    Mem_Clr(HTTPs_InstancesPoolPtr, sizeof(MEM_DYN_POOL));


    Mem_DynPoolCreate("HTTPs Instance Pools",
                       HTTPs_InstancesPoolPtr,
                       p_mem_seg,
                       sizeof(HTTPs_INSTANCE),
                       sizeof(CPU_ALIGN),
                       HTTPs_CFG_POOLS_INIT_NBR,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                      &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
        goto exit;
    }

   *p_err = HTTPs_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                      HTTPsMem_InstanceTaskInit()
*
* Description : Initialize the Memory segment required by the HTTP server instance task.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           HTTPs_ERR_NONE
*                           HTTPs_ERR_OS_INIT_REM_MEM
*                           HTTPs_ERR_OS_OBJ_CREATE
*
* Return(s)   : Pointer to HTTP server instance task object.
*
* Caller(s)   : HTTPsTask_InstanceObjInit().
*
* Note(s)     : none.
*********************************************************************************************************
*/

HTTPs_OS_TASK_OBJ  *HTTPsMem_InstanceTaskInit (HTTPs_ERR  *p_err)
{
#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
           CPU_SIZE_T          octets_rem;
           CPU_SIZE_T          octets_reqd;
#endif
           HTTPs_OS_TASK_OBJ  *p_os_task_obj;
           LIB_ERR             err_lib;


    p_os_task_obj = DEF_NULL;

#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
                                                                /* -------------- VALIDATE REM MEM AVAIL -------------- */
    octets_reqd  = sizeof(HTTPs_OS_TASK_OBJ);

    octets_rem   = Mem_SegRemSizeGet(HTTPs_MemSegPtr,
                                     sizeof(CPU_SIZE_T),
                                     DEF_NULL,
                                    &err_lib);
    if (octets_rem < octets_reqd) {
       *p_err = HTTPs_ERR_TASK_INIT_REM_MEM;
        goto exit;
    }
#endif

                                                                /* ----- ACQUIRE TASK HTTPs_OS_TASK_OBJ MEM SPACE ----- */
    p_os_task_obj = (HTTPs_OS_TASK_OBJ *)Mem_SegAlloc("HTTPs Task object",
                                                       HTTPs_MemSegPtr,
                                                       sizeof(HTTPs_OS_TASK_OBJ),
                                                      &err_lib);
    if (p_os_task_obj == DEF_NULL) {
       *p_err = HTTPs_ERR_TASK_OBJ_CREATE;
        goto exit;
    }

   *p_err = HTTPs_ERR_NONE;


exit:
    return (p_os_task_obj);
}


/*
*********************************************************************************************************
*                                        HTTPsMem_InstanceGet()
*
* Description : (1) Get an HTTP server instance:
*
*                   (a) Validate next HTTP server instance available
*                   (b) Acquire HTTP server instance block
*                   (c) Update instances list
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*               -----   Argument validated in HTTPs_InstanceStart().
*
*                           HTTPs_ERR_NONE                      HTTP server instance      successfully acquired.
*                           HTTPs_ERR_INSTANCES_POOL_EMPTY      HTTP server instances pool is empty.
*                           HTTPs_ERR_INSTANCES_POOL_ERR        HTTP server instance  NOT successfully acquired.
*
* Return(s)   : Pointer to the HTTP server instance acquired, if NO error(s).
*
*               NULL pointer,                                 otherwise.
*
* Caller(s)   : HTTPs_InstanceInit().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : (2) In order for HTTP server initialization complete to be able to be verified from
*                   interrupt service routines,'HTTPs_InstanceNbrNext' MUST be accessed exclusively
*                   in critical sections during initialization.
*
*               (3) Next available HTTP server instance MUST be configured PRIOR to initializing
*                   the specific HTTP server instance so that the initialized HTTP server instance
*                   is valid.
*********************************************************************************************************
*/

HTTPs_INSTANCE  *HTTPsMem_InstanceGet (HTTPs_ERR  *p_err)
{
    HTTPs_INSTANCE  *p_instance;
    CPU_INT32U       instance_nbr;
    LIB_ERR          err_lib;
#if (HTTPs_CFG_DBG_INFO_EN == DEF_ENABLED)
    HTTPs_INSTANCE  *p_instance_item;
#endif
    CPU_SR_ALLOC();


                                                                /* ---------- VALIDATE HTTPS INSTANCE AVAIL ----------- */
    CPU_CRITICAL_ENTER();                                       /* See Note #2.                                         */
    instance_nbr = HTTPsMem_InstanceNbrNext;
    CPU_CRITICAL_EXIT();


    CPU_CRITICAL_ENTER();                                       /* See Note #2.                                         */
    HTTPsMem_InstanceNbrNext++;                                /* Inc to next avail HTTPs instance (see Note #2).      */
    CPU_CRITICAL_EXIT();


                                                                /* --------------- ACQUIRE INSTANCE BLK --------------- */
    p_instance = (HTTPs_INSTANCE *)Mem_DynPoolBlkGet(HTTPs_InstancesPoolPtr,
                                                    &err_lib);
    if (p_instance == DEF_NULL) {
        CPU_CRITICAL_ENTER();                                   /* See Note #2.                                         */
        HTTPsMem_InstanceNbrNext--;                            /* Dec next avail HTTPs instance (see Note #3).         */
        CPU_CRITICAL_EXIT();
       *p_err = HTTPs_ERR_INIT_POOL_INSTANCE;
        return(DEF_NULL);
    }

    Mem_Clr(p_instance, sizeof(HTTPs_INSTANCE));


#if (HTTPs_CFG_DBG_INFO_EN == DEF_ENABLED)
                                                                /* -------------- UPDATE INSTANCES LIST --------------- */
    p_instance->InstanceNextPtr = DEF_NULL;
    if (HTTPsMem_InstancesListPtr == DEF_NULL) {                /* If            instances list is  empty ...           */
        p_instance->InstancePrevPtr = DEF_NULL;
        HTTPsMem_InstancesListPtr  = p_instance;                /* .. first      instance  item is this cur instance.   */

    } else {                                                    /* If            instances list not empty ...           */
        p_instance_item = HTTPsMem_InstancesListPtr;
                                                                /* ... find last instance  item.                        */
        while (p_instance_item->InstanceNextPtr != DEF_NULL) {
            p_instance_item = p_instance_item->InstanceNextPtr;
        }

        p_instance->InstancePrevPtr      = p_instance_item;     /* Prev instance of the cur instance is the last item.  */
        p_instance_item->InstanceNextPtr = p_instance;          /* Next instance item is the current instance.          */
    }
#endif

    p_instance->ID = instance_nbr;

   *p_err = HTTPs_ERR_NONE;

    return (p_instance);
}


/*
*********************************************************************************************************
*                                      HTTPsMem_InstanceRelease()
*
* Description : (1) Release HTTP server instance:
*
*                   (a) Update  instances list
*                   (b) Release instance block
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*               -----   Argument validated in HTTPs_InstanceStart().
*
*                           HTTPs_ERR_NONE                          Instance          successfully released.
*                           HTTPs_ERR_INSTANCES_POOL_BLK_FREE       Instance block already         released.
*                           HTTPs_ERR_INSTANCES_POOL_ERR            Instance NOT      successfully released.
*                           HTTPs_ERR_INSTANCE_LIST_NULL_ITEM       Instance list NOT successfully released.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_InstanceInit().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : (2) In order for HTTP server initialization complete to be able to be verified from
*                   interrupt service routines, following variables MUST be accessed exclusively in
*                   critical sections during initialization.
*
*                   (a) HTTPs_InstanceNbrMax
*                   (b) HTTPs_InstanceNbrNext
*                   (c) HTTPs_InstancesListPtr
*
*               (3) All opened sockets MUST be closed before calling this function.
*********************************************************************************************************
*/

void  HTTPsMem_InstanceRelease (HTTPs_INSTANCE  *p_instance)
{
#if (HTTPs_CFG_DBG_INFO_EN == DEF_ENABLED)
    HTTPs_INSTANCE  *p_instance_item;
#endif
    LIB_ERR          err_lib;
    CPU_SR_ALLOC();


#if (HTTPs_CFG_DBG_INFO_EN == DEF_ENABLED)
                                                                /* -------------- UPDATE INSTANCES LIST --------------- */
    CPU_CRITICAL_ENTER();                                       /* See Note #2.                                         */
    p_instance_item = HTTPsMem_InstancesListPtr;
    CPU_CRITICAL_EXIT();

    if (p_instance == p_instance_item) {                        /* If instance is the first item in instances list.     */
        if (p_instance->InstanceNextPtr == DEF_NULL) {          /* If no other item in instances list.                  */
            CPU_CRITICAL_ENTER();                               /* See Note #2.                                         */
            HTTPsMem_InstancesListPtr = DEF_NULL;
            CPU_CRITICAL_EXIT();

        } else {
            CPU_CRITICAL_ENTER();                               /* See Note #2.                                         */
            HTTPsMem_InstancesListPtr                  = p_instance->InstanceNextPtr;
            HTTPsMem_InstancesListPtr->InstancePrevPtr = DEF_NULL;
            CPU_CRITICAL_EXIT();
        }

    } else {
                                                                /* Find cur instance in instances list.                 */
        while ((p_instance_item != p_instance) &&
               (p_instance_item != DEF_NULL)  ) {
                p_instance_item  = p_instance_item->InstanceNextPtr;
        }

        if (p_instance_item == DEF_NULL) {                      /* Should not be null.                                  */
            return;
        }

        if (p_instance_item->InstancePrevPtr == DEF_NULL) {     /* Should not be null.                                  */
            return;
        }

        p_instance_item                  = p_instance->InstancePrevPtr; /* Set next item of the prev list item ...      */
        p_instance_item->InstanceNextPtr = p_instance->InstanceNextPtr; /* ... to the next element to the current item. */
    }

#else
   (void)p_instance;                                            /* Prevent 'variable unused' compiler warnings.         */
#endif

                                                                /* -------------- RELEASE INSTANCE BLOCK -------------- */
    Mem_DynPoolBlkFree(HTTPs_InstancesPoolPtr, p_instance, &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return;
    }

    CPU_CRITICAL_ENTER();                                       /* See Note #2.                                         */
    HTTPsMem_InstanceNbrNext--;
    CPU_CRITICAL_EXIT();

}


/*
*********************************************************************************************************
*                                       HTTPsMem_ConnPoolInit()
*
* Description :(1) Initialize the HTTP server instance connection pool :
*
*                   (a) Validate remaining memory available
*                   (b) Create connections pool
*                   (c) Create buffers     pool
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*               -----   Argument validated in HTTPs_InstanceStart().
*
*                           HTTPs_ERR_NONE                          Connection pool successfully initialized.
*                           HTTPs_ERR_INSTANCE_INIT_POOL_REM_MEM    Not enough remaining memory.
*                           HTTPs_ERR_INSTANCE_INIT_POOL_CONN       Initialization of connections pool failed.
*                           HTTPs_ERR_INSTANCE_INIT_POOL_BUF        Initialization of buffers     pool failed.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_InstanceInit().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : (2) Validate that the remaining memory available on the heap before creating the connection &
*                   buffer pools is required to avoid wasting heap in the case there is enough memory to create
*                   the connection pool but not enough to create the buffer pool.
*********************************************************************************************************
*/

void  HTTPsMem_ConnPoolInit (HTTPs_INSTANCE  *p_instance,
                             HTTPs_ERR       *p_err)
{
#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
           CPU_SIZE_T   octets_rem;
#endif
#if (HTTPs_CFG_TOKEN_PARSE_EN == DEF_ENABLED)
           CPU_INT16U   val_len;
#endif
    const  HTTPs_CFG   *p_cfg;
           CPU_SIZE_T   octets_reqd;
           CPU_SIZE_T   pool_size_max;
           CPU_INT32U   path_len_max;
           LIB_ERR      err_lib;


    p_cfg = p_instance->CfgPtr;

    switch (p_cfg->FS_Type) {
        case HTTPs_FS_TYPE_NONE:
             path_len_max = ((HTTPs_CFG_FS_NONE *)p_cfg->FS_CfgPtr)->PathLenMax;
             break;

        case HTTPs_FS_TYPE_STATIC:
        case HTTPs_FS_TYPE_DYN:
#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED)
             path_len_max = p_instance->FS_PathLenMax;
#else
            *p_err = HTTPs_ERR_CFG_INVALID_FS_EN;
             return;
#endif
             break;

        default:
            *p_err = HTTPs_ERR_CFG_INVALID_FS_TYPE;
             return;
    }


#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
                                                                /* -------------- VALIDATE REM MEM AVAIL -------------- */
    octets_reqd = (HTTPs_CFG_POOLS_INIT_NBR * sizeof(HTTPs_CONN))
                + (HTTPs_CFG_POOLS_INIT_NBR * p_cfg->BufLen)
                + (HTTPs_CFG_POOLS_INIT_NBR * path_len_max);


#if (HTTPs_CFG_ABSOLUTE_URI_EN == DEF_ENABLED)
    octets_reqd += (HTTPs_CFG_POOLS_INIT_NBR * p_cfg->HostNameLenMax);
#endif


#if (HTTPs_CFG_TOKEN_PARSE_EN == DEF_ENABLED)
    if (p_cfg->TokenCfgPtr != DEF_NULL) {                       /* If token parse is enabled, add space for token.      */


        octets_reqd += ((HTTPs_CFG_POOLS_INIT_NBR * sizeof(HTTPs_TOKEN_CTRL))
                    +   (HTTPs_CFG_POOLS_INIT_NBR * (p_cfg->TokenCfgPtr->ValLenMax
                                                     + HTTP_STR_BUF_TOP_SPACE_REQ_MIN
                                                     + HTTP_STR_BUF_END_SPACE_REQ_MIN)));
    }
#endif

#if (HTTPs_CFG_QUERY_STR_EN == DEF_ENABLED)
    if (p_cfg->QueryStrCfgPtr != DEF_NULL) {
        octets_reqd += ((HTTPs_CFG_POOLS_INIT_NBR * sizeof(HTTPs_KEY_VAL))
                    +   (HTTPs_CFG_POOLS_INIT_NBR * p_cfg->QueryStrCfgPtr->KeyLenMax)
                    +   (HTTPs_CFG_POOLS_INIT_NBR * p_cfg->QueryStrCfgPtr->ValLenMax));
    }
#endif

#if (HTTPs_CFG_FORM_EN == DEF_ENABLED)
    if (p_cfg->FormCfgPtr != DEF_NULL) {                        /* If Form is enabled, add space for key/value pair.    */
        octets_reqd += ((HTTPs_CFG_POOLS_INIT_NBR * sizeof(HTTPs_KEY_VAL))
                    +   (HTTPs_CFG_POOLS_INIT_NBR * p_cfg->FormCfgPtr->KeyLenMax)
                    +   (HTTPs_CFG_POOLS_INIT_NBR * p_cfg->FormCfgPtr->ValLenMax));

#if (HTTPs_CFG_FORM_MULTIPART_EN == DEF_ENABLED)
        if (p_cfg->FormCfgPtr->MultipartEn == DEF_ENABLED) {    /* If multipart is enabled, add boundary space.         */
            octets_reqd += (HTTPs_CFG_POOLS_INIT_NBR * HTTPs_FORM_BOUNDARY_STR_LEN_MAX);
        }
#endif
    }
#endif

                                                                /* Get and validate rem space avail on heap.            */
    octets_rem = Mem_SegRemSizeGet(HTTPs_MemSegPtr,
                                   sizeof(CPU_SIZE_T),
                                   DEF_NULL,
                                  &err_lib);

    if (octets_rem < octets_reqd) {
       *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
        return;
    }
#endif

                                                                /* ----------------- CREATE CONN POOL ----------------- */
    Mem_DynPoolCreate("HTTPs Conn Pool",
                      &p_instance->PoolConn,
                       HTTPs_MemSegPtr,
                       sizeof(HTTPs_CONN),
                       sizeof(CPU_SIZE_T),
                       HTTPs_CFG_POOLS_INIT_NBR,
                       p_cfg->ConnNbrMax,
                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
             return;

        default:
            *p_err = HTTPs_ERR_INIT_POOL_CONN;
             return;
    }

                                                                /* ----------------- CREATE BUF POOL ------------------ */
    Mem_DynPoolCreate("HTTPs Conn Buffer Pool",
                      &p_instance->PoolBuf,
                       HTTPs_MemSegPtr,
                       p_cfg->BufLen,
                       sizeof(CPU_SIZE_T),
                       HTTPs_CFG_POOLS_INIT_NBR,
                       p_cfg->ConnNbrMax,
                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
             return;

        default:
            *p_err = HTTPs_ERR_INIT_POOL_BUF;
             return;
    }

                                                                /* --------------- CREATE PATH POOL --------------- */
    Mem_DynPoolCreate("HTTPs Conn Path Pool",
                      &p_instance->PoolPath,
                       HTTPs_MemSegPtr,
                       path_len_max,
                       sizeof(CPU_SIZE_T),
                       HTTPs_CFG_POOLS_INIT_NBR,
                       p_cfg->ConnNbrMax,
                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
             return;

        default:
            *p_err = HTTPs_ERR_INIT_POOL_PATH;
             return;
    }

#if (HTTPs_CFG_ABSOLUTE_URI_EN == DEF_ENABLED)
                                                                /* ----------------- CREATE HOST POOL ----------------- */
    Mem_DynPoolCreate("HTTPs Conn Host Pool",
                      &p_instance->PoolHost,
                       HTTPs_MemSegPtr,
                       p_cfg->HostNameLenMax,
                       sizeof(CPU_SIZE_T),
                       HTTPs_CFG_POOLS_INIT_NBR,
                       p_cfg->ConnNbrMax,
                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
             return;

        default:
            *p_err = HTTPs_ERR_INIT_POOL_HOST;
             return;
    }
#endif

#if (HTTPs_CFG_TOKEN_PARSE_EN == DEF_ENABLED)
    if (p_cfg->TokenCfgPtr != DEF_NULL) {

        if (p_cfg->TokenCfgPtr->NbrPerConnMax != LIB_MEM_BLK_QTY_UNLIMITED) {
            pool_size_max = p_cfg->ConnNbrMax * p_cfg->TokenCfgPtr->NbrPerConnMax;
        } else {
            pool_size_max = LIB_MEM_BLK_QTY_UNLIMITED;
        }

                                                                /* ---------------- CREATE TOKEN POOL ----------------- */
        Mem_DynPoolCreate("HTTPs Token Ctrl Pool",
                          &p_instance->PoolTokenCtrl,
                           HTTPs_MemSegPtr,
                           sizeof(HTTPs_TOKEN_CTRL),
                           sizeof(CPU_SIZE_T),
                           HTTPs_CFG_POOLS_INIT_NBR,
                           pool_size_max,
                          &err_lib);
        switch (err_lib) {
            case LIB_MEM_ERR_NONE:
                 break;

            case LIB_MEM_ERR_SEG_OVF:
                *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
                 return;

            default:
                *p_err = HTTPs_ERR_INIT_POOL_TOKEN;
                 return;
        }

                                                                /* -------------- CREATE TOKEN VAL POOL --------------- */
        val_len = p_cfg->TokenCfgPtr->ValLenMax + HTTP_STR_BUF_TOP_SPACE_REQ_MIN + HTTP_STR_BUF_END_SPACE_REQ_MIN;

        Mem_DynPoolCreate("HTTPs Token Val Pool",
                          &p_instance->PoolTokenVal,
                           HTTPs_MemSegPtr,
                           val_len,
                           sizeof(CPU_SIZE_T),
                           HTTPs_CFG_POOLS_INIT_NBR,
                           pool_size_max,
                          &err_lib);
        switch (err_lib) {
            case LIB_MEM_ERR_NONE:
                 break;

            case LIB_MEM_ERR_SEG_OVF:
                *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
                 return;

            default:
                *p_err = HTTPs_ERR_INIT_POOL_TOKEN_VAL;
                 return;
        }
    }
#endif

#if ((HTTPs_CFG_FORM_EN      == DEF_ENABLED) || \
     (HTTPs_CFG_QUERY_STR_EN == DEF_ENABLED))

    pool_size_max = 0;

    if (p_cfg->FormCfgPtr != DEF_NULL) {
        if (p_cfg->FormCfgPtr->NbrPerConnMax != LIB_MEM_BLK_QTY_UNLIMITED) {
            pool_size_max = p_cfg->ConnNbrMax * p_cfg->FormCfgPtr->NbrPerConnMax;
        } else {
            pool_size_max = LIB_MEM_BLK_QTY_UNLIMITED;
        }
    }

    if (p_cfg->QueryStrCfgPtr != DEF_NULL) {
        if ((p_cfg->QueryStrCfgPtr->NbrPerConnMax != LIB_MEM_BLK_QTY_UNLIMITED) &&
            (pool_size_max                        != LIB_MEM_BLK_QTY_UNLIMITED)){
            pool_size_max += p_cfg->ConnNbrMax * p_cfg->QueryStrCfgPtr->NbrPerConnMax;
        } else {
            pool_size_max = LIB_MEM_BLK_QTY_UNLIMITED;
        }
    }

    if ((p_cfg->FormCfgPtr     != DEF_NULL) ||
        (p_cfg->QueryStrCfgPtr != DEF_NULL)) {
                                                                /* ------------- CREATE KEY-VALUE BLK POOL ------------ */
        Mem_DynPoolCreate("HTTPs Key-Val Blk Pool",
                          &p_instance->PoolKeyVal,
                           HTTPs_MemSegPtr,
                           sizeof(HTTPs_KEY_VAL),
                           sizeof(CPU_SIZE_T),
                           HTTPs_CFG_POOLS_INIT_NBR,
                           pool_size_max,
                          &err_lib);
        switch (err_lib) {
            case LIB_MEM_ERR_NONE:
                 break;

            case LIB_MEM_ERR_SEG_OVF:
                *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
                 return;

            default:
                *p_err = HTTPs_ERR_INIT_POOL_KEY_VAL;
                 return;
        }
    }
#endif

#if (HTTPs_CFG_QUERY_STR_EN == DEF_ENABLED)
    if (p_cfg->QueryStrCfgPtr != DEF_NULL) {

        if (p_cfg->QueryStrCfgPtr->NbrPerConnMax != LIB_MEM_BLK_QTY_UNLIMITED) {
            pool_size_max = p_cfg->ConnNbrMax * p_cfg->QueryStrCfgPtr->NbrPerConnMax;
        } else {
            pool_size_max = LIB_MEM_BLK_QTY_UNLIMITED;
        }
                                                                /* -------- CREATE QUERY STRING KEY STRING POOL ------- */
        Mem_DynPoolCreate("HTTPs Query Str Key Pool",
                          &p_instance->PoolQueryStrKeyStr,
                           HTTPs_MemSegPtr,
                           p_cfg->QueryStrCfgPtr->KeyLenMax,
                           sizeof(CPU_SIZE_T),
                           HTTPs_CFG_POOLS_INIT_NBR,
                           pool_size_max,
                          &err_lib);
        switch (err_lib) {
            case LIB_MEM_ERR_NONE:
                 break;

            case LIB_MEM_ERR_SEG_OVF:
                *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
                 return;

            default:
                *p_err = HTTPs_ERR_INIT_POOL_QUERY_STR_KEY;
                 return;
        }

                                                                /* ------- CREATE QUERY STRING VAL STRING POOL -------- */
        Mem_DynPoolCreate("HTTPs Query String Val Pool",
                          &p_instance->PoolQueryStrValStr,
                           HTTPs_MemSegPtr,
                           p_cfg->QueryStrCfgPtr->ValLenMax,
                           sizeof(CPU_SIZE_T),
                           HTTPs_CFG_POOLS_INIT_NBR,
                           pool_size_max,
                          &err_lib);
        switch (err_lib) {
            case LIB_MEM_ERR_NONE:
                 break;

            case LIB_MEM_ERR_SEG_OVF:
                *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
                 return;

            default:
                *p_err = HTTPs_ERR_INIT_POOL_QUERY_STR_VAL;
                 return;
        }
    }
#endif


#if (HTTPs_CFG_FORM_EN == DEF_ENABLED)
    if (p_cfg->FormCfgPtr != DEF_NULL) {

        if (p_cfg->FormCfgPtr->NbrPerConnMax != LIB_MEM_BLK_QTY_UNLIMITED) {
            pool_size_max = p_cfg->ConnNbrMax * p_cfg->FormCfgPtr->NbrPerConnMax;
        } else {
            pool_size_max = LIB_MEM_BLK_QTY_UNLIMITED;
        }

                                                                /* ------------ CREATE FORM KEY STRING POOL ----------- */
        Mem_DynPoolCreate("HTTPs Form Key Pool",
                          &p_instance->PoolFormKeyStr,
                           HTTPs_MemSegPtr,
                           p_cfg->FormCfgPtr->KeyLenMax,
                           sizeof(CPU_SIZE_T),
                           HTTPs_CFG_POOLS_INIT_NBR,
                           pool_size_max,
                          &err_lib);
        switch (err_lib) {
            case LIB_MEM_ERR_NONE:
                 break;

            case LIB_MEM_ERR_SEG_OVF:
                *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
                 return;

            default:
                *p_err = HTTPs_ERR_INIT_POOL_FORM_KEY;
                 return;
        }

                                                                /* ----------- CREATE FORM VAL STRING POOL ------------ */
        Mem_DynPoolCreate("HTTPs Form Val Pool",
                          &p_instance->PoolFormValStr,
                           HTTPs_MemSegPtr,
                           p_cfg->FormCfgPtr->ValLenMax,
                           sizeof(CPU_SIZE_T),
                           HTTPs_CFG_POOLS_INIT_NBR,
                           pool_size_max,
                          &err_lib);
        switch (err_lib) {
            case LIB_MEM_ERR_NONE:
                 break;

            case LIB_MEM_ERR_SEG_OVF:
                *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
                 return;

            default:
                *p_err = HTTPs_ERR_INIT_POOL_FORM_VAL;
                 return;
        }

#if (HTTPs_CFG_FORM_MULTIPART_EN == DEF_ENABLED)
        if (p_cfg->FormCfgPtr->MultipartEn == DEF_ENABLED) {
                                                                /* ------------- CREATE FORM BOUNDARY POOL ------------ */
            Mem_DynPoolCreate("HTTPs Form Boundary Pool",
                              &p_instance->PoolFormBoundary,
                               HTTPs_MemSegPtr,
                               HTTPs_FORM_BOUNDARY_STR_LEN_MAX,
                               sizeof(CPU_SIZE_T),
                               HTTPs_CFG_POOLS_INIT_NBR,
                               p_cfg->ConnNbrMax,
                              &err_lib);
            switch (err_lib) {
                case LIB_MEM_ERR_NONE:
                     break;

                case LIB_MEM_ERR_SEG_OVF:
                    *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
                     return;

                default:
                    *p_err = HTTPs_ERR_INIT_POOL_FORM_BOUNDARY;
                     return;
            }
        }
#endif

    }
#endif


#if (HTTPs_CFG_HDR_RX_EN == DEF_ENABLED)
    if (p_cfg->HdrRxCfgPtr != DEF_NULL) {
                                                                /* ------------- INIT REQ HDR FIELD POOL -------------- */
        HTTPsMem_ReqHdrPoolInit(p_instance,
                                p_err);
        if (*p_err != HTTPs_ERR_NONE) {
             return;
        }
    }
#endif

#if (HTTPs_CFG_HDR_TX_EN == DEF_ENABLED)
    if (p_cfg->HdrTxCfgPtr != DEF_NULL) {
                                                                /* -------------- INIT RESP HDR FIELD POOL ------------ */
        HTTPsMem_RespHdrPoolInit(p_instance,
                                 p_err);
        if (*p_err != HTTPs_ERR_NONE) {
             return;
        }
    }
#endif

   (void)octets_reqd;
   (void)pool_size_max;

   *p_err = HTTPs_ERR_NONE;
}


/*
*********************************************************************************************************
*                                          HTTPsMem_ConnGet()
*
* Description : (1) Acquire a new connection:
*
*                   (a) Acquire connection block
*                   (b) Acquire buffer block
*                   (c) Update connections list
*                   (d) Initialize connection parameters
*
* Argument(s) : p_instance      Pointer to the instance used to acquire a connection.
*               ----------      Argument validated in HTTPs_InstanceStart().
*
*               sock_id         Socket ID accepted.
*
*               client_addr     Client address accepted.
*
* Return(s)   : Pointer to the connection, if no error(s).
*
*               Null pointer,              otherwise.
*
* Caller(s)   : HTTPsSock_ConnAccept().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

HTTPs_CONN  *HTTPsMem_ConnGet (HTTPs_INSTANCE  *p_instance,
                               NET_SOCK_ID      sock_id,
                               NET_SOCK_ADDR    client_addr)
{
    const  HTTPs_CFG             *p_cfg;
           HTTPs_CONN            *p_conn;
           HTTPs_CONN            *p_conn_item;
           HTTPs_INSTANCE_STATS  *p_ctr_stats;
           HTTPs_INSTANCE_ERRS   *p_ctr_err;
           LIB_ERR                err_lib;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

    p_cfg = p_instance->CfgPtr;

                                                                /* ----------------- ACQUIRE CONN BLK ----------------- */
    p_conn = (HTTPs_CONN *)Mem_DynPoolBlkGet(&p_instance->PoolConn,
                                             &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             Mem_Clr(p_conn, sizeof(HTTPs_CONN));               /* Invalidate pre-used memory                           */
             break;

        case LIB_MEM_ERR_SEG_OVF:
             HTTPs_ERR_INC(p_ctr_err->Conn_ErrPoolMemSpaceCtr);
             goto exit;

        case LIB_MEM_ERR_POOL_EMPTY:
             HTTPs_ERR_INC(p_ctr_err->Conn_ErrPoolEmptyCtr);
             goto exit;

        default:
             HTTPs_ERR_INC(p_ctr_err->Conn_ErrPoolLibGetCtr);
             goto exit;
    }

                                                                /* ----------------- ACQUIRE BUF BLK ------------------ */
    p_conn->BufPtr = (CPU_CHAR *)Mem_DynPoolBlkGet(&p_instance->PoolBuf,
                                                   &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
             HTTPs_ERR_INC(p_ctr_err->Conn_ErrBufPoolMemSpaceCtr);
             goto exit_release_conn;

        case LIB_MEM_ERR_POOL_EMPTY:
             HTTPs_ERR_INC(p_ctr_err->Conn_ErrBufPoolEmptyCtr);
             goto exit_release_conn;

        default:
             HTTPs_ERR_INC(p_ctr_err->Conn_ErrBufPoolLibGetCtr);
             goto exit_release_conn;
    }

                                                                /* ----------------- ACQUIRE PATH BLK ----------------- */
    p_conn->PathPtr = (CPU_CHAR *)Mem_DynPoolBlkGet(&p_instance->PoolPath,
                                                    &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
             HTTPs_ERR_INC(p_ctr_err->Conn_ErrPathPoolMemSpaceCtr);
             goto exit_release_buf;

        case LIB_MEM_ERR_POOL_EMPTY:
             HTTPs_ERR_INC(p_ctr_err->Conn_ErrPathPoolEmptyCtr);
             goto exit_release_buf;

        default:
             HTTPs_ERR_INC(p_ctr_err->Conn_ErrPathPoolLibGetCtr);
             goto exit_release_buf;
    }

#if (HTTPs_CFG_ABSOLUTE_URI_EN == DEF_ENABLED)
                                                                /* ----------------- ACQUIRE HOST BLK ----------------- */
    p_conn->HostPtr = (CPU_CHAR *)Mem_DynPoolBlkGet(&p_instance->PoolHost,
                                                    &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
             HTTPs_ERR_INC(p_ctr_err->Host_ErrPoolMemSpaceCtr);
             goto exit_release_path;

        case LIB_MEM_ERR_POOL_EMPTY:
             HTTPs_ERR_INC(p_ctr_err->Host_ErrPoolEmptyCtr);
             goto exit_release_path;

        default:
             HTTPs_ERR_INC(p_ctr_err->Host_ErrPoolLibGetCtr);
             goto exit_release_path;
    }
#endif


#if ((HTTPs_CFG_FORM_EN           == DEF_ENABLED) && \
     (HTTPs_CFG_FORM_MULTIPART_EN == DEF_ENABLED))
    if (p_cfg->FormCfgPtr != DEF_NULL) {

        if (p_cfg->FormCfgPtr->MultipartEn == DEF_ENABLED) {
                                                                /* ------------- ACQUIRE FORM BOUNDARY BLK ------------ */
            p_conn->FormBoundaryPtr = (CPU_CHAR *)Mem_DynPoolBlkGet(&p_instance->PoolFormBoundary,
                                                                    &err_lib);
            switch (err_lib) {
                case LIB_MEM_ERR_NONE:
                     break;

                case LIB_MEM_ERR_SEG_OVF:
                    HTTPs_ERR_INC(p_ctr_err->Req_ErrFormBoundaryPoolMemSpaceCtr);
                     goto exit_release_host;

                case LIB_MEM_ERR_POOL_EMPTY:
                     HTTPs_ERR_INC(p_ctr_err->Req_ErrFormBoundaryPoolEmptyCtr);
                     goto exit_release_host;

                default:
                     HTTPs_ERR_INC(p_ctr_err->Req_ErrFormBoundaryPoolLibGetCtr);
                     goto exit_release_host;
            }
        }
    }
#endif

                                                                /* ------------ UPDATE INSTANCE CONN LIST ------------- */
    if (p_instance->ConnFirstPtr == DEF_NULL) {
        p_instance->ConnFirstPtr = p_conn;
        p_instance->ConnLastPtr  = p_conn;

    } else if (p_instance->ConnFirstPtr == p_instance->ConnLastPtr) {
        p_conn_item              = p_instance->ConnFirstPtr;
        p_conn_item->ConnNextPtr = p_conn;
        p_conn->ConnPrevPtr      = p_conn_item;
        p_instance->ConnLastPtr  = p_conn;

    } else {
        p_conn_item = p_instance->ConnLastPtr;
        p_conn_item->ConnNextPtr = p_conn;
        p_conn->ConnPrevPtr      = p_conn_item;
        p_instance->ConnLastPtr  = p_conn;
    }

                                                                /* ----------------- INIT CONN PARAM ------------------ */
    p_conn->SockID             =  sock_id;
    p_conn->SockState          =  HTTPs_SOCK_STATE_RX;
    p_conn->SockFlags          =  HTTPs_FLAG_NONE;
    p_conn->ClientAddr         =  client_addr;

    p_conn->ErrCode            =  HTTPs_ERR_NONE;
    p_conn->State              =  HTTPs_CONN_STATE_UNKNOWN;
    p_conn->Flags              =  HTTPs_FLAG_INIT;

    p_conn->Method             =  HTTP_METHOD_UNKNOWN;
    p_conn->ReqContentType     =  HTTP_CONTENT_TYPE_UNKNOWN;
    p_conn->ReqContentLen      =  0;
    p_conn->ReqContentLenRxd   =  0;

    p_conn->ProtocolVer        =  HTTP_PROTOCOL_VER_1_1;
    p_conn->DataPtr            =  DEF_NULL;
    p_conn->DataLen            =  0u;
    p_conn->DataTxdLen         =  0u;
    p_conn->DataFixPosCur      =  0u;

    switch (p_cfg->FS_Type) {
        case HTTPs_FS_TYPE_NONE:
             p_conn->PathLenMax = ((HTTPs_CFG_FS_NONE *)p_cfg->FS_CfgPtr)->PathLenMax;
             break;

        case HTTPs_FS_TYPE_STATIC:
        case HTTPs_FS_TYPE_DYN:
#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED)
             p_conn->PathLenMax = p_instance->FS_PathLenMax;
#else
             HTTPs_ERR_INC(p_ctr_err->FS_ErrNoEnCtr);
             goto exit_release_host;
#endif
             break;

        default:
             HTTPs_ERR_INC(p_ctr_err->FS_ErrTypeInvalidCtr);
             goto exit_release_host;
    }

    p_conn->StatusCode         =  HTTP_STATUS_OK;
    p_conn->RespBodyDataType   =  HTTPs_BODY_DATA_TYPE_NONE;
    p_conn->RespContentType    =  HTTP_CONTENT_TYPE_UNKNOWN;

    p_conn->BufLen             =  p_cfg->BufLen;
    p_conn->RxBufPtr           =  p_conn->BufPtr;
    p_conn->TxBufPtr           =  p_conn->BufPtr;

    p_conn->RxBufLenRem        =  0u;
    p_conn->RxDataLen          =  0u;
    p_conn->TxDataLen          =  0u;

    p_conn->ConnDataPtr        =  DEF_NULL;

    p_conn->ConnNextPtr        =  DEF_NULL;

    p_instance->ConnActiveCtr++;

    HTTPs_STATS_INC(p_ctr_stats->Conn_StatAcquiredCtr);

    goto exit;

exit_release_host:
#if (HTTPs_CFG_ABSOLUTE_URI_EN == DEF_ENABLED)
    Mem_DynPoolBlkFree(&p_instance->PoolHost,                   /* Release host previously acquired.                    */
                        p_conn->HostPtr,
                       &err_lib);
    p_conn->HostPtr = DEF_NULL;

exit_release_path:
#endif
    Mem_DynPoolBlkFree(&p_instance->PoolPath,                   /* Release path previously acquired.                    */
                        p_conn->PathPtr,
                       &err_lib);
    p_conn->PathPtr = DEF_NULL;

exit_release_buf:
    Mem_DynPoolBlkFree(&p_instance->PoolBuf,                    /* Release buf previously acquired.                     */
                        p_conn->BufPtr,
                       &err_lib);
    p_conn->BufPtr = DEF_NULL;

exit_release_conn:
    Mem_DynPoolBlkFree(&p_instance->PoolConn,                   /* Release conn previously acquired.                    */
                        p_conn,
                       &err_lib);
    p_conn = DEF_NULL;

exit:
    return (p_conn);
}


/*
*********************************************************************************************************
*                                        HTTPsMem_ConnRelease()
*
* Description : (1) Release connection:
*
*                   (a) Update instance connection list
*                   (b) Release buffer     block
*                   (c) Release connection block
*
* Argument(s) : p_instance  Pointer to the instance structure variable.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*               ----------  Argument validated in HTTPsSock_ConnAccept().
*
* Return(s)   : none.
*
* Caller(s)   : HTTPsSock_ConnAccept(),
*               HTTPsConn_Close().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : (2) Opened socket MUST be closed before calling this function.
*********************************************************************************************************
*/

void  HTTPsMem_ConnRelease (HTTPs_INSTANCE  *p_instance,
                            HTTPs_CONN      *p_conn)
{
           HTTPs_CONN            *p_conn_item_prev;
           HTTPs_CONN            *p_conn_item_next;
           HTTPs_INSTANCE_STATS  *p_ctr_stats;
           HTTPs_INSTANCE_ERRS   *p_ctr_err;
           LIB_ERR                err_lib;
#if ((HTTPs_CFG_FORM_EN           == DEF_ENABLED) && \
     (HTTPs_CFG_FORM_MULTIPART_EN == DEF_ENABLED))
    const  HTTPs_CFG             *p_cfg;


    p_cfg = p_instance->CfgPtr;
#endif


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

                                                                /* ------------ UPDATE INSTANCE CONN LIST ------------- */
    if (p_instance->ConnFirstPtr == DEF_NULL) {
        HTTPs_STATS_INC(p_ctr_err->Conn_ErrFreePtrNullCtr);

    } else if (p_conn == p_instance->ConnFirstPtr) {
        if (p_conn->ConnNextPtr != DEF_NULL) {
            p_conn_item_next              = p_conn->ConnNextPtr;
            p_instance->ConnFirstPtr      = p_conn_item_next;
            p_conn_item_next->ConnPrevPtr = DEF_NULL;

        } else {
            p_instance->ConnFirstPtr = DEF_NULL;
            p_instance->ConnLastPtr  = DEF_NULL;
        }

    } else if (p_conn == p_instance->ConnLastPtr) {
        p_instance->ConnLastPtr              = p_conn->ConnPrevPtr;
        p_instance->ConnLastPtr->ConnNextPtr = DEF_NULL;

    } else {
        p_conn_item_next = p_conn->ConnNextPtr;
        p_conn_item_prev = p_conn->ConnPrevPtr;
        if (p_conn_item_prev != DEF_NULL) {
            p_conn_item_prev->ConnNextPtr = p_conn_item_next;
        }

        if (p_conn_item_next != DEF_NULL) {
            p_conn_item_next->ConnPrevPtr = p_conn_item_prev;
        }
    }

#if (HTTPs_CFG_ABSOLUTE_URI_EN == DEF_ENABLED)
                                                                /* ----------------- RELEASE HOST BLK ----------------- */
    Mem_DynPoolBlkFree(&p_instance->PoolHost,
                        p_conn->HostPtr,
                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        HTTPs_ERR_INC(p_ctr_err->Host_ErrPoolLibFreeCtr);
    }

    p_conn->HostPtr = DEF_NULL;
#endif

                                                                /* ----------------- RELEASE PATH BLK ----------------- */
    Mem_DynPoolBlkFree(&p_instance->PoolPath,
                        p_conn->PathPtr,
                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        HTTPs_ERR_INC(p_ctr_err->Conn_ErrPathPoolLibFreeCtr);
    }

    p_conn->PathPtr = DEF_NULL;

                                                                /* ----------------- RELEASE BUF BLK ------------------ */
    Mem_DynPoolBlkFree(&p_instance->PoolBuf,
                        p_conn->BufPtr,
                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        HTTPs_ERR_INC(p_ctr_err->Conn_ErrBufPoolLibFreeCtr);
    }

    p_conn->BufPtr = DEF_NULL;

#if ((HTTPs_CFG_FORM_EN           == DEF_ENABLED) && \
     (HTTPs_CFG_FORM_MULTIPART_EN == DEF_ENABLED))
    if (p_cfg->FormCfgPtr         != DEF_NULL) {

        if (p_cfg->FormCfgPtr->MultipartEn == DEF_ENABLED) {
                                                                /* ------------- RELEASE FORM BOUDNARY BLK ------------ */
            Mem_DynPoolBlkFree(&p_instance->PoolFormBoundary,
                                p_conn->FormBoundaryPtr,
                               &err_lib);
            if (err_lib != LIB_MEM_ERR_NONE) {
                HTTPs_ERR_INC(p_ctr_err->Req_ErrFormBoundaryPoolLibFreeCtr);
            }

            p_conn->FormBoundaryPtr = DEF_NULL;
        }
    }
#endif

                                                                /* ----------------- RELEASE CONN BLK ----------------- */
    Mem_DynPoolBlkFree(&p_instance->PoolConn,
                        p_conn,
                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        HTTPs_ERR_INC(p_ctr_err->Conn_ErrPoolLibFreeCtr);
    }


    HTTPs_STATS_INC(p_ctr_stats->Conn_StatReleasedCtr);

    p_instance->ConnActiveCtr--;
}


/*
*********************************************************************************************************
*                                          HTTPsMem_ConnClr()
*
* Description : (1) Clear objects related to an HTTP request on the given connection.
*
*                   (a) Release Token blocks
*                   (b) Release Key-Value blocks
*                   (c) Release Header blocks
*
*
* Argument(s) : p_instance  Pointer to the instance structure variable.
*
*               p_conn      Pointer to the connection.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPsConn_Process().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPsMem_ConnClr (HTTPs_INSTANCE  *p_instance,
                        HTTPs_CONN      *p_conn)
{
    const  HTTPs_CFG            *p_cfg = p_instance->CfgPtr;
           HTTPs_INSTANCE_ERRS  *p_ctr_err;


    HTTPs_SET_PTR_ERRS(p_ctr_err, p_instance);

#if (HTTPs_CFG_TOKEN_PARSE_EN == DEF_ENABLED)
    if (p_conn->TokenCtrlPtr != DEF_NULL) {
        HTTPsMem_TokenRelease(p_instance, p_conn);
        HTTPs_ERR_INC(p_ctr_err->Resp_ErrTokenClrNotEmptyCtr);
    }
#endif

#if (HTTPs_CFG_QUERY_STR_EN == DEF_ENABLED)
    HTTPsMem_QueryStrKeyValBlkReleaseAll(p_instance, p_conn);
#endif

#if (HTTPs_CFG_FORM_EN == DEF_ENABLED)
    HTTPsMem_FormKeyValBlkReleaseAll(p_instance, p_conn);
#endif

#if ((HTTPs_CFG_HDR_RX_EN == DEF_ENABLED) || \
     (HTTPs_CFG_HDR_TX_EN == DEF_ENABLED))

    switch (p_conn->HdrType) {
        case HTTPs_HDR_TYPE_REQ:
                                                                /* ---------- VALIDATE REQ HDR POOL IS EMPTY ---------- */
#if (HTTPs_CFG_HDR_RX_EN == DEF_ENABLED)
             while (p_conn->HdrListPtr != DEF_NULL) {
                 HTTPsMem_ReqHdrRelease(p_instance,
                                        p_conn);
             }
#endif
             break;

        case HTTPs_HDR_TYPE_RESP:
                                                                /* --------- VALIDATE RESP HDR POOL IS EMPTY ---------- */
#if (HTTPs_CFG_HDR_TX_EN == DEF_ENABLED)
             while (p_conn->HdrListPtr != DEF_NULL) {
                 HTTPsMem_RespHdrRelease(p_instance,
                                         p_conn);
             }
#endif
             break;

        default:
             HTTPs_ERR_INC(p_ctr_err->Conn_ErrHdrTypeInvalidCtr);
             break;
    }

#endif

                                                                /* ----------------- INIT CONN PARAM ------------------ */
    p_conn->SockState          =  HTTPs_SOCK_STATE_RX;
    p_conn->SockFlags          =  HTTPs_FLAG_NONE;

    p_conn->ErrCode            =  HTTPs_ERR_NONE;
    p_conn->State              =  HTTPs_CONN_STATE_UNKNOWN;
    p_conn->Flags              =  HTTPs_FLAG_INIT;

    p_conn->Method             =  HTTP_METHOD_UNKNOWN;
    p_conn->ReqContentType     =  HTTP_CONTENT_TYPE_UNKNOWN;
    p_conn->ReqContentLen      =  0;
    p_conn->ReqContentLenRxd   =  0;

    p_conn->ProtocolVer        =  HTTP_PROTOCOL_VER_1_1;
    p_conn->DataPtr            =  DEF_NULL;
    p_conn->DataLen            =  0u;
    p_conn->DataTxdLen         =  0u;
    p_conn->DataFixPosCur      =  0u;

    switch (p_cfg->FS_Type) {
        case HTTPs_FS_TYPE_NONE:
             p_conn->PathLenMax = ((HTTPs_CFG_FS_NONE *)p_cfg->FS_CfgPtr)->PathLenMax;
             break;

        case HTTPs_FS_TYPE_STATIC:
        case HTTPs_FS_TYPE_DYN:
#if (HTTPs_CFG_FS_PRESENT_EN == DEF_ENABLED)
             p_conn->PathLenMax = p_instance->FS_PathLenMax;
#else
             HTTPs_ERR_INC(p_ctr_err->FS_ErrNoEnCtr);
#endif
             break;

        default:
             HTTPs_ERR_INC(p_ctr_err->FS_ErrTypeInvalidCtr);
             break;
    }

    p_conn->StatusCode         =  HTTP_STATUS_OK;
    p_conn->RespBodyDataType   =  HTTPs_BODY_DATA_TYPE_NONE;
    p_conn->RespContentType    =  HTTP_CONTENT_TYPE_UNKNOWN;

    p_conn->RxBufPtr           =  p_conn->BufPtr;
    p_conn->TxBufPtr           =  p_conn->BufPtr;

    p_conn->RxBufLenRem        =  0u;
    p_conn->RxDataLen          =  0u;
    p_conn->TxDataLen          =  0u;

   (void)p_ctr_err;
}


/*
*********************************************************************************************************
*                                         HTTPsMem_TokenGet()
*
* Description : (1) Acquire a new token block:
*
*                   (a) Acquire token block
*                   (b) Acquire token value block
*                   (c) Update connection token list
*
* Argument(s) : p_instance  Pointer to the instance used to acquire a connection.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*               ------      Argument validated in HTTPsSock_ConnAccept().
*
* Return(s)   : Pointer to token, if no error(s).
*
*               Null pointer,     otherwise.
*
* Caller(s)   : HTTPs_RespFileTransferChunked().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (HTTPs_CFG_TOKEN_PARSE_EN == DEF_ENABLED)
CPU_BOOLEAN  HTTPsMem_TokenGet (HTTPs_INSTANCE  *p_instance,
                                HTTPs_CONN      *p_conn,
                                HTTPs_ERR       *p_err)
{
    HTTPs_TOKEN_CTRL       *p_token;
    HTTPs_INSTANCE_STATS   *p_ctr_stats;
    HTTPs_INSTANCE_ERRS    *p_ctr_err;
    CPU_BOOLEAN             result;
    LIB_ERR                 err_lib;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

                                                                /* ---------------- ACQUIRE TOKEN BLK ----------------- */
    p_token = (HTTPs_TOKEN_CTRL *)Mem_DynPoolBlkGet(&p_instance->PoolTokenCtrl,
                                                    &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
             result = DEF_FAIL;
            *p_err  = HTTPs_ERR_MEM_NO_SPACE;
             HTTPs_ERR_INC(p_ctr_err->Resp_ErrTokenPoolMemSpaceCtr);
             goto exit;

        case LIB_MEM_ERR_POOL_EMPTY:
             result = DEF_FAIL;
            *p_err  = HTTPs_ERR_TOKEN_POOL_EMPTY;
             HTTPs_ERR_INC(p_ctr_err->Resp_ErrTokenPoolEmptyCtr);
             goto exit;

        default:
             result = DEF_FAIL;
            *p_err  = HTTPs_ERR_TOKEN_POOL_LIB_FAULT;
             HTTPs_ERR_INC(p_ctr_err->Resp_ErrTokenPoolLibGetCtr);
             goto exit;
    }

                                                                /* -------------- ACQUIRE TOKEN VAL BLK --------------- */
    p_token->ValPtr = (CPU_CHAR *)Mem_DynPoolBlkGet(&p_instance->PoolTokenVal,
                                                    &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
             result = DEF_FAIL;
            *p_err  = HTTPs_ERR_MEM_NO_SPACE;
             HTTPs_ERR_INC(p_ctr_err->Resp_ErrTokenValPoolMemSpaceCtr);
             goto exit_release_blk;

        case LIB_MEM_ERR_POOL_EMPTY:
             result = DEF_FAIL;
            *p_err  = HTTPs_ERR_TOKEN_POOL_EMPTY;
             HTTPs_ERR_INC(p_ctr_err->Resp_ErrTokenValPoolEmptyCtr);
             goto exit_release_blk;

        default:
             result = DEF_FAIL;
            *p_err  = HTTPs_ERR_TOKEN_POOL_LIB_FAULT;
             HTTPs_ERR_INC(p_ctr_err->Resp_ErrTokenValPoolLibGetCtr);
             goto exit_release_blk;
    }

                                                                /* -------------- UPDATE CONN TOKEN LIST -------------- */
    p_conn->TokenCtrlPtr = p_token;

    HTTPs_STATS_INC(p_ctr_stats->Resp_StatTokenAcquiredCtr);

    result = DEF_OK;
   *p_err  = HTTPs_ERR_NONE;

    goto exit;


exit_release_blk:
    Mem_DynPoolBlkFree(&p_instance->PoolTokenCtrl,              /* Release token previously acquired.                   */
                        p_token,
                       &err_lib);

exit:
    return (result);
}
#endif


/*
*********************************************************************************************************
*                                       HTTPsMem_TokenRelease()
*
* Description : (1) Release token block:
*
*                   (a) Update connection token list
*                   (b) Release token value
*                   (c) Release token block
*
* Argument(s) : p_instance  Pointer to the instance used to acquire a connection.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*               ------      Argument validated in HTTPsSock_ConnAccept().
*
* Return(s)   : none.
*
* Caller(s)   : HTTPsConn_Close(),
*               HTTPsResp_Handle().
*
*               This function is an INTERNAL HTTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (HTTPs_CFG_TOKEN_PARSE_EN == DEF_ENABLED)
void  HTTPsMem_TokenRelease (HTTPs_INSTANCE  *p_instance,
                             HTTPs_CONN      *p_conn)
{
    HTTPs_INSTANCE_STATS   *p_ctr_stats;
    HTTPs_INSTANCE_ERRS    *p_ctr_err;
    LIB_ERR                 err_lib;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

                                                                /* -------------- RELEASE TOKEN VAL BLK --------------- */
    Mem_DynPoolBlkFree(&p_instance->PoolTokenVal,
                        p_conn->TokenCtrlPtr->ValPtr,
                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        HTTPs_ERR_INC(p_ctr_err->Resp_ErrTokenValPoolLibFreeCtr);
    }

    p_conn->TokenCtrlPtr->ValPtr = DEF_NULL;

                                                                /* ---------------- RELEASE TOKEN BLK ----------------- */
    Mem_DynPoolBlkFree(&p_instance->PoolTokenCtrl,
                        p_conn->TokenCtrlPtr,
                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        HTTPs_ERR_INC(p_ctr_err->Resp_ErrTokenPoolLibFreeCtr);
    }

    p_conn->TokenCtrlPtr = DEF_NULL;

    HTTPs_STATS_INC(p_ctr_stats->Resp_StatTokenReleaseCtr);
}
#endif


/*
*********************************************************************************************************
*                                    HTTPsMem_QueryStrKeyValBlkGet()
*
* Description : (1) Acquire a new Key-Value Pair block for an HTTP Query String received:
*
*                   (a) Acquire Key-Value block
*                   (b) Acquire Key string block
*                   (c) Acquire Value string block

*
* Argument(s) : p_instance  Pointer to the instance used to acquire a connection.
*                           Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*                           Argument validated in HTTPsSock_ConnAccept().
*
* Return(s)   : Pointer to the Key-Value block, if no error(s).
*               Null pointer,                   otherwise.
*
* Caller(s)   : HTTPsReq_QueryStrKeyValBlkAdd().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (HTTPs_CFG_QUERY_STR_EN == DEF_ENABLED)
HTTPs_KEY_VAL  *HTTPsMem_QueryStrKeyValBlkGet (HTTPs_INSTANCE  *p_instance,
                                               HTTPs_CONN      *p_conn,
                                               HTTPs_ERR       *p_err)
{
    HTTPs_KEY_VAL         *p_key_val;
    HTTPs_INSTANCE_STATS  *p_ctr_stats;
    HTTPs_INSTANCE_ERRS   *p_ctr_err;
    LIB_ERR                err_lib;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

                                                                /* --------------- ACQUIRE KEY-VAL BLK ---------------- */
    p_key_val = (HTTPs_KEY_VAL *)Mem_DynPoolBlkGet(&p_instance->PoolKeyVal,
                                                   &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_MEM_NO_SPACE;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrKeyValPoolMemSpaceCtr);
             goto exit;

        default:
            *p_err = HTTPs_ERR_QUERY_STR_POOL_LIB_FAULT;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrKeyValPoolLibGetCtr);
             goto exit;
    }

                                                                /* -------------- ACQUIRE KEY STRING BLK -------------- */
    p_key_val->KeyPtr = (CPU_CHAR *)Mem_DynPoolBlkGet(&p_instance->PoolQueryStrKeyStr,
                                                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_MEM_NO_SPACE;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrQueryStrKeyPoolMemSpaceCtr);
             goto exit_release_blk;

        default:
            *p_err = HTTPs_ERR_QUERY_STR_POOL_LIB_FAULT;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrQueryStrKeyPoolLibGetCtr);
             goto exit_release_blk;
    }

                                                                /* -------------- ACQUIRE VAL STRING BLK -------------- */
    p_key_val->ValPtr = (CPU_CHAR *)Mem_DynPoolBlkGet(&p_instance->PoolQueryStrValStr,
                                                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_MEM_NO_SPACE;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrQueryStrValPoolMemSpaceCtr);
             goto exit_release_key;

        default:
            *p_err = HTTPs_ERR_QUERY_STR_POOL_LIB_FAULT;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrQueryStrValPoolLibGetCtr);
             goto exit_release_key;
    }

    p_conn->QueryStrBlkAcquiredCtr++;

    HTTPs_STATS_INC(p_ctr_stats->Req_StatKeyValAcquiredCtr);

   *p_err = HTTPs_ERR_NONE;

    goto exit;


exit_release_key:
    Mem_DynPoolBlkFree(&p_instance->PoolQueryStrKeyStr,         /* Key String block previously acquired.                */
                        p_key_val->KeyPtr,
                       &err_lib);
    p_key_val->KeyPtr = DEF_NULL;

exit_release_blk:
    Mem_DynPoolBlkFree(&p_instance->PoolKeyVal,                 /* Key-Val block previously acquired.                   */
                        p_key_val,
                       &err_lib);
    p_key_val = DEF_NULL;

exit:
    return (p_key_val);
}
#endif


/*
*********************************************************************************************************
*                                HTTPsMem_QueryStrKeyValBlkReleaseAll()
*
* Description : (1) Release the entire Key-Value pairs list associated with Query String received:
*
*                   (a) Release Key-value value block
*                   (b) Release Key-value key block
*                   (c) Release Key-value block
*
*
* Argument(s) : p_instance  Pointer to the instance used to acquire a connection.
*                           Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*                           Argument validated in HTTPsSock_ConnAccept().
*
* Return(s)   : None.
*
* Caller(s)   : HTTPsConn_Close(),
*               HTTPsMem_ConnClr().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (HTTPs_CFG_QUERY_STR_EN == DEF_ENABLED)
void  HTTPsMem_QueryStrKeyValBlkReleaseAll (HTTPs_INSTANCE  *p_instance,
                                            HTTPs_CONN      *p_conn)
{
    HTTPs_KEY_VAL         *p_key_val;
    HTTPs_KEY_VAL         *p_key_val_next;
    HTTPs_INSTANCE_STATS  *p_ctr_stats;
    HTTPs_INSTANCE_ERRS   *p_ctr_err;
    LIB_ERR                err_lib;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

    p_key_val = p_conn->QueryStrListPtr;
    while (p_key_val != DEF_NULL) {

        p_key_val_next     = p_key_val->NextPtr;
        p_key_val->NextPtr = DEF_NULL;

                                                                /* ----------------- RELEASE VAL BLK ------------------ */
        Mem_DynPoolBlkFree(&p_instance->PoolQueryStrValStr,
                            p_key_val->ValPtr,
                           &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            HTTPs_ERR_INC(p_ctr_err->Req_ErrQueryStrValPoolLibFreeCtr);
        }

        p_key_val->ValPtr = DEF_NULL;

                                                                /* ----------------- RELEASE KEY BLK ------------------ */
        Mem_DynPoolBlkFree(&p_instance->PoolQueryStrKeyStr,
                            p_key_val->KeyPtr,
                           &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            HTTPs_ERR_INC(p_ctr_err->Req_ErrQueryStrKeyPoolLibFreeCtr);
        }

        p_key_val->KeyPtr = DEF_NULL;

                                                                /* --------------- RELEASE KEY-VAL BLK ---------------- */
        Mem_DynPoolBlkFree(&p_instance->PoolKeyVal,
                            p_key_val,
                           &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            HTTPs_ERR_INC(p_ctr_err->Req_ErrKeyValPoolLibFreeCtr);
        }

        p_key_val = p_key_val_next;

        p_conn->QueryStrBlkAcquiredCtr--;

        HTTPs_STATS_INC(p_ctr_stats->Req_StatKeyValReleaseCtr);
    }

    p_conn->QueryStrListPtr = DEF_NULL;

}
#endif


/*
*********************************************************************************************************
*                                       HTTPsMem_FormKeyValBlkGet()
*
* Description : (1) Acquire a new Key-Value Pair block for an HTTP Form received:
*
*                   (a) Acquire Key-Value block
*                   (b) Acquire Key string block
*                   (c) Acquire Value string block
*
* Argument(s) : p_instance  Pointer to the instance used to acquire a connection.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*               ----------  Argument validated in HTTPsSock_ConnAccept().
*
* Return(s)   : Pointer to the Key-Value block, if no error(s).
*               Null pointer,                   otherwise.
*
* Caller(s)   : HTTPsReq_BodyFormAppKeyValBlkAdd(),
*               HTTPsReq_BodyFormMultipartCtrlParse().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (HTTPs_CFG_FORM_EN == DEF_ENABLED)
HTTPs_KEY_VAL  *HTTPsMem_FormKeyValBlkGet (HTTPs_INSTANCE  *p_instance,
                                           HTTPs_CONN      *p_conn,
                                           HTTPs_ERR       *p_err)
{
    HTTPs_KEY_VAL         *p_key_val;
    HTTPs_INSTANCE_STATS  *p_ctr_stats;
    HTTPs_INSTANCE_ERRS   *p_ctr_err;
    LIB_ERR                err_lib;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

                                                                /* --------------- ACQUIRE KEY-VAL BLK ---------------- */
    p_key_val = (HTTPs_KEY_VAL *)Mem_DynPoolBlkGet(&p_instance->PoolKeyVal,
                                                   &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_MEM_NO_SPACE;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrKeyValPoolMemSpaceCtr);
             goto exit;

        default:
            *p_err = HTTPs_ERR_FORM_POOL_LIB_FAULT;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrKeyValPoolLibGetCtr);
             goto exit;
    }

                                                                /* -------------- ACQUIRE KEY STRING BLK -------------- */
    p_key_val->KeyPtr = (CPU_CHAR *)Mem_DynPoolBlkGet(&p_instance->PoolFormKeyStr,
                                                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_MEM_NO_SPACE;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrFormKeyPoolMemSpaceCtr);
             goto exit_release_blk;

        default:
            *p_err = HTTPs_ERR_FORM_POOL_LIB_FAULT;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrFormKeyPoolLibGetCtr);
             goto exit_release_blk;
    }

                                                                /* -------------- ACQUIRE VAL STRING BLK -------------- */
    p_key_val->ValPtr = (CPU_CHAR *)Mem_DynPoolBlkGet(&p_instance->PoolFormValStr,
                                                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_MEM_NO_SPACE;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrFormValPoolMemSpaceCtr);
             goto exit_release_key;

        default:
            *p_err = HTTPs_ERR_FORM_POOL_LIB_FAULT;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrFormValPoolLibGetCtr);
             goto exit_release_key;
    }

    p_conn->FormBlkAcquiredCtr++;

    HTTPs_STATS_INC(p_ctr_stats->Req_StatKeyValAcquiredCtr);

   *p_err = HTTPs_ERR_NONE;

    goto exit;


exit_release_key:
    Mem_DynPoolBlkFree(&p_instance->PoolFormKeyStr,             /* Key String block previously acquired.                */
                        p_key_val->KeyPtr,
                       &err_lib);
    p_key_val->KeyPtr = DEF_NULL;

exit_release_blk:
    Mem_DynPoolBlkFree(&p_instance->PoolKeyVal,                 /* Key-Val block previously acquired.                   */
                        p_key_val,
                       &err_lib);
    p_key_val = DEF_NULL;

exit:
    return (p_key_val);
}
#endif


/*
*********************************************************************************************************
*                                  HTTPsMem_FormKeyValBlkReleaseAll()
*
* Description : (1) Release the entire Key-Value pairs list associated with Form received:
*
*                   (a) Release Key-value value block
*                   (b) Release Key-value key block
*                   (c) Release Key-value block
*
* Argument(s) : p_instance  Pointer to the instance used to acquire a connection.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*               ----------  Argument validated in HTTPsSock_ConnAccept().
*
* Return(s)   : none.
*
* Caller(s)   : HTTPsConn_Close(),
*               HTTPsMem_ConnClr().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (HTTPs_CFG_FORM_EN == DEF_ENABLED)
void  HTTPsMem_FormKeyValBlkReleaseAll (HTTPs_INSTANCE  *p_instance,
                                        HTTPs_CONN      *p_conn)
{
    HTTPs_KEY_VAL         *p_key_val;
    HTTPs_KEY_VAL         *p_key_val_next;
    HTTPs_INSTANCE_STATS  *p_ctr_stats;
    HTTPs_INSTANCE_ERRS   *p_ctr_err;
    LIB_ERR                err_lib;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

    p_key_val = p_conn->FormDataListPtr;
    while (p_key_val != DEF_NULL) {

        p_key_val_next     = p_key_val->NextPtr;
        p_key_val->NextPtr = DEF_NULL;

                                                                /* ----------------- RELEASE VAL BLK ------------------ */
        Mem_DynPoolBlkFree(&p_instance->PoolFormValStr,
                            p_key_val->ValPtr,
                           &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            HTTPs_ERR_INC(p_ctr_err->Req_ErrFormValPoolLibFreeCtr);
        }

        p_key_val->ValPtr = DEF_NULL;

                                                                /* ----------------- RELEASE KEY BLK ------------------ */
        Mem_DynPoolBlkFree(&p_instance->PoolFormKeyStr,
                            p_key_val->KeyPtr,
                           &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            HTTPs_ERR_INC(p_ctr_err->Req_ErrFormKeyPoolLibFreeCtr);
        }

        p_key_val->KeyPtr = DEF_NULL;

                                                                /* --------------- RELEASE KEY-VAL BLK ---------------- */
        Mem_DynPoolBlkFree(&p_instance->PoolKeyVal,
                            p_key_val,
                           &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            HTTPs_ERR_INC(p_ctr_err->Req_ErrKeyValPoolLibFreeCtr);
        }

        p_key_val = p_key_val_next;

        p_conn->FormBlkAcquiredCtr--;

        HTTPs_STATS_INC(p_ctr_stats->Req_StatKeyValReleaseCtr);
    }

    p_conn->FormDataListPtr = DEF_NULL;

}
#endif


/*
*********************************************************************************************************
*                                         HTTPsMem_ReqHdrGet()
*
* Description : (1) Acquire a new request header block :
*
*                   (a) Acquire request header block
*                   (b) Acquire the buffer block depending of the value data type
*                   (c) Update request header block
*                   (d) Initialize header block parameters
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*               ------      Argument validated in HTTPsSock_ConnAccept().
*
*               hdr_field   Type of the request header
*
*               val_type    Data type of the request header value
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                            HTTPs_ERR_NONE                         Request header block successfully acquired.
*
*                           HTTPs_ERR_REQ_HDR_POOL_EMPTY            Request header pool is empty
*                           HTTPs_ERR_REQ_HDR_POOL_LIB_ERR          Memory lib error occurred, when acquiring header block.
*
*                           HTTPs_ERR_REQ_HDR_BUF_POOL_EMPTY        Request data buffer pool is empty
*                           HTTPs_ERR_REQ_HDR_BUF_POOL_LIB_ERR      Memory lib error occurred, when acquiring data buffer.
*
*                           HTTPs_ERR_REQ_HDR_TYPE_NOT_SUPPORTED    Data type not yet supported.
*                           HTTPs_ERR_REQ_HDR_TYPE_UNKNOWN          Data type is unknown.
*
* Return(s)   : Pointer to the request header block, if no error(s).
*
*               Null pointer,                        otherwise.
*
* Caller(s)   : HTTPsReq_HdrParse().
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (HTTPs_CFG_HDR_RX_EN == DEF_ENABLED)
HTTP_HDR_BLK  *HTTPsMem_ReqHdrGet(HTTPs_INSTANCE     *p_instance,
                                  HTTPs_CONN         *p_conn,
                                  HTTP_HDR_FIELD      hdr_field,
                                  HTTP_HDR_VAL_TYPE   val_type,
                                  HTTPs_ERR          *p_err)
{
    HTTP_HDR_BLK          *p_req_hdr_blk;
    HTTPs_INSTANCE_STATS  *p_ctr_stats;
    HTTPs_INSTANCE_ERRS   *p_ctr_err;
    LIB_ERR                err_lib;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

                                                                /* ------------ ACQUIRE RESP HDR FIELD BLK ------------ */
    p_req_hdr_blk = (HTTP_HDR_BLK *)Mem_DynPoolBlkGet(&p_instance->PoolReqHdr,
                                                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_POOL_EMPTY:
            *p_err = HTTPs_ERR_REQ_HDR_POOL_EMPTY;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrHdrPoolEmptyCtr);
             return (DEF_NULL);

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_MEM_NO_SPACE;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrHdrPoolMemSpaceCtr);
             return (DEF_NULL);

        default:
            *p_err = HTTPs_ERR_REQ_HDR_POOL_LIB_FAULT;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrHdrPoolLibGetCtr);
             return (DEF_NULL);
    }


    switch (val_type) {
        case HTTP_HDR_VAL_TYPE_NONE:
             p_req_hdr_blk->ValPtr = DEF_NULL;
             break;


        case HTTP_HDR_VAL_TYPE_STR_DYN:
             p_req_hdr_blk->ValPtr = Mem_DynPoolBlkGet(&p_instance->PoolReqHdrStr,
                                                       &err_lib);
             switch (err_lib) {
                 case LIB_MEM_ERR_NONE:
                      break;

                 case LIB_MEM_ERR_POOL_EMPTY:
                     *p_err = HTTPs_ERR_REQ_HDR_POOL_EMPTY;
                      HTTPs_ERR_INC(p_ctr_err->Req_ErrHdrBufPoolEmptyCtr);
                      break;

                 case LIB_MEM_ERR_SEG_OVF:
                     *p_err = HTTPs_ERR_MEM_NO_SPACE;
                      HTTPs_ERR_INC(p_ctr_err->Req_ErrHdrBufPoolMemSpaceCtr);
                      return (DEF_NULL);

                 default:
                     *p_err = HTTPs_ERR_REQ_HDR_POOL_LIB_FAULT;
                      HTTPs_ERR_INC(p_ctr_err->Req_ErrHdrBufPoolLibGetCtr);
                      break;
             }

             if (err_lib != LIB_MEM_ERR_NONE) {
                 Mem_DynPoolBlkFree(&p_instance->PoolReqHdr,    /* Release hdr previously acquired.                    */
                                     p_req_hdr_blk,
                                    &err_lib);
                 return (DEF_NULL);
             }
             break;


        default:
            *p_err = HTTPs_ERR_REQ_HDR_DATA_TYPE_UNKNOWN;
             HTTPs_ERR_INC(p_ctr_err->Req_ErrHdrValTypeUnknown);
             Mem_DynPoolBlkFree(&p_instance->PoolReqHdr,
                                 p_req_hdr_blk,
                                &err_lib);
             return (DEF_NULL);
    }


                                                                /* ----------- UPDATE RESP HDR FIELD LIST ------------- */
    if (p_conn->HdrListPtr == DEF_NULL) {
        p_req_hdr_blk->NextPtr = DEF_NULL;
    } else {
        p_req_hdr_blk->NextPtr = p_conn->HdrListPtr;
    }
    p_conn->HdrListPtr = p_req_hdr_blk;

                                                                /* ----------- UPDATE RESP HDR FIELD PARAM ------------ */
    p_req_hdr_blk->HdrField = hdr_field;
    p_req_hdr_blk->ValType  = val_type;
    p_req_hdr_blk->ValLen   = 0;

    p_conn->HdrCtr++;

    HTTPs_STATS_INC(p_ctr_stats->Req_StatHdrAcquiredCtr);


   *p_err = HTTPs_ERR_NONE;

    return (p_req_hdr_blk);
}
#endif


/*
*********************************************************************************************************
*                                       HTTPsMem_ReqHdrRelease()
*
* Description : (1) Release a request header block
*
*                   (a) Update  request header block  list
*                   (b) Release header  value  buffer block
*                   (c) Release request header block
*
* Argument(s) : p_instance              Pointer to the instance.
*               ----------              Argument validated in HTTPs_InstanceStart().
*
*               p_conn                  Pointer to the connection.
*               ------                  Argument validated in HTTPsSock_ConnAccept().
*
* Return(s)   : none.
*
* Caller(s)   : HTTPsConn_Close(),
*               HTTPsReq_Handle().
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (HTTPs_CFG_HDR_RX_EN == DEF_ENABLED)
void  HTTPsMem_ReqHdrRelease (HTTPs_INSTANCE  *p_instance,
                              HTTPs_CONN      *p_conn)
{
    HTTP_HDR_BLK          *p_req_hdr_blk;
    HTTPs_INSTANCE_STATS  *p_ctr_stats;
    HTTPs_INSTANCE_ERRS   *p_ctr_err;
    LIB_ERR                err_lib;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

                                                                /* ----------- UPDATE REQ HDR FIELD LIST -------------- */
    p_req_hdr_blk = p_conn->HdrListPtr;

    if (p_req_hdr_blk == DEF_NULL) {
        HTTPs_ERR_INC(p_ctr_err->Req_ErrHdrPtrNullCtr);
        return;
    } else {
        p_conn->HdrListPtr     = p_req_hdr_blk->NextPtr;
        p_req_hdr_blk->NextPtr = DEF_NULL;
    }

    switch(p_req_hdr_blk->ValType) {
        case HTTP_HDR_VAL_TYPE_STR_DYN:
                                                                /* -------------- RELEASE STR DATA BLK ---------------- */
             Mem_DynPoolBlkFree(&p_instance->PoolReqHdrStr,
                                 p_req_hdr_blk->ValPtr,
                                &err_lib);
             if (err_lib != LIB_MEM_ERR_NONE) {
                 HTTPs_ERR_INC(p_ctr_err->Req_ErrHdrBufPoolLibFreeCtr);
                 return;
             }
             p_req_hdr_blk->ValPtr = DEF_NULL;
             break;


        case HTTP_HDR_VAL_TYPE_NONE:
        case HTTP_HDR_VAL_TYPE_STR_CONST:
        default:
             break;
    }

                                                                /* ----------- RELEASE RESP HDR FIELD BLK ------------- */
    Mem_DynPoolBlkFree(&p_instance->PoolReqHdr,
                        p_req_hdr_blk,
                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        HTTPs_ERR_INC(p_ctr_err->Req_ErrHdrPoolLibFreeCtr);
        return;
    }

    p_conn->HdrCtr--;

    HTTPs_STATS_INC(p_ctr_stats->Req_StatHdrReleaseCtr);

}
#endif


/*
*********************************************************************************************************
*                                        HTTPsMem_RespHdrGet()
*
* Description : (1) Acquire a new Response Header Field block :
*
*                   (a) Acquire response header block
*                   (b) Acquire the buffer block depending on the value data type
*                   (c) Update response header block list
*                   (d) Initialize header block parameters
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*               ------      Argument validated in HTTPsSock_ConnAccept().
*
*               hdr_field   Type of the response header field
*
*               val_type    Data type of the response header field value
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                           HTTPs_ERR_NONE                          Response header block successfully acquired.
*
*                           HTTPs_ERR_RESP_HDR_OVERFLOW             Exceeded maximum authorized block per connection.
*
*                           HTTPs_ERR_RESP_HDR_POOL_EMPTY           Response header pool is empty
*                           HTTPs_ERR_RESP_HDR_POOL_LIB_ERR         Memory lib error occurred, when acquiring header block.
*
*                           HTTPs_ERR_RESP_HDR_BUF_POOL_EMPTY       Response data buffer pool is empty
*                           HTTPs_ERR_RESP_HDR_BUF_POOL_LIB_ERR     Memory lib error occurred, when acquiring data buffer.
*
*                           HTTPs_ERR_RESP_HDR_TYPE_NOT_SUPPORTED   Data type not yet supported.
*                           HTTPs_ERR_RESP_HDR_TYPE_UNKNOWN         Data type is unknown.
*
* Return(s)   : pointer to the response header block, if no error(s).
*
*               Null pointer,                         otherwise.
*
* Caller(s)   : HTTPs_RespHdrGet().
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (HTTPs_CFG_HDR_TX_EN == DEF_ENABLED)
HTTP_HDR_BLK  *HTTPsMem_RespHdrGet(HTTPs_INSTANCE     *p_instance,
                                   HTTPs_CONN         *p_conn,
                                   HTTP_HDR_FIELD      hdr_field,
                                   HTTP_HDR_VAL_TYPE   val_type,
                                   HTTPs_ERR          *p_err)
{
    const  HTTPs_CFG             *p_cfg;
           HTTP_HDR_BLK          *p_resp_hdr_blk;
           HTTPs_INSTANCE_STATS  *p_ctr_stats;
           HTTPs_INSTANCE_ERRS   *p_ctr_err;
           LIB_ERR                err_lib;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

    p_cfg = p_instance->CfgPtr;

    if (p_cfg->HdrTxCfgPtr == DEF_NULL) {
       *p_err = HTTPs_ERR_CFG_NULL_PTR_RESP_HDR;
        return (DEF_NULL);
    }

    if (p_cfg->HdrTxCfgPtr->NbrPerConnMax != LIB_MEM_BLK_QTY_UNLIMITED) {
        if (p_conn->HdrCtr >= p_cfg->HdrTxCfgPtr->NbrPerConnMax) {
           *p_err = HTTPs_ERR_RESP_HDR_OVERFLOW;
            return (DEF_NULL);
        }
    }

                                                                /* ------------ ACQUIRE RESP HDR FIELD BLK ------------ */
    p_resp_hdr_blk = (HTTP_HDR_BLK *)Mem_DynPoolBlkGet(&p_instance->PoolRespHdr,
                                                       &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_POOL_EMPTY:
             HTTPs_ERR_INC(p_ctr_err->Resp_ErrHdrPoolEmptyCtr);
            *p_err = HTTPs_ERR_RESP_HDR_POOL_EMPTY;
             return (DEF_NULL);

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_MEM_NO_SPACE;
             HTTPs_ERR_INC(p_ctr_err->Resp_ErrHdrPoolMemSpaceCtr);
             return (DEF_NULL);

        default:
             HTTPs_ERR_INC(p_ctr_err->Resp_ErrHdrPoolLibGetCtr);
            *p_err = HTTPs_ERR_RESP_HDR_POOL_LIB_FAULT;
             return (DEF_NULL);
    }


    switch (val_type) {
        case HTTP_HDR_VAL_TYPE_NONE:
        case HTTP_HDR_VAL_TYPE_STR_CONST:
             p_resp_hdr_blk->ValPtr = DEF_NULL;
             break;


        case HTTP_HDR_VAL_TYPE_STR_DYN:
             p_resp_hdr_blk->ValPtr = Mem_DynPoolBlkGet(&p_instance->PoolRespHdrStr,
                                                        &err_lib);
             switch (err_lib) {
                 case LIB_MEM_ERR_NONE:
                      break;

                 case LIB_MEM_ERR_POOL_EMPTY:
                     *p_err = HTTPs_ERR_RESP_HDR_POOL_EMPTY;
                      HTTPs_ERR_INC(p_ctr_err->Resp_ErrHdrBufPoolEmptyCtr);
                      break;

                 case LIB_MEM_ERR_SEG_OVF:
                     *p_err = HTTPs_ERR_MEM_NO_SPACE;
                      HTTPs_ERR_INC(p_ctr_err->Resp_ErrHdrBufPoolMemSpaceCtr);
                      return (DEF_NULL);

                default:
                     *p_err = HTTPs_ERR_RESP_HDR_POOL_LIB_FAULT;
                      HTTPs_ERR_INC(p_ctr_err->Resp_ErrHdrBufPoolLibGetCtr);
                      break;
             }

             if (err_lib != LIB_MEM_ERR_NONE) {
                 Mem_DynPoolBlkFree(&p_instance->PoolRespHdr,   /* Release block previously acquired.                   */
                                     p_resp_hdr_blk,
                                    &err_lib);
                 return (DEF_NULL);
             }
             break;


        default :
             HTTPs_ERR_INC(p_ctr_err->Resp_ErrHdrValTypeUnknown);
            *p_err = HTTPs_ERR_RESP_HDR_DATA_TYPE_UNKNOWN;
             Mem_DynPoolBlkFree(&p_instance->PoolRespHdr,
                                 p_resp_hdr_blk,
                                &err_lib);
             return (DEF_NULL);

    }


                                                                /* ----------- UPDATE RESP HDR FIELD LIST ------------- */
    if (p_conn->HdrListPtr == DEF_NULL) {
        p_resp_hdr_blk->NextPtr = DEF_NULL;
    } else {
        p_resp_hdr_blk->NextPtr = p_conn->HdrListPtr;
    }
    p_conn->HdrListPtr = p_resp_hdr_blk;


                                                                /* ----------- UPDATE RESP HDR FIELD PARAM ------------ */
    p_resp_hdr_blk->HdrField = hdr_field;
    p_resp_hdr_blk->ValType  = val_type;
    p_resp_hdr_blk->ValLen   = 0;

    p_conn->HdrCtr++;

    HTTPs_STATS_INC(p_ctr_stats->Resp_StatHdrAcquiredCtr);

   *p_err = HTTPs_ERR_NONE;

    return (p_resp_hdr_blk);
}
#endif


/*
*********************************************************************************************************
*                                      HTTPsMem_RespHdrRelease()
*
* Description : (1) Release a Response Header Field block
*
*                   (a) Update response header block list
*                   (b) Release header field value buffer block
*                   (c) Release response header Field block
*
* Argument(s) : p_instance      Pointer to the instance.
*               ----------      Argument validated in HTTPs_InstanceStart().
*
*               p_conn          Pointer to the connection.
*               ------          Argument validated in HTTPsSock_ConnAccept().
*
* Return(s)   : none.
*
* Caller(s)   : HTTPsConn_Close(),
*               HTTPsResp_HdrFieldAdd().
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (HTTPs_CFG_HDR_TX_EN == DEF_ENABLED)
void  HTTPsMem_RespHdrRelease (HTTPs_INSTANCE  *p_instance,
                               HTTPs_CONN      *p_conn)
{

    HTTP_HDR_BLK          *p_resp_hdr_blk;
    HTTPs_INSTANCE_STATS  *p_ctr_stats;
    HTTPs_INSTANCE_ERRS   *p_ctr_err;
    LIB_ERR                err_lib;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

                                                                /* ----------- UPDATE RESP HDR FIELD LIST ------------- */
    p_resp_hdr_blk = p_conn->HdrListPtr;

    if (p_resp_hdr_blk == DEF_NULL) {
        HTTPs_ERR_INC(p_ctr_err->Resp_ErrHdrPtrNullCtr);
        return;
    } else {
        p_conn->HdrListPtr      = p_resp_hdr_blk->NextPtr;
        p_resp_hdr_blk->NextPtr = DEF_NULL;
    }


    switch(p_resp_hdr_blk->ValType) {
        case HTTP_HDR_VAL_TYPE_STR_DYN:
                                                                /* -------------- RELEASE STR DATA BLK ---------------- */
             Mem_DynPoolBlkFree(&p_instance->PoolRespHdrStr,
                                 p_resp_hdr_blk->ValPtr,
                                &err_lib);
             if (err_lib != LIB_MEM_ERR_NONE) {
                 HTTPs_ERR_INC(p_ctr_err->Resp_ErrHdrBufPoolLibFreeCtr);
                 return;
             }
             p_resp_hdr_blk->ValPtr = DEF_NULL;
             break;

        case HTTP_HDR_VAL_TYPE_NONE:
        case HTTP_HDR_VAL_TYPE_STR_CONST:
        default:
            break;
    }

                                                                /* ----------- RELEASE RESP HDR FIELD BLK ------------- */
    Mem_DynPoolBlkFree(&p_instance->PoolRespHdr,
                        p_resp_hdr_blk,
                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        HTTPs_ERR_INC(p_ctr_err->Resp_ErrHdrPoolLibFreeCtr);
        return;
    }

    p_conn->HdrCtr--;

    HTTPs_STATS_INC(p_ctr_stats->Resp_StatHdrReleaseCtr);
}
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      HTTPsMem_ReqHdrPoolInit()
*
* Description : (1) Initialize the HTTP server connection request header block pool :
*
*                   (a) Validate remaining memory available
*                   (b) Create request header block pool
*                   (c) Create header value buffers  pool(s)
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                            HTTPs_ERR_NONE                             Request header pool(s) successfully initialized.
*
*                            HTTPs_ERR_INSTANCE_INIT_POOL_REM_MEM       Not enough remaining memory space
*                            HTTPs_ERR_INSTANCE_INIT_POOL_HDR_REQ       Memory lib error occurred,
*                                                                           when initializing request pool
*                            HTTPs_ERR_INSTANCE_INIT_POOL_REQ_HDR_STR   Memory lib error occurred,
*                                                                           when initializing request string pool
*
* Return(s)   : none.
*
* Caller(s)   : HTTPsMem_ConnPoolInit().
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (HTTPs_CFG_HDR_RX_EN == DEF_ENABLED)
static  void  HTTPsMem_ReqHdrPoolInit (HTTPs_INSTANCE  *p_instance,
                                       HTTPs_ERR       *p_err)
{
    const  HTTPs_CFG   *p_cfg;
#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
           CPU_SIZE_T   octets_rem;
#endif
           CPU_SIZE_T   octets_reqd;
           CPU_SIZE_T   pool_size_max;
           LIB_ERR      err_lib;


    p_cfg = p_instance->CfgPtr;

    if (p_cfg->HdrRxCfgPtr == DEF_NULL) {
       *p_err = HTTPs_ERR_CFG_NULL_PTR_REQ_HDR;
        return;
    }

#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    octets_reqd = (HTTPs_CFG_POOLS_INIT_NBR * sizeof(HTTP_HDR_BLK))
                + (HTTPs_CFG_POOLS_INIT_NBR * p_cfg->HdrRxCfgPtr->DataLenMax);

                                                                /* Get and validate rem space avail on heap.            */
    octets_rem = Mem_SegRemSizeGet(HTTPs_MemSegPtr,
                                   sizeof(CPU_SIZE_T),
                                   DEF_NULL,
                                  &err_lib);

    if (octets_rem < octets_reqd) {
       *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
        return;
    }
#endif

    if (p_cfg->HdrRxCfgPtr->NbrPerConnMax != LIB_MEM_BLK_QTY_UNLIMITED) {
        pool_size_max = p_cfg->ConnNbrMax * p_cfg->HdrRxCfgPtr->NbrPerConnMax;
    } else {
        pool_size_max = LIB_MEM_BLK_QTY_UNLIMITED;
    }

                                                                /* ------------ CREATE REQ HDR FIELD POOL ------------- */
    Mem_DynPoolCreate("HTTPs Req Hdr Pool",
                      &p_instance->PoolReqHdr,
                       HTTPs_MemSegPtr,
                       sizeof(HTTP_HDR_BLK),
                       sizeof(CPU_SIZE_T),
                       HTTPs_CFG_POOLS_INIT_NBR,
                       pool_size_max,
                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
             return;

        default:
            *p_err = HTTPs_ERR_INIT_POOL_REQ_HDR;
             return;
    }

                                                                /* ---------- CREATE STR POOL FOR HDR FIELD ----------- */
    Mem_DynPoolCreate("HTTPs Req Hdr Str Pool",
                      &p_instance->PoolReqHdrStr,
                       HTTPs_MemSegPtr,
                       p_cfg->HdrRxCfgPtr->DataLenMax,
                       sizeof(CPU_SIZE_T),
                       HTTPs_CFG_POOLS_INIT_NBR,
                       pool_size_max,
                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
             return;

        default:
            *p_err = HTTPs_ERR_INIT_POOL_REQ_HDR_STR;
             return;
    }

   (void)octets_reqd;

   *p_err = HTTPs_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                      HTTPsMem_RespHdrPoolInit()
*
* Description : (1) Initialize the HTTP server connection response header block pool :
*
*                   (a) Validate remaining memory available
*                   (b) Create response header field block pool
*                   (c) Create header value buffers pool
*
* Argument(s) : p_instance      Pointer to the instance.
*               ----------      Argument validated in HTTPs_InstanceStart().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                            HTTPs_ERR_NONE                             Response header pool(s) successfully initialized.
*
*                            HTTPs_ERR_INSTANCE_INIT_POOL_REM_MEM       Not enough remaining memory space
*                            HTTPs_ERR_INSTANCE_INIT_POOL_HDR_RESP      Memory lib error occurred,
*                                                                           when initializing response pool
*                            HTTPs_ERR_INSTANCE_INIT_POOL_HDR_RESP_STR  Memory lib error occurred,
*                                                                            when initializing response string pool
*
* Return(s)   : none.
*
* Caller(s)   : HTTPsMem_ConnPoolInit().
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (HTTPs_CFG_HDR_TX_EN == DEF_ENABLED)
static  void  HTTPsMem_RespHdrPoolInit(HTTPs_INSTANCE  *p_instance,
                                       HTTPs_ERR       *p_err)
{
     const  HTTPs_CFG   *p_cfg;
#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
            CPU_SIZE_T   octets_rem;
#endif
            CPU_SIZE_T   octets_reqd;
            CPU_SIZE_T   pool_size_max;
            LIB_ERR      err_lib;


    p_cfg = p_instance->CfgPtr;

    if (p_cfg->HdrTxCfgPtr == DEF_NULL) {
       *p_err = HTTPs_ERR_CFG_NULL_PTR_RESP_HDR;
        return;
    }

#if (HTTPs_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    octets_reqd = (HTTPs_CFG_POOLS_INIT_NBR * sizeof(HTTP_HDR_BLK))
                + (HTTPs_CFG_POOLS_INIT_NBR * p_cfg->HdrTxCfgPtr->DataLenMax);

                                                                /* Get and validate rem space avail on heap.            */
    octets_rem = Mem_SegRemSizeGet(HTTPs_MemSegPtr,
                                   sizeof(CPU_SIZE_T),
                                   DEF_NULL,
                                  &err_lib);

    if (octets_rem < octets_reqd) {
       *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
        return;
    }
#endif

    if (p_cfg->HdrTxCfgPtr->NbrPerConnMax != LIB_MEM_BLK_QTY_UNLIMITED) {
        pool_size_max = p_cfg->ConnNbrMax * p_cfg->HdrTxCfgPtr->NbrPerConnMax;
    } else {
        pool_size_max = LIB_MEM_BLK_QTY_UNLIMITED;
    }

                                                                /* ------------ CREATE RESP HDR FIELD POOL ------------ */
    Mem_DynPoolCreate("HTTPs Resp Hdr Pool",
                      &p_instance->PoolRespHdr,
                       HTTPs_MemSegPtr,
                       sizeof(HTTP_HDR_BLK),
                       sizeof(CPU_SIZE_T),
                       HTTPs_CFG_POOLS_INIT_NBR,
                       pool_size_max,
                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
             return;

        default:
            *p_err = HTTPs_ERR_INIT_POOL_RESP_HDR;
             return;
    }

                                                                /* ---------- CREATE STR POOL FOR HDR FIELD ----------- */
    Mem_DynPoolCreate("HTTPs Resp Hdr Str Pool",
                      &p_instance->PoolRespHdrStr,
                       HTTPs_MemSegPtr,
                       p_cfg->HdrTxCfgPtr->DataLenMax,
                       sizeof(CPU_SIZE_T),
                       HTTPs_CFG_POOLS_INIT_NBR,
                       pool_size_max,
                      &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             break;

        case LIB_MEM_ERR_SEG_OVF:
            *p_err = HTTPs_ERR_INIT_POOL_MEM_NO_SPACE;
             return;

        default:
            *p_err = HTTPs_ERR_INIT_POOL_RESP_HDR_STR;
             return;
    }


   (void)octets_reqd;

   *p_err = HTTPs_ERR_NONE;
}
#endif
