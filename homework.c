/*
 * file:        homework.c
 * description: skeleton file for CS 7600 homework 3
 *
 * CS 7600, Intensive Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2015
 */

#define FUSE_USE_VERSION 27

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fuse.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "fs7600.h"
#include "blkdev.h"


extern int homework_part;       /* set by '-part n' command-line option */

/* 
 * disk access - the global variable 'disk' points to a blkdev
 * structure which has been initialized to access the image file.
 *
 * NOTE - blkdev access is in terms of 1024-byte blocks
 */
extern struct blkdev *disk;

/*
 * cache - you'll need to create a blkdev which "wraps" this one
 * and performs LRU caching with write-back.
 */

uint32_t clean_valid [30]; 
int clean_b_num[30];
int clean_age[30];   
char * clean_b_data[30]; 

uint32_t dirty_valid[10]; 
int dirty_b_num[10];
int dirty_age[10];  
char * dirty_b_data[10]; 


void b_cache_initialize(){
	printf("------ in b_cache_initialize function\n"); 	
	int i = 0 ; 
	for(i = 0 ; i < 30; i++){
		clean_b_num[i] = -1;
		clean_valid[i] = 0;  
		clean_age[i] = 0; 

	}

	for (i = 0 ; i < 10 ; i++){
		dirty_b_num[i] = -1; 
		dirty_valid[i] = 0; 
		dirty_age[i] = 0; 
	} 
}

void clean_update_age(int cache_index){
	
	printf("------ in clean_update_age, cache_index is %d\n", cache_index); 
	int i; 

	for (i = 0 ; i < 30; i++){
		if (clean_valid[i] == 1){
			if (i == cache_index){
				clean_age[i] = 1; 
			}else{
				clean_age[i]++; 
			}
		}
	}	
}

void dirty_update_age(int cache_index){
	printf("------ in dirty_update_age, cache_index is %d\n", cache_index); 
	int i; 

	for (i = 0 ; i < 10; i++){
		if (dirty_valid[i] == 1){
			if (i == cache_index){
				dirty_age[i] = 1; 
			}else{
				dirty_age[i]++; 
			}
		}
	}
}
int clean_get_cache_index(int block_index){
	printf("------ in clean_get_cache_index, block_index is %d\n", block_index); 
	int i; 
	for(i = 0 ; i < 30; i ++ ){
		if (clean_valid[i] == 1 && clean_b_num[i] == block_index){	
			clean_update_age(i); 
			return i ; 
		}
	}
	return -1; 
}
int dirty_get_cache_index(int block_index){
	printf("------ in dirty_get_cache_index, block_index is %d\n", block_index); 
	int i; 

	for(i = 0 ; i < 10; i++){
		if (dirty_valid[i] == 1 && dirty_b_num[i] == block_index){
			dirty_update_age(i); 
			return i; 
		}
	}
	return -1; 
}

void clean_insert(int block_index , char * data){
	printf("------ in clean_insert function, block_index is %d\n", block_index); 

	int i; 
	for (i = 0 ; i < 30; i++){
		if(clean_valid[i] == 0){
			clean_valid[i] = 1; 
			clean_b_num[i] = block_index; 
			if (clean_b_data[i] == NULL){
				free( clean_b_data[i]); 
			}
			clean_b_data[i] = malloc(FS_BLOCK_SIZE); 
			memcpy(clean_b_data[i] , data, FS_BLOCK_SIZE); 
			clean_update_age(i); 
			printf("------ insert in free index %d\n", i); 			
			return; 
		}	
	}	
	
	printf("------ no free space on clean cache"); 
	int free_index = 0; 
	int max_age = 0; 
	
	for (i = 0 ; i < 30; i++){
		if (clean_age[i] > max_age){
			free_index = i; 
			max_age = clean_age[i]; 
		}
	
	}
	
	clean_b_num[ free_index] = block_index; 
	clean_valid[ free_index] = 1; 
	if (clean_b_data[free_index] != NULL)
		free(clean_b_data[free_index]); 

	clean_b_data[free_index ] = malloc(FS_BLOCK_SIZE); 
	memcpy(clean_b_data[free_index], data, FS_BLOCK_SIZE); 
	clean_update_age( free_index); 
	return ; 

}

int  dirty_insert ( int block_index, char * data, char ** old_data){

	printf ("------ in dirty_insert function, block_index is %d\n", block_index ); 
	int i; 
	for (i = 0; i < 10; i++){
		if (dirty_valid[i] == 0){
			printf("------ index %d is empty\n", i); 
			dirty_valid[i] = 1; 
			dirty_b_num[i] = block_index; 
			if (dirty_b_data[i] != NULL)
				free(dirty_b_data[i]); 

			dirty_b_data[i] = malloc(FS_BLOCK_SIZE); 
			memcpy(dirty_b_data[i], data, FS_BLOCK_SIZE); 
			
			dirty_update_age(i); 	
		
			return -1; 
		}
	
	}

	printf("------ cache is full\n"); 
	
	int free_index = 0; 
	int max_age = 0; 

	for (i = 0 ; i < 10; i++){
		if (dirty_age[i] > max_age){
			max_age = dirty_age[i]; 
			free_index = i; 
		}
	}

	printf ("------ free index is %d\n", free_index);

	int old_block_index = dirty_b_num[free_index]; 
        * old_data = dirty_b_data[free_index]; 
//	memset(old_data, 0, FS_BLOCK_SIZE); 
		
	//memcpy(old_data, dirty_b_data[free_index], FS_BLOCK_SIZE); 
 
	dirty_valid[free_index] = 1; 
	dirty_b_num[free_index] = block_index; 
	
	dirty_b_data[free_index] = malloc (FS_BLOCK_SIZE); 
	memcpy (dirty_b_data[free_index], data, FS_BLOCK_SIZE); 	
	dirty_update_age(free_index); 

	return old_block_index; 
	 
}


void clean_delete_by_cache_index (int cache_index ){

	printf("------ clean_delete_by_cache_index, cache_index %d\n", cache_index); 
	clean_b_num [cache_index] = -1; 
	clean_valid [cache_index] = 0 ; 
	clean_age [cache_index] = 0;
 
	if ( clean_b_data [cache_index] != NULL) 
		free( clean_b_data [cache_index]);

	return;  
}

void cache_read_by_block(struct blkdev *  dev, int block_index , char * buf){
	
	printf("------ cache_read_by_block\n"); 

	int clean_cache_index = clean_get_cache_index(block_index);  
	
	if (clean_cache_index >= 0){
		memcpy(buf, clean_b_data[clean_cache_index] , FS_BLOCK_SIZE);
		return;  
	}
	
	printf("------ clean_cache_index should be -1, %d\n", clean_cache_index); 

	int dirty_cache_index = dirty_get_cache_index(block_index);
	if(dirty_cache_index >=0 ){

		memcpy(buf, dirty_b_data[dirty_cache_index], FS_BLOCK_SIZE); 
		return; 
	}

	printf("------ dirty_cache_index should be -1, %d\n", dirty_cache_index); 

	struct blkdev *  old_disk = dev->private; 
	char * data = malloc(FS_BLOCK_SIZE); 
	old_disk->ops->read(old_disk, block_index, 1, data );
	clean_insert(block_index, data); 
	memcpy(buf, data, FS_BLOCK_SIZE);
	free(data);  
 
}

void cache_write_by_block(struct blkdev * dev, int block_index, char * buf){
	
	printf("------ cache_write_by_block, block_index is %d \n", block_index); 

	int clean_cache_index = clean_get_cache_index(block_index); 
	printf("------ clean_cache_index is %d\n", clean_cache_index);

	if ( clean_cache_index >= 0 ){
		clean_delete_by_cache_index( clean_cache_index);
 
		char * old_data; 
		int old_b_index = dirty_insert(block_index, buf, &old_data); 

		if (old_b_index != -1 && old_data != NULL){
			
			struct blkdev *  old_disk = dev->private; 
			old_disk->ops->write(old_disk, old_b_index, 1, old_data );
			free(old_data); 
		}	

		return; 
	}

	printf("------ clean_cache_index should be -1, %d\n", clean_cache_index);	

	int dirty_cache_index = dirty_get_cache_index(block_index); 
	printf("------ dirty_cache_index is %d\n", dirty_cache_index);

	if ( dirty_cache_index >= 0){
		memset(dirty_b_data[ dirty_cache_index ], 0, FS_BLOCK_SIZE); 
		memcpy (dirty_b_data[ dirty_cache_index ] , buf, FS_BLOCK_SIZE); 
		return; 
	}
	
	// insert to dirty 
	// maybe write evicted block
	
	char * old_data; 
	int old_b_index = dirty_insert(block_index, buf, & old_data); 

	if (old_b_index != -1 && old_data != NULL){
			
		struct blkdev *  old_disk = dev->private; 
		old_disk->ops->write(old_disk, old_b_index, 1, old_data );
		free(old_data); 
	}	

	return; 
}

int cache_nops(struct blkdev *dev) 
{
	struct blkdev *d = dev->private;
	return d->ops->num_blocks(d);
}
int cache_read(struct blkdev *dev, int first, int n, void *buf)
{
	printf("------ in cache_read function, first is %d, n is %d\n", first, n); 
	int i ; 
	int offset = 0; 	
	
	for ( i = first ; i < (first + n) ; i++ ){
//		struct blkdev *  old_disk = dev->private; 
//		old_disk->ops->read(old_disk, i, 1, ((char *) buf) + offset );
		cache_read_by_block(dev, i, ((char * ) buf) + offset); 
		offset += FS_BLOCK_SIZE;  
		
	}


	//struct blkdev *  old_disk = dev->private;
	//old_disk->ops->read(old_disk, first, n, buf);


	return SUCCESS;
}
int cache_write(struct blkdev *dev, int first, int n, void *buf)
{
	printf("------ in cache_write function, first is %d, n is %d\n", first, n); 
	int i ; 
	int offset = 0; 
/**/	
	for ( i = first ; i < (first +  n) ; i++ ){

		cache_write_by_block(dev, i,((char * )buf) + offset); 
		//struct blkdev *  old_disk = dev->private; 
		//old_disk->ops->write(old_disk, i, 1, buf + offset );
	
		offset += FS_BLOCK_SIZE; 

	}
/**/
//	struct blkdev *  old_disk = dev->private;
//	old_disk->ops->write(old_disk, first, n, buf);

	return SUCCESS;
}
struct blkdev_ops cache_ops = {
	.num_blocks = cache_nops,
	.read = cache_read,
	.write = cache_write
};
struct blkdev *cache_create(struct blkdev *d)
{
	b_cache_initialize(); 
	
	struct blkdev *dev = malloc(sizeof(*d));
	dev->ops = &cache_ops;
	dev->private = d;
	return dev;
}

/* by defining bitmaps as 'fd_set' pointers, you can use existing
 * macros to handle them. 
 *   FD_ISSET(##, inode_map);
 *   FD_CLR(##, block_map);
 *   FD_SET(##, block_map);
 */
fd_set *inode_map;              /* = malloc(sb.inode_map_size * FS_BLOCK_SIZE); */
fd_set *block_map;
struct fs7600_inode * inodes; 
struct fs7600_super sb; 
int max_num_inodes; 


//------------------------------------------------------------------
char * cache_path[50]; 
int cache_inode[50]; 
int cache_age [50]; 
uint32_t cache_valid[50]; 

void cache_initialize(){
	printf("------ cache_initialize\n"); 
	int i; 
	for (i = 0 ; i < 50 ; i++){
		cache_inode[i] = -1; 
		cache_age[i]   = 0;
		cache_valid[i] = 0;  
	}
}

void cache_update_age(int cache_index ){

	printf("------ cache_update_age, cache_index is %d\n", cache_index); 
	int i ; 

	for (i = 0 ; i < 50; i ++){
		if ( cache_valid [i] == 1 && i == cache_index){
			cache_age[i] = 1; 
		}else if (cache_valid [i] == 1 && cache_inode[i] > 0 ){
			cache_age[i] ++ ; 
		}
		 
	}
}


int  cache_get_inode(const char * path){

	printf("------ cache_get_inode, path is %s\n", path); 

	int i ; 
	for (i = 0 ; i < 50; i++){
		if (cache_valid[i] == 1 && cache_path[i] != NULL){
			if ( strcmp(cache_path[i], path) == 0 ){
				printf("------ cache index is %d, inode is %d\n", i, cache_inode[i]); 
				cache_update_age(i);
				return cache_inode[i];  
			}
						
		}
	}
	return -1; 
}
void cache_insert(const char * path, int inode){

	printf("------ cache_insert, path is %s, inode is %d\n", path, inode); 
	if (inode == 0)
		printf("------ it is odd to have inode 0\n"); 
	
	int i ; 
	for (i = 0 ; i < 50; i++){
		if (cache_valid [i] == 0 ){
			
			printf("------ cache has empty space at index %d\n", i);	
			cache_valid[i] = 1; 
			cache_inode[i] = inode; 
			cache_update_age(i); 

			if ( cache_path[i] != NULL){
				free(cache_path[i]); 
				//cache_path[i] = NULL: 
			}			
			cache_path[i] = strdup(path); 
			return; 	
		}			
	
	}
		
	printf("------ cache is full \n"); 

	int free_index = 0; 
	int max = 0; 
	for (i = 0 ; i < 50; i++){
		if (cache_age[i] > max){
			max = cache_age[i]; 
			free_index = i; 
		}
		
	}

	printf("------ free index is %d\n", free_index); 
	cache_inode[free_index] = inode; 
	cache_update_age(free_index); 

	if ( cache_path[free_index] != NULL){
		free(cache_path[free_index]); 
	}			
	cache_path[free_index] = strdup(path); 
	return; 
}

void cache_delete(int inode){
	printf("------ cache_delete, inode is %d\n", inode); 
	int i; 

	for (i = 0 ; i < 50; i++){
		if (cache_valid [i] == 1 && cache_inode[i] == inode){
			printf("------ cache_index %d is delete\n", i); 
			cache_valid[i] = 0; 
			cache_inode[i] = -1; 
			cache_age[i] = 0;   
		}
	}
}

//------------------------------------------------------------------
void update_disk_block(int block_index , int num_block, void * buff){

	printf("------ in update_disk_block function\n"); 

	if (disk->ops->write(disk, block_index, num_block, buff) < 0)
		exit(0); 
}

int get_inode_block_index(int inode_index){
	
	printf("------ in get_inode_block_index function, inode_index is %d\n", inode_index); 

	int inodes_per_block = FS_BLOCK_SIZE / (sizeof(struct fs7600_inode )); 
	//printf("------ inodes_per_block is %d\n", inodes_per_block); 

	int block_index = inode_index / inodes_per_block; 
	block_index += (1 + sb.inode_map_sz + sb.block_map_sz); 
	//printf("------ block_index is %d\n", block_index ); 
	
	return block_index; 	
}

void * get_inode_block_data(int inode_index){
	printf("------ in get_inode_block_data function, inode_index is %d\n", inode_index); 

	int inodes_per_block = FS_BLOCK_SIZE / (sizeof(struct fs7600_inode )); 
	//printf("------ inodes_per_block is %d\n", inodes_per_block); 

	int block_index = inode_index / inodes_per_block;

	void * block_data = malloc(FS_BLOCK_SIZE);
	int  offset = block_index * FS_BLOCK_SIZE;
	//printf("------ offset is %d", offset);  
	memcpy(block_data,( (char *) inodes) + offset , FS_BLOCK_SIZE);  	
}

void update_disk(int inode_index){

	printf ("------ in update_disk function\n"); 
	// write  inode bitmap // no need to do this! 
	// write block bitmap
	// write inodes 	
	
	if (disk->ops->write(disk, 1, sb.inode_map_sz, inode_map) < 0)
		exit(0); 
	
	if (disk->ops->write(disk, 1 + sb.inode_map_sz, sb.block_map_sz, block_map) < 0)
		exit(0); 

	//if (disk->ops->write(disk, 1 + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, inodes) < 0)
	//	exit(0);

	
	int inode_block_index = get_inode_block_index(inode_index);
	void * inode_block_data = get_inode_block_data(inode_index); 	

	update_disk_block(inode_block_index, 1, inode_block_data );	
	free(inode_block_data); 

}
/**/

/**/

/* init - this is called once by the FUSE framework at startup. Ignore
 * the 'conn' argument.
 * recommended actions:
 *   - read superblock
 *   - allocate memory, read bitmaps and inodes
 */
void* fs_init(struct fuse_conn_info *conn)
{
	
	printf("------ in fs_init function\n");
	printf("------ homework part is %d\n",  homework_part); 
	//struct fs7600_super sb;
	// it is global now 
	if (disk->ops->read(disk, 0, 1, &sb) < 0)
		exit(1);


	printf("--------------------------------------------------\n");
	printf("root inode is %d \n", sb.root_inode) ;
	printf("inode map size is %d \n", sb.inode_map_sz);
	printf("block map size is %d \n", sb.block_map_sz); 
	printf("inode region size is %d \n", sb.inode_region_sz); 
	printf("num of blocks are %d \n",sb.num_blocks ); 
	

	inode_map = (fd_set *) malloc(sb.inode_map_sz * FS_BLOCK_SIZE); 
	block_map = (fd_set *) malloc(sb.block_map_sz * FS_BLOCK_SIZE);
	inodes = ( struct fs7600_inode * ) malloc(sb.inode_region_sz * FS_BLOCK_SIZE);

	max_num_inodes = (sb.inode_region_sz * FS_BLOCK_SIZE)/ ( sizeof(struct fs7600_inode));
	printf("max_num_inodes %d \n", max_num_inodes);  
	 

	if (disk->ops->read(disk, 1, sb.inode_map_sz, inode_map) < 0)
		exit(1);	

	if (disk->ops->read(disk, 1 + sb.inode_map_sz, sb.block_map_sz, block_map) < 0)
		exit(1);

	if (disk->ops->read(disk, 1 + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz ,inodes  ) < 0)
		exit(1); 
	printf("--------------------------------------------------\n");
	
	if (homework_part > 2){
		cache_initialize(); 	
	}

	if (homework_part > 3)
		disk = cache_create(disk);

	return NULL;
}

//---------------------------------------------------------------------
void fill_stat(struct stat * sb, int inode_index){

	printf("------ in fill_stat function, inode index is %d\n", inode_index);
	 
	memset(sb,0,sizeof(sb));
	sb -> st_uid = inodes[inode_index].uid; 
	sb -> st_gid = inodes[inode_index].gid; 
	sb -> st_mode = inodes[inode_index].mode; 
	sb -> st_ctime = inodes[inode_index].mtime; 
	sb -> st_mtime = inodes[inode_index].mtime;
	sb -> st_atime = inodes[inode_index].mtime;
	sb -> st_size = inodes[inode_index].size; 
	sb -> st_nlink = 1; 
	sb -> st_ino = inode_index; 
  
}


/* Note on path translation errors:
 * In addition to the method-specific errors listed below, almost
 * every method can return one of the following errors if it fails to
 * locate a file or directory corresponding to a specified path.
 *
 * ENOENT - a component of the path is not present.
 * ENOTDIR - an intermediate component of the path (e.g. 'b' in
 *           /a/b/c) is not a directory
 */

/* note on splitting the 'path' variable:
 * the value passed in by the FUSE framework is declared as 'const',
 * which means you can't modify it. The standard mechanisms for
 * splitting strings in C (strtok, strsep) modify the string in place,
 * so you have to copy the string and then free the copy when you're
 * done. One way of doing this:
 *
 *    char *_path = strdup(path);
 *    int inum = translate(_path);
 *    free(_path);
 */

/* getattr - get file or directory attributes. For a description of
 *  the fields in 'struct stat', see 'man lstat'.
 *
 * Note - fields not provided in CS7600fs are:
 *    st_nlink - always set to 1
 *    st_atime, st_ctime - set to same value as st_mtime
 *
 * errors - path translation, ENOENT
 */
static int fs_getattr(const char *path, struct stat *sb)
{

	printf ("------ in fs_getattr function, path is %s\n", path);

	char * _path = strdup(path);   
	int inode_index = translate(_path);
	free(_path); 
	
	printf("------ inode_index is %d\n",inode_index); 
 
	if (inode_index <0){
		return -ENOENT; 
	}else {
		fill_stat(sb,inode_index);
               	return 0; 

	}
}
// in translate function, path is given and it returns inode number
int translate(char* _path){

	printf("------ in translate function, path is %s\n", _path);
	char * path_cache = strdup(_path); 		

	int inode_index ; 
	//----------------------
	if (homework_part > 2){
		inode_index = cache_get_inode(_path);
		if (inode_index > 0)
			return inode_index;  		
		
		printf("------ _path %s  is not in cache\n", _path); 
	}	  
	//----------------------
	
	const char delimiters[2] = "/";
	char * token = strtok(_path, delimiters); 
//	printf("------- token is %s\n", token);
	

	inode_index = 1;// sb.root_inode 
	int index; 
	//printf("--- %s    %d\n", token, inode_index);
	
	while(token != NULL){
	
		int block_num = inodes[inode_index].direct[0];
	//	printf("--- %d\n", block_num);

		struct fs7600_dirent * entries = malloc(FS_BLOCK_SIZE);

		if(disk->ops->read(disk,block_num, 1, entries ) < 0 )
			exit(0);       

		index = find_index(token,entries );
          //      printf("index in the entries %d  ", index); 
		if (index < 0 ){
			return -ENOENT;      
		}else{
			inode_index = entries[index].inode;
		}
	//	printf("--- this is inode index  %s    %d\n", token, inode_index);
		token = strtok(NULL, delimiters);
	
		free(entries); 
	}
	  
	
	//-------------------------------------
	if (homework_part > 2){
		printf("------ in translate function, _path is %s before inserting in cache\n", _path); 	
		cache_insert(path_cache, inode_index);
		free(path_cache);  
	}
	//-------------------------------------
	return inode_index; 
}


/*
   find_index function get a name file or directory/ folder and search in the entries array, 
   if it finds a valid entry with the same name, then return the index of that entry in the array.  

 */
int find_index (const char * name, struct fs7600_dirent *  entries  ){

	printf("------ in find_index function, name is %s\n", name);
	int i ;
	
	// the assumption is that directories have 32 entries 
	for ( i = 0 ; i < 32 ; i++){
         //      printf("----%d  %d  %s   %s\n",i, entries[i].inode,  entries[i].name , name);
		if ( entries[i].valid && (strcmp( entries[i].name, name) == 0) ){
			return i; 
		}
	}

	return -1; 

}
/**
uint32_t is_dir_f (char * _path ){

	printf("------ in is_dir_f function, path is %s\n", _path); 
        uint32_t is_dir = 1; 
    
	const char delimiters[2] = "/";
	char * token = strtok(_path, delimiters);

	  
	int inode_index = 1; 
	int index; 

	
	while(token != NULL){
	
		int block_num = inodes[inode_index].direct[0];


		struct fs7600_dirent * entries = malloc(FS_BLOCK_SIZE);

		if(disk->ops->read(disk,block_num, 1, entries ) < 0 )
			exit(0);       

		index = find_index(token,entries );
   
		if (index < 0 ){
			return 0;      
		}else{
			inode_index = entries[index].inode;
                        is_dir = entries[index].isDir; 
		}
		token = strtok(NULL, delimiters);
	}
 
	return is_dir;	
}
/**/
/* readdir - get directory contents.
 *
 * for each entry in the directory, invoke the 'filler' function,
 * which is passed as a function pointer, as follows:
 *     filler(buf, <name>, <statbuf>, 0)
 * where <statbuf> is a struct stat, just like in getattr.
 *
 * Errors - path resolution, ENOTDIR, ENOENT
 */
static int fs_readdir(const char *path, void *ptr, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi)
{

        printf("------ in readdir function, path is %s\n", path);

	int inode_index; 
	if (homework_part >= 2 ){
		inode_index = fi->fh; 
	
	}
	else {
		char * _path = strdup(path);
        	inode_index =  translate(_path);
		free(_path); 
	}
        
	int block_num = inodes[inode_index].direct[0];
//		printf("--- %d\n", block_num);

	struct fs7600_dirent * entries = malloc(FS_BLOCK_SIZE);

	if(disk->ops->read(disk,block_num, 1, entries ) < 0 )
		exit(0);       

        int i; 
	for (i = 0 ; i < 32; i++){
		if (entries[i].valid){

			// was 
			//filler(ptr, entries[i].name, NULL, 0 );
			//struct stat * sb  = malloc(sizeof( struct stat)); 
			//fill_stat(sb,entries[i].inode); 
                       	//filler(ptr, entries[i].name, sb, 0 );

			struct stat sb; 
			fill_stat( &sb, entries[i].inode); 
			filler(ptr, entries[i].name, &sb, 0); 
		} 
                  
	}
	
	free(entries); 
	return 0; 
}


/* see description of Part 2. In particular, you can save information 
 * in fi->fh. If you allocate memory, free it in fs_releasedir.
 */
static int fs_opendir(const char *path, struct fuse_file_info *fi)
{
	  
        printf("------ in fs_opendir function, path is %s\n", path);
/**/	
	if(homework_part >= 2){

		printf("------ opendir, part2\n"); 	

		char * _path = strdup(path);
		int inode_index = translate(_path);
		free(_path);

		fi->fh = inode_index; 
	}
/**/
/**
	if (homework_part == 3){

		printf("------ opendir, part3\n"); 	
		char * _path = strdup(path);
		int inode_index = translate(_path);
		free(_path);

		fi->fh = inode_index; 	
	}
/**/
	return 0;
}

static int fs_releasedir(const char *path, struct fuse_file_info *fi)
{

	printf("------ in fs_releasedir function, path is %s\n", path); 
	return 0;
}
//-----------------------------------------------------------------------------
//
int get_parent_inode(const char* path){

	printf("------ in get_parent_inode function, path is %s\n", path); 
	char * _path = strdup(path); 
	const char delimiters[2] = "/";
	char * token = strtok(_path, delimiters); 

	
        int parent_inode_index = 0; 
	int inode_index = 1; 	

	//printf("---token is %s\n", token); 
	while(token != NULL){
	
                parent_inode_index = inode_index; 

		int block_num = inodes[inode_index].direct[0];
	//	printf("--- %d\n", block_num);

		struct fs7600_dirent * entries = malloc(FS_BLOCK_SIZE);

		if(disk->ops->read(disk,block_num, 1, entries ) < 0 )
			exit(0);       

		int index = find_index(token,entries );
        	       
	
   		token = strtok(NULL, delimiters);
	//	printf("---index in the entries %d\n, token is %s\n", index, token);


//		if (index < 0 && token != NULL){
//			return -ENOTDIR;
 //               }else if (index < 0){ 
//			return parent_inode_index;                              
//		}else{
			inode_index = entries[index].inode;
//		}
	//	printf("--- this is inode index  %s    %d\n", token, inode_index);
	
	}
        
	free(_path);
//	if (inode_index > 0)
//		return -EEXIST; 
	return parent_inode_index; 
        
} 
int  get_empty_entry(int inode_index){

	printf("------ in get_empty_entry function, inode_index is %d\n", inode_index); 	
	int block_num = inodes[inode_index].direct[0];
 
	struct fs7600_dirent * entries = malloc(FS_BLOCK_SIZE);

	if(disk->ops->read(disk,block_num, 1, entries ) < 0 )
		exit(0);
 
/*
//	printf("------ name of entry 0: %s, is  valid %d \n", entries[0].name, entries[0].valid ); 	
//	printf("------ name of entry 1: %s, is  valid %d \n", entries[1].name, entries[1].valid );
//	printf("------ name of entry 2: %s, is  valid %d \n", entries[2].name, entries[2].valid );
	printf("------ name of entry 3: %s, is  valid %d \n", entries[3].name, entries[3].valid );
	printf("------ name of entry 4: %s, is  valid %d \n", entries[4].name, entries[4].valid );
	printf("------ name of entry 5: %s, is  valid %d \n", entries[5].name, entries[5].valid );
/**/
	int i ; 
	for (i = 0 ; i < 32 ; i++ ){
//		printf("------ name of entery : %s\n", entries[i].name);
		if ( !entries[i].valid ){
			
			return i; 
		}
	}
	return -1; 
}

int get_empty_inode_num(){
	
	printf("------ in get_empty_inode_num function\n"); 
	int max_limit = max_num_inodes; 
	int i ; 

	for (i = 1; i < max_limit; i++){
		if( ! FD_ISSET(i,inode_map)){
		 	return i ; 	
		}
	}
	return -1; 
}

int get_empty_datablk_num(){

	printf("------ in get_empty_datablk_num function \n"); 

	int max_limit = sb.num_blocks ; 
	int i = 1 + sb.inode_map_sz + sb.block_map_sz + sb.inode_region_sz ; 

	for ( ; i < max_limit ; i ++){
		if( ! FD_ISSET(i,block_map)){
			return i; 
		}
	}
	return -1; 
}
void fill_arr(int * arr, int val , int len){

	int i = 0; 
	for ( ; i < len ; i++){
		arr[i] = val; 
	}
}
int * get_k_empty_datablk_num(int k){

	printf("------ in get_k_empty_datablk_num, k is %d \n", k); 
	
	int * arr = malloc(sizeof(int) * k); 

//	memset (arr, -1, sizeof(int) * k);
	fill_arr(arr, -1, k); 

//	printf("----- arr[0] %d\n", arr[0]); 	 	
	
	int max_limit = sb.num_blocks ; 
	int i = 1 + sb.inode_map_sz + sb.block_map_sz + sb.inode_region_sz ; 

	int cntr = 0; 
	for ( ; i < max_limit ; i ++){
		
		if (cntr == k)
			break; 
		if( ! FD_ISSET(i,block_map)){
//			printf("------ cntr is %d, i is %d\n", cntr, i); 
			arr[cntr] = i; 
			cntr++; 	
		}
	}

	return arr; 

}


char * get_name(const char * path){

	printf("------ in get_name function, path is %s\n", path); 

	char * _path = strdup(path);
	
	const char delimiters[2] = "/"; 

	char * token = strtok(_path, delimiters);
	char * prev_token;  
 
	while (token != NULL){
		prev_token = token; 
		token = strtok(NULL, delimiters); 
	}
	
	//free(_path);
//	printf("------ prev %s \n", prev_token); 
	return prev_token; 
}

void fill_inode (int inode_index, int is_dir, mode_t mode, int block_num){

	
	printf("------ in fill_inode function\n");
	FD_SET(inode_index, inode_map );


	struct fuse_context *ctx = fuse_get_context();  

	inodes[inode_index].uid = ctx->uid; 
	inodes[inode_index].gid = ctx->gid; 
	inodes[inode_index].mode = mode; 
	inodes[inode_index].ctime = time(NULL); 
	inodes[inode_index].mtime = time(NULL); 
	inodes[inode_index].size = 0; 
 	memset(inodes[inode_index].direct, 0, sizeof(inodes[inode_index].direct)); 
	inodes[inode_index].indir_1 = 0; 
	inodes[inode_index].indir_2 = 0;
	
	if ( !is_dir ){
		inodes[inode_index].mode = ((mode &   01777) | S_IFREG);	
		
	}else{
		inodes[inode_index].mode = (mode | S_IFDIR); 
		//int datablk_num = get_empty_datablk_num();
		//printf("------ datablock num is %d\n", datablk_num);
		inodes[inode_index].direct[0] = block_num; 
		FD_SET(block_num, block_map);
		char * clean_block = malloc(FS_BLOCK_SIZE); 
		memset(clean_block, 0, FS_BLOCK_SIZE);
		disk->ops->write(disk, block_num, 1, clean_block);  
	}

	// free ctx; 	 
}

/* mknod - create a new file with permissions (mode & 01777)
 *
 * Errors - path resolution, EEXIST
 *          in particular, for mknod("/a/b/c") to succeed,
 *          "/a/b" must exist, and "/a/b/c" must not.
 *
 * If a file or directory of this name already exists, return -EEXIST.
 * If this would result in >32 entries in a directory, return -ENOSPC
 * if !S_ISREG(mode) return -EINVAL [i.e. 'mode' specifies a device special
 * file or other non-file object]
 */

static int fs_mknod(const char *path, mode_t mode, dev_t dev)
{
 
	printf("------ in fs_mknod function, path is %s\n", path); 
	int p_inode_index = get_parent_inode(path);
	
	
	printf("------ p_inode_index %d\n", p_inode_index);
	
	
	
	int entry_index = get_empty_entry(p_inode_index);
//	printf("------ entry_index is %d\n", entry_index); 
	
	/**/
	if (entry_index < 0)
		return -ENOSPC; 

//	if ( !S_ISREG(mode))i
//	printf("------ mode is reg?, %d\n", S_ISREG(mode));
//		return -EINVAL; 

        int inode_index = get_empty_inode_num();
	printf("------- inode_index is %d\n", inode_index); 
	if (inode_index < 0){

		return -ENOSPC; 
	}		

	int block_num = inodes[p_inode_index].direct[0];
	printf("------ block num is %d\n", block_num);

	struct fs7600_dirent * entries = malloc(FS_BLOCK_SIZE);

	if(disk->ops->read(disk,block_num, 1, entries ) < 0 )
		exit(0);
	
 	entries[entry_index].valid = 1; 
	entries[entry_index].isDir = 0; // is a file
	entries[entry_index].inode = inode_index; 
       
	char * f_name = get_name(path); 
	strcpy(entries[entry_index].name,  f_name);
//	printf("------ %s\n ",(entries[entry_index].name));

	uint32_t is_dir = 0 ;
//  	I ignored dev_t dev

	// 0 means no block need to be assigned to file  
	fill_inode(inode_index, is_dir,  mode, 0);
/**/ 
	// write this block 
	// write inode_bitmap  block
	// write inode block

/**/ 
	if (disk->ops->write(disk, 1, sb.inode_map_sz, inode_map) < 0)
		exit(0); 

	
	int inode_block_index = get_inode_block_index(inode_index);
	void * inode_block_data = get_inode_block_data(inode_index); 	

	update_disk_block(inode_block_index, 1, inode_block_data );	
	free(inode_block_data); 

	if (disk->ops->write(disk, 1 + sb.inode_map_sz, sb.block_map_sz, block_map) < 0)
		exit(0); 

//	if (disk->ops->write(disk, 1 + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, inodes) < 0)
//		exit(0);
/**/


	//update_disk(); 

	if (disk->ops->write(disk, block_num, 1, entries) < 0)
		exit(0); 

		
	free(entries); 
	//free(f_name)
	return 0;
	
}

/* mkdir - create a directory with the given mode.
 * Errors - path resolution, EEXIST
 * Conditions for EEXIST are the same as for create. 
 * If this would result in >32 entries in a directory, return -ENOSPC
 *
 * Note that you may want to combine the logic of fs_mknod and
 * fs_mkdir. 
 */ 
static int fs_mkdir(const char *path, mode_t mode)
{
	printf("------ in fs_mkdir function, path is %s\n", path); 
	int p_inode_index = get_parent_inode(path);
	
	
	printf("------ p_inode_index %d\n", p_inode_index);
	
	
	int entry_index = get_empty_entry(p_inode_index);
//	printf("------ entry_index is %d\n", entry_index); 
	
	
	if (entry_index < 0)
		return -ENOSPC; 

//	if ( !S_ISREG(mode))i
//	printf("------ mode is reg?, %d\n", S_ISREG(mode));
//		return -EINVAL; 

        int inode_index = get_empty_inode_num();
	printf("------- inode_index is %d\n", inode_index); 
	if (inode_index < 0){

		return -ENOSPC; 
	}		

	int dir_block_num = get_empty_datablk_num();
	if (dir_block_num < 0)
		return -ENOSPC; 
 

	int block_num = inodes[p_inode_index].direct[0];
	printf("------ block num is %d\n", block_num);


	struct fs7600_dirent * entries = malloc(FS_BLOCK_SIZE);

	if(disk->ops->read(disk,block_num, 1, entries ) < 0 )
		exit(0);

//	printf("------ name of file is %s\n", get_name(path)); 	
/**/	
 	entries[entry_index].valid = 1; 
	entries[entry_index].isDir = 1; // is a dir
	entries[entry_index].inode = inode_index; 
        strcpy(entries[entry_index].name,  get_name(path));
//	printf("------ %s\n ",(entries[entry_index].name));

	uint32_t is_dir = 1; 
	//struct dev_t devi; 
	fill_inode(inode_index, is_dir,  mode, dir_block_num);
/**/ 
	// write this block 
	// write inode_bitmap  block
	// write inode block
/**/ 
	if (disk->ops->write(disk, 1, sb.inode_map_sz, inode_map) < 0)
		exit(0); 

	if (disk->ops->write(disk, 1 + sb.inode_map_sz, sb.block_map_sz, block_map) < 0)
		exit(0);
	
	int inode_block_index = get_inode_block_index(inode_index);
	void * inode_block_data = get_inode_block_data(inode_index); 	

	update_disk_block(inode_block_index, 1, inode_block_data );	
	free(inode_block_data);  

//	if (disk->ops->write(disk, 1 + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, inodes) < 0)
//		exit(0);
/**/
//	update_disk();
 
	if (disk->ops->write(disk, block_num, 1, entries) < 0)
		exit(0); 

//	printf("------success\n");
	free(entries); 
	return 0;

	//return -EOPNOTSUPP;
}
//----------------------------------------------------------------------
/* truncate - truncate file to exactly 'len' bytes
 * Errors - path resolution, ENOENT, EISDIR, EINVAL
 *    return EINVAL if len > 0.
 */
static int fs_truncate(const char *path, off_t len)
{
	
	printf("------ in fs_truncate function, path is %s\n", path);
	/* you can cheat by only implementing this for the case of len==0,
	 * and an error otherwise.
	 */

	printf("------ len is %lld\n", len);
 
	if (len != 0)
		return -EINVAL;		/* invalid argument */


	char * _path = strdup(path); 
	int inode_index = translate(_path); 
	free(_path); 

	int size = inodes[inode_index].size; 
	printf("------ size is %d\n", size); 


	// this num of blocks is not index 
	int num_of_blocks = ( size ) / FS_BLOCK_SIZE;
	if ( size % FS_BLOCK_SIZE != 0){
		num_of_blocks ++ ; 
	}
	
	printf("------ num_of_blocks is %d\n", num_of_blocks); 

	int indirect_2_num_entries = FS_BLOCK_SIZE / sizeof(uint32_t); 
	int indirect_1_num_entries = FS_BLOCK_SIZE / sizeof(uint32_t); 
	// I expact to be 256; 
	//printf("------ num of direct_1_num_entries %d\n", indirect_1_num_entries ); 	

	int block_num;

 	int i; 
	int max_limit;  

	// directs -------------------------------------------------------------------- 
	max_limit = 6; 
	if (num_of_blocks < 6)
		max_limit = num_of_blocks; 

	printf("------max_limit is %d\n", max_limit); 

	
	printf("------ truncating directs\n");	

	for (i = 0 ; i < max_limit ; i++){
		block_num = inodes[inode_index].direct[i]; 	
		FD_CLR(block_num, block_map); 
		printf("------ free block num %d\n", block_num); 
	}

	// indirect 1 ---------------------------------------------------------------- 
	int is_indir1_valid = 0;
	if (num_of_blocks > 6) 
		is_indir1_valid = 1; 
	
	if (is_indir1_valid ){

		printf("------ truncating indirect 1\n");

		int indir1_block_num = inodes[inode_index].indir_1; 
		uint32_t * indir1_block = malloc(FS_BLOCK_SIZE); 
		
		disk->ops->read(disk, indir1_block_num , 1, indir1_block); 
		
			
		max_limit = indirect_1_num_entries + 6; 
		
		if (num_of_blocks < max_limit)
			max_limit = num_of_blocks; 

		for (i = 6 ; i < max_limit ; i++){
			int tmp_index = i - 6; 

			block_num = indir1_block[tmp_index];
			FD_CLR(block_num, block_map);   
			printf("------ free block num %d\n", block_num);
		}

		FD_CLR( indir1_block_num, block_map);	
		printf("------ free block num %d\n", indir1_block_num); 
 
		free(indir1_block);
	}


	// indirect 2 -----------------------------------------------------------------

	int is_indir2_valid = 0;
	 
	if (num_of_blocks > (6 + indirect_1_num_entries) ){
		is_indir2_valid = 1; 
	}

	if(is_indir2_valid){
	
		printf("------ truncating indirect 2\n");

		int indir2_block_num = inodes[inode_index].indir_2; 
		
		uint32_t * indir2_block = malloc(FS_BLOCK_SIZE); 
	 	disk->ops->read(disk, indir2_block_num, 1, indir2_block);
 	
		
		// error zone
		int last_filled_index = num_of_blocks - 1;
		int num_indir_entries = 256; 
		 
		int max_2 = (last_filled_index - (6 + num_indir_entries)) / num_indir_entries ; 
				
		int j ;
		int k ; 

		for (j = 0 ; j <=  max_2 ; j++ ){
			
			int indir1_block_num = indir2_block[j]; 
		
			uint32_t * indir1_block = malloc(FS_BLOCK_SIZE); 
		 	disk->ops->read(disk, indir1_block_num, 1, indir1_block);	

			int max_1 = num_indir_entries; 
			if ( j == max_2 )
				max_1 =   (last_filled_index - (6 + num_indir_entries)) % num_indir_entries;
 
			for(k = 0 ; k <= max_1 ; k++){
				block_num = indir1_block[k]; 
				FD_CLR(block_num, block_map); 	
				printf("------ free block num %d\n", block_num);
			}
		
			FD_CLR(indir1_block_num, block_map);
 
			free(indir1_block); 
			printf("------ free block num %d\n", indir1_block_num);
		}
	
		FD_CLR(indir2_block_num, block_map);
		free(indir2_block); 
		printf("------ free block num %d\n", indir2_block_num);	
	}

	inodes[inode_index].size = 0 ;  

	// update_disk();
	// I just need to write back block map and size of inode_index 

	if (disk->ops->write(disk, 1 + sb.inode_map_sz, sb.block_map_sz, block_map) < 0)
		exit(0); 

	int inode_block_index = get_inode_block_index(inode_index);
	void * inode_block_data = get_inode_block_data(inode_index); 	

	update_disk_block(inode_block_index, 1, inode_block_data );	
	free(inode_block_data); 
	
	printf("------ truncate return 0\n"); 
	return 0;   
	//return -EOPNOTSUPP;
}

/* unlink - delete a file
 *  Errors - path resolution, ENOENT, EISDIR
 * Note that you have to delete (i.e. truncate) all the data.
 */
static int fs_unlink(const char *path)
{
	printf("------ in fs_unlink function, path is %s\n", path);	

	char * _path = strdup(path); 
	int inode_index = translate(_path);
	free(_path); 

	if ( inodes[inode_index].size > 0 ){
	
		int success = fs_truncate(path, 0);
	
		if ( success <0)
			return success; 	
	 }

	// inode_map
	// parent 
	
	int p_inode_index = get_parent_inode(path); 
	int p_block_num = inodes[p_inode_index].direct[0];

	struct fs7600_dirent * entries = malloc(FS_BLOCK_SIZE);	
	if (disk->ops->read(disk, p_block_num, 1, entries ))
		exit(0);

	char * f_name = get_name(path);
	int index = find_index(f_name, entries); 
	entries[index].valid = 0; 

	FD_CLR(inode_index, inode_map);
	//FD_CLR(); there is no block for a 0 size file

	if (disk->ops->write(disk, p_block_num, 1, entries) < 0)
		exit(0); 	
 
	if (disk->ops->write(disk, 1, sb.inode_map_sz, inode_map) < 0)
		exit(0); 

	// update inode_map 
	int inode_block_index = get_inode_block_index(inode_index);
	void * inode_block_data = get_inode_block_data(inode_index); 	

	update_disk_block(inode_block_index, 1, inode_block_data );	
	free(inode_block_data); 
	//-------------------

	//if (disk->ops->write(disk, 1 + sb.inode_map_sz, sb.block_map_sz, block_map) < 0)
	//	exit(0); 
	//if (disk->ops->write(disk, 1 + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, inodes) < 0)
	//	exit(0);

	free(entries);
	// free(f_name);  
	if (homework_part > 2){
		cache_delete(inode_index); 
	}
	return 0; 	 
}
//---------------------------------------------------------------------------------------------
uint32_t is_dir_empty(int inode_index){
	
	printf("------ in is_dir_empty function, inode_index is %d\n",inode_index); 	
	int block_num = inodes[inode_index].direct[0]; 

	struct fs7600_dirent * entries = malloc(FS_BLOCK_SIZE);	
	if (disk->ops->read(disk, block_num, 1, entries ))
		exit(0); 

	int i ; 
	for (i = 0 ; i < 32 ; i++){
		if (entries[i].valid)
			return 0; 
	}
		
	return 1; 
}
/* rmdir - remove a directory
 *  Errors - path resolution, ENOENT, ENOTDIR, ENOTEMPTY
 */
static int fs_rmdir(const char *path)
{
	printf("------ fs_in rmdir function, path is %s\n", path);

	char * _path = strdup(path); 
	int inode_index = translate(_path);
	free(_path);  
	
	if ( ! is_dir_empty(inode_index)){
		printf("------ dir is not empty, inode index is, %d\n", inode_index); 
		return -ENOTEMPTY; 
	}
 	
	printf("------ dir is empty\n"); 	
 
	int p_index = get_parent_inode(path);  
 
	int block_num_p = inodes[p_index].direct[0]; 

	struct fs7600_dirent * entries = malloc(FS_BLOCK_SIZE);	
	if (disk->ops->read(disk, block_num_p, 1, entries ))
		exit(0); 

	char * name = get_name(path); 	
	int index = find_index(name, entries); 
	entries[index].valid = 0; 

	int block_num = inodes[inode_index].direct[0]; 
	FD_CLR(inode_index, inode_map);
	FD_CLR(block_num,block_map);  
	
	/**/	 	
	if (disk->ops->write(disk, 1, sb.inode_map_sz, inode_map) < 0)
		exit(0); 
	if (disk->ops->write(disk, 1 + sb.inode_map_sz, sb.block_map_sz, block_map) < 0)
		exit(0); 
	
	int inode_block_index = get_inode_block_index(inode_index);
	void * inode_block_data = get_inode_block_data(inode_index); 	

	update_disk_block(inode_block_index, 1, inode_block_data );	
	free(inode_block_data); 

//	if (disk->ops->write(disk, 1 + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, inodes) < 0)
//		exit(0);

	/**/
//	update_disk(); 
	
	if (disk->ops->write(disk, block_num_p, 1, entries) < 0)
		exit(0); 

	free(entries); 	

	//free(name);
	if (homework_part > 2){
		cache_delete(inode_index);
	}
 	 
	return 0; 
	
}

/* rename - rename a file or directory
 * Errors - path resolution, ENOENT, EINVAL, EEXIST
 *
 * ENOENT - source does not exist
 * EEXIST - destination already exists
 * EINVAL - source and destination are not in the same directory
 *
 * Note that this is a simplified version of the UNIX rename
 * functionality - see 'man 2 rename' for full semantics. In
 * particular, the full version can move across directories, replace a
 * destination file, and replace an empty directory with a full one.
 */
static int fs_rename(const char *src_path, const char *dst_path)
{
	printf("------ in fs_rename function, source is %s, dest is %s \n", src_path, dst_path);		
	int dst_parent_inode = get_parent_inode(dst_path);
	int src_parent_inode = get_parent_inode(src_path);

	if (dst_parent_inode != src_parent_inode)
		return -EINVAL;  

	char * new_name  = get_name(dst_path); 
	char * pre_name  = get_name(src_path); 

		
	int block_num = inodes[dst_parent_inode].direct[0];
	struct fs7600_dirent * entries = malloc(FS_BLOCK_SIZE);

	if(disk->ops->read(disk,block_num, 1, entries ) < 0 )
		exit(0);


	int check_exist = find_index(new_name, entries);

	if (check_exist >= 0) {
		// free(new_name);
		// free(pre_name); 

		free(entries); 
		return -EEXIST;
	} 	

	int index = find_index(pre_name, entries ); 
	strcpy (entries[index].name, new_name); 

	if(disk->ops->write(disk,block_num, 1, entries ) < 0 )
		exit(0);

	
		
	free(entries);

	// free(new_name);
	// free(pre_name); 

	if (homework_part > 2){	
		int inode_index = cache_get_inode(src_path); 
		if (inode_index > 0)
			cache_delete(inode_index);  
	}
	
	return 0;  

}

/* chmod - change file permissions
 * utime - change access and modification times
 *         (for definition of 'struct utimebuf', see 'man utime')
 *
 * Errors - path resolution, ENOENT.
 */
static int fs_chmod(const char *path, mode_t mode)
{
	printf("------ in fs_chmod function, path is %s, mode is %d\n",path, mode );

	char * _path = strdup(path); 
	int inode_index = translate(_path);
	free(_path); 

	inodes[inode_index].mode = mode; 	

	int inode_block_index = get_inode_block_index(inode_index);
	void * inode_block_data = get_inode_block_data(inode_index); 	

	update_disk_block(inode_block_index, 1, inode_block_data );  	
	free(inode_block_data); 
//	if (disk->ops->write(disk, 1 + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, inodes) < 0)
//		exit(0);
	
	return 0; 
       
}
int fs_utime(const char *path, struct utimbuf *ut)
{
	printf("------ in fs_utime function, path is %s, ut is %ld\n", path, ut->actime); 

	char * _path = strdup(path); 
	int inode_index = translate(_path);
	free(_path);  
	
	//inodes[inode_index].ctime = ut->actime; 	
	inodes[inode_index].mtime = ut->modtime; 

	int inode_block_index = get_inode_block_index(inode_index);
	void * inode_block_data = get_inode_block_data(inode_index); 	

	update_disk_block(inode_block_index, 1, inode_block_data );	
	free(inode_block_data); 
	//if (disk->ops->write(disk, 1 + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, inodes) < 0)
	//	exit(0);
	
	return 0; 

	//return -EOPNOTSUPP;
}
//------------------------------------------------------------------------------------------
int read_by_block(int inode_index, int ith_block, char * buf, uint32_t offset, uint32_t len){
	 
	printf("------ in read_by_block function, inode_index is %d, ith_block is %d\n", inode_index, ith_block); 
	
	if ( offset + len > FS_BLOCK_SIZE )
	{
		printf("------ read_by_block, this is impossible\n"); 
		exit(0); 
	}  
		 
	if( ith_block >= 0 && ith_block < 6){
		printf("------ read direct part\n"); 	
		int data_block_num = inodes[inode_index].direct[ith_block]; 
			
		char* data_block = malloc(FS_BLOCK_SIZE);

		if(disk->ops->read(disk, data_block_num, 1, data_block ) < 0 )
					exit(0);
 
		
		memcpy(buf, (data_block + offset) , len); 
		
		free(data_block); 

		return len; 
 
	}

	int num_ptr = FS_BLOCK_SIZE / sizeof(uint32_t);// num_ptr --> 256
//	printf("------ num of ptr is %d\n", num_ptr);
	
	int max_limit = 6 + num_ptr;
//	printf("------ max limit is %d\n", max_limit);  

	if( ith_block >= 6 && ith_block < max_limit ){
		
		printf("------ read indirect 1 part\n");
		ith_block = ith_block - 6; 

		int indir1_block_num = inodes[inode_index].indir_1; 
			
		uint32_t * indir1_block = malloc(FS_BLOCK_SIZE);

		if(disk->ops->read(disk, indir1_block_num, 1, indir1_block ) < 0 )
			exit(0); 

		int data_block_num = indir1_block[ith_block]; 
	
		char * data_block = malloc(FS_BLOCK_SIZE);

 		if(disk->ops->read(disk, data_block_num, 1, data_block ) < 0 )
			exit(0);
 
	
		memcpy(buf, (data_block + offset) , len); 
		
		free(indir1_block); 
		free(data_block);	

		return len; 

	}

	int min_limit = max_limit; // = 6 + 256  
	num_ptr = num_ptr * num_ptr; // 256 * 256 
	max_limit = min_limit + num_ptr; 

	//printf("------ min limit is %d\n", min_limit); 
	//printf("------ num_ptr is %d \n", num_ptr); 
	//printf("------ max_limit is %d\n", max_limit); 

	if( ith_block >= min_limit && ith_block < max_limit){
	
		printf("------ read indirect 2 part\n");	
		ith_block = ith_block - min_limit; 

		int indir_2_ptr = ith_block / 256; 
		int indir_1_ptr = ith_block % 256; 

		
		int indir2_block_num = inodes[inode_index].indir_2;

		uint32_t *  indir2_block = malloc(FS_BLOCK_SIZE) ;

		if(disk->ops->read(disk, indir2_block_num, 1, indir2_block ) < 0 )
			exit(0);
 


		int indir1_block_num = indir2_block[indir_2_ptr]; 
 
 		uint32_t *  indir1_block = malloc(FS_BLOCK_SIZE) ;

		if(disk->ops->read(disk, indir1_block_num, 1, indir1_block ) < 0 )
			exit(0);

 

		int data_block_num = indir1_block[indir_1_ptr]; 
	
		char * data_block =  malloc(FS_BLOCK_SIZE) ;
		
		if(disk->ops->read(disk, data_block_num, 1, data_block ) < 0 )
			exit(0);
 
		
		memcpy(buf, (data_block + offset), len); 


		free(indir2_block); 
		free(indir1_block); 
		free(data_block); 
		return len ; 
	}

	// ith_block is less than 6 + 32 + 256 = 294; 	
	printf("------ ith_block is out of range, %d\n", ith_block); 
	
}
//-------------------------------------------------------------------------------
/* read - read data from an open file.
 * should return exactly the number of bytes requested, except:
 *   - if offset >= file len, return 0
 *   - if offset+len > file len, return bytes from offset to EOF
 *   - on error, return <0
 * Errors - path resolution, ENOENT, EISDIR
 */
static int fs_read(const char *path, char *buf, size_t len, off_t offset,
		struct fuse_file_info *fi)
{

	uint32_t bytes_read = 0; 

	printf("------ in fs_read function, path is %s\n", path);

	printf("------ offset is %lld\n", offset);
	printf("------ len %d \n", len);  
 
 		
	int inode_index; 
	if (homework_part >= 2 ){
		inode_index = fi->fh; 
	}
	else {
		char * _path = strdup(path);
        	inode_index =  translate(_path);
		free(_path); 
	}		
 
	uint32_t f_size = inodes[inode_index].size; 
 	printf("------ file size is %d\n", f_size);
	

	if (offset >= f_size)
		return 0; 

	printf("------ offset is less than size\n"); 
	
	uint32_t  max_limit = offset + len;
	if (max_limit > f_size)
		max_limit = f_size;  	
 	 
 	printf("------ offset + len is %lld\n", (offset+len));
	printf("------ max_limit is %d\n", max_limit); 	

	
	int first_block_index = offset / FS_BLOCK_SIZE; 
	int last_block_index = max_limit / FS_BLOCK_SIZE ; // i should not read this block // i should stop at this blcck
	if (max_limit % FS_BLOCK_SIZE != 0)	
		last_block_index ++; 
	
	printf("------ in my logic, we don't read last block\n");
	printf("------ first_block_index is: %d\n", first_block_index);
	printf("------ last_block_index is: %d\n", last_block_index);   
	

	int num_block_to_read = last_block_index - first_block_index; 
	printf("------ num of block to read is %d\n", num_block_to_read);  

	int byte_has_read = 0;
 
	if (num_block_to_read == 1){

		int offset_one_block = offset % FS_BLOCK_SIZE; 
		int len_one_block = max_limit - offset ; //  or (max_limit - offset) % FS_BLOCK_SIZE // no need

		printf("------ offset_one_block is %d\n", offset_one_block); 
		printf("------ len_one_block is %d\n", len_one_block);   
		
		byte_has_read += read_by_block(inode_index, first_block_index, buf, offset_one_block , len_one_block ); 
	
		printf("------ byte_has_read is %d \n", byte_has_read);

		return byte_has_read; 
	}

/**/
	// num_block_to_read > 1
	int first_offset = offset %  FS_BLOCK_SIZE;  
	int first_len = FS_BLOCK_SIZE - first_offset;

	printf("------ first_offset is %d\n", first_offset); 
	printf("------ first_len is %d\n", first_len); 
	  
	byte_has_read += read_by_block(inode_index, first_block_index, buf, first_offset , first_len );
	
	printf("------ byte_has_read is %d \n", byte_has_read);
	
	int i; 
	for (i = first_block_index + 1 ; i < last_block_index -1; i++) {
		byte_has_read += read_by_block(inode_index, i, (buf + byte_has_read) , 0 ,FS_BLOCK_SIZE ); 
	}	

	int last_len;  
	if ((max_limit % FS_BLOCK_SIZE) == 0)
		last_len = FS_BLOCK_SIZE; 
	else{ 
		last_len = (max_limit ) % FS_BLOCK_SIZE; // this should not be max_limit - offset! 
	}
	 
	byte_has_read += read_by_block(inode_index, last_block_index -1, (buf + byte_has_read), 0 , last_len ); 

	printf("------ last_len is %d\n", last_len); 
	printf("------ byte_has_read is %d\n", byte_has_read); 


	return byte_has_read; 	
   	
}
/**
int is_data_block_empty(int inode_index, int ith_block){
	int f_size = inodes[inode_index].size; 
	int max_filled_block = (f_size -1) / FS_BLOCK_SIZE;
	
	if (max_filled_block < ith_block)
		return 1; 
	return 0;  
}

int is_indir1_block_empty(int inode_index, int ith_block){
	int f_size = inodes[inode_index].size; 
	int max_filled_block = (f_size -1)/FS_BLOCK_SIZE; 
	
	if(max_filled_block <6)	
		return 1; 
	return 0; 
}
/**/
/**
int * get_refrences(int inode_index, int ith_block){
	int f_size = inodes[inode_index].size; 
	int max_filled_block = (f_size -1) / FS_BLOCK_SIZE;
 
	 
	if (ith_block >= 0 && ith_block < 6){
		if (max_filled_block < ith_block); 
			return 	get_k_empty_datablk_num(1); 
	
		int * arr = malloc(sizeof(int)); 	
		arr[0] = (int)inodes[inode_index].direct[ith_block];
		return arr;  	
		 
	}else if(ith_block >= 6 &&  ith_block < 6 + 32){
		if(ith_block == 6 && max_filled_block < 6){	
			return get_k_empty_datablk_num(2);

		}else if (max_filled_block < ith_block){
			int * arr = malloc (sizeof(int)*2);
			int * tmp ; 
			arr[0] = inodes[inode_index].indir_1;
			tmp = get_k_empty_datablk_num(1);
			arr[1] = tmp[0];   
			free(tmp); 
			return arr; 
		}
		
		int * arr = malloc (sizeof(int)*2); 
		arr[0] = inodes[inode_index].indir_1; 
		uint32_t *  blk = malloc(FS_BLOCK_SIZE); 
		
		disk->ops->read(disk,arr[0], 1, blk ); 
		int index = ith_block - 6; 
			
		arr[1] = blk[index]; 

	}else if (ith_block >= 6 + 32 && ith_block < 6 + 32 + 256){
		if()	
	
	}else{
		printf("------ get_refrences impossible case\n");
		exit(0); 
	}
	return NULL; 
}/**/

int get_data_block_num_0(int inode_index, int ith_block){

	printf("------ in get_data_block_num_0 function, inode_index is %d, ith_block is %d\n", inode_index, ith_block); 
	// free arr
	int f_size = inodes[inode_index].size;
	
	// this is the index of last filled block, it could be zero  
	int max_filled_block; 
	
	if (f_size > 0)
		max_filled_block =  (f_size -1)/FS_BLOCK_SIZE;
	else 
		max_filled_block = -1; 
	

	if (max_filled_block < ith_block){
		int * arr = get_k_empty_datablk_num(1);
		//printf("------ after get_k\n"); 
		if (arr[0] == -1){

			free(arr); 
			return -1; 

		}
	
		int data_block_num = arr[0];
 
		FD_SET(data_block_num, block_map); 
		inodes[inode_index].direct[ith_block] = data_block_num;
		//printf("------before \n"); 
		free(arr); 
	
	}


	//else{
		return 	inodes[inode_index].direct[ith_block];
	//}	

}

int get_data_block_num_1(int inode_index, int ith_block){

	printf("------ in get_data_block_num_1 function, inode_index is %d, ith_block is %d\n", inode_index, ith_block); 
	//free arr
	int f_size = inodes[inode_index].size;
	// in this point f_size > 0

	// this shows the index can be zero 
	int max_filled_block = (f_size - 1)/FS_BLOCK_SIZE; 
	
	int indir_1; 
	int data_block_num;

	int indir_1_ptr = ith_block - 6;  
	
	if (max_filled_block < 6){
		int * arr = get_k_empty_datablk_num(2); 
		
		if (arr[0] == -1 || arr[1] == -1){
			free(arr); 
			return -1; 
		}
			
//	printf("_______ fill_inode, is set %d, size of direct %d\n", FD_ISSET(inode_index, inode_map),  sizeof(inodes[inode_index].direct));
	
		indir_1 = arr[0]; 
		data_block_num = arr[1]; 

		FD_SET(indir_1, block_map); 
		FD_SET(data_block_num, block_map);
			
		inodes[inode_index].indir_1 = indir_1; 

		uint32_t * indir1_block = malloc(FS_BLOCK_SIZE);
		indir1_block[indir_1_ptr] = data_block_num; 

		disk->ops->write(disk, indir_1, 1, indir1_block);

		free(indir1_block);  
		free(arr); 	
		
	}else{
		if (max_filled_block < ith_block){
			int * arr = get_k_empty_datablk_num(1); 
			
			if (arr[0] == -1){
				free(arr); 	
				return -1;
			} 

			data_block_num = arr[0];
 		
			FD_SET(data_block_num, block_map);
		
			indir_1 = inodes[inode_index].indir_1;
			
		
			uint32_t * indir1_block = malloc(FS_BLOCK_SIZE);
			disk->ops->read(disk, indir_1, 1,  indir1_block); 
			indir1_block[indir_1_ptr] = data_block_num; 

			disk->ops->write(disk, indir_1, 1, indir1_block); 	

			free(indir1_block);
			free(arr);   
		}else{

			indir_1 = inodes[inode_index].indir_1; 
		
			uint32_t * indir1_block = malloc(FS_BLOCK_SIZE);
			disk->ops->read(disk, indir_1, 1, indir1_block);
 
			data_block_num = indir1_block[indir_1_ptr]; 
			free(indir1_block);  
		}

	}

	return data_block_num; 	
	
}

int get_data_block_num_2(int inode_index, int ith_block){

	printf("------ in get_data_block_num_2 function, inode_index is %d, ith_block is %d\n", inode_index, ith_block); 	
	int f_size = inodes[inode_index].size;
	// f_size > 0 here  
	int last_filled_block = (f_size -1 )/FS_BLOCK_SIZE; 
	
	// we know that ith block is at least 6 + 256
	int indir_2_ptr = (ith_block - (6 + 256)) / 256; 
	int indir_1_ptr = (ith_block - (6 + 256)) % 256; 




	

	int indir2;
	int indir1;  
	
	int data_block_num;

	// it does not contain <= because it is index
	if (last_filled_block < 6 + 256){
		int * arr = get_k_empty_datablk_num(3); 
		
		if (arr[0] == -1 || arr[1] == -1 || arr[2] == -1){
			free(arr);
			return -1; 
		}

		indir2 = arr[0]; 
		indir1 = arr[1]; 
		data_block_num = arr[2];

		free(arr); 

		FD_SET( indir2, block_map);  
		FD_SET( indir1, block_map);
		FD_SET( data_block_num, block_map); 

		inodes[inode_index].indir_2 = indir2; 
		
		uint32_t * indir2_block = malloc(FS_BLOCK_SIZE);  
		indir2_block[indir_2_ptr] = indir1; 
		disk->ops->write(disk, indir2, 1, indir2_block); 

		uint32_t * indir1_block = malloc(FS_BLOCK_SIZE); 
		indir1_block[indir_1_ptr] = data_block_num; 
		disk->ops->write(disk, indir1, 1, indir1_block); 
		
		free(indir2_block); 
		free(indir1_block); 
	
	}else{
		int filled_indir2_ptr; 
		int filled_indir1_ptr; 

		if ( last_filled_block < 6 + 256){
			filled_indir2_ptr = -1; 
			filled_indir1_ptr = -1; 
					
		}else{
			filled_indir2_ptr = (last_filled_block - (6 + 256)) / 256; 
			filled_indir1_ptr = (last_filled_block - (6 + 256)) % 256; 
		}
	
	
		if (filled_indir2_ptr < indir_2_ptr){
			indir2 = inodes[inode_index].indir_2; 
	
			int * arr = get_k_empty_datablk_num(2); 
			if (arr[0] == -1 || arr[1] == -1){
				free(arr); 
				return -1; 
			}
			indir1 = arr[0]; 
			data_block_num = arr[1]; 
	
			free(arr); 

			FD_SET( indir1, block_map);
			FD_SET( data_block_num, block_map); 

			uint32_t * indir2_block = malloc(FS_BLOCK_SIZE);
			disk->ops->read(disk, indir2, 1, indir2_block);
			indir2_block[indir_2_ptr] = indir1;
			disk->ops->write(disk, indir2, 1, indir2_block);
 

			// no need to read 
			uint32_t * indir1_block = malloc(FS_BLOCK_SIZE); 
			indir1_block[indir_1_ptr] = data_block_num; 
			disk->ops->write(disk, indir1, 1, indir1_block); 
		
			free(indir2_block); 
			free(indir1_block); 
	

		}else{
			if (filled_indir1_ptr < indir_1_ptr){
				
				int * arr = get_k_empty_datablk_num(1); 
				if (arr[0] == -1){
					free(arr); 
					return -1;
				} 
						
				data_block_num = arr[0]; 
				free(arr); 	
				FD_SET( data_block_num, block_map);
				
				indir2 = inodes[inode_index].indir_2; 
				
				uint32_t * indir2_block = malloc(FS_BLOCK_SIZE);
				disk->ops->read(disk, indir2, 1, indir2_block);
			
				indir1 = indir2_block[indir_2_ptr]; 

				uint32_t * indir1_block = malloc(FS_BLOCK_SIZE); 
				disk->ops->read(disk, indir1, 1, indir1_block);
 				indir1_block[indir_1_ptr] = data_block_num;
				disk->ops->write(disk, indir1, 1, indir1_block); 

				free(indir2_block); 
				free(indir1_block); 				
	 				

			}else{
				indir2 = inodes[inode_index].indir_2; 
				
				uint32_t * indir2_block = malloc(FS_BLOCK_SIZE);
				disk->ops->read(disk, indir2, 1, indir2_block);
			
				indir1 = indir2_block[indir_2_ptr]; 

				uint32_t * indir1_block = malloc(FS_BLOCK_SIZE); 
				disk->ops->read(disk, indir1, 1, indir1_block);
 
				data_block_num = indir1_block[indir_1_ptr]; 	

				free(indir2_block); 
				free(indir1_block); 
			}
		}	
	}	
	
	return data_block_num; 

	// FDSET	
}
// -------------------------------------------------------------------------------------------------
int write_by_block(int inode_index, int ith_block, const char * buf, uint32_t offset, uint32_t len){
		
	printf("------ in write_by_block function, inode_index is %d, ith_block is %d\n", inode_index, ith_block); 
	printf("------ offsest is %d, len %d\n", offset , len); 
	
	int data_block_num;   
	
 
	if (ith_block >= 0 && ith_block < 6){
		
//		printf("----- info1\n"); 
		data_block_num = get_data_block_num_0(inode_index, ith_block);
//		printf("----- info2\n");  

	}else if(ith_block >= 6 &&  ith_block < 6 + 256){

		data_block_num = get_data_block_num_1(inode_index, ith_block);

	}else if (ith_block >= 6 + 256 && ith_block < 6 + 256 + 256 * 256){
		
		data_block_num = get_data_block_num_2(inode_index, ith_block);
				
	}else{ 
		printf("------ such a big file, ith_block is %d, and is out of range!\n", ith_block ); 
		return 0; 
	}

	if (data_block_num == -1)
		return 0; 

	//printf("------right before malloc\n");  
	char* data_block =  malloc(FS_BLOCK_SIZE); 	 
//	printf("------right after malloc\n"); 	
//	printf("offset and len %d , %d \n", offset, len); 

	disk->ops->read(disk, data_block_num, 1, data_block); 		
	memcpy(data_block + offset, buf, len ); 	
	disk->ops->write(disk, data_block_num, 1, data_block); 

	free(data_block);
  
	// inodes[inode_index]. size ?
	
	return len; 
}


/* write - write data to a file
 * It should return exactly the number of bytes requested, except on
 * error.
 * Errors - path resolution, ENOENT, EISDIR
 *  return EINVAL if 'offset' is greater than current file length.
 *  (POSIX semantics support the creation of files with "holes" in them, 
 *   but we don't)
 */
static int fs_write(const char *path, const char *buf, size_t len,
		off_t offset, struct fuse_file_info *fi)
{
	printf("------ in fs_write function, path is %s\n", path); 
	printf("------ offset is %lld\n", offset); 
	printf("------ len is %d\n", len);
		
	 		
	int inode_index; 
	if (homework_part >= 2 ){
		inode_index = fi->fh; 
	}
	else {
		char * _path = strdup(path);
        	inode_index =  translate(_path);
		free(_path); 
	} 

	int f_size = inodes[inode_index].size;

	if (offset > f_size)
		return -EINVAL; 

	//return 0; 

	int first_block_index = offset / FS_BLOCK_SIZE;
	
	// we dont read this block, we should stop before this  
	int last_block_index = (offset + len) / FS_BLOCK_SIZE;  

	if ((offset + len ) % FS_BLOCK_SIZE)	 		
 		last_block_index ++; 

	int num_block_to_write = last_block_index - first_block_index; 
	printf("------ num_block_to_write is %d\n", num_block_to_write); 
	
	int  byte_has_written = 0; 
	int w_len = 0; //written per block;  
/**/
        if (num_block_to_write == 1){
		int one_offset = offset % FS_BLOCK_SIZE ; 
		int one_len = len; 
		
		w_len = write_by_block(inode_index, first_block_index, buf, one_offset, one_len );
		byte_has_written = w_len; 
		
		printf("------ w_len is %d\n", w_len);
 	
		if (w_len == 0 ){
			update_disk(inode_index); 
			return byte_has_written; 
		}
	
		// updata file size		
		if (offset + len > inodes[inode_index].size ){ 
			int size_increase = offset + len -  inodes[inode_index].size; 
			inodes[inode_index].size += size_increase; 	
		}
	}else{

		int first_offset = offset % FS_BLOCK_SIZE; 
		int first_len = FS_BLOCK_SIZE - first_offset; 

		w_len  = write_by_block(inode_index, first_block_index, (buf + byte_has_written), first_offset ,first_len ); 
		byte_has_written += w_len; 

		if (w_len == 0 ){
			update_disk(inode_index); 
			return byte_has_written; 
		}

		//update file size
/**/
		int temp = (first_block_index - 1 ) * FS_BLOCK_SIZE + first_offset + first_len; 

		if (temp > inodes[inode_index].size ){ 
			int size_increase = temp  -  inodes[inode_index].size; 
			inodes[inode_index].size += size_increase; 	
		}
/**/

		int i = first_block_index + 1;
 
		for (i ; i < last_block_index -1; i++){

			w_len= write_by_block(inode_index, i, (buf + byte_has_written), 0 ,FS_BLOCK_SIZE );
			byte_has_written += w_len; 

			if (w_len == 0 ){
				update_disk(inode_index); 
				return byte_has_written; 
			}
			// update file size

			int temp = (i+1) * FS_BLOCK_SIZE; 
			
			if (temp > inodes[inode_index].size ){ 
				int size_increase = temp  -  inodes[inode_index].size; 
				inodes[inode_index].size += size_increase; 	
			}	


		}	
	
		int last_len; 
		if (  ((offset + len ) % FS_BLOCK_SIZE) == 0)
			last_len = FS_BLOCK_SIZE; 
		else{

			last_len = (offset + len ) % FS_BLOCK_SIZE; 
		}
		
		printf("------ last len is %d \n", last_len);
	 
		w_len  = write_by_block(inode_index, last_block_index -1,  (buf + byte_has_written), 0, last_len );
		byte_has_written += w_len; 

		printf("------ w len is %d \n", w_len); 
		if (w_len == 0 ){
			update_disk(inode_index); 	
			return byte_has_written; 
		}
		// update file size

		temp = (last_block_index -1) * FS_BLOCK_SIZE + last_len ; 
		// not sure this is true temp =  (num_blocks_to_write -1)* FS_BLOCK_SIZE + last_len; 


		if (temp > inodes[inode_index].size ){ 
				int size_increase = temp  -  inodes[inode_index].size; 
				inodes[inode_index].size += size_increase; 	
		}
	}

 /**/
	update_disk(inode_index); 
	return byte_has_written; 
//	return -EOPNOTSUPP;
}


static int fs_open(const char *path, struct fuse_file_info *fi)
{
	printf("------ in fs_open function, path is %s\n", path); 
/**/
	if(homework_part >= 2){
		printf("------ open, part2\n"); 	
	
		char * _path = strdup(path);
		int inode_index = translate(_path);
		 free(_path);

		fi->fh = inode_index; 
	}

	/**	
	if (homework_part == 3){
		printf("------ open, part3\n"); 	
	
		char * _path = strdup(path);
		int inode_index = translate(_path);
		free(_path);
	
		fi->fh = inode_index; 

	}
	/**/
/**/
	return 0;

}

static int fs_release(const char *path, struct fuse_file_info *fi)
{
	printf("------ in fs_release function, path is %s\n", path);
	return 0;
}

/* statfs - get file system statistics
 * see 'man 2 statfs' for description of 'struct statvfs'.
 * Errors - none. 
 */
static int fs_statfs(const char *path, struct statvfs *st)
{
	/* needs to return the following fields (set others to zero):
	 *   f_bsize = BLOCK_SIZE
	 *   f_blocks = total image - metadata
	 *   f_bfree = f_blocks - blocks used
	 *   f_bavail = f_bfree
	 *   f_namelen = <whatever your max namelength is>
	 *
	 * this should work fine, but you may want to add code to
	 * calculate the correct values later.
	 */
	
	printf("------ fs_statfs function, path is %s \n", path);
	st->f_bsize = FS_BLOCK_SIZE;
	st->f_blocks = 0;           /* probably want to */
	st->f_bfree = 0;            /* change these */
	st->f_bavail = 0;           /* values */
	st->f_namemax = 27;

	return 0;
}

/* operations vector. Please don't rename it, as the skeleton code in
 * misc.c assumes it is named 'fs_ops'.
 */
struct fuse_operations fs_ops = {
	.init = fs_init,
	.getattr = fs_getattr,
	.opendir = fs_opendir,
	.readdir = fs_readdir,
	.releasedir = fs_releasedir,
	.mknod = fs_mknod,
	.mkdir = fs_mkdir,
	.unlink = fs_unlink,
	.rmdir = fs_rmdir,
	.rename = fs_rename,
	.chmod = fs_chmod,
	.utime = fs_utime,
	.truncate = fs_truncate,
	.open = fs_open,
	.read = fs_read,
	.write = fs_write,
	.release = fs_release,
	.statfs = fs_statfs,
};

