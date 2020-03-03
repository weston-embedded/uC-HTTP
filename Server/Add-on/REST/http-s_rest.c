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
* Filename : http-s_rest.c
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

#define  MICRIUM_SOURCE
#define  HTTPs_REST_MODULE
#include "http-s_rest.h"
#include "http-s_rest_mem.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPs_REST_CHAR_PATTERN_END                '\0'
#define  HTTPs_REST_CHAR_PATTERN_WILDCARD_START     '{'
#define  HTTPs_REST_CHAR_PATTERN_WILDCARD_END       '}'
#define  HTTPs_REST_CHAR_PATTERN_SEP                '/'
#define  HTTPs_REST_CHAR_PATTERN_INVALID            '\\'


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  CPU_INT16S            HTTPsREST_URI_CmpPattern     (const CPU_CHAR                *p_pattern,
                                                                  HTTPs_REST_MATCHED_URI  *p_result);

static  CPU_BOOLEAN           HTTPsREST_ValidatePatternStr (const CPU_CHAR                *p_pattern_str);

static  HTTPs_REST_HOOK_FNCT  HTTPsREST_FindMethodHook     (      HTTP_METHOD              method,
                                                                  HTTPs_REST_METHOD_HOOKS  method_hooks);


/*
*********************************************************************************************************
*                                         HTTPsREST_Publish()
*
* Description : This function adds a resource to a list of resources.
*
* Argument(s) : p_resource  Pointer to the rest resource to publish.
*
*               list_ID     Identification of the list to publish on.
*
*               p_err       Error indicator on publish
*                           It can be :
*                               REST_ERR_PUBLISH_INVALID_PATTERN_STRING
*                               REST_ERR_PUBLISH_MEMORY_NOT_INITIALIZED
*                               REST_ERR_PUBLISH_INSTANCES_NOT_STOPPED
*                               REST_ERR_NONE
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) It must be called after the HTTP-s initialization and before HTTP-s start.
*********************************************************************************************************
*/

void  HTTPsREST_Publish (const HTTPs_REST_RESOURCE  *p_resource,
                               CPU_INT32U            list_ID,
                               HTTPs_REST_ERR       *p_err)
{
    CPU_BOOLEAN  is_valid;
    CPU_BOOLEAN  result;


    is_valid = HTTPsREST_ValidatePatternStr(p_resource->PatternPtr);
    if (is_valid != DEF_OK) {
       *p_err = HTTPs_REST_ERR_PUBLISH_INVALID_PATTERN_STRING;
    }

    if (HTTPs_InstanceInitializedNbr == 0) {
       *p_err = HTTPs_REST_ERR_PUBLISH_MEMORY_NOT_INITIALIZED;

    } else if(HTTPs_InstanceRunningNbr != 0) {
       *p_err = HTTPs_REST_ERR_PUBLISH_INSTANCES_NOT_STOPPED;

    } else {
        result = HTTPsREST_Mem_AllocResource(list_ID, p_resource);
        if (result != DEF_OK) {
            *p_err = HTTPs_REST_ERR_PUBLISH_NOT_ENOUGH_MEMORY;
             return;
        }
       *p_err = HTTPs_REST_ERR_NONE;
    }
}


/*
*********************************************************************************************************
*                                              HTTPsREST_Init()
*
* Description : Initialize REST pools.
*
* Argument(s) : p_instance  Pointer to HTTPs Instance object.
*
*               p_cfg       Pointer to REST configuration object.
*
* Return(s)   : None.
*
* Caller(s)   : Application's Hooks configuration.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsREST_Init (const  HTTPs_INSTANCE  *p_instance,
                             const  void            *p_cfg)
{
    HTTPs_REST_INST_DATA  *p_inst_data;


    p_inst_data = HTTPsREST_Mem_Init_Pools(p_instance->CfgPtr->ConnNbrMax);
    if (p_inst_data == DEF_NULL) {
        return (DEF_FAIL);
    }

    ((HTTPs_INSTANCE*)p_instance)->DataPtr = p_inst_data;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                          HTTPsREST_Authenticate()
*
* Description : REST authentication hook function.
*
* Argument(s) : p_instance  Pointer to HTTPs Instance object.
*
*               p_conn      Pointer to HTTPs Connection object.
*
*               p_cfg       Pointer to REST Configuration object.
*
* Return(s)   : DEF_OK,   if authentication was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application's Hooks configuration.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsREST_Authenticate(const   HTTPs_INSTANCE  *p_instance,
                                            HTTPs_CONN      *p_conn,
                                    const   void            *p_cfg)
{
           HTTPs_REST_CFG            *p_rest_cfg;
    const  HTTPs_REST_RESOURCE       *p_resource;
           HTTPs_REST_REQUEST        *p_req;
    const  HTTPs_REST_RESOURCE       *p_r;
           HTTPs_REST_INST_DATA      *p_inst_data;
           HTTPs_REST_RESOURCE_LIST  *p_list;
           SLIST_MEMBER              *p_head;
           HTTPs_REST_RESOURCE_ENTRY *p_entry;
           HTTPs_REST_HOOK_FNCT       method_hook;
           HTTPs_REST_HOOK_STATE      state;
           CPU_INT16S                 cmp_result;


    p_rest_cfg  = (HTTPs_REST_CFG *)p_cfg;
    p_resource  =  DEF_NULL;
    p_inst_data = (HTTPs_REST_INST_DATA*)p_instance->DataPtr;

    p_list      =  HTTPsREST_Mem_GetResourceList(p_rest_cfg->listID);
    if (p_list == DEF_NULL) {
        return (DEF_FAIL);
    }

    p_req = HTTPsREST_Mem_AllocRequest(p_inst_data);
                                                                /* If there is no more memory to alloc ...              */
                                                                /* ... this request can't be handled.                   */
    if (p_req == DEF_NULL) {
        return (DEF_FAIL);
    }

    p_req->URI.ParsedURI.PathPtr = p_conn->PathPtr;
    p_req->URI.ParsedURI.PathLen = Str_Len(p_conn->PathPtr);

    p_head     = (SLIST_MEMBER*)p_list->ListHeadPtr;

                                                                /* Find the matching resource.                          */
    SLIST_FOR_EACH_ENTRY(p_head,
                         p_entry,
                         HTTPs_REST_RESOURCE_ENTRY,
                         ListNode)
    {
        p_r = p_entry->ResourcePtr;

        cmp_result = HTTPsREST_URI_CmpPattern(p_r->PatternPtr, &(p_req->URI));
        if (cmp_result == 0) {
            p_resource = p_r;
            break;
        }
    }
                                                                /* If there is no resource matching the given URI, the status is unknown*/
                                                                /* And should be handled by the caller                  */
    if (p_resource == DEF_NULL) {
        HTTPsREST_Mem_FreeRequest(p_inst_data, p_req);
        return (DEF_FAIL);
    }

    method_hook = HTTPsREST_FindMethodHook(p_conn->Method,
                                           p_resource->MethodHooks);
    if (method_hook == DEF_NULL) {
        HTTPsREST_Mem_FreeRequest(p_inst_data, p_req);
        return (DEF_FAIL);
    }

    state = method_hook(p_resource,
                       &p_req->URI,
                        HTTPs_REST_STATE_INIT,
                       &p_req->DataPtr,
                        p_instance,
                        p_conn,
                        DEF_NULL,
                        0,
                        DEF_NULL);

    if (state == HTTPs_REST_HOOK_STATE_ERROR) {
        HTTPsREST_Mem_FreeRequest(p_inst_data, p_req);
        return (DEF_FAIL);
    }

    p_req->ResourcePtr  = p_resource;
    p_req->Hook         = method_hook;

    p_conn->ConnDataPtr = p_req;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                         HTTPsREST_RxHeader()
*
* Description : Called upon HTTP-s header parsing. Determines if an header should be kept or not.
*
* Argument(s) : p_instance  Pointer to HTTPs instance object.
*
*               p_conn      Pointer HTTPs connection object.
*
*               p_cfg       Pointer to REST configuration object.
*
*               hdr_field   Header field to make the choice on.
*
* Return(s)   : DEF_YES  if the header should be saved.
*               DEF_NO   otherwise.
*
* Caller(s)   : Application's Hooks configuration.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsREST_RxHeader (const   HTTPs_INSTANCE   *p_instance,
                                 const   HTTPs_CONN       *p_conn,
                                 const   void             *p_cfg,
                                         HTTP_HDR_FIELD    hdr_field)
{
           HTTPs_REST_CFG             *p_rest_cfg;
           HTTPs_REST_RESOURCE_LIST   *p_list;
           SLIST_MEMBER               *p_head;
           HTTPs_REST_RESOURCE_ENTRY  *p_entry;
    const  HTTPs_REST_RESOURCE        *p_resource;
           CPU_INT16U                  i;


    p_rest_cfg = (HTTPs_REST_CFG*)p_cfg;

    p_list     =  HTTPsREST_Mem_GetResourceList(p_rest_cfg->listID);
    if (p_list == DEF_NULL) {
        return (DEF_NO);
    }

    p_head     = (SLIST_MEMBER*)p_list->ListHeadPtr;

                                                                /* Find the matching resource.                          */
    SLIST_FOR_EACH_ENTRY(p_head,
                         p_entry,
                         HTTPs_REST_RESOURCE_ENTRY,
                         ListNode)
    {
        p_resource = p_entry->ResourcePtr;
        for (i = 0; i < p_resource->HTTP_HdrsNbr; ++i) {
            if (p_resource->HTTP_Hdrs[i] == hdr_field) {
                return (DEF_YES);
            }
        }
    }

    return (DEF_NO);
}


/*
*********************************************************************************************************
*                                           HTTPsREST_RxBody()
*
* Description : Received the body from the HTTP server.
*
* Argument(s) : p_instance       Pointer to HTTPs Instance object.
*
*               p_conn           Pointer to HTTPs Connection object.
*
*               p_cfg            Pointer to REST Configuration object.
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
* Caller(s)   : Application's Hooks configuration.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsREST_RxBody(const  HTTPs_INSTANCE  *p_instance,
                                     HTTPs_CONN      *p_conn,
                              const  void            *p_cfg,
                                     void            *p_buf,
                              const  CPU_SIZE_T       buf_size,
                                     CPU_SIZE_T      *p_buf_size_used)
{
    HTTPs_REST_REQUEST     *p_req;
    HTTPs_REST_HOOK_FNCT    method_hook;
    HTTPs_REST_HOOK_STATE   state;


    if (p_conn->ConnDataPtr == DEF_NULL) {
        return (DEF_OK);
    }

    p_req       = (HTTPs_REST_REQUEST *)p_conn->ConnDataPtr;
    method_hook = p_req->Hook;

    state = method_hook(p_req->ResourcePtr,
                       &p_req->URI,
                        HTTPs_REST_STATE_RX,
                       &p_req->DataPtr,
                        p_instance,
                        p_conn,
                        p_buf,
                        buf_size,
                        p_buf_size_used);

    switch (state) {
        case HTTPs_REST_HOOK_STATE_CONTINUE:
            *p_buf_size_used = buf_size;
             return (DEF_YES);

        case HTTPs_REST_HOOK_STATE_STAY:
             return (DEF_YES);

        default:
             p_conn->StatusCode = HTTP_STATUS_INTERNAL_SERVER_ERR;
             p_conn->ErrCode    = HTTPs_ERR_STATE_UNKNOWN;
             return (DEF_NO);
    }
}


/*
*********************************************************************************************************
*                                      HTTPsREST_ReqRdySignal()
*
* Description : Called upon request parsing completion by HTTP server.
*               This function signals the REST resource that it is its last chance to modify the connection.
*
* Argument(s) : p_instance  Pointer to HTTPs instance object.
*
*               p_conn      Pointer to HTTPs connection object.
*
*               p_cfg       Pointer to REST configuration object.
*
*               p_data      UNUSED
*
* Return(s)   : DEF_OK    if the response is ready.
*               DEF_FAIL  otherwise.
*
* Caller(s)   : Application's Hooks configuration.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsREST_ReqRdySignal (const  HTTPs_INSTANCE  *p_instance,
                                            HTTPs_CONN      *p_conn,
                                     const  void            *p_cfg,
                                     const  HTTPs_KEY_VAL  *p_data)
{
    HTTPs_REST_REQUEST     *p_req;
    HTTPs_REST_HOOK_STATE   state;


    if (p_conn->ConnDataPtr == DEF_NULL) {
        return (DEF_FAIL);
    }

    p_req = (HTTPs_REST_REQUEST *)p_conn->ConnDataPtr;

    state = p_req->Hook(p_req->ResourcePtr,
                       &p_req->URI,
                        HTTPs_REST_STATE_RX,
                       &p_req->DataPtr,
                        p_instance,
                        p_conn,
                        DEF_NULL,
                        0,
                        DEF_NULL);

    if (state == HTTPs_REST_HOOK_STATE_ERROR) {
        p_conn->StatusCode = HTTP_STATUS_INTERNAL_SERVER_ERR;
        p_conn->ErrCode    = HTTPs_ERR_STATE_UNKNOWN;
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                          HTTPsREST_GetChunk()
*
* Description : Called after the HTTP response headers have been sent to create the body chunk by chunk.
*               Only called if the the HTTP-s connection's BODY_DATA_TYPE is set to CHUNKED_DATA.
*
* Argument(s) : p_instance      Pointer to HTTPs instance object.
*
*               p_conn          Pointer to HTTPs connection object.
*
*               p_cfg           Pointer to REST configuration object.
*
*               p_buf           Pointer to the data buffer start.
*
*               buf_len_max     Maximum length in bytes that can be written from buffer address.
*
*               len_tx          Variable that will received the length of bytes written.
*
* Return(s)   : DEF_YES        if there is no more data to send on this response.
*               DEF_NO         otherwise.
*
* Caller(s)   : Application's Hooks configuration.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsREST_GetChunk (const  HTTPs_INSTANCE  *p_instance,
                                        HTTPs_CONN      *p_conn,
                                 const  void            *p_hook_cfg,
                                        void            *p_buf,
                                        CPU_SIZE_T       buf_len_max,
                                        CPU_SIZE_T      *len_tx)
{
    HTTPs_REST_REQUEST    *p_req;
    HTTPs_REST_HOOK_FNCT   method_hook;
    HTTPs_REST_HOOK_STATE  state;


    if (p_conn->ConnDataPtr == DEF_NULL) {
        return (DEF_YES);
    }

    p_req       = (HTTPs_REST_REQUEST *)p_conn->ConnDataPtr;
    method_hook = p_req->Hook;

    state = method_hook(p_req->ResourcePtr,
                       &p_req->URI,
                        HTTPs_REST_STATE_TX,
                       &p_req->DataPtr,
                        p_instance,
                        p_conn,
                        p_buf,
                        buf_len_max,
                        len_tx);

    switch (state) {
        case HTTPs_REST_HOOK_STATE_CONTINUE:
             return (DEF_YES);

        case HTTPs_REST_HOOK_STATE_STAY:
             return (DEF_NO);

        default:
             return (DEF_YES);
    }
}


/*
*********************************************************************************************************
*                                      HTTPsREST_OnTransComplete()
*
* Description : Called when an HTTP Transaction has been completed.
*               Frees the connection allocated REST memory.
*
* Argument(s) : p_instance  Pointer to HTTPs instance object.
*
*               p_conn      Pointer to HTTPs connection object.
*
*               p_cfg       Pointer to REST configuration object.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPsCtrlLayer_REST_App.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPsREST_OnTransComplete (const  HTTPs_INSTANCE  *p_instance,
                                        HTTPs_CONN      *p_conn,
                                 const  void            *p_cfg)
{
    HTTPs_REST_INST_DATA  *p_inst_data;
    HTTPs_REST_REQUEST    *p_req;


    if (p_conn->ConnDataPtr == DEF_NULL) {
        return;
    }

    p_inst_data = (HTTPs_REST_INST_DATA*)p_instance->DataPtr;
    p_req       = (HTTPs_REST_REQUEST *)p_conn->ConnDataPtr;

    p_req->Hook(p_req->ResourcePtr,
               &p_req->URI,
                HTTPs_REST_STATE_CLOSE,
               &p_req->DataPtr,
                p_instance,
                p_conn,
                DEF_NULL,
                0,
                DEF_NULL);

    HTTPsREST_Mem_FreeRequest(p_inst_data, p_req);

    p_conn->ConnDataPtr = DEF_NULL;
}


/*
*********************************************************************************************************
*                                        HTTPsREST_OnConnClosed()
*
* Description : Called upon HTTP-s connection closed. Frees the connection allocated REST memory.
*
* Argument(s) : p_instance      Pointer to HTTPs instance object.
*
*               p_conn          Pointer to HTTPs connection object.
*
*               p_cfg           Pointer to REST configuration object.
*
* Return(s)   : None.
*
* Caller(s)   : Application's Hooks configuration.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPsREST_OnConnClosed (const  HTTPs_INSTANCE  *p_instance,
                                     HTTPs_CONN      *p_conn,
                              const  void            *p_cfg)
{
    HTTPs_REST_INST_DATA  *p_inst_data;
    HTTPs_REST_REQUEST    *p_req;


    if (p_conn->ConnDataPtr == DEF_NULL) {
        return;
    }

    p_inst_data = (HTTPs_REST_INST_DATA*)p_instance->DataPtr;
    p_req       = (HTTPs_REST_REQUEST *)p_conn->ConnDataPtr;

    p_req->Hook(p_req->ResourcePtr,
               &p_req->URI,
                HTTPs_REST_STATE_CLOSE,
               &p_req->DataPtr,
                p_instance,
                p_conn,
                DEF_NULL,
                0,
                DEF_NULL);

    HTTPsREST_Mem_FreeRequest(p_inst_data, p_req);

    p_conn->ConnDataPtr = DEF_NULL;
}


/*
*********************************************************************************************************
*                                      HTTPsREST_ResourceListCompare()
*
* Description : Compares two Resources of a list.
*
* Argument(s) : p_item_l    left REST resource
*
*               p_item_r    right REST resource
*
* Return(s)   : DEF_TRUE    if right is lesser or equal to right.
*               DEF_FALSE   otherwise.
*
* Caller(s)   : REST_Alloc_Resource().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsREST_ResourceListCompare (SLIST_MEMBER   *p_item_l,
                                            SLIST_MEMBER   *p_item_r)
{
    const  HTTPs_REST_RESOURCE  *p_left;
    const  HTTPs_REST_RESOURCE  *p_right;


    p_left  = (CONTAINER_OF(p_item_l, HTTPs_REST_RESOURCE_ENTRY, ListNode))->ResourcePtr;
    p_right = (CONTAINER_OF(p_item_r, HTTPs_REST_RESOURCE_ENTRY, ListNode))->ResourcePtr;

    return Str_Cmp(p_left->PatternPtr, p_right->PatternPtr) <= 0;
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
*                                        HTTPsREST_URI_CmpPattern()
*
* Description : Compare URI to a pattern for a match.
*
* Argument(s) : p_pattern   Pattern to be matched.
*
*               p_result    Result structure with the URI to match.
*
* Return(s)   : 0           if it matches.
*               Not 0       otherwise
*
* Caller(s)   : HTTPsREST_Authenticate().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT16S  HTTPsREST_URI_CmpPattern (const CPU_CHAR                *p_pattern,
                                                    HTTPs_REST_MATCHED_URI  *p_result)
{
    HTTPs_REST_KEY_VAL  *p_current_wildcard;
    CPU_CHAR            *p_path;
    CPU_SIZE_T           path_len;
    CPU_SIZE_T           path_char_consumed_nbr;


    p_path                 = p_result->ParsedURI.PathPtr;
    path_len               = p_result->ParsedURI.PathLen;
    path_char_consumed_nbr = 0;
    p_result->WildCardsNbr = 0;

                                                                /* Until the end of pattern is found                    */
    while (*p_pattern != HTTPs_REST_CHAR_PATTERN_END) {

        switch (*p_pattern) {
                                                                /* If the pattern character is a wild card              */
                                                                /* Important note: KeyLen != ValLen.                    */
                                                                /* So, the pattern and the URI aren't consumed equally. */
            case HTTPs_REST_CHAR_PATTERN_WILDCARD_START:

                 ++p_pattern;                                   /* Consume the wild card opening char                   */

                 p_current_wildcard         = &(p_result->WildCards[p_result->WildCardsNbr]);
                 p_current_wildcard->KeyPtr = p_pattern;

                                                                /* Find the end of the wild card key                    */
                 while (*p_pattern != HTTPs_REST_CHAR_PATTERN_WILDCARD_END) {
                     ++p_pattern;
                     ++(p_current_wildcard->KeyLen);
                 }

                 ++p_pattern;                                   /* Consume the wild card closing char                   */

                 p_current_wildcard->ValPtr = p_path;

                 if (*p_pattern != HTTPs_REST_CHAR_PATTERN_END) {

                     while ((*p_path                 != HTTPs_REST_CHAR_PATTERN_SEP) &&
                            ( path_char_consumed_nbr  < path_len)                   ) {
                         ++p_path;
                         ++(p_current_wildcard->ValLen);
                         ++path_char_consumed_nbr;
                     }

                     path_char_consumed_nbr += p_current_wildcard->ValLen;

                 } else {

                     p_current_wildcard->ValLen = path_len - path_char_consumed_nbr;
                     path_char_consumed_nbr = path_len;
                 }

                 p_result->WildCardsNbr++;                      /* Increment the number of wild cards parsed.           */
                 break;


            default:                                            /* Otherwise,                                           */
                 if (*p_pattern == *p_path) {                   /* Compare both chars.                                  */
                                                                /* If they match, increment all the counters.           */
                     ++p_pattern;
                     ++p_path;
                     ++path_char_consumed_nbr;

                 } else if ((*p_path       == HTTPs_REST_CHAR_PATTERN_END)            &&
                            (*p_pattern    == HTTPs_REST_CHAR_PATTERN_SEP)            &&
                            ( p_pattern[1] == HTTPs_REST_CHAR_PATTERN_WILDCARD_START)) {
                                                                     /* if it is a wild card.                                */
                     ++p_pattern;

                 } else {
                                                                     /* Otherwise, they are different return the difference. */
                     return (*p_pattern - *p_path);
                 }
                 break;
        }
    }

    return (path_len - path_char_consumed_nbr);
}


/*
*********************************************************************************************************
*                                        HTTPsREST_ValidatePatternStr()
*
* Description : Validates a REST pattern string.
*
* Argument(s) : p_pattern_str  The pattern string to validate.
*
* Return(s)   : DEF_OK   if the pattern string is valid.
*               DEF_FAIL otherwise.
*
* Caller(s)   : HTTPsREST_Publish().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPsREST_ValidatePatternStr (const  CPU_CHAR  *p_pattern_str)
{
    CPU_INT16U   i;
    CPU_BOOLEAN  wildcard_opened;


    wildcard_opened = DEF_FAIL;

    for (i = 0; p_pattern_str[i] != HTTPs_REST_CHAR_PATTERN_END; ++i) {

        switch (p_pattern_str[i]) {
            case HTTPs_REST_CHAR_PATTERN_WILDCARD_START:
                 if((wildcard_opened      == DEF_OK)                      ||
                    (i                    == 0)                           ||
                    (p_pattern_str[i - 1] != HTTPs_REST_CHAR_PATTERN_SEP)) {
                     return (DEF_FAIL);
                 } else {
                     wildcard_opened = DEF_OK;
                 }
                 break;


            case HTTPs_REST_CHAR_PATTERN_WILDCARD_END:
                 if( (wildcard_opened != DEF_OK)                             ||
                     (i               == 0)                                  ||
                     ((p_pattern_str[i + 1] != HTTPs_REST_CHAR_PATTERN_SEP)  &&
                      (p_pattern_str[i + 1] != HTTPs_REST_CHAR_PATTERN_END))) {
                     return (DEF_FAIL);
                 } else {
                     wildcard_opened = DEF_FAIL;
                 }
                 break;


            case HTTPs_REST_CHAR_PATTERN_INVALID:
                 return (DEF_FAIL);


            default:
                 break;
        }
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        HTTPsREST_FindMethodHook()
*
* Description : Finds the hook function associated with the specified method
*
* Argument(s) : method          HTTP-s method of the request.
*
*               method_hooks    REST hooks of the current resource.
*
* Return(s)   : DEF_NULL        if no function is specified for the method provided.
*               REST_HOOK_FNCT  otherwise.
*
* Caller(s)   : HTTPsREST_Authenticate().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  HTTPs_REST_HOOK_FNCT  HTTPsREST_FindMethodHook (HTTP_METHOD              method,
                                                        HTTPs_REST_METHOD_HOOKS  method_hooks)
{
    switch (method) {
        case HTTP_METHOD_GET:
             return (method_hooks.Get);

        case HTTP_METHOD_HEAD:
             return (method_hooks.Head);

        case HTTP_METHOD_DELETE:
             return (method_hooks.Delete);

        case HTTP_METHOD_POST:
             return (method_hooks.Post);

        case HTTP_METHOD_PUT:
             return (method_hooks.Put);

        default:
             return (DEF_NULL);
    }
}
