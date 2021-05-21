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
*                           HTTP CLIENT SOUNDCLOUD APPLICATION SOURCE CODE
*
* Filename : soundcloud.c
* Version  : V3.01.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/
#define  MICRIUM_SOURCE
#define  SOUNDCLOUD_MODULE


#include  <lib_def.h>

#include  <Common/http.h>
#include  <Client/Source/http-c.h>
#include  <http-c_cfg.h>

#include  <Source/net_util.h>

#include  "soundcloud.h"
#include  <SL/Audio_Buffer_Handler.h>
#include  <MAD/mad.h>
#include  <lib_mem.h>
#include  <os_cpu.h>
#include  <MK70F12.h>
#include  <Client/Source/http-c_task.h>
#include  <os.h>
#include  <audio.h>
/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/



static  void         SoundCloud_ConnCloseCallback       (HTTPc_CONN_OBJ            *p_conn,
                                                         HTTPc_CONN_CLOSE_STATUS    close_status,
                                                         HTTPc_ERR                  err);

static  void         SoundCloud_TransErrCallback        (HTTPc_CONN_OBJ            *p_conn,
                                                         HTTPc_REQ_OBJ             *p_req,
                                                         HTTPc_ERR                  err_code);

static void          SoundCloud_RespBodyHook            (HTTPc_CONN_OBJ            *p_conn,
                                                         HTTPc_REQ_OBJ             *p_req,
                                                         HTTP_CONTENT_TYPE          content_type,
                                                         void                      *p_data,
                                                         CPU_INT16U                 data_len,
                                                         CPU_INT32U                *data_len_read,
                                                         CPU_BOOLEAN                last_chunk);

static void          SoundCloud_RespHdrHook             (HTTPc_CONN_OBJ            *p_conn,
                                                         HTTPc_REQ_OBJ             *p_req,
                                                         HTTP_HDR_FIELD             hdr_field,
                                                         CPU_CHAR                  *p_hdr_val,
                                                         CPU_INT16U                 val_len);

static enum mad_flow SoundCloud_MP3Decode_Input         (void                      *data,
                                                         struct mad_stream         *stream);

static enum mad_flow SoundCloud_MP3Decode_Output        (void                      *data,
                                                         struct mad_header const   *header,
                                                         struct mad_pcm            *pcm,
                                                         struct mad_stream         *stream);

static enum mad_flow SoundCloud_MP3Decode_Error         (void                      *data,
                                                         struct mad_stream         *stream,
                                                         struct mad_frame          *frame);
/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  SOUNDCLOUD_POOL_BLK_NBR                  4
#define  SOUNDCLOUD_TASK_PRIO                     5u                    /* Minimum task priority.   */
#define  SOUNDCLOUD_TASK_STACK_SIZE               20*1024                   /* Minimum task stack size. */
#define  SOUNDCLOUD_TASK_NAME                      "SoundCloud Task"           /* Task Name.               */

#define  SOUNDCLOUD_Q_NAME      "SoundCloud Queue"

KAL_Q_HANDLE          SoundCloudQ_Handle;

MEM_POOL     SoundCloud_MsgPool;

typedef struct buffer {
  unsigned char const *start;
  unsigned long length;
}BUFFER;

typedef struct stream_request {
  CPU_CHAR URL[900];
  CPU_CHAR Host[30];
}STREAM_REQUEST;

typedef struct soundcloud_msg {
  CPU_CHAR   buffer[SOUNDCLOUD_CFG_CONN_BUF_SIZE];
  CPU_INT32U buffer_len;
}SOUNDCLOUD_MSG;

STREAM_REQUEST request;

//CPU_CHAR   buff[];


#define  SOUNDCLOUD_TASK_MSGBUF_SEM_NAME             "SoundCloud Msg Buf Semaphore"

KAL_SEM_HANDLE  SoundCloudTask_MsgBufSem_Handle;


/*
*********************************************************************************************************
*                                           SoundCloud_GetStream()
*
* Description : Get Stream URL from SoundCloud Server
*
* Argument(s) : p_url     Pointer to the sender phone number
*
* Return(s)   : DEF_OK,   if HTTPc transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN SoundCloud_GetStream    (CPU_CHAR         *p_url)
{
    HTTPc_CONN_OBJ        *p_conn;
    HTTPc_REQ_OBJ         *p_req;
    HTTPc_RESP_OBJ        *p_resp;
    CPU_CHAR               buf[SOUNDCLOUD_CFG_CONN_BUF_SIZE];
    CPU_SIZE_T             str_len;
    CPU_BOOLEAN            result;
    HTTPc_ERR              err;
    CPU_INT08U             conn_retry_ctr;


    p_conn = &SoundCloud_ConnTbl[0];
    p_req  = &SoundCloud_ReqTbl[0];
    p_resp = &SoundCloud_RespTbl[0];

    result = DEF_FALSE;
    conn_retry_ctr=0;
                                                        /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(p_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(p_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------- SET CONNECTION CALLBACKS ------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)

    HTTPc_ConnSetParam(         p_conn,
                                HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       (void *)&SoundCloud_TransErrCallback,
                               &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
                                                      /* --------- SET REQ/RESP CALLBACK FUNCTIONS ---------- */
    HTTPc_ReqSetParam(        p_req,
                               HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      (void *)&SoundCloud_TransErrCallback,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqSetParam( p_req,
                                HTTPc_PARAM_TYPE_RESP_HDR_HOOK,
                      (void*)&SoundCloud_RespHdrHook,
                               &err);
             if (err != HTTPc_ERR_NONE) {
                 return (DEF_FAIL);
             }
#endif

                                                                /* ----------------- OPEN CONNECTION ------------------ */
    str_len = Str_Len(SOUNDCLOUD_API_HOSTNAME);
    while (result != DEF_YES) {
        result  = HTTPc_ConnOpen(p_conn,
                                  buf,
                                  SOUNDCLOUD_CFG_CONN_BUF_SIZE,
                                  SOUNDCLOUD_API_HOSTNAME,
                                  str_len,
                                  HTTPc_FLAG_NONE,
                                 &err);
        if (err != HTTPc_ERR_NONE  ||
            result == DEF_FAIL) {
              if (conn_retry_ctr < SOUNDCLOUD_CONN_RETRY_MAX) {
                  conn_retry_ctr++;
              } else {
                  return (DEF_FAIL);
              }
         }
    }                                                           /* ---------------- SEND HTTP REQUEST ----------------- */
    str_len = Str_Len(p_url);
    result  = HTTPc_ReqSend(p_conn,
                            p_req,
                            p_resp,
                             HTTP_METHOD_GET,
                            p_url,
                             str_len,
                             HTTPc_FLAG_NONE,
                            &err);
    if (err != HTTPc_ERR_NONE ||
        result != DEF_OK        ) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                           SoundCloud_PlayStream()
*
* Description : Send a SMS message
*
* Argument(s) : p_from_phone_number     Pointer to the sender phone number
*
*               p_to_phone_number       Pointer to the destination phone number
*
*               p_message               Pointer to the body of the message to be sent
*
*               p_account_id            Pointer to the account SID
*
*               p_auth_token            Pointer to the authentication token
*
* Return(s)   : DEF_OK,   if HTTPc transaction was successful.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN SoundCloud_PlayStream    (CPU_CHAR        *p_host,
                                      CPU_CHAR        *p_url,
                                      CPU_CHAR        *p_buf)
{
    HTTPc_CONN_OBJ        *p_conn;
    HTTPc_REQ_OBJ          *p_req;
    HTTPc_RESP_OBJ         *p_resp;
    CPU_CHAR               buf[SOUNDCLOUD_CFG_CONN_BUF_SIZE];
    CPU_SIZE_T             str_len;
    CPU_BOOLEAN            result;
    HTTPc_ERR              err;
    CPU_INT08U             conn_retry_ctr;


    stream_buff.WrAddress = p_buf;
    stream_buff.Start     = p_buf;
    stream_buff.DecodingStarted = DEF_FALSE;

    result = DEF_FALSE;

    p_conn = &SoundCloud_ConnTbl[1];
    p_req  = &SoundCloud_ReqTbl[1];
    p_resp = &SoundCloud_RespTbl[1];

                                                                /* ----------- INIT NEW CONNECTION & REQUEST ---------- */
    HTTPc_ConnClr(p_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    HTTPc_ReqClr(p_req, &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------- SET CONNECTION CALLBACKS ------------- */
#if (HTTPc_CFG_MODE_ASYNC_TASK_EN == DEF_ENABLED)

    HTTPc_ConnSetParam(        p_conn,
                                HTTPc_PARAM_TYPE_CONN_CLOSE_CALLBACK,
                       (void *)&SoundCloud_ConnCloseCallback,
                               &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }


                                                               /* ------------ SET REQ/RESP HOOK FUNCTIONS ----------- */
    HTTPc_ReqSetParam(        p_req,
                               HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      (void *)&SoundCloud_RespBodyHook,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* --------- SET REQ/RESP CALLBACK FUNCTIONS ---------- */

    HTTPc_ReqSetParam(        p_req,
                               HTTPc_PARAM_TYPE_TRANS_ERR_CALLBACK,
                      (void *)&SoundCloud_TransErrCallback,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

        HTTPc_ReqSetParam(        p_req,
                               HTTPc_PARAM_TYPE_TRANS_COMPLETE_CALLBACK,
                      (void *)&SoundCloud_ConnCloseCallback,
                              &err);
    if (err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

#endif
                                                                /* ----------------- OPEN CONNECTION ------------------ */
    str_len = Str_Len(p_host);
    while (result != DEF_YES) {
        result  = HTTPc_ConnOpen(p_conn,
                                  buf,
                                  SOUNDCLOUD_CFG_CONN_BUF_SIZE,
                                  p_host,
                                  str_len,
                                  HTTPc_FLAG_NONE,
                                 &err);
        if (err != HTTPc_ERR_NONE  ||
            result == DEF_FAIL) {
              if (conn_retry_ctr < SOUNDCLOUD_CONN_RETRY_MAX) {
                  conn_retry_ctr++;
              } else {
                  return (DEF_FAIL);
              }
         }
    }
                                                                /* ---------------- SEND HTTP REQUEST ----------------- */
    str_len = Str_Len(p_url);
    result  = HTTPc_ReqSend(p_conn,
                            p_req,
                            p_resp,
                             HTTP_METHOD_GET,
                            p_url,
                             str_len,
                             HTTPc_FLAG_REQ_NO_BLOCK,
                            &err);
    if (err != HTTPc_ERR_NONE ||
        result != DEF_OK        ) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
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
*                                        SoundCloud_RespHdrHook()
*
* Description : Retrieve header fields in the HTTP response received.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection object.
*
*               p_req       Pointer to current HTTPc Request object.
*
*               hdr_field   HTTP header type of the header field received in the HTTP response.
*
*               p_hdr_val   Pointer to the value string received in the Response header field.
*
*               val_len     Length of the value string.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcResp_ParseHdr() via 'p_req->OnHdrRx()'
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  SoundCloud_RespHdrHook (HTTPc_CONN_OBJ    *p_conn,
                             HTTPc_REQ_OBJ     *p_req,
                             HTTP_HDR_FIELD     hdr_field,
                             CPU_CHAR          *p_hdr_val,
                             CPU_INT16U         val_len)
{
  CPU_CHAR  *str_parse;
  CPU_INT16U host_len;

  if (hdr_field == HTTP_HDR_FIELD_LOCATION) {
      if (Str_Cmp_N(p_hdr_val, "https://",8)==0) {
          p_hdr_val +=8;
          val_len -=8;
          str_parse =  Str_Char(p_hdr_val,ASCII_CHAR_SOLIDUS);
          host_len =  str_parse - p_hdr_val;
          Str_Copy_N(request.Host, p_hdr_val,host_len);
          val_len -= host_len;
          Str_Copy_N(request.URL, str_parse, val_len);
      }
  }
}


/*
*********************************************************************************************************
*                                       SoundCloud_RespBodyHook()
*
* Description : Retrieve data in HTTP Response received.
*
* Argument(s) : p_conn          Pointer to current HTTPc Connection object.
*
*               p_req           Pointer to current HTTPc Request object.
*
*               content_type    HTTP Content Type of the HTTP Response body's data.
*
*               p_data          Pointer to a data piece of the HTTP Response body.
*
*               data_len        Length of the data piece received.
*
*               last_chunk      DEF_YES, if this is the last piece of data.
*
*                               DEF_NO,  if more data is up coming.
*
* Return(s)   : None.
*
* Caller(s)   : HTTPcResp_BodyStd()   via 'p_req->OnBodyRx()'
*               HTTPcResp_BodyChunk() via 'p_req->OnBodyRx()'
*
* Note(s)     : None.
*********************************************************************************************************
*/


void  SoundCloud_RespBodyHook   (HTTPc_CONN_OBJ     *p_conn,
                                 HTTPc_REQ_OBJ      *p_req,
                                 HTTP_CONTENT_TYPE   content_type,
                                 void               *p_data,
                                 CPU_INT16U          data_len,
                                  CPU_INT32U         *data_len_read,
                                 CPU_BOOLEAN         last_chunk)
{
  KAL_ERR err_kal;
  SOUNDCLOUD_MSG *p_msg;
  LIB_ERR     err_lib;


    KAL_SemPend(SoundCloudTask_MsgBufSem_Handle,
                KAL_OPT_PEND_NON_BLOCKING,
                1,
                &err_kal);

    switch (err_kal){
        case KAL_ERR_NONE:

              p_msg = (SOUNDCLOUD_MSG *)Mem_PoolBlkGet(&SoundCloud_MsgPool,
                                                        sizeof(SOUNDCLOUD_MSG),
                                                       &err_lib);

              Mem_Copy(p_msg->buffer, p_data, data_len);
              p_msg->buffer_len = data_len;

              KAL_QPost( SoundCloudQ_Handle,
                         p_msg,
                         KAL_OPT_NONE,
                       &err_kal);

              *data_len_read = data_len;
             break;

        default:
            *data_len_read = 0;
            break;

     }

}


/*
*********************************************************************************************************
*                                     SoundCloud_ConnCloseCallback()
*
* Description : Callback to notify application that an HTTP connection was closed.
*
* Argument(s) : p_conn          Pointer to current HTTPc Connection.
*
*               close_status    Status of the connection closing:
*                                   HTTPc_CONN_CLOSE_STATUS_ERR_INTERNAL
*                                   HTTPc_CONN_CLOSE_STATUS_SERVER
*                                   HTTPc_CONN_CLOSE_STATUS_NO_PERSISTENT
*                                   HTTPc_CONN_CLOSE_STATUS_APP
*
*               err             Error Code when connection was closed.
*
* Return(s)   : None.
*
* Caller(s)   : Various, via 'p_conn->OnClose()'
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef HTTPc_TASK_MODULE_EN
static void  SoundCloud_ConnCloseCallback (HTTPc_CONN_OBJ             *p_conn,
                                           HTTPc_CONN_CLOSE_STATUS   close_status,
                                           HTTPc_ERR                 err)
{


}
#endif


/*
*********************************************************************************************************
*                                      SoundCloud_TransErrCallback()
*
* Description : Callback to notify application that an error occurred during an HTTP transaction.
*
* Argument(s) : p_conn      Pointer to current HTTPc Connection object.
*
*               p_req       Pointer to current HTTPc Request object.
*
*               err_code    Error Code.
*
* Return(s)   : None.
*
* Caller(s)   : Various, via 'p_req->OnErr()'.
*
* Note(s)     : None.
*********************************************************************************************************
*/
#ifdef HTTPc_TASK_MODULE_EN
static void SoundCloud_TransErrCallback (HTTPc_CONN_OBJ  *p_conn,
                                         HTTPc_REQ_OBJ   *p_req,
                                         HTTPc_ERR        err_code)
{

}
#endif

void SoundCloudTask_Init(void)
{
    KAL_TASK_HANDLE   task_handle;
    KAL_ERR           err_kal;
    LIB_ERR           err_lib;
    OS_SEM           *MsgBufSem;

    SoundCloudTask_MsgBufSem_Handle = KAL_SemCreate( SOUNDCLOUD_TASK_MSGBUF_SEM_NAME,
                                                     DEF_NULL,
                                                    &err_kal);

    MsgBufSem = SoundCloudTask_MsgBufSem_Handle.SemObjPtr;

    MsgBufSem->Ctr = SOUNDCLOUD_POOL_BLK_NBR;

                                                                /* ------- ALLOCATE MEMORY SPACE FOR SoundCloud TASK  ------ */
    task_handle = KAL_TaskAlloc((const  CPU_CHAR *)SOUNDCLOUD_TASK_NAME,
                                                   DEF_NULL,
                                                   SOUNDCLOUD_TASK_STACK_SIZE,
                                                   DEF_NULL,
                                                  &err_kal);

                                                                    /* ---------------- CREATE MSG Q ------------------ */
    SoundCloudQ_Handle = KAL_QCreate(SOUNDCLOUD_Q_NAME,
                                     SOUNDCLOUD_POOL_BLK_NBR,
                                     DEF_NULL,
                                     &err_kal);

                                                                /* ---------------- CREATE SOUNDCLOUD TASK ----------------- */
    KAL_TaskCreate(task_handle,
                   SoundCloudTask,
                   DEF_NULL,
                   SOUNDCLOUD_TASK_PRIO,
                   DEF_NULL,
                   &err_kal);

    Mem_PoolCreate(&SoundCloud_MsgPool,
                    DEF_NULL,
                    SOUNDCLOUD_POOL_BLK_NBR* sizeof(SOUNDCLOUD_MSG),
                    SOUNDCLOUD_POOL_BLK_NBR,
                    sizeof(SOUNDCLOUD_MSG),
                    sizeof(CPU_SIZE_T),
                    0,
                    &err_lib);
}


/*
*********************************************************************************************************
*                                          SoundCloudTask()
*
* Description : SoundCloud Mp3 playback main loop.
*
* Argument(s) : p_data      Pointer to task initialization (required by uC/OS-III).
*
* Return(s)   : None.
*
* Caller(s)   :
*
* Note(s)     : None.
*********************************************************************************************************
*/

void SoundCloudTask(void *p_data)
{

   (void)p_data;

    audio_start();

    SoundCloud_MP3Decode();

}


/*
*********************************************************************************************************
*                                          SoundCloud_MP3Decode()
*
* Description : SoundCloud Mp3 playback main loop.
*
* Argument(s) : start      Pointer to start of the MP3 buffer
*
*               length     Length of the MP3 buffer
*
* Return(s)   : None.
*
* Caller(s)   :
*
* Note(s)     : None.
*********************************************************************************************************
*/
CPU_INT32U SoundCloud_MP3Decode()
{
    MAD_DECODER decoder;
    CPU_INT32U result;


     /* configure input, output, and error functions */

    mad_decoder_init(&decoder,
                     0,
                     SoundCloud_MP3Decode_Input,
                     0 /* header */,
                     0 /* filter */,
                     SoundCloud_MP3Decode_Output,
                     SoundCloud_MP3Decode_Error,
                     0 /* message */);

    /* start decoding */

    result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

    /* release the decoder */

    mad_decoder_finish(&decoder);

    return result;
}


/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

static enum mad_flow SoundCloud_MP3Decode_Input(void              *data,
                                                struct mad_stream *stream)
{
    KAL_ERR         err_kal;
    SOUNDCLOUD_MSG *p_msg;
    LIB_ERR         err_lib;
    CPU_INT32U      leftovers_len;


    leftovers_len = 0;

    if (stream_buff.DecodingStarted == DEF_TRUE) {

        leftovers_len = stream->bufend - stream->this_frame;

        stream_buff.WrAddress = stream_buff.Start;

        Mem_Copy(stream_buff.Start, stream->this_frame, leftovers_len );

        stream_buff.WrAddress = stream_buff.Start + leftovers_len;

        Mem_PoolBlkFree(&SoundCloud_MsgPool,
                         stream_buff.p_msg,
                        &err_lib);

        KAL_SemPost( SoundCloudTask_MsgBufSem_Handle,
                     KAL_OPT_POST_NONE,
                   &err_kal);
    }


    p_msg =   (SOUNDCLOUD_MSG*)KAL_QPend(SoundCloudQ_Handle,
                                         KAL_OPT_PEND_BLOCKING,
                                         0,
                                         &err_kal);


    Mem_Copy(stream_buff.WrAddress, p_msg->buffer, p_msg->buffer_len + leftovers_len);
    stream_buff.p_msg = p_msg;

    mad_stream_buffer(stream,stream_buff.Start, p_msg->buffer_len + leftovers_len);

    stream_buff.DecodingStarted = DEF_TRUE;

    return MAD_FLOW_CONTINUE;
}


static enum mad_flow SoundCloud_MP3Decode_Error(void              *data,
                                                struct mad_stream *stream,
                                                struct mad_frame  *frame)
{
  return MAD_FLOW_CONTINUE;
}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */

static enum mad_flow SoundCloud_MP3Decode_Output(void                    *data,
                                                 struct mad_header const *header,
                                                 struct mad_pcm          *pcm,
                                                 struct mad_stream       *stream)
{
  CPU_INT32U  nchannels;
  CPU_INT32U  nsamples;
  CPU_INT32U *left_ch;
  CPU_INT32U *right_ch;
  KAL_ERR     err_kal;
  CPU_INT16U  index;
  CPU_INT32U  loop_factor;
  CPU_INT32U  temp_buff[2304];



  loop_factor = 1152;

  /* pcm->samplerate contains the sampling frequency */

  nchannels = pcm->channels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];


  while (nsamples != 0) {

    if (nsamples < loop_factor) {
        loop_factor = nsamples;
    }


    if(AUDIO_BUFFER_HANDLER_CHECK_STATUS(AUDIO_BUFFER_HANDLER_TX_STARTED)) {

       AudioBufferHandler_WriteSignalWait(&err_kal);
    }

    for (index=0;index < loop_factor;index++) {

         temp_buff[2*index]   = (CPU_INT32U) (*left_ch++);
         temp_buff[2*index+1] = (CPU_INT32U) (*right_ch++);
    }

    vfnAudioBufferHandler_WriteBuffer(&temp_buff[0],2304);

    nsamples -= loop_factor;
  }

  return MAD_FLOW_CONTINUE;
}


/*
*********************************************************************************************************
*                                        SoundCloud_PlayTrack()
*
* Description : Retrieve header fields in the HTTP response received.
*
* Argument(s) : track_id      ID of the track to be played.
*
*               client_id     Your SoundCloud ClientId.
*
* Return(s)   : None.
*
* Caller(s)   : Application
*
* Note(s)     : None.
*********************************************************************************************************
*/

void SoundCloud_PlaytTrack(CPU_CHAR *track_id,
                           CPU_CHAR *client_id)
{
    CPU_CHAR *p_buf;
    LIB_ERR   mem_err;
    CPU_CHAR  url[SOUNDCLOUD_URL_LEN_MAX];

    Mem_Clr(&url[0], SOUNDCLOUD_URL_LEN_MAX);

    Str_Copy(&url[0], SOUNDCLOUD_URL_TRACKS);
    Str_Cat( &url[0], track_id);
    Str_Cat( &url[0], "/stream?client_id=");
    Str_Cat( &url[0], client_id);


    p_buf = Mem_HeapAlloc((SOUNDCLOUD_CFG_CONN_BUF_SIZE+850),
                          DEF_NULL, DEF_NULL,
                          &mem_err);

    SoundCloud_GetStream(&url[0]);
    SoundCloud_PlayStream(request.Host,
                          request.URL,
                          p_buf);

}
