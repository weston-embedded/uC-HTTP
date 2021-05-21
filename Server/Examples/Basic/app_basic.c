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
*                                   HTTP INSTANCE BASIC APPLICATION
*
* Filename : app_basic.c
* Version  : V3.01.01
*********************************************************************************************************
* Note(s)  : (1) This example shows how to initialize uC/HTTPs, initialize a web server instance and start it.
*
*            (2) This example is for :
*
*                  (a) uC/TCPIP - V3.00.01 and up
*                  (b) uC/FS    - V4.x
*
*            (3) This file is an example about how to use uC/HTTPs, It may not cover all case needed by a real
*                application. Also some modification might be needed, insert the code to perform the stated
*                actions wherever 'TODO' comments are found.
*
*                (a) For example this example doesn't manage the link state (plugs and unplugs), this can
*                    be a problem when switching from a network to another network.
*
*                (b) This example is not fully tested, so it is not guaranteed that all cases are cover
*                    properly.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*
* Note(s) : (1) (a) The uC/TCP-IP Network file system abstraction layer folder for traditional file system;
*
*                       '$uC-TCPIP/FS/<fs>/net_<fs>.h'
*
*
*               (b) The static file system API is located under uC/HTTPs File system folder;
*
*                       '$uc-HTTPs/FS/Static/http-s_fs_static.h'
*********************************************************************************************************
*/
#define    MICRIUM_SOURCE
#define    APP_BASIC_MODULE

#include  "app_basic.h"

#include  <Server/Source/http-s.h>

#if (APP_BASIC_FS_DYN_EN == DEF_ENABLED)
#include  <FS/uC-FS-V4/net_fs_v4.h>                             /* TODO Location of File system port API. See Note 1.   */
#include  <fs_app.h>
#else
#include  <Server/FS/Static/http-s_fs_static.h>                 /* TODO Location of File system port API. See Note 1.   */
#include  "../Common/StaticFiles/generated_fs.h"
#endif

#include  "app_basic_http-s_instance_cfg.h"


/*
*********************************************************************************************************
*                                            AppBasic_Init()
*
* Description : (1) Initialize uC/HTTPs and start a web server instance:
*
*                   (a) Initialize the File System.
*                   (b) Initialize uC/HTTPs module.
*                   (c) Initialize a web server instance.
*                   (d) Start that web server instance.
*
* Argument(s) : none.
*
* Return(s)   : DEF_OK, if successfully initialized and started (can access the web server).*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (2) Prerequisite modules must be initialized before calling any uC/HTTPs functions:
*
*                   (a) RTOS, such as uCOS-II or uCOS-III
*                   (b) uC/LIB
*                   (c) uC/CPU
*                   (d) uC/TCP-IP
*                   (e) File system, such as uC/FS.
*
*               (3) Dynamic File System: No files are loaded inside the File System at this point.
*                                        One way to add files to the FS is to use to send POST request to the HTTP
*                                        server with HTML form for file upload.
*
*                   HTTP Static FS     : The files can be loaded in the file system with the function
*                                        HTTPs_FS_AddFile().
*
*               (4) Prior to do any call to uC/HTTPs, the module must be initialized. This is done by
*                   calling HTTPs_Init(). If the process is successful, the Web server internal data structures are
*                   initialized.
*
*               (5) Each web server must be initialized before it can be started or stopped. HTTPs_InstanceInit()
*                   is responsible to allocate memory for the instance, initialize internal data structure and
*                   create the web server instance's task.
*
*                   (a) The first argument is the instance configuration, which should be modified following you
*                       requirements. The intance's configuration set the server's port, the number of connection that
*                       can be accepted, the hooks functions, etc.
*
*                   (b) The second argument is the pointer to the instance task configuration. It set the task priority,
*                       the stack size of the task, etc.
*
*               (6) Once a web server instance is initialized, it can be started using HTTPs_InstanceStart() to
*                   become come accessible. This function start the web server instance's task. Each instance has
*                   is own task and all accepted connection is processed with this single task.
*********************************************************************************************************
*/

CPU_BOOLEAN  AppBasic_Init (void)
{

    HTTPs_INSTANCE  *p_https_instance;
    CPU_BOOLEAN      success;
    HTTPs_ERR        http_err;


    /* TODO: Prerequisites modules must be initialized prior calling any of the following functions. See Note #2.       */


#if (APP_BASIC_FS_DYN_EN == DEF_ENABLED)                        /* REAL DYNAMIC FILE SYSTEM CASE                        */
    /* TODO: Make sure File System (such as uC/FS) is initialized before starting the instance. See Note #3.            */

                                                                /* --------------- INITIALIZE FS STACK ---------------- */
    success = App_FS_Init();
    if (success != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

#else                                                           /* HTTP STATIC FILE SYSTEM CASE                         */
    /* TODO: Make sure the HTTP Static File System is initialized before starting the instance. See Note #3.            */
    /*       Make sure files that must be served by the web server is loaded in the Static file system. See Note #3.    */

                                                                /* -------- INITALIZE HTTP STATIC FILE SYSTEM --------- */
    success = HTTPs_FS_Init();
    if (success != DEF_YES) {
        return (DEF_FAIL);
    }

                                                                /* ------- ADD FILES IN THE STATIC FILE SYSTEM -------- */
                                                                /* TODO change the following call to add only the ...   */
                                                                /* file required by you web application.                */
    success = GeneratedFS_FileAdd();
    if (success != DEF_YES) {
        return (DEF_FAIL);
    }
#endif

                                                                /* -------------- INITALIZE HTTPS MODULE -------------- */
    HTTPs_Init(DEF_NULL, &http_err);                            /* See Note #4.                                         */
    if (http_err != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ---------- INITALIZE WEB SERVER INSTANCE ----------- */
    p_https_instance = HTTPs_InstanceInit(&HTTPs_CfgInstance_AppBasic,    /* Instance configuration. See Note #5a.      */
                                          &HTTPs_TaskCfgInstance_AppBasic,/* Instance task configuration. See Note #5b. */
                                          &http_err);
    if (http_err != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------ START WEB SERVER INSTANCE ------------- */
    HTTPs_InstanceStart(p_https_instance,                       /* Instance handle. See Note #6.                        */
                       &http_err);
    if (http_err != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

    AppBasic_ErrCtr = 0;

    return (DEF_OK);
}

