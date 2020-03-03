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

#include  <Server/FS/Static/http-s_fs_static.h>
#include  "generated_fs.h"
#include  <lib_def.h>

typedef  struct  gen_file_desc {
    CPU_CHAR    *Name;
    CPU_CHAR    *Content;
    CPU_INT32U   Size;
} GEN_FILE_DESC;

static  const  GEN_FILE_DESC  FileTbl[] = {
    {_404_HTML_NAME,    _404_HTML_CONTENT,    _404_HTML_SIZE},
    {FORM_HTML_NAME,    FORM_HTML_CONTENT,    FORM_HTML_SIZE},
    {INDEX_HTML_NAME,   INDEX_HTML_CONTENT,   INDEX_HTML_SIZE},
    {LIST_HTML_NAME,    LIST_HTML_CONTENT,    LIST_HTML_SIZE},
    {LOGIN_HTML_NAME,   LOGIN_HTML_CONTENT,   LOGIN_HTML_SIZE},
    {LOGO_GIF_NAME,     LOGO_GIF_CONTENT,     LOGO_GIF_SIZE},
    {UC_STYLE_CSS_NAME, UC_STYLE_CSS_CONTENT, UC_STYLE_CSS_SIZE},
    {0, 0, 0}
};

CPU_BOOLEAN GeneratedFS_FileAdd() {
    const  GEN_FILE_DESC  *p_file_desc = &FileTbl[0];
           CPU_BOOLEAN     result      = DEF_OK;


    while ((p_file_desc->Name != 0) && (result == DEF_OK)) {
        result = HTTPs_FS_AddFile(p_file_desc->Name, p_file_desc->Content, p_file_desc->Size);
        p_file_desc++;
    }

    return (result);
}
