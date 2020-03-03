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
*                                      HTTP SERVER STRING MODULE
*
* Filename : http-s_str.c
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
#define    HTTPs_STR_MODULE

#include  "http-s_str.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         HTTPs_StrPathFormat()
*
* Description : (1) Format file path:
*
*                   (a) Copy folder    in path destination buffer.
*                   (b) Copy file name in path destination buffer.
*                   (c) Replace path separator character.
*
* Argument(s) : p_filename      Pointer to file name string.
*               ----------      Argument validated in HTTPsSock_ConnAccept().
*
*               p_folder        Pointer to the folder string.
*               --------        Argument validated in HTTPs_InstanceStart().
*
*               p_path_dst      Pointer to string buffer where to format the path.
*               ----------      Argument validated in HTTPsSock_ConnAccept().
*
*               path_len_max    Maximum length of the path.
*
*               path_sep        Path character separator.
*
* Return(s)   : DEF_OK,   path successfully formated.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPsReq_BodyFormMultipartCtrlParse().
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if ((HTTPs_CFG_FORM_EN           == DEF_ENABLED) && \
     (HTTPs_CFG_FORM_MULTIPART_EN == DEF_ENABLED))
CPU_BOOLEAN  HTTPs_StrPathFormat (CPU_CHAR    *p_filename,
                                  CPU_CHAR    *p_folder,
                                  CPU_CHAR    *p_path_dst,
                                  CPU_SIZE_T   path_len_max,
                                  CPU_CHAR     path_sep)
{
    CPU_CHAR    *p_str_file;
    CPU_CHAR    *p_str_folder;
    CPU_CHAR    *p_str;
    CPU_CHAR    *p_dst;
    CPU_SIZE_T   len_folder;
    CPU_SIZE_T   len;


    p_str_file = p_filename;
    p_dst      = p_path_dst;
    len        = path_len_max;

                                                                /* ----------- COPY FOLDER IN PATH DST BUF ------------ */
    if (p_folder != DEF_NULL) {                                 /* Only if folder is not null.                          */
        p_str_folder = p_folder;
        while ((*p_str_folder == ASCII_CHAR_SOLIDUS)         || /* Must NOT start with a path sep.                      */
              (*p_str_folder == ASCII_CHAR_REVERSE_SOLIDUS)) {
            p_str_folder++;
        }

        (void)Str_Copy_N(p_dst, p_str_folder, path_len_max);    /* Copy folder to the dst.                              */
        len_folder = Str_Len_N(p_str_folder, path_len_max);

        if (len_folder > 0) {                                   /* Verify that folder name is not empty.                */
            p_dst     += (len_folder - 1);                      /* Verify if folder name end with path sep.             */
            len       -= (len_folder - 1);
                                                                /* Must have a path sep before filename.                */
            if ((*p_dst == ASCII_CHAR_SOLIDUS)          ||      /* Verify if folder name end with path sep.             */
                (*p_dst == ASCII_CHAR_REVERSE_SOLIDUS)) {
                  if (len_folder > 1) {
                      p_dst++;
                      len--;
                  }

            } else {                                            /* Add a path sep. at end of folder name.               */
                p_dst++;
               *p_dst = ASCII_CHAR_REVERSE_SOLIDUS;
                p_dst++;
                len -= 2;
            }
        }
    }

                                                                /* ---------- COPY FILE NAME IN PATH DST BUF ---------- */
    while ((*p_str_file == ASCII_CHAR_SOLIDUS)         ||       /* Must NOT start with a path sep.                      */
           (*p_str_file == ASCII_CHAR_REVERSE_SOLIDUS)) {
        p_str_file++;
    }

    (void)Str_Copy_N(p_dst, p_str_file, path_len_max);          /* Copy file name.                                      */


                                                                /* --------- REPLACE PATH SEP IN PATH DST BUF --------- */
    if (path_sep != ASCII_CHAR_SOLIDUS) {
        p_str = Str_Char_Replace_N(p_dst,
                                   ASCII_CHAR_SOLIDUS,
                                   path_sep,
                                   len);
        if (p_str == DEF_NULL) {
            return (DEF_FAIL);
        }
    }

    return (DEF_OK);
}
#endif


/*
*********************************************************************************************************
*                                        HTTPs_StrFilePathGet()
*
* Description : Retrieve file path in a file path that contains the host in the path.
*
* Argument(s) : p_path              Pointer to the path that can contains the host.
*
*               path_len_max        Path length maximum.
*
*               p_host              Pointer to a string that contains the host name.
*
*               host_len_max        Host string length maximum.
*
*               p_resp_location     Pointer to a variable that will be set if the location field header should be send.
*
* Return(s)   : Pointer to the file path.
*
* Caller(s)   : HTTPsFS_Open(),
*               HTTPsResp_HdrFieldAdd().
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (HTTPs_CFG_ABSOLUTE_URI_EN == DEF_ENABLED)
CPU_CHAR  *HTTPs_StrPathGet (CPU_CHAR     *p_path,
                             CPU_INT16U    path_len_max,
                             CPU_CHAR     *p_host,
                             CPU_INT16U    host_len_max,
                             CPU_BOOLEAN  *p_resp_location)
{
    CPU_CHAR    *p_file_path;
    CPU_INT32U   host_len;


    p_file_path = Str_Str_N(p_path, p_host, path_len_max);
    if (p_file_path != DEF_NULL) {
        host_len         = Str_Len_N(p_host, host_len_max);
        p_file_path     += host_len;
        if (p_resp_location != DEF_NULL) {
           *p_resp_location  = DEF_YES;
        }

    } else {
        p_file_path = p_path;
    }

    return (p_file_path);
}
#endif


/*
*********************************************************************************************************
*                                          HTTPs_StrMemSrch()
*
* Description : Search for a string into a memory section.
*
* Argument(s) : p_data      Pointer to a buffer that may contain the boundary token.
*
*               data_len    Length of the buffer.
*
*               p_str       Pointer to string.
*
*               str_len     String length.
*
* Return(s)   : Pointer to the beginning of boundary token, if found,
*
*               DEF_NULL                                  , otherwise.
*
* Caller(s)   : HTTPsReq_BodyFormMultipartBoundarySrch().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if ((HTTPs_CFG_FORM_EN           == DEF_ENABLED) && \
     (HTTPs_CFG_FORM_MULTIPART_EN == DEF_ENABLED))
CPU_CHAR  *HTTPs_StrMemSrch (const  CPU_CHAR    *p_data,
                                    CPU_INT32U   data_len,
                             const  CPU_CHAR    *p_str,
                                    CPU_INT32U   str_len)
{
    CPU_CHAR    *p_search;
    CPU_INT32S   search_len;
    CPU_INT32S   i;
    CPU_BOOLEAN  cmp_identical;


                                                                /* ----------------- INPUT ARG CHECK ------------------ */
    if (p_data == DEF_NULL) {
        return (DEF_NULL);
    }

    if (p_str == DEF_NULL) {
        return (DEF_NULL);
    }

    if (data_len < 1) {
        return (DEF_NULL);
    }

    if (str_len < 1) {
        return (DEF_NULL);
    }

    if (data_len < str_len) {
        return (DEF_NULL);
    }

                                                                /* ------------------ MEM SEARCH-CMP ------------------ */
    p_search   = (CPU_CHAR *)p_data;
    search_len = (CPU_INT32S)(data_len - str_len);
    for (i = search_len; i >= 0; i--) {
        cmp_identical = Mem_Cmp(p_search, p_str, str_len);
        if (cmp_identical == DEF_YES) {
            return (p_search);
        }

        p_search++;
    }

    return (DEF_NULL);
}
#endif

