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
*                                     NET FS STATIC CONFIGURATION
*
* Filename : http-s_fs_static_cfg.h
* Version  : V3.01.01
*********************************************************************************************************
*/

#ifndef HTTPs_FS_STATIC_CFG_MODULE_PRESENT
#define HTTPs_FS_STATIC_CFG_MODULE_PRESENT


/*
*********************************************************************************************************
*                                             NET FS CFG
*********************************************************************************************************
*/

                                                                /* Configure external argument check feature ...        */
#define  HTTPs_FS_CFG_ARG_CHK_EXT_EN                     DEF_ENABLED
                                                                /*   DEF_DISABLED     External argument check DISABLED  */
                                                                /*   DEF_ENABLED      External argument check ENABLED   */


#define  HTTPs_FS_CFG_MAX_FILE_NAME_LEN                  256    /* Configure maximum file name length.                  */
#define  HTTPs_FS_CFG_NBR_FILES                           15    /* Configure number of files.                           */
#define  HTTPs_FS_CFG_NBR_DIRS                             1    /* Configure number of directories.                     */


#endif
