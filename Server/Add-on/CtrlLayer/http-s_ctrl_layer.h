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
*                                         HTTPs CONTROL LAYER
*
* Filename : http-s_ctrl_layer.h
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

#ifndef HTTPs_CTRL_LAYER_MODULE_PRESENT
#define HTTPs_CTRL_LAYER_MODULE_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include "../../Source/http-s.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                               CTRL LAYER AUTHENTICATION HOOKS DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_auth_hooks {
    HTTPs_INSTANCE_INIT_HOOK   OnInstanceInit;
    HTTPs_REQ_HDR_RX_HOOK      OnReqHdrRx;
    HTTPs_REQ_HOOK             OnReqAuth;
    HTTPs_RESP_HDR_TX_HOOK     OnRespHdrTx;
    HTTPs_TRANS_COMPLETE_HOOK  OnTransComplete;
    HTTPs_CONN_CLOSE_HOOK      OnConnClose;
} HTTPs_CTRL_LAYER_AUTH_HOOKS;


/*
*********************************************************************************************************
*                                CTRL LAYER APPLICATION HOOKS DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_app_hooks {
    HTTPs_INSTANCE_INIT_HOOK   OnInstanceInit;
    HTTPs_REQ_HDR_RX_HOOK      OnReqHdrRx;
    HTTPs_REQ_HOOK             OnReq;
    HTTPs_REQ_BODY_RX_HOOK     OnReqBodyRx;
    HTTPs_REQ_RDY_SIGNAL_HOOK  OnReqSignal;
    HTTPs_REQ_RDY_POLL_HOOK    OnReqPoll;
    HTTPs_RESP_HDR_TX_HOOK     OnRespHdrTx;
    HTTPs_RESP_TOKEN_HOOK      OnRespToken;
    HTTPs_RESP_CHUNK_HOOK      OnRespChunk;
    HTTPs_TRANS_COMPLETE_HOOK  OnTransComplete;
    HTTPs_ERR_HOOK             OnError;
    HTTPs_CONN_CLOSE_HOOK      OnConnClose;
} HTTPs_CTRL_LAYER_APP_HOOKS;


/*
*********************************************************************************************************
*                                CTRL LAYER AUTHENTIFIACTION DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_auth_inst {
    HTTPs_CTRL_LAYER_AUTH_HOOKS    *HooksPtr;
    void                           *HooksCfgPtr;
} HTTPs_CTRL_LAYER_AUTH_INST;


/*
*********************************************************************************************************
*                                CTRL LAYER APPLICATION DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_app_inst {
    HTTPs_CTRL_LAYER_APP_HOOKS     *HooksPtr;
    void                           *HooksCfgPtr;
} HTTPs_CTRL_LAYER_APP_INST;


/*
*********************************************************************************************************
*                                CTRL LAYER CONFIGUATION DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_cfg {
    HTTPs_CTRL_LAYER_AUTH_INST    **AuthInstsPtr;
    CPU_SIZE_T                      AuthInstsNbr;
    HTTPs_CTRL_LAYER_APP_INST     **AppInstsPtr;
    CPU_SIZE_T                      AppInstsNbr;
} HTTPs_CTRL_LAYER_CFG;


/*
*********************************************************************************************************
*                               CTRL LAYER CONFIGURATION LIST DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_cfg_List {
    HTTPs_CTRL_LAYER_CFG  **CfgsPtr;
    CPU_SIZE_T              Size;
} HTTPs_CTRL_LAYER_CFG_LIST;


/*
*********************************************************************************************************
*                                CTRL LAYER DATA ENTRY DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_data_entry  HTTPs_CTRL_LAYER_DATA_ENTRY;

struct  https_ctrl_layer_data_entry {
    CPU_INT32U                      OwnerId;
    void                           *DataPtr;
    HTTPs_CTRL_LAYER_DATA_ENTRY    *NextPtr;
};


/*
*********************************************************************************************************
*                                CTRL LAYER INSTANCE DATA TYPE
*
* Notes: (1) Structure of the memory management of the control layer.
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_inst_data {
    MEM_DYN_POOL                    ConnDataPool;
    MEM_DYN_POOL                    ConnDataEntryPool;
    MEM_DYN_POOL                    InstDataEntryPool;
    HTTPs_CTRL_LAYER_DATA_ENTRY    *InstDataHeadPtr;
} HTTPs_CTRL_LAYER_INST_DATA;


/*
*********************************************************************************************************
*                                CTRL LAYER CONNECTION DATA TYPE
*
* Notes: (1) Structure for the ConnDataPtr substitution in the HTTPs_CONN
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_conn_data {
    HTTPs_CTRL_LAYER_CFG           *TargetCfgPtr;
    HTTPs_CTRL_LAYER_APP_INST      *TargetAppInstPtr;
    HTTPs_CTRL_LAYER_DATA_ENTRY    *ConnDataHeadPtr;
} HTTPs_CTRL_LAYER_CONN_DATA;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

extern  const  HTTPs_HOOK_CFG  HTTPsCtrlLayer_HookCfg;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

CPU_BOOLEAN     HTTPsCtrlLayer_OnInstanceInit  (const  HTTPs_INSTANCE         *p_instance,
                                                const  void                   *p_cfg);

CPU_BOOLEAN     HTTPsCtrlLayer_OnReqRxHdr      (const  HTTPs_INSTANCE         *p_instance,
                                                const  HTTPs_CONN             *p_conn,
                                                const  void                   *p_cfg,
                                                       HTTP_HDR_FIELD          hdr_field);

CPU_BOOLEAN     HTTPsCtrlLayer_OnReq           (const  HTTPs_INSTANCE         *p_instance,
                                                       HTTPs_CONN             *p_conn,
                                                const  void                   *p_cfg);

CPU_BOOLEAN     HTTPsCtrlLayer_OnReqRxBody     (const  HTTPs_INSTANCE         *p_instance,
                                                       HTTPs_CONN             *p_conn,
                                                const  void                   *p_cfg,
                                                       void                   *p_buf,
                                                const  CPU_SIZE_T              buf_size,
                                                       CPU_SIZE_T             *p_buf_size_used);

CPU_BOOLEAN     HTTPsCtrlLayer_OnReqRdySignal  (const  HTTPs_INSTANCE         *p_instance,
                                                       HTTPs_CONN             *p_conn,
                                                const  void                   *p_cfg,
                                                const  HTTPs_KEY_VAL         *p_data);

CPU_BOOLEAN     HTTPsCtrlLayer_OnReqRdyPoll    (const  HTTPs_INSTANCE         *p_instance,
                                                       HTTPs_CONN             *p_conn,
                                                const  void                   *p_cfg);

CPU_BOOLEAN     HTTPsCtrlLayer_OnRespTxHdr     (const  HTTPs_INSTANCE         *p_instance,
                                                       HTTPs_CONN             *p_conn,
                                                const  void                   *p_cfg);

CPU_BOOLEAN     HTTPsCtrlLayer_OnRespToken     (const  HTTPs_INSTANCE         *p_instance,
                                                       HTTPs_CONN             *p_conn,
                                                const  void                   *p_cfg,
                                                const  CPU_CHAR               *p_token,
                                                       CPU_INT16U              token_len,
                                                       CPU_CHAR               *p_val,
                                                       CPU_INT16U              val_len_max);

CPU_BOOLEAN     HTTPsCtrlLayer_OnRespChunk     (const  HTTPs_INSTANCE         *p_instance,
                                                       HTTPs_CONN             *p_conn,
                                                const  void                   *p_cfg,
                                                       void                   *p_buf,
                                                       CPU_SIZE_T              buf_len_max,
                                                       CPU_SIZE_T             *p_tx_len);

void            HTTPsCtrlLayer_OnTransComplete (const  HTTPs_INSTANCE         *p_instance,
                                                       HTTPs_CONN             *p_conn,
                                                const  void                   *p_cfg);

void            HTTPsCtrlLayer_OnErr           (const  HTTPs_INSTANCE         *p_instance,
                                                       HTTPs_CONN             *p_conn,
                                                const  void                   *p_cfg,
                                                       HTTPs_ERR               err);

void            HTTPsCtrlLayer_OnErrFileGet    (const  void                   *p_cfg,
                                                       HTTP_STATUS_CODE        status_code,
                                                       CPU_CHAR               *p_file_str,
                                                       CPU_INT32U              file_len_max,
                                                       HTTPs_BODY_DATA_TYPE   *p_file_type,
                                                       HTTP_CONTENT_TYPE      *p_content_type,
                                                       void                  **p_data,
                                                       CPU_INT32U             *p_date_len);

void            HTTPsCtrlLayer_OnConnClose     (const  HTTPs_INSTANCE         *p_instance,
                                                       HTTPs_CONN             *p_conn,
                                                const  void                   *p_cfg);

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif /* HTTPs_CTRL_LAYER_MODULE_PRESENT */
