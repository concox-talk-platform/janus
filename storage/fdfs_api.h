//fdfs_api.h

#ifndef _FDFS_API_H
#define _FDFS_API_H

#ifdef __cplusplus
extern "C" {
#endif

/*
*init function
* param proccess_index the process index based 0
* param conf_filename the config filename
* return 0 if success, none zero for error
*/
int dfs_init(const int proccess_index, const char *conf_filename);

/*
*destroy function
* return void
*/
void dfs_destroy(void);

/*
* upload file to the storage server
* param file_buff the file content
* param file_size the file size (bytes)
* param file_id return the file id (max length 63)
* param storage_ip return the storage server ip address (max length 15)
* return 0 if success, none zero for error
*/
int upload_file_by_buff(const char *file_buff, const int file_size, const char *file_ext_name, char *file_id, char *storage_ip);

/*
* upload file to the storage server
* param file_name the file name
* param file_id return the file id (max length 63)
* param storage_ip return the storage server ip address (max length 15)
* return 0 if success, none zero for error
*/
int upload_file_by_filename(const char *file_name, const char *file_ext_name, char *file_id, char *storage_ip);


/*
* download file from the storage server
* param file_id the file id
* param file_size return the file size (bytes)
* param storage_ip return the storage server ip address (max length 15)
* return 0 if success, none zero for error
*/
int download_file(const char *file_id, int *file_size, char *storage_ip);

/*
* delete file from the storage server
* param file_id the file id
* param storage_ip return the storage server ip address (max length 15)
* return 0 if success, none zero for error
*/
int delete_file(const char *file_id, char *storage_ip);

#ifdef __cplusplus
}
#endif

#endif
