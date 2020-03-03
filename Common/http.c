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
*                                                HTTP
*
* Filename : http.c
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
#define    HTTP_MODULE
#include  "http.h"
#include  "http_dict.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void         HTTP_CharToStrHex     (       CPU_CHAR     character,
                                                   CPU_CHAR    *p_str_char);

static  CPU_CHAR     HTTP_StrHexToChar     (const  CPU_CHAR    *p_str_char);

#if 0
static  CPU_INT08U   HTTP_StrSizeHexDigReq (       CPU_INT32U   nbr);
#endif

static  CPU_CHAR    *HTTP_StrFileExtGet    (       CPU_CHAR    *p_file_name,
                                                   CPU_SIZE_T   path_len);


/*
*********************************************************************************************************
*                                   HTTP_HdrParseFieldValueGet()
*
* Description : Get the beginning of a field value.
*
* Argument(s) : p_field         Pointer to the beginning of the field line.
*
*               field_len       Field length.
*
*               p_field_end     Pointer to the end of the field line.
*
*               p_len_rem       Pointer to a variable that will receive the remaining length.
*
* Return(s)   : Pointer to the beginning of the field value.
*
* Caller(s)   : Various.
*
* Note(s)     : (1) RFC #2616, section "4.2 Message Headers" describe how Header field should be formated.
*
*                   (a) HTTP header fields, which include general-header (section 4.5), request-header
*                       (section 5.3), response-header (section 6.2), and entity-header (section 7.1)
*                       fields, follow the same generic format as that given in Section 3.1 of RFC 822 [9].
*                       Each header field consists of a name followed by a colon (":") and the field value.
*                       Field names are case-insensitive. The field value MAY be preceded by any amount of
*                       LWS, though a single SP is preferred. Header fields can be extended over multiple
*                       lines by preceding each extra line with at least one SP or HT. Applications ought
*                       to follow "common form", where one is known or indicated, when generating HTTP
*                       constructs, since there might exist some implementations that fail to accept
*                       anything
*
*                       beyond the common forms.
*
*                              message-header = field-name ":" [ field-value ]
*                              field-name     = token
*                              field-value    = *( field-content | LWS )
*                              field-content  = <the OCTETs making up the field-value
*                                               and consisting of either *TEXT or combinations
*                                               of token, separators, and quoted-string>
*********************************************************************************************************
*/

CPU_CHAR  *HTTP_HdrParseFieldValueGet (CPU_CHAR    *p_field,
                                       CPU_INT16U   field_len,
                                       CPU_CHAR    *p_field_end,
                                       CPU_INT16U  *p_len_rem)
{
    CPU_INT16U   len;
    CPU_CHAR    *p_val;


    p_val     =  p_field + field_len;
    len       = (p_field_end - p_val);

    p_val     =  Str_Char_N(p_val, len, ASCII_CHAR_COLON);      /* Field val located after ':' (see Note #1a).          */
    p_val++;

    len       = (p_field_end - p_val);
    p_val     =  HTTP_StrGraphSrchFirst(p_val, len);            /* Remove blank space before field value.               */

   *p_len_rem = (p_field_end - p_val);

    return (p_val);
}


/*
*********************************************************************************************************
*                                       HTTP_StrGraphSrchFirst()
*
* Description : Get pointer to the first graphic character of a string.
*
* Argument(s) : p_str       Pointer to the string to search in.
*
*               str_len     String length.
*
* Return(s)   : Pointer to the first graphic character, if successfully found.
*
*               DEF_NULL,                               otherwise.
*
* Caller(s)   : Various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_CHAR  *HTTP_StrGraphSrchFirst (CPU_CHAR    *p_str,
                                   CPU_INT32U   str_len)
{
    CPU_CHAR    *p_char;
    CPU_INT32U   len_rem;


    len_rem = str_len;
    p_char  = p_str;
    while ((ASCII_IS_GRAPH(*p_char) == DEF_NO) &&
           (len_rem > 0)                      ) {
        p_char++;
        len_rem--;
    }

    if (len_rem == 0) {
        return ((CPU_CHAR *)0);
    }

    return (p_char);
}


/*
*********************************************************************************************************
*                                         HTTP_URL_EncodeStr()
*
* Description : Perform URL encoding on the given string.
*
* Argument(s) : p_str_src       Pointer to source string to encode.
*
*               p_str_dest      Pointer to destination string where the encoded string will be copied.
*
*               p_str_len       Argument used to pass length of the source string and
*                                   to return length of the encoded destination string.
*
*               str_len_max     Maximum length of the encoded string.
*
* Return(s)   : DEF_OK,   if encoding succeeded.
*
*               DEF_FAIL, otherwise
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTP_URL_EncodeStr (const  CPU_CHAR    *p_str_src,
                                        CPU_CHAR    *p_str_dest,
                                        CPU_SIZE_T  *p_str_len,
                                        CPU_SIZE_T   str_len_max)
{
    CPU_BOOLEAN   alpha_num;
    CPU_BOOLEAN   unreserved;
    CPU_BOOLEAN   result;
    CPU_INT16U    len;
    CPU_INT16U    len_copy;
    CPU_INT16U    ctr;


                                                                /* -------- VALIDATE DESTINATION BUFFER LENGTH -------- */
    ctr      = 0u;
    for (len = 0u; len < *p_str_len; len++) {

        alpha_num = ASCII_IsAlphaNum(*p_str_src);               /* Check if char is alphabetic or numeric.              */

        switch (*p_str_src) {                                   /* Check if char is an unreserved character.            */
            case ASCII_CHAR_FULL_STOP:
            case ASCII_CHAR_HYPHEN_MINUS:
            case ASCII_CHAR_TILDE:
            case ASCII_CHAR_LOW_LINE:
                 unreserved = DEF_YES;
                 break;


            default:
                 unreserved = DEF_NO;
                 break;
        }

        if ((alpha_num  == DEF_NO) &&
            (unreserved == DEF_NO)) {
            ctr++;                                              /* Increment number of char to encode.                  */
        }

        p_str_src++;
    }

    len = *p_str_len + HTTP_URL_ENCODING_JUMP*ctr;

    if (len > str_len_max) {
        len_copy = 0u;
        result   = DEF_FAIL;
        goto exit;
    }

                                                                /* --------- COPY URL ENCODING STRING BUFFER ---------- */
    p_str_src -= *p_str_len;                                    /* Reset pointer of source string.                      */

    len_copy = 0u;
    for (len = 0u; len < *p_str_len; len++) {

        alpha_num = ASCII_IsAlphaNum(*p_str_src);               /* Check if char is alphabetic or numeric.              */

        switch (*p_str_src) {                                   /* Check if char is an unreserved character.            */
            case ASCII_CHAR_FULL_STOP:
            case ASCII_CHAR_HYPHEN_MINUS:
            case ASCII_CHAR_TILDE:
            case ASCII_CHAR_LOW_LINE:
                 unreserved = DEF_YES;
                 break;


            default:
                 unreserved = DEF_NO;
                 break;
        }

        if ((alpha_num  == DEF_YES) ||
            (unreserved == DEF_YES)) {
           *p_str_dest = *p_str_src;
            len_copy++;
        } else {
            HTTP_CharToStrHex(*p_str_src, p_str_dest);          /* Convert char to its %-encoding equivalent.           */
            if (*p_str_dest != ASCII_CHAR_NULL) {
                len_copy   += HTTP_URL_ENCODING_LEN;
                p_str_dest += HTTP_URL_ENCODING_JUMP;
            } else {
                result = DEF_FAIL;
                goto exit;
            }
        }

        p_str_src++;
        p_str_dest++;
    }

    result = DEF_OK;


exit:
   *p_str_len = len_copy;

    return (result);
}


/*
*********************************************************************************************************
*                                         HTTP_URL_DecodeStr()
*
* Description : Perform URL decoding on the given string.
*
* Argument(s) : p_str_src   Pointer to source string to decode.
*
*               p_str_dest  Pointer to destination string where the decoded string will be copied.
*
*               p_str_len   Argument used to pass the length of the source string and
*                               to return the length of the decoded destination string.
*
* Return(s)   : DEF_OK,   if decoding succeeded.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTP_URL_DecodeStr (const  CPU_CHAR    *p_str_src,
                                        CPU_CHAR    *p_str_dest,
                                        CPU_SIZE_T  *p_str_len)
{
    CPU_CHAR     char_tmp;
    CPU_INT16U   len;
    CPU_INT16U   len_copy;
    CPU_BOOLEAN  result;


    len_copy = 0;

    for (len = 0u; len < *p_str_len; len++) {
        if (*p_str_src == ASCII_CHAR_PERCENTAGE_SIGN) {
             char_tmp = HTTP_StrHexToChar(p_str_src);
             if (char_tmp != ASCII_CHAR_NULL) {
                *p_str_dest = char_tmp;
                 len_copy++;
                 p_str_src += HTTP_URL_ENCODING_JUMP;
                 len       += HTTP_URL_ENCODING_JUMP;
             } else {
                  result = DEF_FAIL;
                  goto exit;
             }
        } else if (*p_str_src == ASCII_CHAR_PLUS_SIGN) {
            *p_str_dest = ASCII_CHAR_SPACE;
             len_copy++;
        } else {
            *p_str_dest = *p_str_src;
             len_copy++;
        }
        p_str_src++;
        p_str_dest++;
    }

    result = DEF_OK;


exit:
   *p_str_len = len_copy;

    return (result);
}


/*
*********************************************************************************************************
*                                      HTTP_URL_DecodeReplaceStr()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_str       Pointer to the string to scan and replace characters.
*
*               p_str_len   Argument used to pass the length of the source string and
*                               to return the length of the decoded destination string.
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTP_URL_DecodeReplaceStr (CPU_CHAR    *p_str,
                                        CPU_INT16U  *p_str_len)
{
    CPU_CHAR    *p_str_char;
    CPU_CHAR    *p_str_copy_src;
    CPU_CHAR    *p_str_copy_dest;
    CPU_INT32U   len;
    CPU_INT32U   len_copy;
    CPU_BOOLEAN  result;


    p_str_char =  p_str;
    len        = *p_str_len;

    p_str[*p_str_len] = ASCII_CHAR_NULL;                        /* Make a sure to create a str.                         */

    while (*p_str_char != ASCII_CHAR_NULL) {

        if (*p_str_char == ASCII_CHAR_PLUS_SIGN) {              /* See Note #1a.                                        */
            *p_str_char  = ASCII_CHAR_SPACE;

        } else if (*p_str_char == ASCII_CHAR_PERCENTAGE_SIGN) { /* See Note #1b.                                        */

            *p_str_char      = HTTP_StrHexToChar(p_str_char);   /* Convert Hex str to 1 char.                           */
             if (*p_str_char == ASCII_CHAR_NULL) {
                  result = DEF_FAIL;
                  goto exit;
             }
             p_str_copy_dest = p_str_char + 1;                  /* Rem str must be moved after the char.                */
             p_str_copy_src  = p_str_char + 3;                  /* Rem str is located after hex str (3 char).           */

             len_copy = len - 2;

             Str_Copy_N(p_str_copy_dest,                        /* Copy rem str.                                        */
                        p_str_copy_src,
                        len_copy);

            *p_str_len -= 2;                                    /* Str has lost 2 char.                                 */

        }

        p_str_char++;                                           /* Move the next char.                                  */
        len--;
    }

    result = DEF_OK;

exit:
    return (result);
}


/*
*********************************************************************************************************
*                                       HTTP_URL_CharEncodeNbr()
*
* Description : Calculate number of characters that would be encoded if an URL encoding is performed on the
*               given string.
*
* Argument(s) : p_str_src   Pointer to string that would be encoded.
*
*               str_len     Length of string.
*
* Return(s)   : Number of character that would be encoded.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT16U  HTTP_URL_CharEncodeNbr (const  CPU_CHAR    *p_str_src,
                                           CPU_SIZE_T   str_len)
{
    CPU_BOOLEAN  alpha_num;
    CPU_BOOLEAN  unreserved;
    CPU_INT16U   ctr;
    CPU_INT16U   len;


    ctr = 0u;
    for (len = 0u; len < str_len; len++) {
        alpha_num = ASCII_IsAlphaNum(*p_str_src);               /* Check if char is alphabetic or numeric.              */

        switch (*p_str_src) {                                   /* Check if char is an unreserved character.            */
            case ASCII_CHAR_FULL_STOP:
            case ASCII_CHAR_HYPHEN_MINUS:
            case ASCII_CHAR_TILDE:
            case ASCII_CHAR_LOW_LINE:
                 unreserved = DEF_YES;
                 break;


            default:
                 unreserved = DEF_NO;
                 break;
        }

        if ((alpha_num  == DEF_NO) &&
            (unreserved == DEF_NO)) {
             ctr++;                                             /* Increment number of char to encode.                  */
        }

        p_str_src++;
    }

    return (ctr);
}


/*
*********************************************************************************************************
*                                      HTTP_ChunkTransferWrSize()
*
* Description : Write to buffer the chunk size value in hexadecimal representation, plus the CRLF characters.
*
* Argument(s) : p_buf_wr    Pointer to buffer in which to write chunk size value.
*
*               buf_len     Buffer length remaining.
*
*               buf_len_max Maximum buffer length.
*
*               size        Chunk size value to write.
*
* Return(s)   : Pointer to end of written data in buffer.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_CHAR  *HTTP_ChunkTransferWrSize (CPU_CHAR    *p_buf_wr,
                                     CPU_SIZE_T   buf_len,
                                     CPU_SIZE_T   nbr_dig_max,
                                     CPU_INT16U   size)
{
    CPU_CHAR    *p_str;
    CPU_SIZE_T   size_max;


    size_max = nbr_dig_max + STR_CR_LF_LEN;

    if (buf_len < size_max) {
        return (DEF_NULL);
    }

    p_str = Str_FmtNbr_Int32U(size,
                              nbr_dig_max,
                              DEF_NBR_BASE_HEX,
                              '0',
                              DEF_YES,
                              DEF_NO,
                              p_buf_wr);
    if (p_str == DEF_NULL) {
        return (DEF_NULL);
    }

    p_str += nbr_dig_max;

    Str_Copy_N(p_str, STR_CR_LF, STR_CR_LF_LEN);
    if (p_str == DEF_NULL) {
        return (DEF_NULL);
    }

    p_str += STR_CR_LF_LEN;

    return (p_str);
}


/*
*********************************************************************************************************
*                                        HTTP_StrSizeHexDigReq()
*
* Description : Find the number of digital characters needed to represent in hexadecimal the given number.
*
* Argument(s) : nbr     Number to convert in hexadecimal.
*
* Return(s)   : Number of characters needed for conversion.
*
* Caller(s)   : HTTP_ChunkTransferWrSize().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT08U  HTTP_StrSizeHexDigReq (CPU_INT32U  nbr)
{
    if (nbr <= 0x000Fu) {
        return (1);

    } else if (nbr <= 0x00FFu) {
        return (2);

    } else if(nbr <= 0x0FFFu) {
        return (3);

    } else if(nbr <= 0xFFFFu) {
        return (4);

    } else if (nbr <= 0xFFFFFu) {
        return (5);

    } else if (nbr <= 0xFFFFFFu) {
        return (6);

    } else if (nbr <= 0xFFFFFFFu) {
        return (7);

    } else {
        return (8);
    }
}


/*
*********************************************************************************************************
*                                   HTTP_GetContentTypeFromFileExt()
*
* Description : Get the HTTP Content Type from the extension of the given filename string.
*
* Argument(s) : p_file_path     Pointer to filename string.
*
*               path_len_max    Length of the filename string.
*
* Return(s)   : HTTP Content Type.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

HTTP_CONTENT_TYPE  HTTP_GetContentTypeFromFileExt (CPU_CHAR   *p_file_path,
                                                   CPU_SIZE_T  path_len_max)
{
    CPU_CHAR    *p_ext;
    CPU_INT32U   content_type;


    p_ext = HTTP_StrFileExtGet(p_file_path, path_len_max);      /* Srch file ext.                                       */
    if (p_ext == DEF_NULL) {
        return (HTTP_CONTENT_TYPE_OCTET_STREAM);
    }

                                                                /* Get content type based on the file ext.              */
    content_type = HTTP_Dict_KeyGet(HTTP_Dict_FileExt,
                                    HTTP_Dict_FileExtSize,
                                    p_ext,
                                    DEF_YES,
                                    path_len_max);
    if (content_type == HTTP_DICT_KEY_INVALID) {
        return (HTTP_CONTENT_TYPE_OCTET_STREAM);
    }

    return ((HTTP_CONTENT_TYPE)content_type);
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
*                                          HTTP_CharToStrHex()
*
* Description : Get the hexadecimal code of a ASCII character in form of a string.
*
* Argument(s) : character   Character from which to get the hexadecimal code.
*
*               p_str_char  Pointer to string that will received the hexademacimal code.
*
*                           String will be ASCII_CHAR_NULL in case of error.
*
* Return(s)   : None.
*
* Caller(s)   : HTTP_URL_EndodeStr().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  HTTP_CharToStrHex (CPU_CHAR   character,
                                 CPU_CHAR  *p_str_char)
{
    CPU_CHAR     char_tmp;
    CPU_INT08U   shift;
    CPU_INT08U   i;


   *p_str_char = ASCII_CHAR_PERCENTAGE_SIGN;
    p_str_char++;

    shift = DEF_NIBBLE_NBR_BITS;

    for (i = 0; i < 2; i++) {
        char_tmp = (character >> shift) & 0x0F;

        if (char_tmp <= 0x09) {
            char_tmp += 0x30;
        } else if ((char_tmp >= 0x0A) &&
                   (char_tmp <= 0x0F)) {
            char_tmp -= 10;
            char_tmp += 0x41;
        } else {
            p_str_char--;
            p_str_char = ASCII_CHAR_NULL;
            return;
        }

        shift     -= DEF_NIBBLE_NBR_BITS;
       *p_str_char = char_tmp;
        p_str_char++;
    }
}


/*
*********************************************************************************************************
*                                          HTTP_StrHexToChar()
*
* Description : Convert an hexadecimal code string to its related ASCII character.
*
* Argument(s) : pstr_char   Pointer to string with hexadecimal code to convert.
*
* Return(s)   : The ASCII character, if conversion succeeded.
*
*               The Null character,  otherwise.
*
* Caller(s)   : HTTP_URL_DecodeStr().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_CHAR  HTTP_StrHexToChar (const  CPU_CHAR  *p_str_char)
{
           CPU_CHAR     char_rtn;
    const  CPU_CHAR    *p_char;
           CPU_INT08U   shift;
           CPU_INT08U   i;


    if (*p_str_char != ASCII_CHAR_PERCENTAGE_SIGN) {            /* First char must be '%', See note #1.                 */
         return (ASCII_CHAR_NULL);
    }

    p_char   = p_str_char + 1;
    char_rtn = ASCII_CHAR_NULL;
    shift    = DEF_NIBBLE_NBR_BITS;
    for (i = 0; i < 2; i++) {
        if ((*p_char >= ASCII_CHAR_DIGIT_ZERO) &&
            (*p_char <= ASCII_CHAR_DIGIT_NINE)) {

            char_rtn += (*p_char - ASCII_CHAR_DIGIT_ZERO);

        } else if ((*p_char >= ASCII_CHAR_LATIN_UPPER_A) &&
                   (*p_char <= ASCII_CHAR_LATIN_UPPER_F)) {

            char_rtn += (*p_char - ASCII_CHAR_LATIN_UPPER_A) + 10u;

        } else if ((*p_char >= ASCII_CHAR_LATIN_LOWER_A) &&
                   (*p_char <= ASCII_CHAR_LATIN_LOWER_F)) {

            char_rtn += (*p_char - ASCII_CHAR_LATIN_LOWER_A) + 10u;

        } else {
            return (ASCII_CHAR_NULL);
        }

        p_char++;
        char_rtn <<= shift;                                     /* First char is the upper part of the conv char.       */
        shift     -= DEF_NIBBLE_NBR_BITS;                       /* Next char is the lower part.                         */
    }

    return (char_rtn);
}


/*
*********************************************************************************************************
*                                         HTTP_StrFileExtGet()
*
* Description : Get the file extension characters from the given filename.
*
* Argument(s) : p_file_name     Pointer to filename string.
*
*               path_len        Length of the filename string.
*
* Return(s)   : Pointer to begin of the extension in the given filename string.
*
* Caller(s)   : HTTP_GetContentTypeFromFileExt().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_CHAR  *HTTP_StrFileExtGet (CPU_CHAR    *p_file_name,
                                       CPU_SIZE_T   path_len)
{
    CPU_CHAR  *p_last_dot;


    p_last_dot = Str_Char_Last_N(p_file_name, path_len, ASCII_CHAR_FULL_STOP);
    if (p_last_dot != DEF_NULL) {
        p_last_dot++;
    }

    return (p_last_dot);
}
