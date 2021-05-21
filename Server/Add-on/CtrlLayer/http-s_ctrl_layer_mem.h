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
* Filename : http-s_ctrl_layer_mem.h
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

#ifndef HTTPs_CTRL_LAYER_MEM_MODULE_PRESENT
#define HTTPs_CTRL_LAYER_MEM_MODULE_PRESENT

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include "http-s_ctrl_layer.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

HTTPs_CTRL_LAYER_INST_DATA   *HTTPsCtrlLayerMem_InstDataAlloc         (void);

CPU_BOOLEAN                   HTTPsCtrlLayerMem_InstDataPoolInit      (HTTPs_CTRL_LAYER_INST_DATA  *p_seg,
                                                                       CPU_SIZE_T                   pool_size_max);

CPU_BOOLEAN                   HTTPsCtrlLayerMem_ConnDataPoolInit      (HTTPs_CTRL_LAYER_INST_DATA  *p_seg,
                                                                       CPU_SIZE_T                   pool_size_max);

CPU_BOOLEAN                   HTTPsCtrlLayerMem_ConnDataEntryPoolInit (HTTPs_CTRL_LAYER_INST_DATA  *p_seg,
                                                                       CPU_SIZE_T                   pool_size_max);

HTTPs_CTRL_LAYER_DATA_ENTRY  *HTTPsCtrlLayerMem_InstDataEntryAlloc    (HTTPs_CTRL_LAYER_INST_DATA  *p_seg);

HTTPs_CTRL_LAYER_CONN_DATA   *HTTPsCtrlLayerMem_ConnDataAlloc         (HTTPs_CTRL_LAYER_INST_DATA  *p_seg);

void                          HTTPsCtrlLayer_ConnDataFree             (HTTPs_CTRL_LAYER_INST_DATA  *p_seg,
                                                                       HTTPs_CTRL_LAYER_CONN_DATA  *p_conn_data);

HTTPs_CTRL_LAYER_DATA_ENTRY  *HTTPsCtrlLayer_ConnDataEntryAlloc       (HTTPs_CTRL_LAYER_INST_DATA  *p_seg,
                                                                       HTTPs_CTRL_LAYER_CONN_DATA  *p_conn_data);

void                          HTTPsCtrlLayer_ConnDataEntriesFree      (HTTPs_CTRL_LAYER_INST_DATA  *p_seg,
                                                                       HTTPs_CTRL_LAYER_CONN_DATA  *p_conn_data);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif /* HTTPs_CTRL_LAYER_MEM_MODULE_PRESENT */
