#! /usr/bin/env python

#    General imports
import re
import os
import io
import sys
import logging
import argparse

#    Specific imports
from itertools import islice
from collections import namedtuple
from string import Template

FORMAT = '%(message)s'
logging.basicConfig(format=FORMAT, level=logging.ERROR)

TEMPLATE_FUNCTION_NAME = 'GeneratedFS_FileAdd'

#    This section defines the working material for the generation section

#    Define the working structure of the generator
Header        = namedtuple("Header",         "macro name data")
#    Define a lighter version of the structure above.
#    This version allows the garbage collector to dispose of the used
#    file binary data
LightHeader = namedtuple("LightHeader", "macro name")

#    Template of the include guard. (Mostly wraps the content of a header file)
includeGuardTemplate     = "#ifndef {macro}_H\n#define {macro}_H\n\n{content}\n\n#endif /* {macro}_H */"

#    Template of a file's constants definitions. (_NAME, CONTENT, _SIZE)
headerTemplate           = "#define {macro}_NAME \"{name}\"\n#define {macro}_CONTENT \"{content}\"\n#define {macro}_SIZE (sizeof({macro}_CONTENT)-1)"
guardedHeaderTemplate    = includeGuardTemplate.format(macro="{macro}", content=headerTemplate)

#    Template of the globale include file definition.
addToFSHeaderTemplate    = "{content}\n\nCPU_BOOLEAN %s();" % TEMPLATE_FUNCTION_NAME
guardedFSHeaderTemplate    = includeGuardTemplate.format(macro="{macro}", content=addToFSHeaderTemplate)

#    Template of a single include.
includeTemplate      = "#include \"{name}.h\""
#    Template of the name of the header files
headerNameTemplate   = "{name}.h"

#    Template of the add to DYNAMIC file system.
addToFSTemplate      = "#include <fs_app.h>\n#include <fs_file.h>\n#include <fs_entry.h>\n#include \"generated_fs.h\"\n\nCPU_BOOLEAN %s() {{\n    FS_ERR  err;\n    FS_FILE *file;\n\n{create_dirs}\n{create_files}\n    return (DEF_TRUE);\n}}" % TEMPLATE_FUNCTION_NAME
#    Templates to create the file and the folders in the DYNAMIC file system
createFileTemplate   = "    file = FSFile_Open({macro}_NAME, FS_FILE_ACCESS_MODE_WR | FS_FILE_ACCESS_MODE_CREATE, &err);\n    if(err != FS_ERR_NONE) {{\n        return (DEF_FALSE);\n    }}\n    FSFile_Wr(file, {macro}_CONTENT, {macro}_SIZE, &err);\n    FSFile_Close(file, &err);\n    if(err != FS_ERR_NONE) {{\n        return (DEF_FALSE);\n    }}"
createDirTemplate     = "    FSEntry_Create(\"{name}\", FS_ENTRY_TYPE_DIR, DEF_YES, &err);\n    if(err != FS_ERR_NONE) {{\n        return (DEF_FALSE);\n    }}"

#    Template of the add to STATIC file system.
#buildStaticFSTemplate= "#include  <Server/FS/Static/http-s_fs_static.h>\n#include \"generated_fs.h\"\n\nCPU_BOOLEAN %s() {{\n{create_files}\n    return (DEF_TRUE);\n}}" % TEMPLATE_FUNCTION_NAME

buildStaticFSTemplate='''
#include  <Server/FS/Static/http-s_fs_static.h>
#include  "generated_fs.h"
#include  <lib_def.h>

typedef  struct  gen_file_desc {{
    CPU_CHAR    *Name;
    CPU_CHAR    *Content;
    CPU_INT32U   Size;
}} GEN_FILE_DESC;

static  const  GEN_FILE_DESC  FileTbl[] = {{
{create_files}
    {{0, 0, 0}}
}};

CPU_BOOLEAN GeneratedFS_FileAdd() {{
    const  GEN_FILE_DESC  *p_file_desc = &FileTbl[0];
           CPU_BOOLEAN     result      = DEF_OK;


    while ((p_file_desc->Name != 0) && (result == DEF_OK)) {{
        result = HTTPs_FS_AddFile(p_file_desc->Name, p_file_desc->Content, p_file_desc->Size);
        p_file_desc++;
    }}

    return (result);
}}
'''

#    Template to add a file to the STATIC file system.
createStaticFile     = "    {{{macro}_NAME, {name_gap}{macro}_CONTENT, {content_gap}{macro}_SIZE}},"

#    This function reads a file by "n" bytes
def readBytes(filename, n):
    with open(filename, "rb") as file:
        l = file.read(n)
        while len(l) == n:
            yield l
            l = file.read(n)
        yield l
    # end with
#end readBytes()

#    This function formats the content string of the file.
def buildData(filename, n):
    return '\"\\\n\"'.join([''.join(["\\x{0:02x}".format(b) for b in chunk]) for chunk in readBytes(filename, n)])
# end buildData()

def FinalFileNameGet(destinationPath):
    m    = re.search(r'[^\\/]',      destinationPath)
    name = re.sub(r'[. \\/-]+', "_", destinationPath[m.start():]);

    if re.match(r'[^a-zA-Z_]',name[:1]) != None:
        name = "_"+name
    # end if

    return (name)
# end destinationPath()

#    Builds and header structure from a given file and a destination folder.
def buildHeaderFromFile(filename, destinationPath, relativePath):
    logging.debug('filename        = %s' % filename)
    logging.debug('destinationPath = %s' % destinationPath)
    logging.debug('relativePath    = %s' % relativePath)

    name = FinalFileNameGet(destinationPath)

    logging.debug('name            = %s' % name)

    macro           = name.upper()
    formattedData   = buildData(filename, 20)
    formattedHeader = guardedHeaderTemplate.format(macro=macro, name=relativePath, content=formattedData)
    return Header(macro=macro, name=name, data=formattedHeader)
# end buildHeaderFromFile()

#    Formats path char to escape the backslashes
def formatPathChar(c):
    if c == '\\' or c == '/':
        return '\\\\'
    else:
        return c
    # end if
# end formatPathChar()

#    Translate a given rooted path into a relative path and escape the backslashes.
def createFSPath(path):
    logging.debug('path = %s' % path)
    return ''.join([formatPathChar(c) for c in path])
# end createFSPath()

#    Enumerates the sub directories of a given path
def getSubDirs(file_list):
    logging.debug('path = %s' % file_list)

    directory_dict = {}

    for relative_path, source_path, destination_path in file_list:
        path_list = relative_path.split(os.sep)

        logging.debug('len(path_list) = %d' % len(path_list))
        if (len(path_list) > 0):

            for ix in range(0, len(path_list) - 1):
                directory = os.sep.join(path_list[0:ix+1])

                if (directory not in directory_dict):
                    directory_dict[directory] = createFSPath(directory)
                # end if
            # end ofr
        # end if
    # end for

    key_list = list(directory_dict.keys())
    key_list.sort()

    for key in key_list:
        yield directory_dict[key]
    # end for
# end getSubDirs()

#    Writes the given string at the given file path
def writeFile(p, s):
    with open(p, "w+") as f:
        f.write(s)
    # end with
# end writeFile



def Generate(sourceDir, generateDir, fsType, exclude_template=False, force=False):
    #    List of the generated headers
    generatedHeaders = []

    #    GENERATION SECTION

    #    Create the generated directory if it doesn't exist
    try:
        os.makedirs(generateDir)
    except OSError as exc:
        #    the directory already exists.
        pass
    # end try

    # Get file list
    file_list = []

    if (os.path.isdir(sourceDir)):
        source_dir_real = os.path.realpath(sourceDir)
        logging.debug('source_dir_real = %s' % source_dir_real)
        for path, subdirs, files in os.walk(sourceDir):
            for name in files:
                source_path      = os.path.realpath(os.path.join(path, name))
                relative_path    = os.path.relpath(source_path, source_dir_real)
                destination_path = FinalFileNameGet(createFSPath(relative_path))

                logging.debug('%s : %s' % (relative_path, source_path))
                file_list.append((relative_path, source_path, destination_path))
            # end for
        #end for
    else:
        source_path      = os.path.realpath(sourceDir)
        relative_path    = os.path.basename(sourceDir)
        destination_path = FinalFileNameGet(createFSPath(relative_path))
        file_list.append((relative_path, source_path, destination_path))
    # end if

    logging.debug('\n%s' % str(file_list))

    # Check for existing files. Do not overwrite file unless --force is specified.
    existing_file_list = []
    for relative_path, source_path, destination_path in file_list:
        dest_file_path = os.path.join(generateDir, destination_path + '.h')
        if (os.path.exists(dest_file_path)):
            existing_file_list.append(dest_file_path)
            logging.debug('Adding %s to existing_file_list.' % dest_file_path)
        else:
            logging.debug('%s does not exists' % dest_file_path)
        # end if
    # end for


    if (len(existing_file_list) > 0) and (force == False):
        if (len(existing_file_list) == 1):
            file_plurial  = ''
            verb_plurial  = 'it'
            exist_plurial = 's'
        else:
            file_plurial  = 's'
            verb_plurial  = 'they'
            exist_plurial = ''
        # end if

        logging.error("fatal: Can't generate the following file%s since %s already exist%s:" % (file_plurial, verb_plurial, exist_plurial))
        logging.error('  ' + '\n  '.join(existing_file_list))
        logging.error('\nUse --force to overwrite files.')

        sys.exit(1)
    # end if

    #    generate the header files
    for relative_path, source_path, destination_path in file_list:
        #    remove destination files from the generated directory

        if (os.path.exists(destination_path)):
            os.remove(destination_path)
        # end if

        h = buildHeaderFromFile(source_path, destination_path, relative_path)

        generatedHeaders.append(LightHeader(macro=h.macro, name=h.name))
        headerpath = os.path.join(generateDir, headerNameTemplate.format(name=h.name))
        writeFile(headerpath, h.data)
        print(headerpath)
    #end for

    if (exclude_template == False):
        #    Generate the appropriate add to fs function content.
        if fsType == "dynamic":
            dirCreateBlock  = "\n".join([createDirTemplate.format(name=dir) for dir in sorted(set(getSubDirs(file_list)))]);
            fileCreateBlock = "\n".join([createFileTemplate.format(macro=h.macro) for h in generatedHeaders])
            writeFile(os.path.join(generateDir, "generated_fs.c"), addToFSTemplate.format(create_dirs=dirCreateBlock, create_files=fileCreateBlock))
        elif fsType == "static":
            max_name_len    = 0

            for h in generatedHeaders:
                if (len(h.macro) > max_name_len):
                    max_name_len = len(h.macro)
                # end if
            # end for

            fileCreateBlock = "\n".join([createStaticFile.format(macro      =h.macro,
                                                                 name_gap   =' '*(max_name_len - len(h.macro)),
                                                                 content_gap=' '*(max_name_len - len(h.macro))) for h in generatedHeaders])

            writeFile(os.path.join(generateDir, "generated_fs.c"), buildStaticFSTemplate.format(create_files=fileCreateBlock))
        # end if


        #    Include all the generated headers files in the generated files header.
        includeBlock = '\n'.join([includeTemplate.format(name=h.name) for h in generatedHeaders])
        writeFile(os.path.join(generateDir, "generated_fs.h"), guardedFSHeaderTemplate.format(macro="GENERATED_FS", content=includeBlock));
    # end if
# end Generate()

# If called from command line
if __name__ == '__main__':
    #    This section parses the input arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("source",   metavar='source',          help="Source Folder or File.")
    parser.add_argument("generateDir", metavar='target_folder',          help="Destination for generated files.")
    #parser.add_argument("fsType",      choices=['static', 'dynamic'],    help="Type of file-system to be used.")
    parser.add_argument("--exclude-template", "-x", action='store_true', help="Exclude Static and Dynamic FS %s()." % TEMPLATE_FUNCTION_NAME)
    parser.add_argument("--force", "-f",            action='store_true', help="Overwrite generated files.")
    args = parser.parse_args()

    #Generate(args.source, args.generateDir, args.fsType, args.exclude_template, args.force)
    Generate(args.source, args.generateDir, 'static', args.exclude_template, args.force)
# end if
