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
*                         HTTP INSTANCE INITALIZATION WITH NO FS APPLICATION
*
* Filename : app_no_fs.c
* Version  : V3.01.01
*********************************************************************************************************
* Note(s)  : (1) This example shows how to initialize uC/HTTPs without the need of a File System,
*                 initialize a web server instance and start it.
*
*            (2) This example is for :
*
*                  (a) uC/TCPIP - V3.00.01 and up
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
*********************************************************************************************************
*/

#include  <Server/Source/http-s.h>
#include  "app_no_fs.h"
#include  "app_no_fs_http-s_instance_cfg.h"


/*
*********************************************************************************************************
*                                            AppNoFS_Init()
*
* Description : (1) Initialize uC/HTTPs and start a web server instance:
*
*                   (a) Initialize uC/HTTPs module.
*                   (b) Initialize a web server instance.
*                   (c) Start that web server instance.
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
*
*               (3) Prior to do any call to uC/HTTPs, the module must be initialized. This is done by
*                   calling HTTPs_Init(). If the process is successful, the Web server internal data structures are
*                   initialized.
*
*               (4) Each web server must be initialized before it can be started or stopped. HTTPs_InstanceInit()
*                   is responsible to allocate memory for the instance, initialize internal data structure and
*                   create the web server instance's task.
*
*                   (a) The first argument is the instance configuration, which should be modified following you
*                       requirements. The intance's configuration set the server's port, the number of connection that
*                       can be accepted, the hooks functions, etc.
*
*                   (b) The second argument is the instance task configuration. It set the task priority, the task
*                       stack size, etc.
*
*               (5) Once a web server instance is initialized, it can be started using HTTPs_InstanceStart() to
*                   become come accessible. This function start the web server instance's task. Each instance has
*                   is own task and all accepted connection is processed with this single task.
*********************************************************************************************************
*/

CPU_BOOLEAN  AppNoFS_Init (void)
{
    HTTPs_INSTANCE  *p_https_instance;
    HTTPs_ERR        http_err;


    /* TODO: Prerequisites modules must be initialized prior calling any of the following functions. See Note #2.       */

                                                                /* -------------- INITALIZE HTTPS MODULE -------------- */
    HTTPs_Init(DEF_NULL, &http_err);                            /* See Note #3.                                         */
    if (http_err != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ---------- INITALIZE WEB SERVER INSTANCE ----------- */
    p_https_instance = HTTPs_InstanceInit(&HTTPs_CfgInstance_NoFS,     /* Instance configuration. See Note #4a.         */
                                          &HTTPs_TaskCfgInstance_NoFS, /* Instance Task Configuration. See Note #4b.    */
                                          &http_err);
    if (http_err != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------ START WEB SERVER INSTANCE ------------- */
    HTTPs_InstanceStart(p_https_instance,                       /* Instance handle. See Note #5.                        */
                       &http_err);
    if (http_err != HTTPs_ERR_NONE) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}

