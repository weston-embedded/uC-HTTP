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
*                                      HTTP SERVER SOCKET MODULE
*
* Filename : http-s_sock.c
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
#define    HTTPs_SOCK_MODULE

#include  "http-s_sock.h"
#include  "http-s_mem.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPs_SOCK_SEL_TIMEOUT_MS                          1u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  HTTPsSock_ConnAccept (HTTPs_INSTANCE  *p_instance,
                                    NET_SOCK_ID      sock_listen_id);


/*
*********************************************************************************************************
*                                        HTTPsSock_ListenInit()
*
* Description : (1) Initialize listen socket:
*
*                   (a) Open socket.
*                   (b) Configure socket secure option.
*                   (c) Bind socket.
*                   (d) Configure socket for listen.
*                   (e) Configure socket blocking option.
*
* Argument(s) : p_cfg   Pointer to the instance configuration structure.
*               -----   Argument validated in HTTPs_InstanceStart().
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           HTTPs_ERR_NONE                      Listen socket successfully initialized.
*                           HTTPs_ERR_SOCK_OPEN                 Socket open   failed.
*                           HTTPs_ERR_SOCK_BIND                 Socket bind   failed.
*                           HTTPs_ERR_SOCK_LISTEN               Socket listen failed.
*
*                           HTTPs_ERR_SOCK_SET_OPT_SECURE       Set socket option secure failed.
*                           HTTPs_ERR_SOCK_SET_OPT_BLOCK        Set socket option block  failed
*
*                           HTTPs_ERR_CFG_INVALID_SECURE_EN     Socket secure module not present.
*
* Return(s)   : Socket descriptor/handle identifier for the listen, if NO error(s).
*
*               NET_SOCK_BSD_ERR_OPEN,                              otherwise.
*
* Caller(s)   : HTTPs_InstanceStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

NET_SOCK_ID  HTTPsSock_ListenInit (const  HTTPs_CFG                 *p_cfg,
                                          NET_SOCK_PROTOCOL_FAMILY   family,
                                          HTTPs_ERR                 *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
    NET_IPv4_ADDR         net_ipv4_addr_any;
#endif
    NET_SOCK_ID           sock_id;
    NET_SOCK_ADDR         sock_addr;
    NET_SOCK_ADDR_LEN     sock_addr_len;
    CPU_INT08U           *p_addr;
    NET_SOCK_ADDR_LEN     addr_len;
    NET_ERR               err;


                                                                /* -------------------- OPEN SOCK --------------------- */
    sock_id = NetSock_Open(family,
                           NET_SOCK_TYPE_STREAM,
                           NET_SOCK_PROTOCOL_TCP,
                          &err);
    if (err != NET_SOCK_ERR_NONE) {
       *p_err = HTTPs_ERR_SOCK_OPEN;
        return (NET_SOCK_BSD_ERR_OPEN);
    }


                                                                /* --------------- CFG SOCK SECURE OPT----------------- */
    if (p_cfg->SecurePtr != DEF_NULL) {
#ifndef NET_SECURE_MODULE_EN                                    /* Set or clear socket secure mode.                     */
       *p_err = HTTPs_ERR_CFG_INVALID_SECURE_EN;
        (void)NetSock_Close(sock_id, &err);
        return (NET_SOCK_BSD_ERR_OPEN);
#else

       (void)NetSock_CfgSecure(sock_id,
                               DEF_YES,
                              &err);
        switch (err) {
            case NET_SOCK_ERR_NONE:
                 break;

            case NET_SOCK_ERR_NOT_USED:
            case NET_SOCK_ERR_INVALID_ARG:
            case NET_SOCK_ERR_INVALID_TYPE:
            case NET_SOCK_ERR_INVALID_STATE:
            case NET_SOCK_ERR_INVALID_OP:
            case NET_ERR_FAULT_FEATURE_DIS:
            case NET_INIT_ERR_NOT_COMPLETED:
            case NET_SOCK_ERR_INVALID_SOCK:
            case NET_SECURE_ERR_NOT_AVAIL:
            case NET_ERR_FAULT_LOCK_ACQUIRE:
            default:
                 (void)NetSock_Close(sock_id, &err);
                *p_err = HTTPs_ERR_SOCK_SET_OPT_SECURE;
                 return (NET_SOCK_BSD_ERR_OPEN);
        }


       (void)NetSock_CfgSecureServerCertKeyInstall(sock_id,
                                                   p_cfg->SecurePtr->CertPtr,
                                                   p_cfg->SecurePtr->CertLen,
                                                   p_cfg->SecurePtr->KeyPtr,
                                                   p_cfg->SecurePtr->KeyLen,
                                                   p_cfg->SecurePtr->Fmt,
                                                   p_cfg->SecurePtr->CertChain,
                                                  &err);
        switch (err) {
            case NET_SOCK_ERR_NONE:
                 break;

            case NET_SOCK_ERR_NOT_USED:
            case NET_SOCK_ERR_INVALID_ARG:
            case NET_SOCK_ERR_INVALID_TYPE:
            case NET_SOCK_ERR_INVALID_STATE:
            case NET_SOCK_ERR_INVALID_OP:
            case NET_ERR_FAULT_FEATURE_DIS:
            case NET_INIT_ERR_NOT_COMPLETED:
            case NET_SOCK_ERR_INVALID_SOCK:
            case NET_SECURE_ERR_NOT_AVAIL:
            case NET_ERR_FAULT_LOCK_ACQUIRE:
            default:
                 (void)NetSock_Close(sock_id, &err);
                *p_err = HTTPs_ERR_SOCK_SET_OPT_SECURE;
                 return (NET_SOCK_BSD_ERR_OPEN);
        }
#endif
    }

                                                                /* -------------------- BIND SOCK --------------------- */
    switch (family) {
        case NET_SOCK_ADDR_FAMILY_IP_V4:
#ifdef  NET_IPv4_MODULE_EN
             net_ipv4_addr_any =  NET_IPv4_ADDR_ANY;
             p_addr            = (CPU_INT08U *)&net_ipv4_addr_any;
             addr_len          =  NET_IPv4_ADDR_SIZE;
             sock_addr_len     =  sizeof(sock_addr);
#else
             (void)NetSock_Close(sock_id, &err);
            *p_err = HTTPs_ERR_SOCK_BIND;
             return (NET_SOCK_BSD_ERR_OPEN);
#endif
             break;


        case NET_SOCK_ADDR_FAMILY_IP_V6:
#ifdef  NET_IPv6_MODULE_EN
             p_addr        = (CPU_INT08U *)&NET_IPv6_ADDR_ANY;
             addr_len      =  NET_IPv6_ADDR_SIZE;
             sock_addr_len =  sizeof(sock_addr);
#else
             (void)NetSock_Close(sock_id, &err);
            *p_err = HTTPs_ERR_SOCK_BIND;
             return (NET_SOCK_BSD_ERR_OPEN);
#endif
             break;


        default:
            (void)NetSock_Close(sock_id, &err);
           *p_err = HTTPs_ERR_SOCK_BIND;
            return (NET_SOCK_BSD_ERR_OPEN);
    }


    NetApp_SetSockAddr(&sock_addr,
                       family,
                       p_cfg->Port,
                       p_addr,
                       addr_len,
                      &err);
    switch (err) {
        case NET_APP_ERR_NONE:
            break;

        case NET_APP_ERR_FAULT:
        case NET_APP_ERR_NONE_AVAIL:
        case NET_APP_ERR_INVALID_ARG:
        default:
             (void)NetSock_Close(sock_id, &err);
            *p_err = HTTPs_ERR_SOCK_BIND;
             return (NET_SOCK_BSD_ERR_OPEN);
    }


    (void)NetSock_Bind(sock_id,
                      &sock_addr,
                       sock_addr_len,
                      &err);
    if (err != NET_SOCK_ERR_NONE) {
       (void)NetSock_Close(sock_id, &err);
       *p_err = HTTPs_ERR_SOCK_BIND;
        return (NET_SOCK_BSD_ERR_OPEN);
    }


                                                                /* ------------------- LISTEN SOCK -------------------- */
   (void)NetSock_Listen(sock_id, p_cfg->ConnNbrMax, &err);
    if (err != NET_SOCK_ERR_NONE) {
       (void)NetSock_Close(sock_id, &err);
       *p_err = HTTPs_ERR_SOCK_LISTEN;
        return (NET_SOCK_BSD_ERR_OPEN);
    }


                                                                /* ---------------- CFG SOCK BLOCK OPT ---------------- */
   (void)NetSock_CfgBlock(sock_id, NET_SOCK_BLOCK_SEL_NO_BLOCK, &err);
    switch(err) {
        case NET_SOCK_ERR_NONE:
             break;

        case NET_SOCK_ERR_INVALID_ARG:
        case NET_INIT_ERR_NOT_COMPLETED:
        case NET_SOCK_ERR_INVALID_SOCK:
        case NET_SOCK_ERR_NOT_USED:
        case NET_ERR_FAULT_LOCK_ACQUIRE:
        default:
             (void)NetSock_Close(sock_id, &err);
            *p_err = HTTPs_ERR_SOCK_SET_OPT_BLOCK;
             return (NET_SOCK_BSD_ERR_OPEN);
    }

   *p_err = HTTPs_ERR_NONE;

    return (sock_id);
}


/*
*********************************************************************************************************
*                                        HTTPsSock_ListenClose()
*
* Description : (1) Close listen socket.
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_InstanceTaskHandler().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPsSock_ListenClose(HTTPs_INSTANCE  *p_instance,
                            NET_SOCK_ID      sock_listen_id)
{
    HTTPs_INSTANCE_STATS  *p_ctr_stats;
    HTTPs_INSTANCE_ERRS   *p_ctr_err;
    CPU_BOOLEAN            done;
    NET_ERR                err;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

    done = DEF_NO;
    while (done != DEF_YES) {
       (void)NetSock_Close(sock_listen_id, &err);
        switch (err) {
            case NET_SOCK_ERR_NONE:
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
                 done = DEF_YES;
                 HTTPs_STATS_INC(p_ctr_stats->Sock_StatListenCloseCtr);
                 break;


            case NET_INIT_ERR_NOT_COMPLETED:
            case NET_ERR_FAULT_LOCK_ACQUIRE:                    /* Transitory fault(s); close aborted but MIGHT ...     */
                 break;                                         /* ... close in a subsequent attempt.                   */


            default:
                 done = DEF_YES;
                 HTTPs_ERR_INC(p_ctr_err->Sock_ErrListenCloseCtr);
                 break;
        }
    }
}


/*
*********************************************************************************************************
*                                            HTTPsSock_ConnSel()
*
* Description : (1) Update connections that are ready to be processed.
*
*                   (a) Prepare socket descriptor
*                   (b) Socket select
*                   (c) Update connection states
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               accept      Boolean that determines whether or not to accept the HTTP client connection.
*
* Return(s)   : Number of connection ready to be processed.
*
* Caller(s)   : HTTPsTask_InstanceTaskHandler().
*
* Note(s)     : (1) When this function is called, the request's protocol version has not yet been parsed
*                   and thus the protocol version (and the connection's persistence status) are unknown
*                   to the instance at this point. However, if the instance is closing gracefully we will
*                   allow the connection's inactivity timeout to expire.
*********************************************************************************************************
*/

NET_SOCK_QTY  HTTPsSock_ConnSel (HTTPs_INSTANCE  *p_instance,
                                 CPU_BOOLEAN      accept)
{
    const  HTTPs_CFG            *p_cfg;
           HTTPs_CONN           *p_conn;
           HTTPs_INSTANCE_ERRS  *p_ctr_err;
           NET_SOCK_TIMEOUT     *p_sock_timeout;
           NET_SOCK_TIMEOUT      sock_timeout;
           NET_SOCK_QTY          sock_nbr_rdy;
           NET_SOCK_DESC         sock_desc_rd;
           NET_SOCK_DESC         sock_desc_wr;
           NET_SOCK_DESC         sock_desc_err;
           NET_SOCK_QTY          sock_nbr_max;
           NET_SOCK_RTN_CODE     sel_rtn_code;
#if (HTTPs_CFG_PERSISTENT_CONN_EN == DEF_ENABLED)
           NET_SOCK             *p_sock;
           CPU_BOOLEAN           persistent;
#endif
           CPU_BOOLEAN           child_present;
           NET_ERR               err;


    HTTPs_SET_PTR_ERRS(p_ctr_err, p_instance);
    p_cfg = p_instance->CfgPtr;

    child_present = DEF_NO;

                                                                /* ---------------- PREPARE SOCK DESC ----------------- */
    NET_SOCK_DESC_INIT(&sock_desc_rd);
    NET_SOCK_DESC_INIT(&sock_desc_wr);
    NET_SOCK_DESC_INIT(&sock_desc_err);

    sock_nbr_rdy = 0;
    sock_nbr_max = 0;

    if (accept == DEF_YES) {
        switch (p_cfg->SockSel) {
            case HTTPs_SOCK_SEL_IPv4:
#ifdef NET_IPv4_MODULE_EN
                 NET_SOCK_DESC_SET(p_instance->SockListenID_IPv4, &sock_desc_rd);
                 sock_nbr_max = p_instance->SockListenID_IPv4 + 1;
#endif
                 break;


            case HTTPs_SOCK_SEL_IPv6:
#ifdef NET_IPv6_MODULE_EN
                 NET_SOCK_DESC_SET(p_instance->SockListenID_IPv6, &sock_desc_rd);
                 sock_nbr_max = p_instance->SockListenID_IPv6 + 1;
#endif
                 break;


            case HTTPs_SOCK_SEL_IPv4_IPv6:
#ifdef NET_IPv4_MODULE_EN
                 NET_SOCK_DESC_SET(p_instance->SockListenID_IPv4, &sock_desc_rd);
                 sock_nbr_max = p_instance->SockListenID_IPv4 + 1;
#endif
#ifdef NET_IPv6_MODULE_EN
                 NET_SOCK_DESC_SET(p_instance->SockListenID_IPv6, &sock_desc_rd);
                 if (sock_nbr_max < p_instance->SockListenID_IPv6) {
                     sock_nbr_max = p_instance->SockListenID_IPv6 + 1;
                 } else if (sock_nbr_max == p_instance->SockListenID_IPv6) {
                     sock_nbr_max = p_instance->SockListenID_IPv6 + 1;
                 }
#endif
                 break;


            default:
                 break;
        }
    }

    p_conn = p_instance->ConnFirstPtr;
    while (p_conn != DEF_NULL) {
        child_present = DEF_YES;
        switch (p_conn->SockState) {
            case HTTPs_SOCK_STATE_RX:                           /* Conn rdy to receive data.                            */
                 NET_SOCK_DESC_SET(p_conn->SockID, &sock_desc_rd);
                 NET_SOCK_DESC_SET(p_conn->SockID, &sock_desc_err);
                 break;


            case HTTPs_SOCK_STATE_TX:                           /* Conn rdy to tx data.                                 */
                 NET_SOCK_DESC_SET(p_conn->SockID, &sock_desc_wr);
                 NET_SOCK_DESC_SET(p_conn->SockID, &sock_desc_err);
                 break;


            case HTTPs_SOCK_STATE_ERR:                          /* Conn is wainting for sock err.                       */
            case HTTPs_SOCK_STATE_CLOSE:
            case HTTPs_SOCK_STATE_NONE:
                 NET_SOCK_DESC_SET(p_conn->SockID, &sock_desc_err);
                 sock_nbr_rdy++;                                /* Socket close completed in HTTPsConn_Close().         */
                 break;


            default:
                 break;
        }

        if (sock_nbr_max <= p_conn->SockID) {                   /* Update highest sock nbr.                             */
            sock_nbr_max = p_conn->SockID + 1;
        } else if ((sock_nbr_max   == p_conn->SockID) &&
                   (p_conn->SockID == 0)             ) {
            sock_nbr_max = p_conn->SockID + 1;
        }

        p_conn = p_conn->ConnNextPtr;
    }


                                                                /* -------------------- SOCK SEL ---------------------- */
    if ((accept        == DEF_YES) &&
        (child_present == DEF_NO) ) {
        p_sock_timeout = DEF_NULL;
    } else {
        sock_timeout.timeout_sec =  0;
        sock_timeout.timeout_us  =  HTTPs_SOCK_SEL_TIMEOUT_MS * DEF_TIME_NBR_uS_PER_SEC / DEF_TIME_NBR_mS_PER_SEC;
        p_sock_timeout           = &sock_timeout;
    }


    sel_rtn_code = NetSock_Sel(sock_nbr_max,
                              &sock_desc_rd,
                              &sock_desc_wr,
                              &sock_desc_err,
                               p_sock_timeout,
                              &err);
    switch (err) {
        case NET_SOCK_ERR_NONE:
        case NET_SOCK_ERR_TIMEOUT:
             sock_nbr_rdy += sel_rtn_code;
             break;


        case NET_INIT_ERR_NOT_COMPLETED:
        case NET_SOCK_ERR_INVALID_DESC:
        case NET_SOCK_ERR_INVALID_TIMEOUT:
        case NET_SOCK_ERR_INVALID_SOCK:
        case NET_SOCK_ERR_INVALID_TYPE:
        case NET_SOCK_ERR_NOT_USED:
        case NET_ERR_FAULT_LOCK_ACQUIRE:
        default:
             HTTPs_ERR_INC(p_ctr_err->Sock_ErrSelCtr);
             return (sock_nbr_rdy);
    }

                                                                /* -------------- ACCEPT NEW CONNECTIONS -------------- */
    if (accept == DEF_YES) {
        switch (p_cfg->SockSel) {
            case HTTPs_SOCK_SEL_IPv4:
#ifdef NET_IPv4_MODULE_EN
                 if (NET_SOCK_DESC_IS_SET(p_instance->SockListenID_IPv4, &sock_desc_rd)) {
                     HTTPsSock_ConnAccept(p_instance, p_instance->SockListenID_IPv4);
                 }
#endif
                 break;


            case HTTPs_SOCK_SEL_IPv6:
#ifdef NET_IPv6_MODULE_EN
                 if (NET_SOCK_DESC_IS_SET(p_instance->SockListenID_IPv6, &sock_desc_rd)) {
                     HTTPsSock_ConnAccept(p_instance, p_instance->SockListenID_IPv6);
                 }
#endif
                 break;


            case HTTPs_SOCK_SEL_IPv4_IPv6:
#ifdef NET_IPv4_MODULE_EN
                 if (NET_SOCK_DESC_IS_SET(p_instance->SockListenID_IPv4, &sock_desc_rd)) {
                     HTTPsSock_ConnAccept(p_instance, p_instance->SockListenID_IPv4);
                 }
#endif
#ifdef NET_IPv6_MODULE_EN
                 if (NET_SOCK_DESC_IS_SET(p_instance->SockListenID_IPv6, &sock_desc_rd)) {
                     HTTPsSock_ConnAccept(p_instance, p_instance->SockListenID_IPv6);
                 }
#endif
                 break;


            default:
                 break;
        }
    }


                                                                /* ------------ UPDATE CONN SOCK STATE  --------------- */
    p_conn = p_instance->ConnFirstPtr;
    while (p_conn != DEF_NULL) {
#if (HTTPs_CFG_PERSISTENT_CONN_EN == DEF_ENABLED)               /* Check if conn is deemed persistent. See Note (1).    */
        persistent = DEF_BIT_IS_SET(p_conn->Flags, HTTPs_FLAG_CONN_PERSISTENT)      ||
                     (p_conn->ProtocolVer                  == HTTP_PROTOCOL_VER_1_1 &&
                      p_instance->CfgPtr->ConnPersistentEn == DEF_TRUE);

        if (persistent == DEF_TRUE) {
            p_sock = NetSock_GetObj(p_conn->SockID);            /* Check if instance is in the midst of closing.        */
            if (p_sock != DEF_NULL) {
                p_sock = NetSock_GetObj(p_sock->ID_SockParent);
                if (p_sock == DEF_NULL && !accept) {            /* If listen sock is free, consider instance is closing.*/
                    if (p_conn->SockState == HTTPs_SOCK_STATE_RX) {
                                                                /* Wait for idle timeout expiry; do not process RX state*/
                                                                /* Force RESP_COMPLETED state. (See HTTPsConn_Process())*/
                        p_conn->SockState = HTTPs_SOCK_STATE_NONE;
                        p_conn->State     = HTTPs_CONN_STATE_RESP_COMPLETED;
                    }
                }
            }
        }
#endif
        switch (p_conn->SockState) {
            case HTTPs_SOCK_STATE_RX:                           /* Conn rdy to receive data.                            */
                 if (NET_SOCK_DESC_IS_SET(p_conn->SockID, &sock_desc_rd)) {
                     DEF_BIT_SET(p_conn->SockFlags, HTTPs_FLAG_SOCK_RDY_RD);    /* Sock rdy.                            */
                 }

                 if (NET_SOCK_DESC_IS_SET(p_conn->SockID, &sock_desc_err)) {
                     DEF_BIT_SET(p_conn->SockFlags, HTTPs_FLAG_SOCK_RDY_ERR);   /* Sock Err is pending.                 */
                     p_conn->SockState  = HTTPs_SOCK_STATE_ERR;
                 }
                 break;


            case HTTPs_SOCK_STATE_TX:
                 if (NET_SOCK_DESC_IS_SET(p_conn->SockID, &sock_desc_wr)) {
                     DEF_BIT_SET(p_conn->SockFlags, HTTPs_FLAG_SOCK_RDY_WR);    /* Sock rdy.                            */
                 }

                 if (NET_SOCK_DESC_IS_SET(p_conn->SockID, &sock_desc_err)) {
                     DEF_BIT_SET(p_conn->SockFlags, HTTPs_FLAG_SOCK_RDY_ERR);   /* Sock Err is pending.                 */
                     p_conn->SockState  = HTTPs_SOCK_STATE_ERR;
                 }
                 break;


            case HTTPs_SOCK_STATE_ERR:
            case HTTPs_SOCK_STATE_NONE:                         /* Conn needs to be processed.                          */
                 if (NET_SOCK_DESC_IS_SET(p_conn->SockID, &sock_desc_err)) {
                     DEF_BIT_SET(p_conn->SockFlags, HTTPs_FLAG_SOCK_RDY_ERR);   /* Sock Err is pending.                 */
                     p_conn->SockState  = HTTPs_SOCK_STATE_ERR;
                 }
                 sock_nbr_rdy++;
                 break;


            default:
                 break;
        }

        p_conn = p_conn->ConnNextPtr;
    }

    return (sock_nbr_rdy);
}


/*
*********************************************************************************************************
*                                        HTTPsSock_ConnDataRx()
*
* Description : Receive data from the network and update connection.
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*               ------      Argument validated in HTTPsSock_ConnAccept().
*
* Return(s)   : DEF_OK,   if data received successfully.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPsConn_Process().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPsSock_ConnDataRx (HTTPs_INSTANCE  *p_instance,
                                   HTTPs_CONN      *p_conn)
{
    CPU_CHAR              *p_buf;
    NET_SOCK_ADDR_LEN      addr_len_client;
    CPU_INT16U             rx_len;
    CPU_INT32U             buf_len;
    HTTPs_INSTANCE_STATS  *p_ctr_stats;
    HTTPs_INSTANCE_ERRS   *p_ctr_err;
    CPU_BOOLEAN            rtn_val;
    NET_ERR                err;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

    if ((p_conn->RxBufLenRem > 0) &&
        (p_conn->RxBufPtr   != p_conn->BufPtr)) {               /* If data is still present in the rx buf.              */
                                                                /* Move rem data to the beginning of the rx buf.        */
        Mem_Copy(p_conn->BufPtr, p_conn->RxBufPtr, p_conn->RxBufLenRem);
    }

    p_buf   = p_conn->BufPtr + p_conn->RxBufLenRem;
    buf_len = p_conn->BufLen - p_conn->RxBufLenRem;

    if (buf_len == 0) {
        rtn_val = DEF_OK;
        goto exit;
    }

    addr_len_client = sizeof(p_conn->ClientAddr);
    rx_len = (CPU_INT16U)NetSock_RxDataFrom(        p_conn->SockID,
                                            (void *)p_buf,
                                                    buf_len,
                                                    NET_SOCK_FLAG_NO_BLOCK,
                                                   &p_conn->ClientAddr,
                                                   &addr_len_client,
                                                    DEF_NULL,
                                                    DEF_NULL,
                                                    DEF_NULL,
                                                   &err);
    switch (err) {
        case NET_SOCK_ERR_NONE:                                 /* Data received.                                       */
        case NET_SOCK_ERR_INVALID_DATA_SIZE:
             p_conn->RxBufPtr      = p_conn->BufPtr;
             p_conn->RxBufLenRem  += rx_len;
             HTTPs_STATS_OCTET_INC(p_ctr_stats->Sock_StatOctetRxdCtr, rx_len);
             break;


        case NET_ERR_RX:                                        /* Transitory rx err(s).                                */
        case NET_INIT_ERR_NOT_COMPLETED:
        case NET_SOCK_ERR_RX_Q_EMPTY:
        case NET_ERR_FAULT_LOCK_ACQUIRE:
             p_conn->RxBufPtr = p_conn->BufPtr;
             HTTPs_ERR_INC(p_ctr_err->Sock_ErrRxCtr);
             rtn_val = DEF_FAIL;
             goto exit;


        case NET_SOCK_ERR_CLOSED:                               /* Conn closed by peer.                                 */
        case NET_SOCK_ERR_RX_Q_CLOSED:
             HTTPs_ERR_INC(p_ctr_err->Sock_ErrRxConnClosedCtr);
             p_conn->SockState = HTTPs_SOCK_STATE_CLOSE;
             rtn_val = DEF_FAIL;
             goto exit;


        default:                                                /* Fatal err.                                           */
             HTTPs_ERR_INC(p_ctr_err->Sock_ErrRxFaultCtr);
             p_conn->SockState = HTTPs_SOCK_STATE_ERR;
             rtn_val = DEF_FAIL;
             goto exit;

    }

    rtn_val = DEF_OK;


exit:
    return (rtn_val);
}


/*
*********************************************************************************************************
*                                        HTTPsSock_ConnDataTx()
*
* Description : Transmit data on the network and update connection parameters.
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*               ------      Argument validated in HTTPsSock_ConnAccept().
*
* Return(s)   : DEF_OK,   if data transmitted successfully.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPsConn_Process().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN HTTPsSock_ConnDataTx (HTTPs_INSTANCE  *p_instance,
                                  HTTPs_CONN      *p_conn)
{
    NET_SOCK_ADDR_LEN      addr_len_client;
    NET_SOCK_RTN_CODE      tx_len;
    HTTPs_INSTANCE_STATS  *p_ctr_stats;
    HTTPs_INSTANCE_ERRS   *p_ctr_err;
    NET_ERR                err;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

    addr_len_client = sizeof(p_conn->ClientAddr);

    tx_len = NetSock_TxDataTo(p_conn->SockID,
                              p_conn->TxBufPtr,
                              p_conn->TxDataLen,
                              NET_SOCK_FLAG_NO_BLOCK,
                             &p_conn->ClientAddr,
                              addr_len_client,
                             &err);
    switch (err) {
        case NET_SOCK_ERR_NONE:                                 /* Data transmitted.                                    */
        case NET_ERR_TX:
        case NET_INIT_ERR_NOT_COMPLETED:
        case NET_SOCK_ERR_PORT_NBR_NONE_AVAIL:
        case NET_CONN_ERR_NONE_AVAIL:
        case NET_ERR_FAULT_LOCK_ACQUIRE:
             if (tx_len != NET_SOCK_BSD_ERR_DFLT) {
                 HTTPs_STATS_OCTET_INC(p_ctr_stats->Sock_StatOctetTxdCtr, tx_len);
                 p_conn->TxDataLen -= tx_len;
                 if (p_conn->TxDataLen > 0u) {                      /* If data is not entirely transmitted.                 */
                     p_conn->TxBufPtr = (CPU_CHAR *)p_conn->TxBufPtr + tx_len;
                     return (DEF_FAIL);
                 } else {
                     p_conn->TxBufPtr = p_conn->BufPtr;
                 }
             } else {
                 return (DEF_FAIL);
             }
             break;


        case NET_SOCK_ERR_CLOSED:                               /* Conn closed by peer.                                 */
        case NET_SOCK_ERR_TX_Q_CLOSED:
             HTTPs_ERR_INC(p_ctr_err->Sock_ErrTxConnClosedCtr);
             p_conn->SockState = HTTPs_SOCK_STATE_CLOSE;
             return (DEF_FAIL);


        default:                                                /* Fatal err.                                           */
             HTTPs_ERR_INC(p_ctr_err->Sock_ErrTxFaultCtr);
             p_conn->SockState = HTTPs_SOCK_STATE_ERR;
             return (DEF_FAIL);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                         HTTPsSock_ConnClose()
*
* Description : Close connection socket.
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
*               p_conn      Pointer to the connection.
*               ------      Argument validated in HTTPsSock_ConnAccept().
*
* Return(s)   : none.
*
* Caller(s)   : HTTPsConn_Close().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  HTTPsSock_ConnClose (HTTPs_INSTANCE  *p_instance,
                           HTTPs_CONN      *p_conn)
{
    HTTPs_INSTANCE_STATS  *p_ctr_stats;
    HTTPs_INSTANCE_ERRS   *p_ctr_err;
    CPU_BOOLEAN            done;
    NET_ERR                err;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);

    done  = DEF_NO;
    while (done != DEF_YES) {

        (void)NetSock_Close(p_conn->SockID, &err);
         switch (err) {
             case NET_SOCK_ERR_NONE:
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
                  done = DEF_YES;
                  HTTPs_STATS_INC(p_ctr_stats->Conn_StatClosedCtr);
                  break;

             case NET_INIT_ERR_NOT_COMPLETED:
             case NET_ERR_FAULT_LOCK_ACQUIRE:                    /* Transitory fault(s); close aborted but MIGHT ...     */
                  break;                                         /* ... close in a subsequent attempt.                   */

             default:
                  done = DEF_YES;
                  HTTPs_ERR_INC(p_ctr_err->Sock_ErrCloseCtr);
                  break;
         }
    }
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
*                                          HTTPsSock_ConnAccept()
*
* Description : (1) Accept new incoming connection:
*
*                   (a) Accept incoming connection.
*                   (b) Configure TCP   connection MSL timeout.
*                   (b) Acquire free    connection structure for the new accepted connection request.
*
* Argument(s) : p_instance  Pointer to the instance.
*               ----------  Argument validated in HTTPs_InstanceStart().
*
* Return(s)   : none.
*
* Caller(s)   : HTTPsSock_ConnSel().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  HTTPsSock_ConnAccept (HTTPs_INSTANCE  *p_instance,
                                    NET_SOCK_ID      sock_listen_id)
{
        HTTPs_CONN            *p_conn;
        HTTPs_INSTANCE_ERRS   *p_ctr_err;
        HTTPs_INSTANCE_STATS  *p_ctr_stats;
    const  HTTPs_CFG             *p_cfg = p_instance->CfgPtr;
        NET_SOCK_ID            sock_id;
        NET_SOCK_ADDR          client_addr;
        NET_SOCK_ADDR_LEN      addr_len;
        CPU_BOOLEAN            done;
        NET_TCP_CONN_ID        conn_id_tcp;
        NET_ERR                err_net;


    HTTPs_SET_PTR_STATS(p_ctr_stats, p_instance);
    HTTPs_SET_PTR_ERRS( p_ctr_err,   p_instance);


    done = DEF_NO;
    while (done != DEF_YES) {
        CPU_BOOLEAN  close_sock = DEF_NO;
        CPU_BOOLEAN  flag       = DEF_DISABLED;

                                                                /* ---------------- ACCEPT INCOMING REQ --------------- */
        addr_len = sizeof(client_addr);

        sock_id = NetSock_Accept(sock_listen_id,
                                &client_addr,
                                &addr_len,
                                &err_net);
        switch (err_net) {
            case NET_SOCK_ERR_NONE:                             /* New conn accepted.                                   */
                 HTTPs_STATS_INC(p_ctr_stats->Conn_StatAcceptedCtr);

                (void)NetSock_OptSet(sock_id,                   /* Set inactivity timeout.                              */
                                     NET_SOCK_PROTOCOL_TCP,
                                     NET_SOCK_OPT_TCP_KEEP_IDLE,
                            (void *)&p_cfg->ConnInactivityTimeout_s,
                                     sizeof(p_cfg->ConnInactivityTimeout_s),
                                    &err_net);
                 if (err_net != NET_SOCK_ERR_NONE) {
                     HTTPs_ERR_INC(p_ctr_err->Conn_ErrTmrStartCtr);
                     close_sock = DEF_YES;
                     break;
                 }


                (void)NetSock_OptSet(sock_id,                   /* Set NO DELAY option.                                 */
                                     NET_SOCK_PROTOCOL_TCP,
                                     NET_SOCK_OPT_TCP_NO_DELAY,
                            (void *)&flag,
                                     sizeof(flag),
                                    &err_net);
                 if (err_net != NET_SOCK_ERR_NONE) {
                     close_sock = DEF_YES;
                     break;
                 }

                                                                /* ----------------- CFG TCP CONN MSL ----------------- */
                 conn_id_tcp = NetSock_GetConnTransportID(sock_id, &err_net);

                (void)NetTCP_ConnCfgMSL_Timeout(conn_id_tcp, 0u, &err_net);
                 if (err_net != NET_TCP_ERR_NONE) {
                     close_sock = DEF_YES;
                     break;
                 }

                                                                /* --------------- ACQUIRE CONN STRUCT ---------------- */
                 p_conn = HTTPsMem_ConnGet(p_instance,
                                            sock_id,
                                            client_addr);
                 if (p_conn != DEF_NULL) {
                     p_conn->State = HTTPs_CONN_STATE_REQ_INIT;

                 } else {                                       /* If no free conn struct avail...                      */
                                                                /* ... close sock.                                      */
                     HTTPs_ERR_INC(p_ctr_err->Conn_ErrNoneAvailCtr);
                     close_sock = DEF_YES;
                     break;
                 }
                 break;

            case NET_INIT_ERR_NOT_COMPLETED:
            case NET_SOCK_ERR_NONE_AVAIL:
            case NET_SOCK_ERR_CONN_ACCEPT_Q_NONE_AVAIL:
            case NET_SOCK_ERR_CONN_SIGNAL_TIMEOUT:
            case NET_ERR_FAULT_LOCK_ACQUIRE:
                 done = DEF_YES;
                 break;

            default:
                 HTTPs_ERR_INC(p_ctr_err->Sock_ErrAcceptCtr);
                 close_sock = DEF_YES;
                 break;
        }

        if ((close_sock == DEF_YES) &&
            (sock_id != NET_SOCK_ID_NONE)) {
            (void)NetSock_Close(sock_id, &err_net);
        }
    }
}
