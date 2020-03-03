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
*                                        NET FILE SYSTEM PORT
*
*                                      HTTPs STATIC FILE SYSTEM
*
* Filename : http-s_fs_static.c
* Version  : V3.01.00
*********************************************************************************************************
* Note(s)  : (1) All files MUST be added prior to the first file or directory access.
*
*            (2) The application is responsible for validating file name compatibility with external
*                code modules, if additional restrictions must be imposed (e.g., certain characters
*                are not allowed).
*
*                (a) Importantly, file names should use ONLY the selected path separator character,
*                    ASCII_CHAR_REVERSE_SOLIDUS (= '\\'), to separate path components.  This software
*                    does not check for the common alternative, ASCII_CHAR_SOLIDUS (= '/'), when
*                    parsing file names, so any ASCII_CHAR_SOLIDUS character will be considered part
*                    of the file name component in which it is embedded.  This may break external
*                    code modules that convert names from this module's convention to the other.
*
*            (3) Should be compatible with the following TCP/IP application versions (or more recent):
*
*                (a) uC/HTTPs V2.00.00
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define    HTTPs_FS_STATIC_MODULE

#include  "http-s_fs_static.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  HTTPs_FS_PATH_SEP_CHAR                 ASCII_CHAR_REVERSE_SOLIDUS

#define  HTTPs_FS_SEEK_ORIGIN_START                        1u     /* Origin is beginning of file.                         */
#define  HTTPs_FS_SEEK_ORIGIN_CUR                          2u     /* Origin is current file position.                     */
#define  HTTPs_FS_SEEK_ORIGIN_END                          3u     /* Origin is end of file.                               */

#define  NET_FS_ENTRY_ATTRIB_RD                 DEF_BIT_00      /* Entry is readable.                                   */
#define  NET_FS_ENTRY_ATTRIB_WR                 DEF_BIT_01      /* Entry is writeable.                                  */
#define  NET_FS_ENTRY_ATTRIB_HIDDEN             DEF_BIT_02      /* Entry is hidden from user-level processes.           */
#define  NET_FS_ENTRY_ATTRIB_DIR                DEF_BIT_03      /* Entry is a directory.                                */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         FILE DATA DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_fs_file_data {
    CPU_CHAR           *NamePtr;                                /* Ptr to file name.                                    */
    void               *DataPtr;                                /* Ptr to file data.                                    */
    CPU_INT32S          Size;                                   /* Size of file, in octets.                             */
    CPU_SIZE_T          NameSimilarity;                         /* Max similarity between name & prev file name.        */
} HTTPs_FS_FILE_DATA;

/*
*********************************************************************************************************
*                                           FILE DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_fs_file {
    HTTPs_FS_FILE_DATA  *FileDataPtr;                           /* Ptr to file info.                                    */
    CPU_INT32S           Pos;                                   /* File pos, in octets.                                 */
} HTTPs_FS_FILE;


/*
*********************************************************************************************************
*                                         DIRECTORY DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_fs_dir {
    CPU_INT16U          FileDataIxFirst;                        /* File info ix first.                                  */
    CPU_INT16U          FileDataIxNext;                         /* File info ix next.                                   */
    CPU_SIZE_T          DirNameLen;                             /* Len of dir name.                                     */
} HTTPs_FS_DIR;



/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  HTTPs_FS_FILE       HTTPs_FS_FileTbl[HTTPs_FS_CFG_NBR_FILES];       /* Info about open files.                   */

static  HTTPs_FS_FILE_DATA  HTTPs_FS_FileDataTbl[HTTPs_FS_CFG_NBR_FILES];   /* Data about files on file system.         */

static  HTTPs_FS_DIR        HTTPs_FS_DirTbl[HTTPs_FS_CFG_NBR_DIRS];         /* Info about open dirs.                    */

static  CPU_INT16U          HTTPs_FS_FileAddedCnt;                          /* Nbr of files added.                      */

static  NET_FS_DATE_TIME    HTTPs_FS_Time;                                  /* Date/time of files and directories.      */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void         *HTTPs_FS_DirOpen              (CPU_CHAR            *p_name);

static  void          HTTPs_FS_DirClose             (void                *p_dir);

static  CPU_BOOLEAN   HTTPs_FS_DirRd                (void                *p_dir,
                                                     NET_FS_ENTRY        *p_entry);

static  CPU_BOOLEAN   HTTPs_FS_EntryCreate          (CPU_CHAR            *p_name,
                                                     CPU_BOOLEAN          dir);

static  CPU_BOOLEAN   HTTPs_FS_EntryDel             (CPU_CHAR            *p_name,
                                                     CPU_BOOLEAN          file);

static  CPU_BOOLEAN   HTTPs_FS_EntryRename          (CPU_CHAR            *p_name_old,
                                                     CPU_CHAR            *p_name_new);

static  CPU_BOOLEAN   HTTPs_FS_EntryTimeSet         (CPU_CHAR            *p_name,
                                                     NET_FS_DATE_TIME    *p_time);

static  CPU_BOOLEAN   HTTPs_WorkingFolderGet        (CPU_CHAR            *p_path,
                                                     CPU_SIZE_T           path_len_max);

static  CPU_BOOLEAN   HTTPs_WorkingFolderSet        (CPU_CHAR            *p_path);

static  CPU_INT32U    HTTPs_FS_CfgPathGetLenMax     (void);

static  CPU_CHAR      HTTPs_FS_CfgPathGetSepChar    (void);

static  void         *HTTPs_FS_FileOpen             (CPU_CHAR            *p_name,
                                                     NET_FS_FILE_MODE     mode,
                                                     NET_FS_FILE_ACCESS   access);

static  void          HTTPs_FS_FileClose            (void                *p_file);

static  CPU_BOOLEAN   HTTPs_FS_FileRd               (void                *p_file,
                                                     void                *p_dest,
                                                     CPU_SIZE_T           size,
                                                     CPU_SIZE_T          *p_size_rd);

static  CPU_BOOLEAN   HTTPs_FS_FileWr               (void                *p_file,
                                                     void                *p_src,
                                                     CPU_SIZE_T           size,
                                                     CPU_SIZE_T          *p_size_wr);

static  CPU_BOOLEAN   HTTPs_FS_FilePosSet           (void                *p_file,
                                                     CPU_INT32S           offset,
                                                     CPU_INT08U           origin);

static  CPU_BOOLEAN   HTTPs_FS_FileSizeGet          (void                *p_file,
                                                     CPU_INT32U          *p_size);

static  CPU_BOOLEAN   HTTPs_FS_FileDateTimeCreateGet(void                *p_file,
                                                     NET_FS_DATE_TIME    *p_time);

static  CPU_SIZE_T    HTTPs_FS_CalcSimilarity      (CPU_CHAR             *p_name_1,
                                                    CPU_CHAR             *p_name_2);


/*
*********************************************************************************************************
*                                           FILE SYSTEM API
*
* Note(s) : (1) File system API structures are used by network applications during calls.  This API structure
*               allows network application to call specific file system functions via function pointer instead
*               of by name.  This enables the network application suite to compile & operate with multiple
*               file system.
*
*           (2) In most cases, the API structure provided below SHOULD suffice for most network application
*               exactly as is with the exception that the API structure's name which MUST be unique &
*               SHOULD clearly identify the file system being implemented.  For example, the Micrium file system
*               V4's API structure should be named HTTPs_FS_API_FS_V4[].
*
*               The API structure MUST also be externally declared in the File system port header file
*               ('net_fs_&&&.h') with the exact same name & type.
*********************************************************************************************************
*/
                                                                            /* Net FS static API fnct ptrs :               */
const  NET_FS_API  HTTPs_FS_API_Static = {
                                           HTTPs_FS_CfgPathGetLenMax,       /*   Path max len.                             */
                                           HTTPs_FS_CfgPathGetSepChar,      /*   Path sep char.                            */
                                           HTTPs_FS_FileOpen,               /*   Open                                      */
                                           HTTPs_FS_FileClose,              /*   Close                                     */
                                           HTTPs_FS_FileRd,                 /*   Rd                                        */
                                           HTTPs_FS_FileWr,                 /*   Wr                                        */
                                           HTTPs_FS_FilePosSet,             /*   Set Position                              */
                                           HTTPs_FS_FileSizeGet,            /*   Get Size                                  */
                                           HTTPs_FS_DirOpen,                /*   Open directory.                           */
                                           HTTPs_FS_DirClose,               /*   Close directory.                          */
                                           HTTPs_FS_DirRd,                  /*   Read directory.                           */
                                           HTTPs_FS_EntryCreate,            /*   Entry create.                             */
                                           HTTPs_FS_EntryDel,               /*   Entry delete.                             */
                                           HTTPs_FS_EntryRename,            /*   Entry rename.                             */
                                           HTTPs_FS_EntryTimeSet,           /*   Entry time set.                           */
                                           HTTPs_FS_FileDateTimeCreateGet,  /*   Create a date time.                       */
                                           HTTPs_WorkingFolderGet,          /*   Get working folder.                       */
                                           HTTPs_WorkingFolderSet,          /*   Set working folder.                       */
                                         };





/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       HTTPs_WorkingFolderGet()
*
* Description : Get current working folder.
*
* Argument(s) : p_path          Pointer to string that will receive the working folder.
*
*               path_len_max    Maximum length of the string.
*
* Return(s)   : DEF_OK,   if p_path successfully copied.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_WorkingFolderGet (CPU_CHAR    *p_path,
                                             CPU_SIZE_T   path_len_max)
{
   *p_path = ASCII_CHAR_NULL;

   return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       HTTPs_WorkingFolderSet()
*
* Description : Set current working folder.
*
* Argument(s) : p_path  Pointer to string that contains the working path to use.
*
* Return(s)   : DEF_OK,   if p_path successfully configured.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_WorkingFolderSet (CPU_CHAR  *p_path)
{
    if (p_path[0] != ASCII_CHAR_NULL) {
        return (DEF_FAIL);
    }

   return (DEF_OK);
}


/*
*********************************************************************************************************
*                                      HTTPs_FS_CfgPathGetLenMax()
*
* Description : Get maximum path length
*
* Argument(s) : none.
*
* Return(s)   : maximum path length.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_INT32U  HTTPs_FS_CfgPathGetLenMax (void)
{
    return ((CPU_INT32U)HTTPs_FS_CFG_MAX_FILE_NAME_LEN);
}


/*
*********************************************************************************************************
*                                     HTTPs_FS_CfgPathGetSepChar()
*
* Description : Get path separator character
*
* Argument(s) : none.
*
* Return(s)   : separator character.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_CHAR  HTTPs_FS_CfgPathGetSepChar (void)
{
    return ((CPU_CHAR)HTTPs_FS_PATH_SEP_CHAR);
}


/*
*********************************************************************************************************
*                                            HTTPs_FS_Init()
*
* Description : Initialize the static file system.
*
* Argument(s) : none.
*
* Return(s)   : DEF_OK,   if file system initialized.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
*               This function is a HTTPs server application programming interface (API) function
*               & MAY be called by application function(s).
*
* Notes       : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPs_FS_Init (void)
{
    HTTPs_FS_DIR        *p_dir;
    HTTPs_FS_FILE       *p_file;
    HTTPs_FS_FILE_DATA  *p_file_data;
    CPU_INT16U           i;


                                                                /* -------------------- INIT FILES -------------------- */
    for (i = 0u; i < HTTPs_FS_CFG_NBR_FILES; i++) {
        p_file = &HTTPs_FS_FileTbl[i];

        p_file->FileDataPtr = (HTTPs_FS_FILE_DATA *)0;
        p_file->Pos         =  0u;
    }

    HTTPs_FS_FileAddedCnt = 0u;



                                                                /* ------------------ INIT FILE INFO ------------------ */
    for (i = 0u; i < HTTPs_FS_CFG_NBR_FILES; i++) {
        p_file_data = &HTTPs_FS_FileDataTbl[i];

        p_file_data->NamePtr        = (CPU_CHAR *)0;
        p_file_data->DataPtr        = (void     *)0;
        p_file_data->Size           =  0u;
        p_file_data->NameSimilarity =  0u;
    }



                                                                /* --------------------- INIT DIRS -------------------- */
    for (i = 0u; i < HTTPs_FS_CFG_NBR_DIRS; i++) {
        p_dir                  = &HTTPs_FS_DirTbl[i];

        p_dir->FileDataIxFirst =  DEF_INT_16U_MAX_VAL;
        p_dir->FileDataIxNext  =  DEF_INT_16U_MAX_VAL;
        p_dir->DirNameLen      =  0u;
    }



                                                                /* --------------------- INIT TIME -------------------- */
    HTTPs_FS_Time.Sec   = 0u;
    HTTPs_FS_Time.Min   = 0u;
    HTTPs_FS_Time.Hr    = 0u;
    HTTPs_FS_Time.Day   = 0u;
    HTTPs_FS_Time.Month = 0u;
    HTTPs_FS_Time.Yr    = 0u;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                          HTTPs_FS_AddFile()
*
* Description : Add a file to the static file system.
*
* Argument(s) : p_name  Name of the file.
*
*               p_data  Pointer to buffer holding file data.
*
*               size    Size of file, in octets.
*
* Return(s)   : DEF_OK,   if file added.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
*               This function is a HTTPs server application programming interface (API) function
*               & MAY be called by application function(s).
*
* Note(s)     : (1) The file name ...
*                   (a) MUST begin with a path separator character.
*                   (b) MUST be no longer than HTTPs_FS_MAX_PATH_NAME_LEN.
*                   (c) MUST not end with a path separator character.
*                   (d) MUST not duplicate the parent directory of a file already added.
*                   (e) MUST not duplicate a file already added.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPs_FS_AddFile (CPU_CHAR    *p_name,
                               void        *p_data,
                               CPU_INT32U   size)

{
    CPU_CHAR            *p_filename;
    CPU_INT16S           cmp_val;
    HTTPs_FS_FILE_DATA  *p_file_data;
    CPU_INT16U           i;
    CPU_SIZE_T           len;
    CPU_SIZE_T           similarity;
    CPU_SIZE_T           similarity_max;


#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                /* ------------------ VALIDATE PTRS ------------------- */
    if (p_name == (CPU_CHAR *)0) {                              /* Validate NULL name.                                  */
        return (DEF_FAIL);
    }
    if (p_data == (void *)0) {                                  /* Validate NULL buf.                                   */
        return (DEF_FAIL);
    }
#endif

    p_filename = p_name;
    if (p_filename[0] == HTTPs_FS_PATH_SEP_CHAR) {              /* Require init path sep char (see Note #1a).           */
        p_filename++;
    }

                                                                /* Validate name len (see Note #1b).                    */
    len = Str_Len_N(p_filename, HTTPs_FS_CFG_MAX_FILE_NAME_LEN + 1u);
    if (len > HTTPs_FS_CFG_MAX_FILE_NAME_LEN) {
        return (DEF_FAIL);
    }

    if (p_filename[len - 1u] == HTTPs_FS_PATH_SEP_CHAR) {       /* Require final char NOT path sep char (see Note #1c). */
        return (DEF_FAIL);
    }



                                                                /* ----------------- CHK FOR DUP FILE ----------------- */
    similarity_max =  0u;
    p_file_data    = &HTTPs_FS_FileDataTbl[0];
    for (i = 0u; i < HTTPs_FS_FileAddedCnt; i++) {              /* For each file ...                                    */
                                                                /*               ... chk if name begins with file name..*/
        cmp_val = Str_CmpIgnoreCase_N(p_file_data->NamePtr, p_filename, len);
        if (cmp_val == 0) {
                                                                /*               ... & following char is path sep char..*/
            if (p_file_data->NamePtr[len] == HTTPs_FS_PATH_SEP_CHAR) {
                return (DEF_FAIL);                              /* (see Note #1d)       ... file is name of parent dir. */

                                                                /*               ... if following char is NULL        ..*/
            } else if (p_file_data->NamePtr[len] == ASCII_CHAR_NULL) {
                return (DEF_FAIL);                              /* (see Note #1e)       ... file dup's file name.       */
            }
        }

        similarity = HTTPs_FS_CalcSimilarity(p_filename, p_file_data->NamePtr);
        if (similarity_max < similarity) {
            similarity_max = similarity;
        }

        p_file_data++;
    }



                                                                /* ---------------- FIND FREE FILE DATA --------------- */
    if (HTTPs_FS_FileAddedCnt >= HTTPs_FS_CFG_NBR_FILES) {
        return (DEF_FAIL);
    }

    p_file_data = &HTTPs_FS_FileDataTbl[HTTPs_FS_FileAddedCnt];
    HTTPs_FS_FileAddedCnt++;



                                                                /* --------------------- ADD FILE --------------------- */
    p_file_data->NamePtr        = p_filename;                   /* Populate file data.                                  */
    p_file_data->DataPtr        = p_data;
    p_file_data->Size           = size;
    p_file_data->NameSimilarity = similarity_max;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                          HTTPs_FS_SetTime()
*
* Description : Set date/time of files and directories.
*
* Argument(s) : p_time  Pointer to date/time to set.
*
* Return(s)   : DEF_OK,   if time set.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : none.
*
* Note(s)     : (1) This time will be returned in the directory entry for ALL files and directories.
*
*               (2) #### Validate date/time members.
*********************************************************************************************************
*/

CPU_BOOLEAN  HTTPs_FS_SetTime (NET_FS_DATE_TIME  *p_time)
{
#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* -------------- VALIDATE DATE/TIME PTR -------------- */
    if (p_time == (NET_FS_DATE_TIME *)0) {                     /* Validate NULL date/time.                             */
        return (DEF_FAIL);
    }
#endif



                                                                /* --------------------- SET TIME --------------------- */
    HTTPs_FS_Time.Sec   = p_time->Sec;
    HTTPs_FS_Time.Min   = p_time->Min;
    HTTPs_FS_Time.Hr    = p_time->Hr;
    HTTPs_FS_Time.Day   = p_time->Day;
    HTTPs_FS_Time.Month = p_time->Month;
    HTTPs_FS_Time.Yr    = p_time->Yr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         DIRECTORY FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          HTTPs_FS_DirOpen()
*
* Description : Open a directory.
*
* Argument(s) : p_name  Name of the directory.
*
* Return(s)   : Pointer to a directory, if NO errors.
*
*               Pointer to NULL,        otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : (1) The file search ends when :
*                   (a) A file lying in the directory 'pname' is found.
*                   (b) A file with name 'pname' is found.
*                   (c) All files have been examined.
*********************************************************************************************************
*/

void  *HTTPs_FS_DirOpen (CPU_CHAR  *p_name)
{
    CPU_INT16S         cmp_val;
    HTTPs_FS_DIR        *p_dir;
    CPU_SIZE_T         dir_name_len;
    HTTPs_FS_FILE_DATA  *p_file_data;
    CPU_INT16U         file_data_ix;
    CPU_BOOLEAN        found;
    CPU_INT16U         i;
    CPU_CHAR          *p_path_sep;
    CPU_SR_ALLOC();


#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* ---------------- VALIDATE DIR NAME ----------------- */
    if (p_name == (CPU_CHAR *)0) {                              /* Validate NULL name.                                  */
        return ((void *)0);
    }
#endif

                                                                /* Validate name len.                                   */
    dir_name_len = Str_Len_N(p_name, HTTPs_FS_CFG_MAX_FILE_NAME_LEN + 1u);
    if (dir_name_len > HTTPs_FS_CFG_MAX_FILE_NAME_LEN) {
        return (DEF_FAIL);
    }
    if (dir_name_len == 0u) {
        return (DEF_FAIL);
    }

    p_path_sep = Str_Char_Last(p_name, HTTPs_FS_PATH_SEP_CHAR);
    if (p_path_sep != (CPU_CHAR *)0) {
        if (p_name + dir_name_len - 1u == p_path_sep) {         /* If final dir name char is path sep char ...          */
            dir_name_len--;                                     /* ... dec dir name len to ignore path sep char.        */
        }
    }



                                                                /* --------------------- OPEN DIR --------------------- */
    found       =  DEF_NO;
    p_file_data = &HTTPs_FS_FileDataTbl[0];
                                                                /* For each file ...                                    */
    for (file_data_ix = 0u; file_data_ix < HTTPs_FS_FileAddedCnt; file_data_ix++) {
                                                                /*               ... if name begins with dir name     ..*/
        cmp_val = Str_CmpIgnoreCase_N(p_file_data->NamePtr, p_name, dir_name_len);
        if (cmp_val == 0) {
                                                                /*               ... & following char is path sep char..*/
            if (p_file_data->NamePtr[dir_name_len] == HTTPs_FS_PATH_SEP_CHAR) {
                found = DEF_YES;                                /* (see Note #1b)       ... file lies in dir.           */
                break;

                                                                /*              ... if following char is NULL ...       */
            } else if (p_file_data->NamePtr[dir_name_len] == ASCII_CHAR_NULL) {
                return ((void *)0);                             /* (see Note #1c)       ... dir name is file name.      */
            }
        }

        p_file_data++;
    }

    if (found == DEF_NO) {                                      /* If file data NOT found (see Note #1d) ...            */
        return ((void *)0);                                     /* ... rtn NULL ptr.                                    */
    }



                                                                /* ------------------- FIND FREE DIR ------------------ */
    found =  DEF_NO;
    p_dir  = &HTTPs_FS_DirTbl[0];

    CPU_CRITICAL_ENTER();
    for (i = 0u; i < HTTPs_FS_CFG_NBR_FILES; i++) {
        if (p_dir->FileDataIxFirst == DEF_INT_16U_MAX_VAL) {
            found = DEF_YES;
            break;
        }

        p_dir++;
    }

    if (found == DEF_NO) {                                      /* If no free dir found ...                             */
        CPU_CRITICAL_EXIT();
        return ((void *)0);                                     /* ... rtn NULL ptr.                                    */
    }



    p_dir->FileDataIxFirst = file_data_ix;                      /* Populate dir data.                                   */
    p_dir->FileDataIxNext  = file_data_ix;
    p_dir->DirNameLen      = dir_name_len;
    CPU_CRITICAL_EXIT();

    return ((void *)p_dir);
}


/*
*********************************************************************************************************
*                                          HTTPs_FS_DirClose()
*
* Description : Close a directory.
*
* Argument(s) : p_dir   Pointer to a directory.
*
* Return(s)   : none.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  HTTPs_FS_DirClose (void  *p_dir)
{
    HTTPs_FS_DIR  *pdir_fs;
    CPU_SR_ALLOC();


#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* ------------------ VALIDATE DIR -------------------- */
    if (p_dir == (void *)0) {                                    /* Validate NULL dir ptr.                               */
        return;
    }
#endif



                                                                /* --------------------- CLOSE DIR -------------------- */
    pdir_fs = (HTTPs_FS_DIR *)p_dir;

    CPU_CRITICAL_ENTER();
    pdir_fs->FileDataIxFirst = DEF_INT_16U_MAX_VAL;
    pdir_fs->FileDataIxNext  = DEF_INT_16U_MAX_VAL;
    pdir_fs->DirNameLen      = 0u;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                           HTTPs_FS_DirRd()
*
* Description : Read a directory entry from a directory.
*
* Argument(s) : p_dir       Pointer to a directory.
*
*               p_entry     Pointer to variable that will receive directory entry information.
*
* Return(s)   : DEF_OK,   if directory entry read.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : (1) If the entry name contains a path separator character after the path separator
*                   character immediately following directory name, the entry is a directory.
*
*               (2) The maximum similarity between the file name & previous file names is a measure of
*                   the number of shared path components.  If the similarity is greater than the
*                   directory name length, then either :
*                   (a) ... the next path component in the file name (a directory name) was returned
*                           in a previous directory read operation.
*                   (b) ... the portion of the file name shared with a previous file name was examined
*                           in a previous directory read operation, but failed to match the directory
8                           name.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_FS_DirRd (void          *p_dir,
                                     NET_FS_ENTRY  *p_entry)
{
    CPU_INT16S          cmp_val;
    HTTPs_FS_DIR        *p_dir_fs;
    HTTPs_FS_FILE_DATA  *p_file_data;
    CPU_INT16U          file_data_ix;
    CPU_BOOLEAN         found;
    CPU_SIZE_T          len;
    CPU_CHAR           *p_name_entry;
    CPU_CHAR           *p_name_first;
    CPU_CHAR           *p_path_sep;


#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* ------------------ VALIDATE PTRS ------------------- */
    if (p_dir == (void *)0) {                                   /* Validate NULL dir   ptr.                             */
        return (DEF_FAIL);
    }
    if (p_entry == (NET_FS_ENTRY *)0) {                        /* Validate NULL entry ptr.                             */
        return (DEF_FAIL);
    }
#endif

    p_dir_fs     = (HTTPs_FS_DIR *)p_dir;
    p_name_first =  HTTPs_FS_FileDataTbl[p_dir_fs->FileDataIxFirst].NamePtr;



                                                                /* --------------- FIND NEXT FILE IN DIR -------------- */
    found       =  DEF_NO;
    p_file_data = &HTTPs_FS_FileDataTbl[p_dir_fs->FileDataIxNext];
                                                                /* For each file ...                                    */
    for (file_data_ix = p_dir_fs->FileDataIxNext; file_data_ix < HTTPs_FS_FileAddedCnt; file_data_ix++) {

                                                                /*        ... w/ name NOT found prev'ly (see Note #2) ..*/
        if (p_file_data->NameSimilarity <= p_dir_fs->DirNameLen) {

                                                                /*               ... if name begins with dir name     ..*/
            cmp_val = Str_CmpIgnoreCase_N(p_file_data->NamePtr, p_name_first, p_dir_fs->DirNameLen);
            if (cmp_val == 0) {
                                                                /*               ... & following char is path sep char..*/
                if (p_file_data->NamePtr[p_dir_fs->DirNameLen] == HTTPs_FS_PATH_SEP_CHAR) {
                    found = DEF_YES;                            /*                      ... file lies in dir.           */
                    break;
                }
            }
        }

        p_file_data++;
    }

    if (found == DEF_NO) {                                      /* If file data NOT found ...                           */
        return (DEF_FAIL);                                      /* ... end of dir.                                      */
    }



                                                                /* ------------------ GET ENTRY INFO ------------------ */
    p_name_entry = p_file_data->NamePtr + p_dir_fs->DirNameLen + 1u;
    p_path_sep   = Str_Char_N(p_name_entry, HTTPs_FS_CFG_MAX_FILE_NAME_LEN, HTTPs_FS_PATH_SEP_CHAR);
    if (p_path_sep == (CPU_CHAR *)0) {                          /* Chk if entry is file ...                             */
        Str_Copy_N(p_entry->NamePtr, p_name_entry, HTTPs_FS_CFG_MAX_FILE_NAME_LEN);
        p_entry->Attrib = NET_FS_ENTRY_ATTRIB_RD;
        p_entry->Size   = p_file_data->Size;

    } else {                                                    /*                      ... or dir (see Note #1).       */
        len = (CPU_SIZE_T)(p_path_sep - p_name_entry);
        Str_Copy_N(p_entry->NamePtr, p_name_entry, len);
        p_entry->Attrib = NET_FS_ENTRY_ATTRIB_RD | NET_FS_ENTRY_ATTRIB_DIR;
        p_entry->Size   = 0u;
    }

    p_entry->DateTimeCreate.Sec   = HTTPs_FS_Time.Sec;
    p_entry->DateTimeCreate.Min   = HTTPs_FS_Time.Min;
    p_entry->DateTimeCreate.Hr    = HTTPs_FS_Time.Hr;
    p_entry->DateTimeCreate.Day   = HTTPs_FS_Time.Day;
    p_entry->DateTimeCreate.Month = HTTPs_FS_Time.Month;
    p_entry->DateTimeCreate.Yr    = HTTPs_FS_Time.Yr;



                                                                /* -------------------- UPDATE DIR -------------------- */
    p_dir_fs->FileDataIxNext = file_data_ix + 1u;
    return (DEF_OK);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           ENTRY FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        HTTPs_FS_EntryCreate()
*
* Description : Create a file or directory.
*
* Argument(s) : p_name  Name of the entry.
*
*               dir     Indicates whether the new entry shall be a directory :
*
* Return(s)   : DEF_OK,   if entry created.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : (1) Read operations ONLY supported; entry CANNOT be created.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_FS_EntryCreate (CPU_CHAR     *p_name,
                                           CPU_BOOLEAN   dir)
{
   (void)p_name;                                                /* Prevent 'variable unused' compiler warning.          */
   (void)dir;

    return (DEF_FAIL);                                          /* See Note #1.                                         */
}


/*
*********************************************************************************************************
*                                          HTTPs_FS_EntryDel()
*
* Description : Delete a file or directory.
*
* Argument(s) : p_name  Name of the entry.
*
*               file    Indicates whether the entry MAY be a file :
*
* Return(s)   : DEF_OK,   if entry created.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : (1) Read operations ONLY supported; entry CANNOT be deleted.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_FS_EntryDel (CPU_CHAR     *p_name,
                                        CPU_BOOLEAN   file)
{
   (void)p_name;                                                /* Prevent 'variable unused' compiler warning.          */
   (void)file;

                                                                /* #### NET-451                                         */

    return (DEF_FAIL);                                          /* See Note #1.                                         */
}


/*
*********************************************************************************************************
*                                        HTTPs_FS_EntryRename()
*
* Description : Rename a file or directory.
*
* Argument(s) : p_name_old  Old path of the entry.
*
*               p_name_new  New path of the entry.
*
* Return(s)   : DEF_OK,   if entry renamed.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : (1) Read operations ONLY supported; entry CANNOT be renamed.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_FS_EntryRename (CPU_CHAR  *p_name_old,
                                           CPU_CHAR  *p_name_new)
{
   (void)p_name_old;                                            /* Prevent 'variable unused' compiler warning.          */
   (void)p_name_new;

    return (DEF_FAIL);                                          /* See Note #1.                                         */
}


/*
*********************************************************************************************************
*                                        HTTPs_FS_EntryTimeSet()
*
* Description : Set a file or directory's date/time.
*
* Argument(s) : p_name  Name of the entry.
*
*               p_time  Pointer to date/time.
*
* Return(s)   : DEF_OK,   if date/time set.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : (1) Read operations ONLY supported; entry date/time CANNOT be set.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_FS_EntryTimeSet (CPU_CHAR          *p_name,
                                            NET_FS_DATE_TIME  *p_time)
{
   (void)p_name;                                                /* Prevent 'variable unused' compiler warning.          */
   (void)p_time;

    return (DEF_FAIL);                                          /* See Note #1.                                         */
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           FILE FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          HTTPs_FS_FileOpen()
*
* Description : Open a file.
*
* Argument(s) : p_name  Name of the file.
*
*               mode    Mode of the file :
*
*               access  Access rights of the file :
*
* Return(s)   : Pointer to a file, if NO errors.
*
*               Pointer to NULL,   otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : (1) Read operations ONLY supported; file can ONLY be opened in a read mode.
*********************************************************************************************************
*/

static  void  *HTTPs_FS_FileOpen (CPU_CHAR            *p_name,
                                  NET_FS_FILE_MODE     mode,
                                  NET_FS_FILE_ACCESS   access)
{
    CPU_INT16S          cmp_val;
    HTTPs_FS_FILE       *p_file;
    HTTPs_FS_FILE_DATA  *p_file_data;
    CPU_BOOLEAN         found;
    CPU_INT32U          i;
    CPU_SR_ALLOC();


#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* ------------------ VALIDATE PTRS ------------------- */
    if (p_name     == (CPU_CHAR *)0) {                          /* Validate NULL name.                                  */
        return ((void *)0);
    }
#endif


                                                                /* ------------------- VALIDATE MODE ------------------ */
    switch (mode) {                                             /* Validate mode (see Note #1).                         */
        case NET_FS_FILE_MODE_OPEN:
             break;


        case NET_FS_FILE_MODE_APPEND:
        case NET_FS_FILE_MODE_CREATE:
        case NET_FS_FILE_MODE_CREATE_NEW:
        case NET_FS_FILE_MODE_TRUNCATE:
        default:
             return (DEF_NULL);
    }


    switch (access) {

        case NET_FS_FILE_ACCESS_RD:
             break;


        case NET_FS_FILE_ACCESS_RD_WR:
        case NET_FS_FILE_ACCESS_WR:
        default:
                                                                /* #### NET-451                                         */
             return (DEF_NULL);
    }



                                                                /* ------------------- FIND FILE DATA ----------------- */
    found      =  DEF_NO;
    p_file_data = &HTTPs_FS_FileDataTbl[0];
    for (i = 0u; i < HTTPs_FS_FileAddedCnt; i++) {
        cmp_val = Str_CmpIgnoreCase(p_file_data->NamePtr, p_name);
        if (cmp_val == 0) {
            found = DEF_YES;
            break;
        }

        p_file_data++;
    }

    if (found == DEF_NO) {                                      /* If file data NOT found ...                           */
        return ((void *)0);                                     /* ... rtn NULL ptr.                                    */
    }



                                                                /* ------------------ FIND FREE FILE ------------------ */
    found =  DEF_NO;
    p_file = &HTTPs_FS_FileTbl[0];

    CPU_CRITICAL_ENTER();
    for (i = 0u; i < HTTPs_FS_CFG_NBR_FILES; i++) {
        if (p_file->FileDataPtr == (HTTPs_FS_FILE_DATA *)0) {
            found = DEF_YES;
            break;
        }

        p_file++;
    }

    if (found == DEF_NO) {                                      /* If no free file found ...                            */
        CPU_CRITICAL_EXIT();
        return ((void *)0);                                     /* ... rtn NULL ptr.                                    */
    }



                                                                /* -------------------- OPEN FILE --------------------- */
    p_file->FileDataPtr = p_file_data;
    p_file->Pos         = 0u;
    CPU_CRITICAL_EXIT();

    return ((void *)p_file);
}


/*
*********************************************************************************************************
*                                         HTTPs_FS_FileClose()
*
* Description : Close a file.
*
* Argument(s) : p_file  Pointer to a file.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  HTTPs_FS_FileClose (void  *p_file)
{
    HTTPs_FS_FILE  *p_file_fs;
    CPU_SR_ALLOC();


#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* ---------------- VALIDATE FILE PTR ----------------- */
    if (p_file == (void *)0) {                                  /* Validate NULL file ptr.                              */
        return;
    }
#endif

    p_file_fs = (HTTPs_FS_FILE *)p_file;



                                                                /* -------------------- CLOSE FILE -------------------- */
    CPU_CRITICAL_ENTER();
    p_file_fs->FileDataPtr = (HTTPs_FS_FILE_DATA *)0;
    p_file_fs->Pos         =  0u;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                           HTTPs_FS_FileRd()
*
* Description : Read from a file.
*
* Argument(s) : p_file      Pointer to a file.
*
*               p_dest      Pointer to destination buffer.
*
*               size        Number of octets to read.
*
*               p_size_rd   Pointer to variable that will receive the number of octets read.
*
* Return(s)   : DEF_OK,   if no error occurred during read (see Note #2).
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : (1) Pointers to variables that return values MUST be initialized PRIOR to all other
*                   validation or function handling in case of any error(s).
*
*               (2) If the read request could not be fulfilled because the EOF was reached, the return
*                   value should be 'DEF_OK'.  The application should compare the value in 'psize_rd' to
*                   the value passed to 'size' to detect an EOF reached condition.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_FS_FileRd (void        *p_file,
                                      void        *p_dest,
                                      CPU_SIZE_T   size,
                                      CPU_SIZE_T  *p_size_rd)
{
    CPU_INT08U          *p_data_cur;
    HTTPs_FS_FILE_DATA  *p_file_data;
    HTTPs_FS_FILE       *p_file_fs;
    CPU_INT32S           size_rd;


#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* ---------------- VALIDATE SIZE PTR ----------------- */
    if (p_size_rd == (CPU_SIZE_T *)0) {                         /* Validate NULL size ptr.                              */
        return (DEF_FAIL);
    }
#endif

   *p_size_rd = 0u;                                             /* Init to dflt size for err (see Note #1).             */

#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* ------------------ VALIDATE PTRS ------------------- */
    if (p_file == (void *)0) {                                  /* Validate NULL file ptr.                              */
        return (DEF_FAIL);
    }
    if (p_dest == (void *)0) {                                  /* Validate NULL dest ptr.                              */
        return (DEF_FAIL);
    }
#endif

    p_file_fs   = (HTTPs_FS_FILE *)p_file;
    p_file_data =  p_file_fs->FileDataPtr;



                                                                /* --------------------- RD FILE ---------------------- */
    if (p_file_fs->Pos >= p_file_data->Size) {                  /* If pos beyond EOF ...                                */
        return (DEF_OK);                                        /* ... NO data rd.                                      */
    }

    size_rd = size;                                             /* Calculate rd size.                                   */
    if (size_rd > (p_file_data->Size - p_file_fs->Pos)) {
        size_rd = (p_file_data->Size - p_file_fs->Pos);
    }

    p_data_cur = (CPU_INT08U *)p_file_data->DataPtr + p_file_fs->Pos;
    Mem_Copy(p_dest, (void *)p_data_cur, size_rd);              /* 'Rd' data.                                           */

    p_file_fs->Pos += size_rd;                                  /* Advance file pos.                                    */
   *p_size_rd       = size_rd;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                           HTTPs_FS_FileWr()
*
* Description : Write to a file.
*
* Argument(s) : p_file      Pointer to a file.
*
*               p_src       Pointer to source buffer.
*
*               size        Number of octets to write.
*
*               p_size_wr   Pointer to variable that will receive the number of octets written.
*
* Return(s)   : DEF_OK,   if no error occurred during write.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : (1) Write operations are NOT permitted.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_FS_FileWr (void        *p_file,
                                      void        *p_src,
                                      CPU_SIZE_T   size,
                                      CPU_SIZE_T  *p_size_wr)
{
#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                /* ---------------- VALIDATE SIZE PTR ----------------- */
    if (p_size_wr == (CPU_SIZE_T *)0) {                         /* Validate NULL size ptr.                              */
        return (DEF_FAIL);
    }
#endif

   *p_size_wr = 0u;                                             /* Init to dflt size (see Note #1).                     */

   (void)p_file;                                                /* Prevent 'variable unused' compiler warning.          */
   (void)p_src;
   (void)size;

                                                                /* #### NET-451                                         */

    return (DEF_FAIL);
}


/*
*********************************************************************************************************
*                                         HTTPs_FS_FilePosSet()
*
* Description : Set file position indicator.
*
* Argument(s) : p_file  Pointer to a file.
*
*               offset  Offset from the file position specified by 'origin'.
*
*               origin  Reference position for offset :
*
* Return(s)   : DEF_OK,   if file position set.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : (1) File position NOT checked for overflow.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_FS_FilePosSet (void        *p_file,
                                          CPU_INT32S   offset,
                                          CPU_INT08U   origin)
{
    HTTPs_FS_FILE_DATA  *p_file_data;
    HTTPs_FS_FILE       *p_file_fs;
    CPU_BOOLEAN         ok;
    CPU_INT32U          pos_new;


#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* ---------------- VALIDATE FILE PTR ----------------- */
    if (p_file == (void *)0) {                                  /* Validate NULL file ptr.                              */
        return (DEF_FAIL);
    }
#endif

    p_file_fs   = (HTTPs_FS_FILE *)p_file;
    p_file_data =  p_file_fs->FileDataPtr;



                                                                /* ------------------- SET FILE POS ------------------- */
    ok = DEF_FAIL;
    switch (origin) {
        case HTTPs_FS_SEEK_ORIGIN_START:
             if (offset >= 0) {
                 pos_new = (CPU_INT32U)offset;
                 ok      =  DEF_OK;
             }
             break;

        case HTTPs_FS_SEEK_ORIGIN_CUR:
             if (offset < 0) {
                 if (-offset <= p_file_fs->Pos) {
                     pos_new  = p_file_fs->Pos - (CPU_INT32U)-offset;
                     ok       = DEF_OK;
                 }
             } else {
                 pos_new = p_file_fs->Pos + (CPU_INT32U)offset;
                 ok      = DEF_OK;
             }
             break;

        case HTTPs_FS_SEEK_ORIGIN_END:
             if (offset < 0) {
                 if (-offset <= p_file_data->Size) {
                     pos_new  = p_file_data->Size - (CPU_INT32U)-offset;
                     ok       = DEF_OK;
                 }
             } else {
                 pos_new = p_file_data->Size + (CPU_INT32U)offset;
                 ok      = DEF_OK;
             }
             break;

        default:
             break;
    }
    if (ok != DEF_OK) {
        return (DEF_FAIL);
    }

    if (pos_new > p_file_data->Size) {                          /* Chk for pos beyond EOF.                              */
        return (DEF_FAIL);
    }

    p_file_fs->Pos = pos_new;                                   /* Set pos.                                             */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        HTTPs_FS_FileSizeGet()
*
* Description : Get file size.
*
* Argument(s) : p_file  Pointer to a file.
*
*               p_size  Pointer to variable that will receive the file size.
*
* Return(s)   : DEF_OK,   if file size gotten.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : (1) Pointers to variables that return values MUST be initialized PRIOR to all other
*                   validation or function handling in case of any error(s).
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_FS_FileSizeGet (void        *p_file,
                                           CPU_INT32U  *p_size)
{
    HTTPs_FS_FILE_DATA  *p_file_data;
    HTTPs_FS_FILE       *p_file_fs;


#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* ---------------- VALIDATE SIZE PTR ----------------- */
    if (p_size == (CPU_INT32U *)0) {                            /* Validate NULL size ptr.                              */
        return (DEF_FAIL);
    }
#endif

   *p_size = 0u;                                                /* Init to dflt size for err (see Note #1).             */

#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* ---------------- VALIDATE FILE PTR ----------------- */
    if (p_file == (void *)0) {                                  /* Validate NULL file ptr.                              */
        return (DEF_FAIL);
    }
#endif

                                                                /* ------------------ GET FILE SIZE ------------------- */
    p_file_fs   = (HTTPs_FS_FILE *)p_file;
    p_file_data =  p_file_fs->FileDataPtr;
   *p_size      =  p_file_data->Size;

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
*                                       HTTPs_FS_CalcSimilarity()
*
* Description : Calculate 'similarity' between two file names.
*
* Argument(s) : p_name_1    Pointer to first  file name.
*
*               p_name_2    Pointer to second file name.
*
* Return(s)   : Similarity, in characters.
*
* Caller(s)   : HTTPs_FS_AddFile().
*
* Note(s)     : (1) The similarity between two file names is the length of the initial path components
*                   (including leading & embedded path separator characters) shared by the file names.
*********************************************************************************************************
*/

static  CPU_SIZE_T  HTTPs_FS_CalcSimilarity (CPU_CHAR  *p_name_1,
                                             CPU_CHAR  *p_name_2)
{
    CPU_SIZE_T  similarity;
    CPU_SIZE_T  cmp_len;

                                                                /* ---------------- VALIDATE STR PTRS ----------------- */
    if ((p_name_1 == (CPU_CHAR *)0) ||
        (p_name_2 == (CPU_CHAR *)0)) {
         return (0u);
    }


    similarity = 0u;
    cmp_len    = 0u;
    while ((*p_name_1 == *p_name_2) &&                          /* Cmp strs until non-matching chars ...                */
           (*p_name_1 !=  ASCII_CHAR_NULL)) {                   /* ... or NULL chars found.                             */
        if (*p_name_1 == HTTPs_FS_PATH_SEP_CHAR) {
            similarity = cmp_len;
        }
        cmp_len++;
        p_name_1++;
        p_name_2++;
    }

                                                                /* Chk whether one file is ancestor of other.           */
    if (((*p_name_1 == ASCII_CHAR_NULL) && (*p_name_2 == HTTPs_FS_PATH_SEP_CHAR)) ||
        ((*p_name_2 == ASCII_CHAR_NULL) && (*p_name_1 == HTTPs_FS_PATH_SEP_CHAR))) {
        similarity = cmp_len;
    }

    return (similarity);
}


/*
*********************************************************************************************************
*                                   HTTPs_FS_FileDateTimeCreateGet()
*
* Description : Get file creation date/time.
*
* Argument(s) : p_file  Pointer to a file.
*
*               p_time  Pointer to variable that will receive the date/time :
*
* Return(s)   : DEF_OK,   if file creation date/time gotten.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : HTTPs_FS_API_Static.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  HTTPs_FS_FileDateTimeCreateGet (void              *p_file,
                                                     NET_FS_DATE_TIME  *p_time)
{

#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                  /* ---------------- VALIDATE DATE/TIME PTR ------------ */
    if (p_time == (NET_FS_DATE_TIME *)0) {
        return (DEF_FAIL);
    }
#endif

#if (HTTPs_FS_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                  /* ---------------- VALIDATE FILE PTR ----------------- */
    if (p_file == (void *)0) {                                  /* Validate NULL file ptr.                              */
        return (DEF_FAIL);
    }
#endif

                                                                /* Set each creation date/time field to be rtn'd.       */
    p_time->Yr    = HTTPs_FS_Time.Yr;
    p_time->Month = HTTPs_FS_Time.Month;
    p_time->Day   = HTTPs_FS_Time.Day;
    p_time->Hr    = HTTPs_FS_Time.Hr;
    p_time->Min   = HTTPs_FS_Time.Min;
    p_time->Sec   = HTTPs_FS_Time.Sec;

    return (DEF_OK);
}
