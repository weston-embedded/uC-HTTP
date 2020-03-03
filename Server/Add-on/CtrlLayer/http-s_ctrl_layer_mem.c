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
*                                     HTTPs CONTROL LAYER MEMORY
*
* Filename : http-s_ctrl_layer_mem.c
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

#define   MICRIUM_SOURCE
#define   HTTPs_CTRL_LAYER_MEM_MODULE
#include "http-s_ctrl_layer_mem.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPs_CTRL_LAYER_MEM_CACHE_LINE_LEN        32u


/*
*********************************************************************************************************
*                                   HTTPsCtrlLayerMem_InstDataAlloc()
*
* Description : Dynamically allocate a control layer's instance data structure.
*
* Argument(s) : None.
*
* Return(s)   : Pointer to Control Layer instance data object.
*               DEF_NULL if the structure couldn't be allocated.
*
* Caller(s)   : HTTPsCtrlLayer_OnInstanceInit().
*
* Note(s)     : None.
*********************************************************************************************************
*/

HTTPs_CTRL_LAYER_INST_DATA  *HTTPsCtrlLayerMem_InstDataAlloc (void)
{
    HTTPs_CTRL_LAYER_INST_DATA  *p_seg;
    LIB_ERR                      err_lib;


    p_seg = Mem_SegAlloc("CtllLayer_MemSeg",
                          0u,
                          sizeof(HTTPs_CTRL_LAYER_INST_DATA),
                         &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (DEF_NULL);
    }

    return (p_seg);
}


/*
*********************************************************************************************************
*                                  HTTPsCtrlLayerMem_InstDataPoolInit()
*
* Description : Initialize the pool of instance data for substitution.
*
* Argument(s) : p_seg           Pointer to the Ctrl Layer instance data previously created.
*
*               pool_size_max   Size of the pool to allocate.
*
* Return(s)   : DEF_OK, if the pool was initialize successfully.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPsCtrlLayer_OnInstanceInit().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsCtrlLayerMem_InstDataPoolInit (HTTPs_CTRL_LAYER_INST_DATA  *p_seg,
                                                 CPU_SIZE_T                   pool_size_max)
{
    LIB_ERR   err_lib;


    p_seg->InstDataHeadPtr = DEF_NULL;

    Mem_DynPoolCreate("CtrlL_InstData_DynMemPool",
                      &p_seg->InstDataEntryPool,
                       0u,
                       sizeof(HTTPs_CTRL_LAYER_DATA_ENTRY),
                       HTTPs_CTRL_LAYER_MEM_CACHE_LINE_LEN,
                       pool_size_max,
                       pool_size_max,
                      &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   HTTPsCtrlLayerMem_ConnDataPoolInit()
*
* Description : Initialize the connection data pool for Ctrl Layer contextual informations.
*
* Argument(s) : p_seg           Pointer to the Ctrl Layer instance data previously allocated.
*
*               pool_size_max   Maximum size of the pool to allocate. (Number of connection)
*
* Return(s)   : None.
*
* Caller(s)   : HTTPsCtrlLayer_OnInstanceInit().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsCtrlLayerMem_ConnDataPoolInit (HTTPs_CTRL_LAYER_INST_DATA  *p_seg,
                                                 CPU_SIZE_T                   pool_size_max)
{
    LIB_ERR   err_lib;


    Mem_DynPoolCreate("CtrlLayer_ConnData_DynMemPool",
                      &p_seg->ConnDataPool,
                       0u,
                       sizeof(HTTPs_CTRL_LAYER_CONN_DATA),
                       HTTPs_CTRL_LAYER_MEM_CACHE_LINE_LEN,
                       pool_size_max,
                       pool_size_max,
                      &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                  HTTPsCtrlLayerMem_ConnDataEntryPoolInit()
*
* Description : Initialize the pool of connection data for sub layer substitution.
*
* Argument(s) : p_seg             Pointer to Control Layer Instance data previously allocated.
*
*               pool_size_max     Maximum size of the pool to allocate. (Nb conn * (Max(nbAuth) + 1))
*
* Return(s)   : None.
*
* Caller(s)   : HTTPsCtrlLayer_OnInstanceInit().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsCtrlLayerMem_ConnDataEntryPoolInit (HTTPs_CTRL_LAYER_INST_DATA  *p_seg,
                                                      CPU_SIZE_T                   pool_size_max)
{
    LIB_ERR   err_lib;


    Mem_DynPoolCreate("CtrlLayer_ConnDataEntry_DynMemPool",
                      &p_seg->ConnDataEntryPool,
                       0u,
                       sizeof(HTTPs_CTRL_LAYER_DATA_ENTRY),
                       HTTPs_CTRL_LAYER_MEM_CACHE_LINE_LEN,
                       pool_size_max,
                       pool_size_max,
                      &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   HTTPsCtrlLayerMem_InstDataEntryAlloc()
*
* Description : Allocates an instance data entry for substitution.
*
* Argument(s) : p_seg     Pointer to Control Layer instance data previously allocated.
*
* Return(s)   : Pointer to Control Layer data entry object allocated.
*               DEF_NULL  if no data entry could be allocated.
*
* Caller(s)   : HTTPsCtrlLayer_InstDataSet().
*
* Note(s)     : None.
*********************************************************************************************************
*/

HTTPs_CTRL_LAYER_DATA_ENTRY  *HTTPsCtrlLayerMem_InstDataEntryAlloc (HTTPs_CTRL_LAYER_INST_DATA  *p_seg)
{
    HTTPs_CTRL_LAYER_DATA_ENTRY  *p_inst_data_entry;
    LIB_ERR                       err_lib;


    p_inst_data_entry = (HTTPs_CTRL_LAYER_DATA_ENTRY *)Mem_DynPoolBlkGet(&p_seg->InstDataEntryPool,
                                                                         &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             p_inst_data_entry->NextPtr = p_seg->InstDataHeadPtr;
             p_seg->InstDataHeadPtr     = p_inst_data_entry;
             break;

        default:
             return (DEF_NULL);
    }

    return (p_inst_data_entry);
}


/*
*********************************************************************************************************
*                                        CtrlL_Alloc_ConnData()
*
* Description : Allocates a connection data for the Ctrl Layer.
*
* Argument(s) : p_seg     Pointer to Ctrl Layer instance data previously allocated.
*
* Return(s)   : Pointer to Ctrl Layer Connection Data object allocated.
*               DEF_NULL if no ConnData could be allocated.
*
* Caller(s)   : HTTPsCtrlLayer_CreateConnDataIfNull().
*
* Note(s)     : None.
*********************************************************************************************************
*/

HTTPs_CTRL_LAYER_CONN_DATA  *HTTPsCtrlLayerMem_ConnDataAlloc (HTTPs_CTRL_LAYER_INST_DATA  *p_seg)
{
    HTTPs_CTRL_LAYER_CONN_DATA  *p_conn_data;
    LIB_ERR                      err_lib;


    p_conn_data = (HTTPs_CTRL_LAYER_CONN_DATA *)Mem_DynPoolBlkGet(&p_seg->ConnDataPool,
                                                                  &err_lib);

    if (p_conn_data != DEF_NULL) {
        p_conn_data->TargetCfgPtr     = DEF_NULL;
        p_conn_data->TargetAppInstPtr = DEF_NULL;
        p_conn_data->ConnDataHeadPtr  = DEF_NULL;
    }

   (void)err_lib;

    return (p_conn_data);
}


/*
*********************************************************************************************************
*                                       HTTPsCtrlLayer_ConnDataFree()
*
* Description : Frees a given ConnData from the pool.
*
* Argument(s) : p_seg           Pointer to Ctrl Layer instance data previously allocated.
*
*               p_conn_data     Pointer to Ctrl Layer connection data previously allocated.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPsCtrlLayer_OnConnClose(),
*               HTTPsCtrlLayer_OnConnReq().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPsCtrlLayer_ConnDataFree (HTTPs_CTRL_LAYER_INST_DATA  *p_seg,
                                   HTTPs_CTRL_LAYER_CONN_DATA  *p_conn_data)
{
    LIB_ERR  err_lib;


    HTTPsCtrlLayer_ConnDataEntriesFree(p_seg,
                                       p_conn_data);

    Mem_DynPoolBlkFree(        &p_seg->ConnDataPool,
                       (void *) p_conn_data,
                               &err_lib);

    (void)err_lib;
}


/*
*********************************************************************************************************
*                                   HTTPsCtrlLayer_ConnDataEntryAlloc()
*
* Description : Allocates a ConnData for substitution.
*
* Argument(s) : p_seg         Pointer to Ctrl Layer instance data previously allocated.
*
*               p_conn_data   Pointer to Ctrl Layer connection data previously allocated.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPsCtrlLayer_ConnDataEntrySet().
*
* Note(s)     : None.
*********************************************************************************************************
*/

HTTPs_CTRL_LAYER_DATA_ENTRY  *HTTPsCtrlLayer_ConnDataEntryAlloc (HTTPs_CTRL_LAYER_INST_DATA  *p_seg,
                                                                 HTTPs_CTRL_LAYER_CONN_DATA  *p_conn_data)
{
    HTTPs_CTRL_LAYER_DATA_ENTRY  *p_conn_data_entry;
    LIB_ERR                       err_lib;


    p_conn_data_entry = (HTTPs_CTRL_LAYER_DATA_ENTRY *)Mem_DynPoolBlkGet(&p_seg->ConnDataEntryPool,
                                                                         &err_lib);
    switch (err_lib) {
        case LIB_MEM_ERR_NONE:
             p_conn_data_entry->NextPtr   = p_conn_data->ConnDataHeadPtr;
             p_conn_data->ConnDataHeadPtr = p_conn_data_entry;
             break;

        default:
             return (DEF_NULL);
    }

    return (p_conn_data_entry);
}


/*
*********************************************************************************************************
*                                   HTTPsCtrlLayer_ConnDataEntriesFree()
*
* Description : Frees all the ConnDataEntry used for substitution.
*
* Argument(s) : p_seg         Pointer to Ctrl Layer instance data previously allocated.
*
*               p_conn_data   Pointer to Ctrl Layer connection data previously allocated.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPsCtrlLayer_ConnDataFree(),
*               HTTPsCtrlLayer_OnConnReq().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPsCtrlLayer_ConnDataEntriesFree (HTTPs_CTRL_LAYER_INST_DATA  *p_seg,
                                          HTTPs_CTRL_LAYER_CONN_DATA  *p_conn_data)
{
    HTTPs_CTRL_LAYER_DATA_ENTRY  *p_current;
    HTTPs_CTRL_LAYER_DATA_ENTRY  *p_entry_to_del;
    LIB_ERR                       err_lib;


    p_current = p_conn_data->ConnDataHeadPtr;

    while (p_current != DEF_NULL) {

        p_entry_to_del = p_current;
        p_current      = p_current->NextPtr;

        Mem_DynPoolBlkFree(       &p_seg->ConnDataEntryPool,
                           (void *)p_entry_to_del,
                                  &err_lib);

    }

    p_conn_data->ConnDataHeadPtr = DEF_NULL;

    (void)err_lib;
}

