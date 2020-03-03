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

#ifndef  APP_STATIC_FILES_PRESENT
#define  APP_STATIC_FILES_PRESENT


#define  STATIC_LOGO_GIF_NAME               "\\logo.gif"
#define  STATIC_INDEX_HTML_NAME             "\\index.html"

#define  STATIC_LOGO_GIF_LEN                2066u
#define  STATIC_INDEX_HTML_LEN              3080u


extern const unsigned char logo_gif[];
extern const unsigned char index_html[];

#endif
