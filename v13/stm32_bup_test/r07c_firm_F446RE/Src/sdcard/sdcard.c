/**
 *  SatCom Library
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/satcom/
 *
 *  See LICENSE file for details.
**/

#include <string.h> /* For memset,memcpy */
#include <stdarg.h>
#include <stdio.h>

#include "sdcard.h"
#include "sdcard_fat.h"




#include "../log.h"

char logout_buff[256];
void sdc_logout_internal(char* string, ...)
{
#if SDC_LOGOUT_USE == 1
    va_list argptr;

    /* Print the log message to the memory. */
    va_start(argptr, string);
    vsnprintf(logout_buff, sizeof(logout_buff)-1, string, argptr);
    va_end(argptr);

    logout_buff[sizeof(logout_buff)-1] = '\0';

    termout(WL_LOG_DEBUGNORMAL, "%s", logout_buff);
#endif // SDC_LOGOUT_USE == 1
}





sdcard_t sdc_global = { 0 };
sdcard_t* sc_get_sdcard_buff(void)
{
    return &sdc_global;
}



void scf_init(void)
{
    sdc_logout("@@@scd_init start ...", 0);

    /* Init SD card library. */
    sdc_fat_reset();
    sdc_fat_init();
}


unsigned long scf_size(unsigned char dev_id, char* filename)
{
    unsigned long size = 0;

    sdc_logout("@@@scf_size(dev_id=%d, name=\"%s\")", dev_id, filename);

    if(dev_id == SC_FILE_SDCARD)
    { /* Use file from SD card. */
        sdc_fat_file_size(filename, &size);
    }

    sdc_logout("@@@scf_size end size=%d", size);

    return size;
}



unsigned long scf_read(unsigned char dev_id, char* filename, unsigned long offset, unsigned long size, void* ptr)
{
    unsigned long retsize = 0;

    sdc_logout("@@@scf_fread(0ffset=%d, size=%d, dev_id=%d, name=\"%s\")", offset, size, dev_id, filename);

    if(dev_id == SC_FILE_SDCARD)
    { /* Use file from SD card. */
        sdc_ret_t ret;
        unsigned long file_size = scf_size(dev_id, filename);

        /* Check parameters. */
        retsize = size;
        if(offset > file_size)
        {
            offset = 0;
        }
        if((offset + retsize) > file_size)
        {
            retsize = file_size - offset;
        }

        if(retsize)
        {
            sdc_logout("@@@[SD]sdc_fat_read_file(offset=%d, size=%d) ...", offset, retsize);
            ret = sdc_fat_read_file(filename, offset, retsize, (unsigned char*)ptr, &retsize);
            if(ret != SDC_OK) retsize = 0;
        }
    }

    sdc_logout("@@@scf_read end retsize=%d", retsize);

    return retsize;
}


unsigned long scf_write(void* ptr, unsigned long size, unsigned char dev_id, char* filename)
{
    unsigned long retsize = 0;

    sdc_logout("@@@scf_write(size=%d, dev_id=%d, name=\"%s\")", size, dev_id, filename);

    if(dev_id == SC_FILE_SDCARD)
    { /* Use file from SD card. */
        sdc_ret_t ret;
        ret = sdc_fat_write_file(filename, size, (unsigned char*)ptr, &retsize);
        if(ret != SDC_OK) retsize = 0;
    }

    sdc_logout("@@@scf_write end retsize=%d", retsize);

    return retsize;
}


unsigned long scf_append(void* ptr, unsigned long size, unsigned char dev_id, char* filename)
{
    unsigned long retsize = 0;

    if(dev_id == SC_FILE_SDCARD)
    { /* Use file from SD card. */
        sdc_ret_t ret;
        ret = sdc_fat_append_file(filename, size, (unsigned char*)ptr, &retsize);
        if(ret != SDC_OK) retsize = 0;
    }

    return retsize;
}


unsigned long scf_mkdir(unsigned char dev_id, char* folder)
{
    unsigned long retval = 0;

    if(dev_id == SC_FILE_SDCARD)
    { /* Create folder on SD card. */
        sdc_ret_t ret;
        ret = sdc_fat_mkdir(folder);
        retval = (ret == SDC_OK ? 1 : 0);
    }

    return retval;
}


/**
 * Remove file specified in path.
 *
 * Return 1 on success, 0 else.
**/
unsigned long scf_delete(unsigned char dev_id, char* path)
{
    unsigned long retval = 0;

    if(dev_id == SC_FILE_SDCARD)
    { /* Delete file on SD card. */
        sdc_ret_t ret;
        ret = sdc_fat_unlink(path);
        if(ret == SDC_OK)
        {
            retval = 1;
        }
    }

    return retval;
}





char sc_toupper(char c)
{
    if((c >= 'a') && (c <= 'z'))
    {
        c -= 'a';
        c += 'A';
    }
    return c;
}
char sc_tolower(char c)
{
    if((c >= 'A') && (c <= 'Z'))
    {
        c -= 'A';
        c += 'a';
    }
    return c;
}


int scf_compare_extension(char* str, char* ext)
{
    int ret = 0;
    int i;
    char e1[4];
    char e2[sizeof(e1)];

    for(i=0; i<sizeof(e1); i++)
    {
        e1[i] = '\0';
        e2[i] = '\0';
    }

    i = strlen(str);
    if(i > 4)
    {
        str += i-4;
    }
    for(i=0; i<4; i++)
    {
        if((*str) == '.')
        {
            str++;
            break;
        }
        str++;
    }
    for(i=0; i<3; i++)
    {
        if(str[i] == '\0') break;
        e1[i] = sc_toupper(str[i]);
    }

    if(ext[0] == '.')
    {
        for(i=0; i<3; i++)
        {
            if(ext[i+1] == '\0') break;
            e2[i] = sc_toupper(ext[i+1]);
        }
    }
    else
    {
        for(i=0; i<3; i++)
        {
            if(ext[i] == '\0') break;
            e2[i] = sc_toupper(ext[i]);
        }
    }

    if((e1[0] == e2[0]) && (e1[1] == e2[1]) && (e1[2] == e2[2]))
    {
        ret = 1;
    }

    return ret;
}


char* scf_get_filename(char* path)
{
    char* ret = path;
    int n, i;

    n = strlen(path);

    /* Fix backslash in path. */
    for(i=0; i<(n-1); i++)
    {
        if((path[i] == '/' )
        || (path[i] == '\\'))
        {
            ret = path+i+1;
        }
    }

    return ret;
}

void scf_parent_path(char* path)
{
    int n, i;

    n = strlen(path);

    /* Fix backslash in path. */
    for(i=0; i<n; i++)
    {
        if((path[i] == '/' )
        || (path[i] == '\\'))
        {
            path[i] = '/';
        }
    }

    /* Remove eventual final slash. */
    if(n != 0)
    {
        if(path[n-1] == '/' )
        {
            n--;
            path[n] = '\0';
        }
    }

    if(n != 0)
    {
        for(i=n-1; i>=0; i--)
        {
            if(path[i] == '/')
            {
                path[i] = '\0';
                break;
            }
            /* Back to first folder -> root folder. */
            if(i == 0) path[0] = '\0';
        }
    }
}


/**
 * Return 1 if f1's name is greater than f2's name, zero else.
 * Note : case insensitive.
 * (for use in scf_filelist function only.)
**/
int scf_file_is_greater(scdf_file_entry_t* f1, scdf_file_entry_t* f2)
{
    int ret = 0;
    int i;
    char c1, c2;

    if((f1->flags & FILEENTRY_FLAG_FOLDER) != (f2->flags & FILEENTRY_FLAG_FOLDER))
    { /* Put folder on top of list. */
        ret = (f1->flags & FILEENTRY_FLAG_FOLDER ? 0 : 1);
    }
    else
    {
        for(i=0; i<SC_FILENAME_MAXLEN; i++)
        {
            c1 = sc_tolower(f1->filename[i]);
            c2 = sc_tolower(f2->filename[i]);

            if(c1 != c2)
            {
                ret = (c1 > c2 ? 1 : 0);
                break;
            }
        }
    }

    return ret;
}

/**
 * List files in specified folder.
 *
 * Return the count of files in the specified folder.
**/
unsigned long scf_filelist(scdf_file_entry_t* list_ptr, unsigned long list_count, unsigned long list_offset, unsigned char list_sort, char* filter, unsigned char dev_id, char* path)
{
    unsigned long retval = 0;
    long file_index;
    unsigned long items_count;

    if((list_ptr)
    && (list_count != 0))
    {
        /* Reset file list data. */
        memset((void*)list_ptr, 0x00, list_count*sizeof(scdf_file_entry_t));
    }


    if(dev_id == SC_FILE_SDCARD)
    { /* Use file from SD card. */
        sdc_fat_filelist(list_ptr, list_count, list_offset, path, &retval);
    }


    /* Get number if items in list. */
    items_count = retval;
    if(items_count > list_count)
    {
        items_count = list_count;
    }

    if(list_ptr)
    {
        /* Sanitize folder names : append slash on begining. */
        for(file_index=0; file_index<list_count; file_index++)
        {
            if((list_ptr[file_index].flags & FILEENTRY_FLAG_ENABLED)
            && (list_ptr[file_index].flags & FILEENTRY_FLAG_FOLDER ))
            {
                /* Append slash on begining of folder names. */
                if((list_ptr[file_index].filename[0] != '/' )
                && (list_ptr[file_index].filename[0] != '\\'))
                {
                    int i;
                    for(i=SC_FILENAME_MAXLEN-2; i>=0; i--)
                    {
                        list_ptr[file_index].filename[i+1] = list_ptr[file_index].filename[i];
                    }

                    /* Add slash. */
                    list_ptr[file_index].filename[0] = '/';

                    /* Don't forget terminating null character. */
                    list_ptr[file_index].filename[SC_FILENAME_MAXLEN-1] = '\0';
                }

                /* Remove "." and ".." folders. */
                if((list_ptr[file_index].filename[1] ==  '.')
                && (list_ptr[file_index].filename[2] == '\0'))
                {
                    list_ptr[file_index].flags &= ~FILEENTRY_FLAG_ENABLED;
                }
                if((list_ptr[file_index].filename[1] ==  '.')
                && (list_ptr[file_index].filename[2] ==  '.')
                && (list_ptr[file_index].filename[3] == '\0'))
                {
                    list_ptr[file_index].flags &= ~FILEENTRY_FLAG_ENABLED;
                }
            }
        }

        /* Reset sort index. */
        for(file_index=0; file_index<list_count; file_index++)
        {
            list_ptr[file_index].sort_index = file_index;
        }

        /* If required, apply file filtering. */
        if(filter != NULL)
        {
            char ext[5];
            int n = strlen(filter);
            int i, j;

            /* Mark all folders as selected by filtering. */
            for(file_index=0; file_index<items_count; file_index++)
            {
                if(list_ptr[file_index].flags & FILEENTRY_FLAG_FOLDER)
                {
                    list_ptr[file_index].flags |= FILEENTRY_FLAG_FILTERED;
                }
            }

            for(i = 0; i<n; i++)
            {
                if((i == 0)
                || (filter[i] == ',')
                || (filter[i] == '\0'))
                {
                    if(filter[i] == ',')
                    {
                        i++;
                    }

                    for(j=0; j<sizeof(ext); j++)
                    {
                        ext[j] = '\0';
                    }

                    for(j=0; j<(sizeof(ext)-1); j++)
                    {
                        if(filter[i] == '\0') break;
                        ext[j] = filter[i];
                        if(filter[i+1] == ',') break;
                        i++;
                    }

                    for(file_index=0; file_index<items_count; file_index++)
                    {
                        /* Don't compare already selected files. */
                        if((list_ptr[file_index].flags & FILEENTRY_FLAG_FILTERED) == 0)
                        {
                            if(scf_compare_extension(list_ptr[file_index].filename, ext))
                            {
                                list_ptr[file_index].flags |= FILEENTRY_FLAG_FILTERED;
                            }
                        }
                    }
                }
            }

            /* Disable non filtered files. */
            for(file_index=0; file_index<items_count; file_index++)
            {
                if((list_ptr[file_index].flags & FILEENTRY_FLAG_FILTERED) == 0)
                {
                    list_ptr[file_index].flags &= ~FILEENTRY_FLAG_ENABLED;
                }
                list_ptr[file_index].flags &= ~FILEENTRY_FLAG_FILTERED;
            }
        }

        /* Sort entries */
        if(list_sort)
        {
            // Sort faces. (from mic's flatcube)
            // Bubble-sort the whole lot.. yeehaw!
            if(items_count > 1)
            {
                unsigned short i, j, idi, idj;
                unsigned short temp;
                for(i=0; i<items_count-1; i++)
                {

                    for(j=i+1; j<items_count; j++)
                    {
                        idi = list_ptr[i].sort_index;
                        idj = list_ptr[j].sort_index;

                        if(scf_file_is_greater(list_ptr+idi, list_ptr+idj))
                        {
                            temp                   = list_ptr[i].sort_index;
                            list_ptr[i].sort_index = list_ptr[j].sort_index;
                            list_ptr[j].sort_index = temp;
                        }
                    }
                }
            }
        } // if(list_sort)
    }

    return retval;
}


/**
 * Return the count of files/folders in specified search result data.
**/
unsigned long scf_filecount(scdf_file_entry_t* list_ptr, unsigned long list_count)
{
    unsigned short list_index;
    unsigned long ret = 0;

    for(list_index=0; list_index<list_count; list_index++)
    {
        if(list_ptr[list_index].flags & FILEENTRY_FLAG_ENABLED)
        {
            ret++;
        }
    }

    return ret;
}


