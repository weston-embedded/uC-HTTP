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
*                                     HTTP CLIENT 2LEMETRY MODULE
*
* Filename : 2lemetry.h
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

#ifndef  TWOLEMETRY_MODULE_PRESENT
#define  TWOLEMETRY_MODULE_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef   TWOLEMETRY_MODULE
#define  TWOLEMETRY_EXT
#else
#define  TWOLEMETRY_EXT  extern
#endif

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  TWOLEMETRY_CFG_REQ_NBR_MAX                        2u


#define  TWOLEMETRY_CFG_CONN_NBR_MAX                       3u

#define  TWOLEMETRY_CFG_CONN_BUF_SIZE                   4096u


#define  TWOLEMETRY_CFG_HDR_NBR_MAX                       10u
#define  TWOLEMETRY_CFG_HDR_VAL_LEN_MAX                  300u

#if 0
#define  TWOLEMETRY_CFG_QUERY_STR_NBR_MAX                  3u
#define  TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX             50u
#define  TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX            100u
#endif

#define  TWOLEMETRY_URL_LEN_MAX                          300u


#define  TWOLEMETRY_API_HOSTNAME                  "api.m2m.io"
#define  TWOLEMETRY_URL_MESSAGES                  "/2/publish"
#define  TWOLEMETRY_URL_DOMAIN                    "/2/account/domain/"

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPE
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  void         (*TWOLEMETRY_RX_DATA)     (void                      *p_data,
                                                 CPU_INT16U                 data_len,
                                                 CPU_BOOLEAN                last_chunk,
                                                 void                      *p_user_ctx);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/



TWOLEMETRY_EXT   CPU_CHAR                 Twolemetry_Buf[1024];
#if 0
TWOLEMETRY_EXT   HTTPc_KEY_VAL            Twolemetry_ReqQueryStrTbl[TWOLEMETRY_CFG_QUERY_STR_NBR_MAX];

TWOLEMETRY_EXT   CPU_CHAR                 Twolemetry_ReqQueryStrKeyTbl[TWOLEMETRY_CFG_QUERY_STR_NBR_MAX][TWOLEMETRY_CFG_QUERY_STR_KEY_LEN_MAX];
TWOLEMETRY_EXT   CPU_CHAR                 Twolemetry_ReqQueryStrValTbl[TWOLEMETRY_CFG_QUERY_STR_NBR_MAX][TWOLEMETRY_CFG_QUERY_STR_VAL_LEN_MAX];
#endif

/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/


CPU_BOOLEAN Twolemetry_Publish       (CPU_CHAR             *p_client_id,
                                      CPU_CHAR             *p_topic,
                                      CPU_CHAR             *p_payload,
                                      CPU_CHAR             *p_qos,
                                      CPU_CHAR             *p_credentials);

CPU_BOOLEAN Twolemetry_GetData       (CPU_CHAR             *p_domain,
                                      CPU_CHAR             *p_stuff,
                                      CPU_CHAR             *p_thing,
                                      CPU_CHAR             *p_whatever,
                                      CPU_CHAR             *p_credentials,
                                      TWOLEMETRY_RX_DATA    fnct,
                                      void                 *p_ctx);

CPU_BOOLEAN Twolemetry_SendSMS       (CPU_CHAR             *p_message,
                                      CPU_CHAR             *p_to_phone_number,
                                      CPU_CHAR             *p_topic,
                                      CPU_CHAR             *p_domain,
                                      CPU_CHAR             *p_credentials);

CPU_BOOLEAN Twolemetry_PushStoreKey  (CPU_CHAR             *p_domain,
                                      CPU_CHAR             *p_key,
                                      CPU_CHAR             *p_kvp,
                                      CPU_BOOLEAN           protect,
                                      CPU_CHAR             *p_credentials);

CPU_BOOLEAN Twolemetry_GetStoreKey   (CPU_CHAR             *p_domain,
                                      CPU_CHAR             *p_key,
                                      CPU_BOOLEAN           protect,
                                      CPU_CHAR             *p_credentials,
                                      TWOLEMETRY_RX_DATA    fnct,
                                      void                 *p_ctx);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* TWOLEMTRY_MODULE_PRESENT */
