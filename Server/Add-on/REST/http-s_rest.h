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
*                                             HTTPs REST
*
* Filename : http-s_rest.h
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

#ifndef HTTPs_REST_MODULE_PRESENT
#define HTTPs_REST_MODULE_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include <lib_def.h>
#include <lib_str.h>

#include <Collections/slist.h>

#include "../../Source/http-s.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define HTTPs_REST_MAX_URI_WILD_CARD           5

#define HTTPs_REST_MAX_PUBLISHED_RESOURCE     50


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            REST STATE TYPE
*********************************************************************************************************
*/

typedef  enum  https_rest_state {
    HTTPs_REST_STATE_INIT,
    HTTPs_REST_STATE_RX,
    HTTPs_REST_STATE_ERROR,
    HTTPs_REST_STATE_TX,
    HTTPs_REST_STATE_CLOSE,
} HTTPs_REST_STATE;


/*
*********************************************************************************************************
*                                           REST HOOK STATE TYPE
*********************************************************************************************************
*/

typedef  enum  https_rest_hook_state {
    HTTPs_REST_HOOK_STATE_ERROR,
    HTTPs_REST_HOOK_STATE_CONTINUE,
    HTTPs_REST_HOOK_STATE_STAY,
} HTTPs_REST_HOOK_STATE;


/*
*********************************************************************************************************
*                                             REST ERROR TYPE
*********************************************************************************************************
*/

typedef  enum  https_rest_err {
    HTTPs_REST_ERR_NONE,
    HTTPs_REST_ERR_PUBLISH_INSTANCES_NOT_STOPPED,
    HTTPs_REST_ERR_PUBLISH_MEMORY_NOT_INITIALIZED,
    HTTPs_REST_ERR_PUBLISH_NOT_ENOUGH_MEMORY,
    HTTPs_REST_ERR_PUBLISH_INVALID_PATTERN_STRING
}  HTTPs_REST_ERR;


/*
*********************************************************************************************************
*                                       REST KEY-VALUE PAIR TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_key_val {
    const   CPU_CHAR     *KeyPtr;
            CPU_SIZE_T    KeyLen;
    const   CPU_CHAR     *ValPtr;
            CPU_SIZE_T    ValLen;
} HTTPs_REST_KEY_VAL;


/*
*********************************************************************************************************
*                                          REST PARSED URI TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_parsed_uri {
    CPU_CHAR     *PathPtr;
    CPU_SIZE_T    PathLen;
} HTTPs_REST_PARSED_URI;


/*
*********************************************************************************************************
*                                          REST MATCHED URI TYPE
*********************************************************************************************************
*/

typedef  struct https_rest_matched_uri {
    HTTPs_REST_PARSED_URI  ParsedURI;
    HTTPs_REST_KEY_VAL     WildCards[HTTPs_REST_MAX_URI_WILD_CARD];
    CPU_SIZE_T             WildCardsNbr;
} HTTPs_REST_MATCHED_URI;


/*
*********************************************************************************************************
*                                          REST HOOK FUNCTION TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_resource  HTTPs_REST_RESOURCE;

typedef  HTTPs_REST_HOOK_STATE  (*HTTPs_REST_HOOK_FNCT)  (const  HTTPs_REST_RESOURCE      *p_resource,
                                                          const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                          const  HTTPs_REST_STATE          state,
                                                                 void                    **p_data,
                                                          const  HTTPs_INSTANCE           *p_instance,
                                                                 HTTPs_CONN               *p_conn,
                                                                 void                     *p_buf,
                                                          const  CPU_SIZE_T                buf_size,
                                                                 CPU_SIZE_T               *p_buf_size_used);

/*
*********************************************************************************************************
*                                             REST METHOD HOOKS TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_method_hooks {
    HTTPs_REST_HOOK_FNCT  Delete;
    HTTPs_REST_HOOK_FNCT  Get;
    HTTPs_REST_HOOK_FNCT  Head;
    HTTPs_REST_HOOK_FNCT  Post;
    HTTPs_REST_HOOK_FNCT  Put;
} HTTPs_REST_METHOD_HOOKS;


/*
*********************************************************************************************************
*                                              REST RESOURCE TYPE
*********************************************************************************************************
*/

struct  https_rest_resource {
    const   CPU_CHAR                 *PatternPtr;               /* Access path to the resource ending with an EOF char. */
    const   HTTP_HDR_FIELD           *HTTP_Hdrs;                /* HTTP headers to keep.                                */
    const   CPU_SIZE_T                HTTP_HdrsNbr;
    const   HTTPs_REST_METHOD_HOOKS   MethodHooks;
};


/*
*********************************************************************************************************
*                                           REST RESOURCE ENTRY TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_resource_entry {
     const  HTTPs_REST_RESOURCE    *ResourcePtr;
            SLIST_MEMBER            ListNode;
} HTTPs_REST_RESOURCE_ENTRY;


/*
*********************************************************************************************************
*                                          REST CONFIGURATION TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_cfg {
    const  CPU_INT32U   listID;
} HTTPs_REST_CFG;


/*
*********************************************************************************************************
*                                         REST RESOURCE LIST TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_resource_list {
    CPU_INT32U     Id;
    SLIST_MEMBER  *ListHeadPtr;
    SLIST_MEMBER   ListNode;
} HTTPs_REST_RESOURCE_LIST;


/*
*********************************************************************************************************
*                                           REST REQUEST TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_request {
     const  HTTPs_REST_RESOURCE     *ResourcePtr;
            HTTPs_REST_MATCHED_URI   URI;
            HTTPs_REST_HOOK_FNCT     Hook;
            void                    *DataPtr;
} HTTPs_REST_REQUEST;


/*
*********************************************************************************************************
*                                    REST HTTP INSTANCE DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_inst_data {
     MEM_DYN_POOL    Pool;
} HTTPs_REST_INST_DATA;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        API FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void         HTTPsREST_Publish             (const  HTTPs_REST_RESOURCE  *p_resource,
                                                   CPU_INT32U            list_ID,
                                                   HTTPs_REST_ERR       *p_err);


/*
*********************************************************************************************************
*                                      HOOK FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsREST_Init                (const  HTTPs_INSTANCE       *p_instance,
                                            const  void                 *p_cfg);

CPU_BOOLEAN  HTTPsREST_Authenticate        (const  HTTPs_INSTANCE       *p_instance,
                                                   HTTPs_CONN           *p_conn,
                                            const  void                 *p_cfg);

CPU_BOOLEAN  HTTPsREST_RxHeader            (const  HTTPs_INSTANCE       *p_instance,
                                            const  HTTPs_CONN           *p_conn,
                                            const  void                 *p_cfg,
                                                   HTTP_HDR_FIELD        hdr_field);

CPU_BOOLEAN  HTTPsREST_RxBody              (const  HTTPs_INSTANCE       *p_instance,
                                                   HTTPs_CONN           *p_conn,
                                            const  void                 *p_cfg,
                                                   void                 *p_buf,
                                            const  CPU_SIZE_T            buf_size,
                                                   CPU_SIZE_T           *p_buf_size_used);

CPU_BOOLEAN  HTTPsREST_ReqRdySignal        (const  HTTPs_INSTANCE       *p_instance,
                                                   HTTPs_CONN           *p_conn,
                                            const  void                 *p_cfg,
                                            const  HTTPs_KEY_VAL       *p_data);

CPU_BOOLEAN  HTTPsREST_GetChunk            (const  HTTPs_INSTANCE       *p_instance,
                                                   HTTPs_CONN           *p_conn,
                                            const  void                 *p_cfg,
                                                   void                 *p_buf,
                                                   CPU_SIZE_T            buf_len_max,
                                                   CPU_SIZE_T           *len_tx);

void         HTTPsREST_OnTransComplete     (const  HTTPs_INSTANCE       *p_instance,
                                                   HTTPs_CONN           *p_conn,
                                            const  void                 *p_cfg);

void         HTTPsREST_OnConnClosed        (const  HTTPs_INSTANCE       *p_instance,
                                                   HTTPs_CONN           *p_conn,
                                            const  void                 *p_cfg);


/*
*********************************************************************************************************
*                                     INTERNAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsREST_ResourceListCompare (       SLIST_MEMBER         *p_item_l,
                                                   SLIST_MEMBER         *p_item_r);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif /* HTTPs_REST_MODULE_PRESENT */
