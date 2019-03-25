#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define O_RD 0xf000
#define O_WR 0x0f00
#define D_TY 0x00f0
#define F_TY 0x000f

typedef unsigned int uint;
typedef unsigned short ushort;
typedef struct dir_node dir_node;
typedef struct i_node i_node;
typedef struct head_node head_node;
typedef struct i_free_node i_free_node;
typedef struct d_free_node d_free_node;

struct dir_node //32B
{
	uint i_node;
	char file_name[28];
};

struct i_node //64B
{
	ushort type;
	ushort owner;
	uint size;
	time_t ctime;
	uint addr[12];
};

struct head_node //64B
{
	ushort usr_num;
	ushort usr_id[23];
	uint i_free_num;
	uint i_first_free;
	uint d_free_num;
	uint d_first_free;
};

struct i_free_node //64B
{
	uint free_num;
	uint free_nodes[15];
};

struct d_free_node //512B
{
	uint free_num;
	uint free_nodes[127];
};

FILE *fr;
ushort usr_id;		 //当前登录用户
ushort path_len;	 //路径长度
char path[1024][28]; //当前路径
uint now_dr_id;		 //当前文件夹i节点号

int init(char *file_name, int size, int i_num);
FILE *log_in(char *file_name, ushort id);
int sig(char *file_name, ushort id);
void ll();
void cd(char *path_cd);
void mkdir(char *dir_name);
void chmod(char *type, char *file_name);
void touch(char *file_name);
void rm(char *file_name);
void gedit(char *file_name);
void format_dir();
uint search_file(uint dr_i_id, char *file_name, int ret_type);
uint new_file(uint dr_i_id, char *file_name, ushort type);
uint del_file(uint dr_i_id, char *file_name);
uint del_d(uint dr_i_id);
int iread(uint i_id, uint offset, void *buf, uint size);
int iwrite(uint i_id, uint offset, void *buf, uint size);
void i_clt_free(uint node_id);
uint i_out_free();
void d_clt_free(uint node_id);
uint d_out_free();
uint get_block_id(uint i_id, uint block_num);
uint make_block(uint i_id, uint block_num);
void clt_block(i_node *i_nd, uint block_num);
