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
*                                     HTTP CLIENT DROPBOX MODULE
*
* Filename : dropbox.h
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

#ifndef  DROPBOX_MODULE_PRESENT
#define  DROPBOX_MODULE_PRESENT

/*
*********************************************************************************************************
*                                             FS MODULE
*
* Note(s): If the uC/FS is present in the project, you can enable it for the example application.
*********************************************************************************************************
*/

#define  HTTPc_APP_FS_MODULE_PRESENT   DEF_YES


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <Client/Source/http-c.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/



#define  DROPBOX_CFG_CONN_BUF_SIZE              512u

#define  DROPBOX_CFG_HDR_VAL_LEN_MAX            300u

#define  DROPBOX_URL_LEN_MAX                     50u


#define  DROPBOX_API_CONTENT_IPv4_ADDR         "23.23.153.198"
#define  DROPBOX_API_IPv4_ADDR                 "50.19.116.212"

#define  DROPBOX_API_CONTENT_HOSTNAME          "api-content.dropbox.com"
#define  DROPBOX_API_HOSTAME                   "api.dropbox.com"

#define  DROPBOX_URL_FILES_PUT                 "/1/files_put/auto/"
#define  DROPBOX_URL_FILES_GET                 "/1/files/auto/"
#define  DROPBOX_URL_FILES_OPS                 "/1/fileops/"




/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPE
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  void         (*DROPBOX_RX_DATA)        (CPU_CHAR                  *remote_file_name,
                                                 void                      *p_data,
                                                 CPU_INT16U                 data_len,
                                                 CPU_BOOLEAN                last_chunk,
                                                 void                      *p_user_ctx);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/


CPU_BOOLEAN  Dropbox_Upload_File   (CPU_CHAR              *p_remote_file_name,
                                    CPU_CHAR              *p_local_file_name,
                                    HTTP_CONTENT_TYPE      content_type,
                                    CPU_SIZE_T             content_len,
                                    CPU_CHAR              *p_access_token);

CPU_BOOLEAN  Dropbox_Upload_Buf    (CPU_CHAR              *p_remote_file_name,
                                    CPU_INT08U              *p_buf,
                                    HTTP_CONTENT_TYPE      content_type,
                                    CPU_SIZE_T             content_len,
                                    CPU_CHAR              *p_access_token);

CPU_BOOLEAN  Dropbox_Download_Buf  (CPU_CHAR              *p_remote_file_name,
                                    CPU_CHAR              *p_access_token,
                                    DROPBOX_RX_DATA        fnct,
                                    void                  *p_ctx);

CPU_BOOLEAN  Dropbox_Download_File (CPU_CHAR              *p_remote_file_name,
                                    CPU_CHAR              *p_local_file_name,
                                    CPU_CHAR              *p_access_token);

CPU_BOOLEAN  Dropbox_Delete_File   (CPU_CHAR              *p_remote_file_name,
                                    CPU_CHAR              *p_access_token);



/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif
