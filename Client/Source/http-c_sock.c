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
*                                      HTTP CLIENT SOCKET MODULE
*
* Filename : http-c_sock.c
* Version  : V3.01.00
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
#define    HTTPc_SOCK_MODULE
#include  <Source/net_ascii.h>
#include  <Source/net_util.h>

#include  <Source/net.h>
#include  <Source/net_cfg_net.h>
#include  <Source/net_app.h>
#include  <Source/net_sock.h>
#include  <Source/net_tcp.h>

#include  "http-c_sock.h"
#include  "http-c_conn.h"
#ifdef  HTTPc_WEBSOCK_MODULE_EN
#include  "http-c_websock.h"
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/


#define  HTTPc_SOCK_TIMEOUT_CONNECT_MS_DFLT               2000u
#define  HTTPc_SOCK_TIMEOUT_CONNECT_MS_MAX               10000u
#define  HTTPc_SOCK_TIMEOUT_CONNECT_MS_MIN                1000u

#define  HTTPc_SOCK_TIMEOUT_INACTIVITY_S_DFLT               60u
#define  HTTPc_SOCK_TIMEOUT_INACTIVITY_S_MAX               255u
#define  HTTPc_SOCK_TIMEOUT_INACTIVITY_S_MIN                 1u

#define  HTTPc_SOCK_SEL_TIMEOUT_MS                           1u


/*
*********************************************************************************************************
*                                         HTTPcSock_Connect()
*
* Description : (1) Open Socket.
*               (2) Connect to Server.
*               (3) Set Inactivity Connection Timeout.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Connection with HTTP server successfully.
*                               HTTPc_ERR_CONN_SOCK_OPEN        Opening socket error.
*                               HTTPc_ERR_CONN_SOCK_CONN        Connect with server error.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcTask_Handler(),
*               HTTPc_ConnGet().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPcSock_Connect (HTTPc_CONN  *p_conn,
                         HTTPc_ERR   *p_err)
{
    NET_APP_SOCK_SECURE_CFG   *p_secure_cfg;
    NET_CONN_ID                conn_id_tcp;
    NET_ERR                    err_net;
    CPU_BOOLEAN                no_delay;
    CPU_BOOLEAN                keep_alive;


#ifdef NET_SECURE_MODULE_EN
    p_secure_cfg = &p_conn->SockSecureCfg;
    if (p_secure_cfg->TrustCallback == DEF_NULL) {
        p_secure_cfg = DEF_NULL;
    } else {
        if (p_secure_cfg->CommonName == DEF_NULL) {
            p_secure_cfg->CommonName = p_conn->HostNamePtr;
        }
        if (p_conn->ServerPort == HTTP_DFLT_PORT_NBR) {
            p_conn->ServerPort = HTTP_DFLT_PORT_NBR_SECURE;
        }
    }
#else
    p_secure_cfg = DEF_NULL;
#endif

    if (p_conn->ConnectTimeout_ms <= 0) {
       *p_err = HTTPc_ERR_CONN_PARAM_CONNECT_TIMEOUT_INVALID;
        goto exit;
    }

    if (p_conn->InactivityTimeout_s <= 0) {
       *p_err = HTTPc_ERR_CONN_PARAM_INACTIVITY_TIMEOUT_INVALID;
        goto exit;
    }

                                                                /* ---------------- RESOLVE HOST NAME ----------------- */
    NetApp_ClientStreamOpenByHostname(&p_conn->SockID,
                                       p_conn->HostNamePtr,
                                       p_conn->ServerPort,
                                      &p_conn->ServerSockAddr,
                                       p_secure_cfg,
                                       p_conn->ConnectTimeout_ms,
                                      &err_net);
    switch (err_net) {
        case NET_APP_ERR_NONE:
        case NET_APP_ERR_CONN_IN_PROGRESS:
             break;

        default:
             p_conn->SockID = NET_SOCK_ID_NONE;
            *p_err = HTTPc_ERR_CONN_SOCK_CONNECT;
             goto exit;
    }
                                                                /* ---------------- CFG SOCK BLOCK OPT ---------------- */
   (void)NetSock_CfgBlock(p_conn->SockID, NET_SOCK_BLOCK_SEL_NO_BLOCK, &err_net);
    if (err_net != NET_SOCK_ERR_NONE) {
       *p_err = HTTPc_ERR_CONN_SOCK_CONNECT;
        goto exit_close;
    }

                                                                /* ----- SET SOCKET CONNECTION INACTIVITY TIMEOUT ----- */
   (void)NetSock_OptSet(         p_conn->SockID,
                                 NET_SOCK_PROTOCOL_TCP,
                                 NET_SOCK_OPT_TCP_KEEP_IDLE,
                        (void *)&p_conn->InactivityTimeout_s,
                                 sizeof(p_conn->InactivityTimeout_s),
                                &err_net);
    if (err_net != NET_SOCK_ERR_NONE) {
       *p_err = HTTPc_ERR_CONN_SOCK_CONNECT;
        goto exit_close;
    }

                                                                /* ---------- SET SOCKET KEEP-ALIVE TIMEOUT ----------- */
    keep_alive = p_conn->IsKeepAlive;
   (void)NetSock_OptSet( p_conn->SockID,
                         NET_SOCK_PROTOCOL_SOCK,
                         NET_SOCK_OPT_SOCK_KEEP_ALIVE,
                        &keep_alive,
                         sizeof(keep_alive),
                        &err_net);
    if (err_net != NET_SOCK_ERR_NONE) {
       *p_err = HTTPc_ERR_CONN_SOCK_CONNECT;
        goto exit_close;
    }
    
                                                                /* --------------- SET NO DELAY FEATURE --------------- */
    no_delay = DEF_FALSE;                                       /* Set to false to disable the naggle algorithm.        */
   (void)NetSock_OptSet(         p_conn->SockID,
                                 NET_SOCK_PROTOCOL_TCP,
                                 NET_SOCK_OPT_TCP_NO_DELAY,
                        (void *)&no_delay,
                                 sizeof(no_delay),
                                &err_net);
    if (err_net != NET_SOCK_ERR_NONE) {
       *p_err = HTTPc_ERR_CONN_SOCK_CONNECT;
        goto exit_close;
    }
                                                                /* -------------- SET TCP CONNECTION MSL -------------- */
    conn_id_tcp = NetSock_GetConnTransportID(p_conn->SockID, &err_net);
    if (err_net != NET_SOCK_ERR_NONE) {
       *p_err = HTTPc_ERR_CONN_SOCK_CONNECT;
        goto exit_close;
    }

   (void)NetTCP_ConnCfgMSL_Timeout(conn_id_tcp, 0u, &err_net);
    if (err_net != NET_TCP_ERR_NONE) {
       *p_err = HTTPc_ERR_CONN_SOCK_CONNECT;
        goto exit_close;
    }

   *p_err = HTTPc_ERR_NONE;

    goto exit;


exit_close:
    NetSock_Close(p_conn->SockID, &err_net);
    p_conn->SockID = NET_SOCK_ID_NONE;

exit:
    return;
}


/*
*********************************************************************************************************
*                                        HTTPcSock_ConnDataRx()
*
* Description : Receive data from the network and update connection.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               max_len     Specify the maximum length to get from the socket.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                   Data successfully received.
*                               HTTPc_ERR_CONN_SOCK_RX           Transitory error.
*                               HTTPc_ERR_CONN_SOCK_CONN_CLOSED  Connection is fully or half closed.
*                               HTTPc_ERR_CONN_SOCK_RX_FATAL     Fatal error occurred.
*
* Return(s)   : DEF_OK,     if data received successfully.
*               DEF_FAIL,   otherwise.
*
* Caller(s)   : HTTPcResp().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcSock_ConnDataRx (HTTPc_CONN  *p_conn,
                                   CPU_INT32U   max_len,
                                   HTTPc_ERR   *p_err)
{
    CPU_CHAR              *p_buf;
    NET_SOCK_ADDR_LEN      addr_len_server;
    CPU_INT16S             rx_len;
    CPU_INT32U             buf_len;
    CPU_BOOLEAN            rtn_val;
    NET_ERR                err_net;


    if (p_conn->RxDataLenRem > 0) {                             /* If data is still present in the rx buf.              */
                                                                /* Move rem data to the beginning of the rx buf.        */
        Mem_Copy(p_conn->BufPtr, p_conn->RxBufPtr, p_conn->RxDataLenRem);
    }
    p_conn->RxBufPtr = p_conn->BufPtr;

    p_buf   = p_conn->BufPtr + p_conn->RxDataLenRem;
    buf_len = p_conn->BufLen - (p_conn->RxDataLenRem + p_conn->TxDataLen);

    if (buf_len == 0) {
        rtn_val = DEF_OK;                                       /* HTTP buffer already full of data rx.                 */
       *p_err   = HTTPc_ERR_NONE;
        goto exit;
    }
    if (max_len != 0) {
        buf_len = DEF_MIN(buf_len, max_len);
    }
    addr_len_server = sizeof(p_conn->ServerSockAddr);

                                                                /* ------------------ RECEIVED DATA ------------------- */
    rx_len = NetSock_RxDataFrom(        p_conn->SockID,
                                (void *)p_buf,
                                        buf_len,
                                        NET_SOCK_FLAG_NO_BLOCK,
                                       &p_conn->ServerSockAddr,
                                       &addr_len_server,
                                        DEF_NULL,
                                        DEF_NULL,
                                        DEF_NULL,
                                       &err_net);
    switch (err_net) {
        case NET_SOCK_ERR_NONE:                                 /* Data received.                                       */
        case NET_SOCK_ERR_INVALID_DATA_SIZE:
             p_conn->RxDataLenRem += rx_len;
             break;


        case NET_ERR_RX:                                        /* Transitory rx err(s).                                */
        case NET_INIT_ERR_NOT_COMPLETED:
        case NET_SOCK_ERR_RX_Q_EMPTY:
        case NET_ERR_FAULT_LOCK_ACQUIRE:
             rtn_val = DEF_FAIL;
            *p_err   = HTTPc_ERR_CONN_SOCK_RX;
             goto exit;


        case NET_SOCK_ERR_CLOSED:                               /* Conn closed by peer.                                 */
        case NET_SOCK_ERR_RX_Q_CLOSED:
             rtn_val = DEF_FAIL;
            *p_err   = HTTPc_ERR_CONN_SOCK_CLOSED;
             goto exit;


        default:                                                /* Fatal err.                                           */
             rtn_val = DEF_FAIL;
            *p_err   = HTTPc_ERR_CONN_SOCK_RX_FATAL;
             goto exit;

    }


    rtn_val = DEF_OK;
   *p_err   = HTTPc_ERR_NONE;


exit:
    return (rtn_val);
}


/*
*********************************************************************************************************
*                                        HTTPcSock_ConnDataTx()
*
* Description : Transmit data on the network and update HTTPc connection parameters.
*
* Argument(s) : p_conn      Pointer to the current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Data transmit to TCPIP stack successfully.
*                               HTTPc_ERR_CONN_SOCK_TX          Transitory error.
*                               HTTPc_ERR_CONN_SOCK_CLOSED      Connection is fully or half closed.
*                               HTTPc_ERR_CONN_SOCK_TX_FATAL    Fatal error occurred.
*
* Return(s)   : DEF_OK,   if entire data transmitted successfully.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPc_Req().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcSock_ConnDataTx (HTTPc_CONN  *p_conn,
                                   HTTPc_ERR   *p_err)
{
    NET_SOCK_ADDR_LEN      addr_len_server;
    CPU_INT16U             tx_len;
    CPU_BOOLEAN            rtn_val;
    NET_ERR                err_net;


    addr_len_server = sizeof(p_conn->ServerSockAddr);

    tx_len = NetSock_TxDataTo(p_conn->SockID,
                              p_conn->TxDataPtr,
                              p_conn->TxDataLen,
                              NET_SOCK_FLAG_NO_BLOCK,
                             &p_conn->ServerSockAddr,
                              addr_len_server,
                             &err_net);
    switch (err_net) {
        case NET_SOCK_ERR_NONE:                                 /* Data transmitted.                                    */
        case NET_INIT_ERR_NOT_COMPLETED:
        case NET_SOCK_ERR_PORT_NBR_NONE_AVAIL:
        case NET_CONN_ERR_NONE_AVAIL:
        case NET_ERR_FAULT_LOCK_ACQUIRE:
             p_conn->TxDataLen -= tx_len;
             if (p_conn->TxDataLen > 0u) {                      /* If data is not entirely transmitted.                 */
                 p_conn->TxDataPtr = (CPU_CHAR *)p_conn->TxDataPtr + tx_len;
                 rtn_val  = DEF_FAIL;
                *p_err    = HTTPc_ERR_CONN_SOCK_TX;
                 goto exit;
             }
             break;

        case NET_ERR_TX:
             rtn_val  = DEF_FAIL;
            *p_err    = HTTPc_ERR_CONN_SOCK_TX;
             goto exit;

        case NET_SOCK_ERR_CLOSED:                               /* Conn closed by peer.                                 */
        case NET_SOCK_ERR_TX_Q_CLOSED:
            *p_err   = HTTPc_ERR_CONN_SOCK_CLOSED;
             rtn_val = DEF_FAIL;
             goto exit;


        default:                                                /* Fatal err.                                           */
             rtn_val = DEF_FAIL;
            *p_err   = HTTPc_ERR_CONN_SOCK_TX_FATAL;
             goto exit;
    }

    rtn_val = DEF_OK;
   *p_err   = HTTPc_ERR_NONE;


exit:
    return (rtn_val);
}


/*
*********************************************************************************************************
*                                           HTTPcSock_Close()
*
* Description : Close Socket Connection with Server.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                      Socket successfully closed.
*                               HTTPc_ERR_CONN_SOCK_CLOSE_FATAL     Fatal error occurred.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcConn_Close().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPcSock_Close (HTTPc_CONN  *p_conn,
                       HTTPc_ERR   *p_err)
{
    CPU_BOOLEAN  done;
    NET_ERR      err_net;


    done  = DEF_NO;
    while (done != DEF_YES) {
        NetSock_Close(p_conn->SockID, &err_net);
        switch (err_net) {
            case NET_SOCK_ERR_NONE:                             /* Socket successfully closed.                          */
            case NET_SOCK_ERR_CLOSED:
            case NET_SOCK_ERR_CONN_CLOSE_IN_PROGRESS:
            case NET_SOCK_ERR_INVALID_FAMILY:
            case NET_SOCK_ERR_INVALID_TYPE:
            case NET_SOCK_ERR_INVALID_STATE:
            case NET_SOCK_ERR_CONN_SIGNAL_TIMEOUT:
            case NET_SOCK_ERR_CONN_FAIL:
            case NET_SOCK_ERR_FAULT:
            case NET_CONN_ERR_INVALID_CONN:
            case NET_CONN_ERR_NOT_USED:
                 done           = DEF_YES;
                 p_conn->SockID = NET_SOCK_ID_NONE;
                 break;


            case NET_INIT_ERR_NOT_COMPLETED:                    /* Transitory fault(s); close aborted but MIGHT ...     */
            case NET_ERR_FAULT_LOCK_ACQUIRE:                    /* ... close in a subsequent attempt.                   */
                 break;


            default:                                            /* Fatal err.                                           */
               *p_err = HTTPc_ERR_CONN_SOCK_CLOSE_FATAL;
                goto exit;
        }
    }

   *p_err = HTTPc_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                            HTTPcSock_Sel()
*
* Description : (1) Check all HTTPc Connection sockets for available resources &/or operations.
*               (2) Set each HTTPc Connection descriptors.
*
*
* Argument(s) : p_conn      Pointer to fist item in Connection list or Single Connection pointer.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               HTTPc_ERR_NONE                  Select Operation successful.
*                               HTTPc_ERR_CONN_INVALID_STATE    HTTPc Connection invalid state.
*                               HTTPc_ERR_CONN_SOCK_SEL         Socket Select operation faulted.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcTask(),
*               HTTPc_ReqSend().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  HTTPcSock_Sel (HTTPc_CONN  *p_conn,
                     HTTPc_ERR   *p_err)
{
    HTTPc_CONN        *p_conn_tmp;
    NET_SOCK_TIMEOUT   sock_timeout;
    NET_SOCK_TIMEOUT  *p_sock_timeout;
    NET_SOCK_DESC      sock_desc_rd;
    NET_SOCK_DESC      sock_desc_wr;
    NET_SOCK_DESC      sock_desc_err;
    NET_SOCK_QTY       sock_nbr_max;
    NET_SOCK_QTY       sock_nbr_rdy;
    CPU_INT08U         state_family;
    NET_ERR            err_net;
    CPU_BOOLEAN        rx_data;
#ifdef  HTTPc_WEBSOCK_MODULE_EN
    HTTPc_WEBSOCK     *p_ws;
#endif

                                                                /* ---------------- PREPARE SOCK DESC ----------------- */
    NET_SOCK_DESC_INIT(&sock_desc_rd);
    NET_SOCK_DESC_INIT(&sock_desc_wr);
    NET_SOCK_DESC_INIT(&sock_desc_err);

    sock_nbr_max             = 0;
    p_conn_tmp               = p_conn;
    p_sock_timeout           = DEF_NULL;
    sock_timeout.timeout_sec = 0;
    sock_timeout.timeout_us  = HTTPc_SOCK_SEL_TIMEOUT_MS * DEF_TIME_NBR_uS_PER_SEC / DEF_TIME_NBR_mS_PER_SEC;

    while (p_conn_tmp != DEF_NULL) {

        if (p_conn_tmp->SockID != NET_SOCK_ID_NONE) {

            state_family = HTTPc_CONN_STATE_FAMILY_MASK & p_conn_tmp->State;
            switch (state_family) {
                case HTTPc_CONN_STATE_FLOW_FAMILY:
                     p_sock_timeout = &sock_timeout;
                     NET_SOCK_DESC_SET(p_conn_tmp->SockID, &sock_desc_err);
                     break;

                case HTTPc_CONN_STATE_REQ_FAMILY:
                     NET_SOCK_DESC_SET(p_conn_tmp->SockID, &sock_desc_wr);
                     NET_SOCK_DESC_SET(p_conn_tmp->SockID, &sock_desc_err);
                     break;

                case HTTPc_CONN_STATE_RESP_FAMILY:
                     rx_data = DEF_BIT_IS_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_RX_MORE_DATA);
                     if (rx_data == DEF_NO) {
                         p_sock_timeout = &sock_timeout;
                     }

                     NET_SOCK_DESC_SET(p_conn_tmp->SockID, &sock_desc_rd);
                     NET_SOCK_DESC_SET(p_conn_tmp->SockID, &sock_desc_err);
                     break;
#ifdef  HTTPc_WEBSOCK_MODULE_EN
                case HTTPc_CONN_STATE_WEBSOCK_FAMILY:
                     p_ws = p_conn_tmp->WebSockPtr;
                     if (p_ws->TxMsgListHeadPtr != DEF_NULL) {
                         NET_SOCK_DESC_SET(p_conn_tmp->SockID, &sock_desc_wr);
                     }

                     rx_data = DEF_BIT_IS_SET(p_conn->RespFlags, HTTPc_FLAG_RESP_RX_MORE_DATA);
                     if (rx_data == DEF_NO) {
                         p_sock_timeout = &sock_timeout;
                     }

                     NET_SOCK_DESC_SET(p_conn_tmp->SockID, &sock_desc_rd);
                     NET_SOCK_DESC_SET(p_conn_tmp->SockID, &sock_desc_err);
                     break;
#endif
                default:
                    *p_err = HTTPc_ERR_CONN_INVALID_STATE;
                     goto exit;
            }
            p_conn_tmp->SockFlags = HTTPc_FLAG_NONE;
            if (sock_nbr_max <= p_conn_tmp->SockID) {           /* Update highest sock nbr.                             */
                 sock_nbr_max = p_conn_tmp->SockID + 1;
            }
        }
        p_conn_tmp = p_conn_tmp->NextPtr;
    }

    if (sock_nbr_max == 0u) {
        p_sock_timeout = &sock_timeout;
    }
                                                           /* -------------------- SOCK SEL ---------------------- */
    sock_nbr_rdy = NetSock_Sel( sock_nbr_max,
                               &sock_desc_rd,
                               &sock_desc_wr,
                               &sock_desc_err,
                                p_sock_timeout,
                               &err_net);
    switch (err_net) {
        case NET_SOCK_ERR_NONE:
        case NET_SOCK_ERR_TIMEOUT:
             break;


        default:
            *p_err = HTTPc_ERR_CONN_SOCK_SEL;
             goto exit;
    }

    if (sock_nbr_rdy > 0) {
        p_conn_tmp = p_conn;
        while (p_conn_tmp != DEF_NULL) {

            if (p_conn_tmp->SockID != NET_SOCK_ID_NONE) {

                state_family = HTTPc_CONN_STATE_FAMILY_MASK & p_conn_tmp->State;
                switch (state_family) {
                    case HTTPc_CONN_STATE_FLOW_FAMILY:
                         if (NET_SOCK_DESC_IS_SET(p_conn_tmp->SockID, &sock_desc_err)) {
                             DEF_BIT_SET(p_conn_tmp->SockFlags, HTTPc_FLAG_SOCK_RDY_ERR);
                         }
                         break;

                    case HTTPc_CONN_STATE_REQ_FAMILY:
                         if (NET_SOCK_DESC_IS_SET(p_conn_tmp->SockID, &sock_desc_wr)) {
                             DEF_BIT_SET(p_conn_tmp->SockFlags, HTTPc_FLAG_SOCK_RDY_WR);
                         }
                         if (NET_SOCK_DESC_IS_SET(p_conn_tmp->SockID, &sock_desc_err)) {
                             DEF_BIT_SET(p_conn_tmp->SockFlags, HTTPc_FLAG_SOCK_RDY_ERR);
                         }
                         break;

                    case HTTPc_CONN_STATE_RESP_FAMILY:
                         if (NET_SOCK_DESC_IS_SET(p_conn_tmp->SockID, &sock_desc_rd)) {
                             DEF_BIT_SET(p_conn_tmp->SockFlags, HTTPc_FLAG_SOCK_RDY_RD);
                         }
                         if (NET_SOCK_DESC_IS_SET(p_conn_tmp->SockID, &sock_desc_err)) {
                             DEF_BIT_SET(p_conn_tmp->SockFlags, HTTPc_FLAG_SOCK_RDY_ERR);
                         }
                         break;
#ifdef  HTTPc_WEBSOCK_MODULE_EN
                    case HTTPc_CONN_STATE_WEBSOCK_FAMILY:
                         if (NET_SOCK_DESC_IS_SET(p_conn_tmp->SockID, &sock_desc_wr)) {
                             DEF_BIT_SET(p_conn_tmp->SockFlags, HTTPc_FLAG_SOCK_RDY_WR);
                         }
                         if (NET_SOCK_DESC_IS_SET(p_conn_tmp->SockID, &sock_desc_rd)) {
                             DEF_BIT_SET(p_conn_tmp->SockFlags, HTTPc_FLAG_SOCK_RDY_RD);
                         }
                         if (NET_SOCK_DESC_IS_SET(p_conn_tmp->SockID, &sock_desc_err)) {
                             DEF_BIT_SET(p_conn_tmp->SockFlags, HTTPc_FLAG_SOCK_RDY_ERR);
                         }
                         break;
#endif
                    default:
                        *p_err = HTTPc_ERR_CONN_INVALID_STATE;
                         goto exit;
                }
            }
            p_conn_tmp = p_conn_tmp->NextPtr;
        }
     }

   *p_err = HTTPc_ERR_NONE;

exit:
    return;
}


/*
*********************************************************************************************************
*                                        HTTPcSock_IsRxClosed()
*
* Description : Check if the TCP Connection is half closed (FIN flag was received from Server).
*
* Argument(s) : p_conn  Pointer to current HTTPc Connection.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           HTTPc_ERR_NONE                      Close status of socket successfully checked.
*
*                           ------------ RETURNED BY NetSock_GetConnTransportID() ------------
*                           See NetSock_GetConnTransportID() for additional return error codes.
*
* Return(s)   : DEF_YES, if TCP connection is half-closed.
*               DEF_NO,  otherwise.
*
* Caller(s)   : HTTPcTask_Handler().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPcSock_IsRxClosed (HTTPc_CONN  *p_conn,
                                   HTTPc_ERR   *p_err)
{
    NET_TCP_CONN_ID      conn_id_tcp;
    NET_TCP_CONN_STATE   state;
    CPU_BOOLEAN          result;
    NET_ERR              err_net;


    result = DEF_NO;

    conn_id_tcp =  NetSock_GetConnTransportID(p_conn->SockID, &err_net);
    if (err_net != NET_SOCK_ERR_NONE) {
        result = DEF_YES;
        goto exit;
    }

    state = NetTCP_ConnStateGet(conn_id_tcp);

    if (state == NET_TCP_CONN_STATE_CLOSE_WAIT) {
        result = DEF_YES;
    }

   *p_err = HTTPc_ERR_NONE;


exit:
    return (result);
}


