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
*                                          HTTPs REST MEMORY
*
* Filename : http-s_rest_mem.c
* Version  : V3.01.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#define  HTTPs_REST_MEM_MODULE
#include "http-s_rest_mem.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPs_REST_MEM_CACHE_LINE_LEN          32u
#define  HTTPs_REST_MEM_RESOURCE_LIST_MAX       10u
#define  HTTPs_REST_MEM_RESOURCE_MAX            50u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/


static            MEM_DYN_POOL    HTTPsREST_Mem_ResourceListPool;
static            MEM_DYN_POOL    HTTPsREST_Mem_ResourcePool;

volatile  static  CPU_BOOLEAN     HTTPsREST_Mem_PoolsInitialized = DEF_NO;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       HTTPsREST_Mem_Init_Pools()
*
* Description : Initialize the rest memory pools.
*               This function is called during the initialization of the HTTP-s
*
* Argument(s) : max_request     Maximum number of simultaneous request the server can handle
*
* Return(s)   : DEF_NULL    if out of memory
*               OBJ         otherwise
*
* Caller(s)   : HTTPsREST_Init().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

HTTPs_REST_INST_DATA  *HTTPsREST_Mem_Init_Pools (CPU_SIZE_T  max_request)
{
    HTTPs_REST_INST_DATA  *p_inst_data;
    LIB_ERR                err_lib;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    if (HTTPsREST_Mem_PoolsInitialized == DEF_NO) {

        Mem_DynPoolCreate("REST_ResourceList_MemPool",
                          &HTTPsREST_Mem_ResourceListPool,
                           DEF_NULL,
                           sizeof(HTTPs_REST_RESOURCE_LIST),
                           HTTPs_REST_MEM_CACHE_LINE_LEN,
                           0,                                   /* Min block */
                           HTTPs_REST_MEM_RESOURCE_LIST_MAX,    /* Max block */
                          &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            CPU_CRITICAL_EXIT();
            return (DEF_NULL);
        }

        Mem_DynPoolCreate("REST_Resource_MemPool",
                          &HTTPsREST_Mem_ResourcePool,
                           DEF_NULL,
                           sizeof(HTTPs_REST_RESOURCE_ENTRY),
                           HTTPs_REST_MEM_CACHE_LINE_LEN,
                           0,                                   /* Min block */
                           HTTPs_REST_MEM_RESOURCE_MAX,         /* Max block */
                          &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            CPU_CRITICAL_EXIT();
            return (DEF_NULL);
        }

        SList_Init(&HTTPsREST_Mem_ResourceList);
    }
    CPU_CRITICAL_EXIT();

    p_inst_data = Mem_SegAlloc("REST_InstanceSeg",
                                DEF_NULL,
                                sizeof(HTTPs_REST_INST_DATA),
                               &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (DEF_NULL);
    }

    Mem_DynPoolCreate("REST_Conn_MemPool",
                      &p_inst_data->Pool,
                       DEF_NULL,
                       sizeof(HTTPs_REST_REQUEST),
                       HTTPs_REST_MEM_CACHE_LINE_LEN,
                       0,                               /* Min block */
                       max_request,                     /* Max block */
                      &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (DEF_NULL);
    }

    return (p_inst_data);
}

/*
*********************************************************************************************************
*                                     HTTPsREST_Mem_GetResourceList()
*
* Description : Get the resource list associated with an ID.
*               If the list doesn't exist, the list is created.
*
* Argument(s) : list_ID      Differentiate two or more lists
*
* Return(s)   : Pointer to resource list object.
*               DEF_NULL if out of memory.
*
* Caller(s)   : REST_Alloc_Resource(),
*               HTTPsREST_Authenticate(),
*               HTTPsREST_RxHeader().
*
* Note(s)     : (1)  Once a list is created it is never freed.
*********************************************************************************************************
*/

HTTPs_REST_RESOURCE_LIST  *HTTPsREST_Mem_GetResourceList (CPU_INT32U  list_ID)
{
    CPU_BOOLEAN                found;
    HTTPs_REST_RESOURCE_LIST  *p_list;
    LIB_ERR                    err_lib;


    found = DEF_NO;

    SLIST_FOR_EACH_ENTRY(HTTPsREST_Mem_ResourceList, p_list, HTTPs_REST_RESOURCE_LIST, ListNode)
    {
        if (p_list->Id == list_ID) {
            found = DEF_TRUE;
            break;
        }
    }

    if (found != DEF_TRUE) {
        p_list = (HTTPs_REST_RESOURCE_LIST *)Mem_DynPoolBlkGet(&HTTPsREST_Mem_ResourceListPool,
                                                               &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            return (DEF_NULL);
        }

        p_list->Id          = list_ID;
        p_list->ListHeadPtr = DEF_NULL;

        SList_Push(&HTTPsREST_Mem_ResourceList, &(p_list->ListNode));
    }

    return (p_list);
}


/*
*********************************************************************************************************
*                                       HTTPsREST_Mem_AllocResource()
*
* Description : Allocate a resource token to be chained.
*
* Argument(s) : list_ID     ID of the list to chain the resource token.
*
*               p_resource  Pointer to the resource to add to the list.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPsREST_Publish().
*
* Note(s)     : (1) This function must be called after the HTTP-s init but before the HTTP-s start.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsREST_Mem_AllocResource (       CPU_INT32U            list_ID,
                                          const  HTTPs_REST_RESOURCE  *p_resource)
{
    HTTPs_REST_RESOURCE_LIST   *p_list;
    HTTPs_REST_RESOURCE_ENTRY  *p_entry;
    LIB_ERR                     err_lib;


    p_list  = HTTPsREST_Mem_GetResourceList(list_ID);
    if (p_list == DEF_NULL) {
        return (DEF_FAIL);
    }

    p_entry = (HTTPs_REST_RESOURCE_ENTRY *)Mem_DynPoolBlkGet(&HTTPsREST_Mem_ResourcePool,
                                                             &err_lib);
    if (p_entry == DEF_NULL) {
        return (DEF_FAIL);
    }

    p_entry->ResourcePtr = p_resource;

    SList_Push(&p_list->ListHeadPtr, &p_entry->ListNode);
    SList_Sort(&p_list->ListHeadPtr,  HTTPsREST_ResourceListCompare);

    (void)err_lib;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       HTTPsREST_Mem_AllocRequest()
*
* Description : Allocate a REST request data structure.
*
* Argument(s) : p_inst_data     Pointer to REST instance with the pool to allocate on.
*
* Return(s)   : DEF_NULL    if out of memory.
*               OBJ         otherwise
*
* Caller(s)   : HTTPsREST_Authenticate().
*
* Note(s)     : None.
*********************************************************************************************************
*/

HTTPs_REST_REQUEST  *HTTPsREST_Mem_AllocRequest (HTTPs_REST_INST_DATA  *p_inst_data)
{
    HTTPs_REST_REQUEST  *p_req;
    LIB_ERR              err_lib;


    p_req = (HTTPs_REST_REQUEST *)Mem_DynPoolBlkGet(&p_inst_data->Pool, &err_lib);

   (void)err_lib;

    return (p_req);
}


/*
*********************************************************************************************************
*                                        HTTPsREST_Mem_FreeRequest()
*
* Description : Frees a previously allocated request.
*
* Argument(s) : p_inst_data   Pointer to REST instance data containing the pool to free the request.
*
*               p_request     Pointer to REST request to free.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPsREST_Authenticate(),
*               HTTPsREST_OnConnClosed().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPsREST_Mem_FreeRequest (HTTPs_REST_INST_DATA  *p_inst_data,
                                 HTTPs_REST_REQUEST    *p_request)
{
    LIB_ERR err_lib;


    Mem_DynPoolBlkFree(&p_inst_data->Pool,
                        p_request,
                       &err_lib);

    (void)err_lib;
}

