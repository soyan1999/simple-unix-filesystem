#include "filesys.h"

int init(char *file_name, int size, int i_num)
{
	fr = fopen(file_name, "wb+");
	if (fr == NULL)
		return 0;
	//初始化文件
	char buf[1024];
	for (int i = 0; i < size; i++)
		fwrite(buf, 1024, 1, fr);

	head_node head;
	i_node first_node;
	i_free_node i_fr_nd;
	d_free_node d_fr_nd;
	dir_node root_dir[2];
	//初始化头节点
	head.usr_num = 0;
	head.i_first_free = 2;
	head.i_free_num = 0;
	head.d_first_free = i_num / 8;
	head.d_free_num = 0;
	fseek(fr, 0, SEEK_SET);
	fwrite(&head, sizeof(head_node), 1, fr);
	//初始化首空闲i节点
	i_fr_nd.free_num = 1;
	fseek(fr, 128, SEEK_SET);
	fwrite(&i_fr_nd, sizeof(i_free_node), 1, fr);
	//初始化空闲i节点管理
	for (int i = 3; i < i_num; i++)
		i_clt_free(i);
	//初始化首空闲块
	d_fr_nd.free_num = 1;
	fseek(fr, head.d_first_free * 512, SEEK_SET);
	fwrite(&d_fr_nd, sizeof(d_free_node), 1, fr);
	//初始化空闲块管理
	for (int i = i_num / 8 + 1; i < size * 2; i++)
		d_clt_free(i);
	//初始化根目录i节点
	first_node.ctime = time(NULL);
	first_node.owner = 0;
	first_node.type = O_RD | D_TY;
	first_node.size = 2 * sizeof(dir_node);
	first_node.addr[0] = d_out_free();
	fseek(fr, 64, SEEK_SET);
	fwrite(&first_node, sizeof(i_node), 1, fr);
	//初始化根目录项
	strcpy(root_dir[0].file_name, ".");
	strcpy(root_dir[1].file_name, "..");
	root_dir[0].i_node = 1;
	root_dir[1].i_node = 1;
	fseek(fr, first_node.addr[0] * 512, SEEK_SET);
	fwrite(root_dir, 2 * sizeof(dir_node), 1, fr);

	fclose(fr);
	return 1;
}

FILE *log_in(char *file_name, ushort id)
{
	fr = fopen(file_name, "rwb+");
	if (fr == NULL)
		return NULL;
	//root
	if (id == 0)
	{
		usr_id = 0;
		path_len = 0;
		return fr;
	}

	//查看账号是否存在
	head_node head;
	fread(&head, sizeof(head_node), 1, fr);
	for (int i = 0; i < head.usr_num; i++)
	{
		if (id == head.usr_id[i])
		{
			usr_id = id;
			path_len = 0;
			return fr;
		}
	}
	return NULL;
}

int sig(char *file_name, ushort id)
{
	if (id == 0)
		return -1;				   //与root重复
	fr = fopen(file_name, "rwb+"); //打开文件
	if (fr == NULL)
		return 0;
	//读取头节点
	head_node head;
	fread(&head, sizeof(head_node), 1, fr);

	if (head.usr_num == 23)
		return -2; //账号数量满
	//查重
	for (int i = 0; i < head.usr_num; i++)
	{
		if (id == head.usr_id[i])
			return -1;
	}
	//写入账号
	head.usr_id[head.usr_num++] = id;
	fseek(fr, 0, SEEK_SET);
	fwrite(&head, sizeof(head_node), 1, fr);
	return id;
}

void ll()
{
	//读目录项
	i_node i_nd, now_dr_nd;
	dir_node now_dr_nds[1024];
	struct tm *tblock;
	char *tim;
	fseek(fr, now_dr_id * sizeof(i_node), SEEK_SET);
	fread(&now_dr_nd, sizeof(i_node), 1, fr);
	iread(now_dr_id, 0, &now_dr_nds, now_dr_nd.size);
	for (int i = 0; i < now_dr_nd.size / sizeof(dir_node); i++)
	{
		//读取i节点
		fseek(fr, now_dr_nds[i].i_node * sizeof(i_node), SEEK_SET);
		fread(&i_nd, sizeof(i_node), 1, fr);
		//读取权限
		if ((i_nd.type & D_TY) != 0)
			printf("d");
		else
			printf("-");
		if ((i_nd.type & O_RD) != 0)
			printf("r");
		else
			printf("-");
		if ((i_nd.type & O_WR) != 0)
			printf("w");
		else
			printf("-");
		//读取所有者
		printf("%3d", (int)i_nd.owner);
		//读取大小
		printf("%10u", i_nd.size);
		//读取时间
		tblock = localtime(&i_nd.ctime);
		tim = asctime(tblock);
		tim[strlen(tim) - 1] = '\0';
		printf("%28s", tim);
		//读文件名
		printf("  %s\n", now_dr_nds[i].file_name);
	}
}

void cd(char *path_cd)
{
	//分割路径
	int dir_num = 0;
	uint dr_i_id;
	char *dirs[1024];
	char *token = strtok(path_cd, "/");
	i_node dr_i_nd;
	while (token != NULL)
	{
		dirs[dir_num++] = token;
		token = strtok(NULL, "/");
	}
	dr_i_id = path_cd[0] == '/' ? 1 : now_dr_id;
	//查找目录
	for (int i = 0; i < dir_num; i++)
	{
		dr_i_id = search_file(dr_i_id, dirs[i], 1);
		if (dr_i_id == 0)
		{
			printf("dir wrong!\n");
			return;
		}
		fseek(fr, dr_i_id * sizeof(i_node), SEEK_SET);
		fread(&dr_i_nd, sizeof(i_node), 1, fr);
		if ((dr_i_nd.type) & D_TY == 0)
		{
			printf("dir wrong!\n");
			return;
		}
	}
	//全路径
	if (path_cd[0] == '/')
	{
		for (int i = 0; i < dir_num; i++)
			strcpy(path[i], dirs[i]);
		path_len = dir_num;
	}
	//相对路径
	else
	{
		for (int i = 0; i < dir_num; i++)
			strcpy(path[path_len + i], dirs[i]);
		path_len += dir_num;
	}
	now_dr_id = dr_i_id;
	format_dir();
}

void mkdir(char *dir_name)
{
	if (strstr(dir_name, "/") != NULL)
	{
		printf("wrong name!\n");
		return;
	}
	uint dr_i_id = new_file(now_dr_id, dir_name, O_RD | D_TY);
	if (dr_i_id == 0)
		return;
}

void chmod(char *type, char *file_name)
{
	if (strlen(type) != 2)
	{
		printf("commend wrong!\n");
		return;
	}
	uint i_id = search_file(now_dr_id, file_name, 1);
	if (i_id == 0)
	{
		printf("wrong name!\n");
		return;
	}
	i_node i_nd;
	fseek(fr, i_id * 64, SEEK_SET);
	fread(&i_nd, sizeof(i_node), 1, fr);
	i_nd.type = (type[0] == 'r' ? O_RD : 0) | (type[1] == 'w' ? O_WR : 0) | ((i_nd.type & D_TY) != 0 ? D_TY : F_TY);
	fseek(fr, i_id * 64, SEEK_SET);
	fwrite(&i_nd, sizeof(i_node), 1, fr);
}

void touch(char *file_name)
{
	if (strstr(file_name, "/") != NULL)
	{
		printf("wrong name!\n");
		return;
	}
	uint i_id = new_file(now_dr_id, file_name, O_RD | F_TY);
	if (i_id == 0)
		return;
}

void cat(char *file_name)
{
	uint i_id = search_file(now_dr_id, file_name, 1);
	if (i_id == 0)
	{
		printf("wrong name!\n");
		return;
	}
	i_node i_nd;
	fseek(fr, i_id * 64, SEEK_SET);
	fread(&i_nd, sizeof(i_node), 1, fr);
	if ((i_nd.type & D_TY) != 0)
	{
		printf("can not print dir!\n");
		return;
	}
	if ((i_nd.type & O_RD) == 0)
	{
		printf("you do not have power to read '%s'\n", file_name);
		return;
	}
	char *buf = (char *)malloc(i_nd.size + 1);
	buf[i_nd.size] = '\0';
	iread(i_id, 0, buf, i_nd.size);
	printf("%s\n", buf);
	free(buf);
}

void rm(char *file_name)
{
	del_file(now_dr_id, file_name);
}

void gedit(char *file_name)
{
	uint i_id = search_file(now_dr_id, file_name, 1);
	i_node i_nd;
	if (i_id == 0)
	{
		if (strstr(file_name, "/") != NULL)
		{
			printf("wrong name!\n");
			return;
		}
		i_id = new_file(now_dr_id, file_name, O_RD | F_TY);
		if (i_id == 0)
			return;
	}
	fseek(fr, i_id * 64, SEEK_SET);
	fread(&i_nd, sizeof(i_node), 1, fr);
	if ((i_nd.type & D_TY) != 0)
	{
		printf("can not print dir!\n");
		return;
	}
	if ((i_nd.type & O_RD) == 0)
	{
		printf("you do not have power to read '%s'\n", file_name);
		return;
	}
	char *buf = (char *)malloc(i_nd.size);
	iread(i_id, 0, buf, i_nd.size);
	FILE *temp = fopen(".tempfile", "wb");
	fwrite(buf, i_nd.size, 1, temp);
	fclose(temp);
	free(buf);
	if (fork() == 0)
	{
		char *args[3] = {"/usr/bin/gedit", ".tempfile", NULL};
		execv("/usr/bin/gedit", args);
	}
	else
	{
		wait(NULL);
		if (usr_id != 0 && usr_id != i_nd.owner && (i_nd.type & O_WR) == 0)
		{
			printf("you do not have power to write '%s'\n", file_name);
			remove(".tempfile");
			return;
		}
		temp = fopen(".tempfile", "rb");
		fseek(temp, 0, SEEK_END);
		int file_size = ftell(temp);
		buf = (char *)malloc(file_size);
		fseek(temp, 0, SEEK_SET);
		fread(buf, file_size, 1, temp);
		fclose(temp);
		remove(".tempfile");
		//删除原文件
		if (i_nd.size != 0)
		{
			for (int i = (i_nd.size + 511) / 512 - 1; i >= 0; i--) //收集文件物理块
				clt_block(&i_nd, i);
		}
		i_nd.size = 0;
		fseek(fr, i_id * 64, SEEK_SET);
		fwrite(&i_nd, sizeof(i_node), 1, fr);
		iwrite(i_id, 0, buf, file_size);
		free(buf);
	}
}

void format_dir()
{
	int i = 0;
	while (i < path_len)
	{
		if (strcmp(path[i], ".") == 0 || strcmp(path[i], "..") == 0 && i == 0)
		{
			for (int j = i; j < path_len - 1; j++)
				strcpy(path[j], path[j + 1]);
			path_len--;
		}
		else if (strcmp(path[i], "..") == 0)
		{
			for (int j = i - 1; j < path_len - 2; j++)
				strcpy(path[j], path[j + 2]);
			path_len -= 2;
			i--;
		}
		else
			i++;
	}
}

uint search_file(uint dr_i_id, char *file_name, int ret_type) //根据文件名在文件夹中找出文件
{
	//读取目录i节点
	i_node dr_i_nd;
	fseek(fr, dr_i_id * 64, SEEK_SET);
	fread(&dr_i_nd, sizeof(i_node), 1, fr);

	//查找
	dir_node *dr_nds = (dir_node *)malloc(dr_i_nd.size);
	iread(dr_i_id, 0, dr_nds, dr_i_nd.size);
	uint i = 0;
	for (; i < dr_i_nd.size / sizeof(dir_node); i++)
	{
		if (strcmp(dr_nds[i].file_name, file_name) == 0)
		{
			uint i_id = dr_nds[i].i_node;
			free(dr_nds);
			if (ret_type)
				return i_id;
			else
				return i;
		}
	}
	free(dr_nds);
	if (ret_type)
		return 0;
	else
		return i;
}

uint new_file(uint dr_i_id, char *file_name, ushort type)
{
	//读取目录i节点
	i_node dr_i_nd;
	fseek(fr, dr_i_id * 64, SEEK_SET);
	fread(&dr_i_nd, sizeof(i_node), 1, fr);

	if (search_file(dr_i_id, file_name, 1) != 0) //查重名
	{
		printf("name repeat!\n");
		return 0;
	}
	//验证权限
	if (usr_id != 0 && usr_id != dr_i_nd.owner && (dr_i_nd.type & O_WR) == 0)
	{
		printf("you do not have power to creat '%s'\n", file_name);
		return 0;
	}
	//验证空闲块是否够
	head_node h_nd;
	fseek(fr, 0, SEEK_SET);
	fread(&h_nd, sizeof(head_node), 1, fr);
	if (h_nd.d_free_num < 1 || h_nd.i_free_num < 1)
	{
		printf("no free node!\n");
		return 0;
	}
	//写回目录文件
	dir_node dr_nd;
	strcpy(dr_nd.file_name, file_name);
	dr_nd.i_node = i_out_free();
	iwrite(dr_i_id, dr_i_nd.size, &dr_nd, sizeof(dir_node));
	//写i节点
	i_node i_nd;
	i_nd.owner = usr_id;
	i_nd.ctime = time(NULL);
	i_nd.type = type;
	i_nd.size = 0;
	if ((type & D_TY) != 0) //目录文件初始化
	{
		i_nd.size = 2 * sizeof(dir_node);
		i_nd.addr[0] = d_out_free();
		dir_node root_dir[2];
		strcpy(root_dir[0].file_name, ".");
		strcpy(root_dir[1].file_name, "..");
		root_dir[0].i_node = dr_nd.i_node;
		root_dir[1].i_node = dr_i_id;
		fseek(fr, i_nd.addr[0] * 512, SEEK_SET);
		fwrite(root_dir, 2 * sizeof(dir_node), 1, fr);
	}

	fseek(fr, dr_nd.i_node * 64, SEEK_SET);
	fwrite(&i_nd, sizeof(i_node), 1, fr);
	return dr_nd.i_node;
}

uint del_file(uint dr_i_id, char *file_name)
{
	//验证.与..
	if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0)
	{
		printf("you can not delete '%s'\n", file_name);
		return 0;
	}
	//读取目录i节点
	i_node dr_i_nd;
	fseek(fr, dr_i_id * 64, SEEK_SET);
	fread(&dr_i_nd, sizeof(i_node), 1, fr);
	//获取文件i节点
	i_node i_nd;
	uint i_num = search_file(dr_i_id, file_name, 0);
	//文件不存在
	if (i_num == dr_i_nd.size / sizeof(dir_node))
	{
		printf("no file named '%s'\n", file_name);
		return 0;
	}
	dir_node i_dir;
	iread(dr_i_id, i_num * sizeof(dir_node), &i_dir, sizeof(dir_node));
	uint i_id = i_dir.i_node;

	fseek(fr, i_id * 64, SEEK_SET);
	fread(&i_nd, sizeof(i_node), 1, fr);
	//验证权限
	if (usr_id != 0 && usr_id != dr_i_nd.owner && (dr_i_nd.type & O_WR) == 0)
	{
		//是目录文件
		if ((i_nd.type & D_TY) != 0)
			del_d(i_id);
		printf("you do not have power to delete '%s'\n", file_name);
		return 0;
	}
	else
	{
		//是普通文件
		if ((i_nd.type & F_TY) != 0)
		{
			if (i_nd.size != 0)
			{
				for (int i = (i_nd.size + 511) / 512 - 1; i >= 0; i--) //收集文件物理块
					clt_block(&i_nd, i);
			}
		}
		else
		{
			if (del_d(i_id) == 0)
			{
				printf("you can not delete '%s'\n", file_name);
				return 0;
			}
		}
		i_clt_free(i_id); //收集i节点
		//填充末尾目录项到空缺处
		dir_node last_dir;
		iread(dr_i_id, dr_i_nd.size - sizeof(dir_node), &last_dir, sizeof(dir_node));
		iwrite(dr_i_id, i_num * sizeof(dir_node), &last_dir, sizeof(dir_node));
		//写回目录i节点
		fseek(fr, dr_i_id * 64, SEEK_SET);
		fread(&dr_i_nd, sizeof(i_node), 1, fr);
		dr_i_nd.size -= sizeof(dir_node);
		fseek(fr, dr_i_id * 64, SEEK_SET);
		fwrite(&dr_i_nd, sizeof(i_node), 1, fr);
		//回收多余块
		if (dr_i_nd.size % 512 == 0)
			clt_block(&dr_i_nd, dr_i_nd.size / 512);

		printf("deleted '%s'\n", file_name);

		return 1;
	}
}

uint del_d(uint dr_i_id) //删除目录文件
{
	//获取文件i节点
	i_node dr_i_nd;
	fseek(fr, dr_i_id * 64, SEEK_SET);
	fread(&dr_i_nd, sizeof(i_node), 1, fr);

	//获取目录项
	dir_node *dr_nds = (dir_node *)malloc(dr_i_nd.size);
	iread(dr_i_id, 0, dr_nds, dr_i_nd.size);

	i_node i_nd;
	//验证权限
	if (usr_id != 0 && usr_id != dr_i_nd.owner && (dr_i_nd.type & O_WR) == 0)
	{
		for (int i = 2; i < dr_i_nd.size / sizeof(dir_node); i++)
		{
			//读取文件i节点
			fseek(fr, dr_nds[i].i_node * 64, SEEK_SET);
			fread(&i_nd, sizeof(i_node), 1, fr);
			//目录文件
			if ((i_nd.type & D_TY) != 0)
				del_d(dr_nds[i].i_node);
			printf("you do not have power to delete '%s'\n", dr_nds[i].file_name);
		}
		return 0;
	}
	else
	{
		uint del_success = 1;
		for (int i = 2; i < dr_i_nd.size / sizeof(dir_node); i++)
		{
			//读取文件i节点
			fseek(fr, dr_nds[i].i_node * 64, SEEK_SET);
			fread(&i_nd, sizeof(i_node), 1, fr);
			//普通文件
			if ((i_nd.type & F_TY) != 0)
			{
				if (i_nd.size != 0)
				{
					for (int i = (i_nd.size + 511) / 512 - 1; i >= 0; i--) //收集文件物理块
						clt_block(&i_nd, i);
				}
			}
			//目录文件
			else
			{
				if (del_d(dr_nds[i].i_node) == 0) //递归调用
				{
					printf("you can not delete '%s'\n", dr_nds[i].file_name);
					del_success = 0;
					continue;
				}
			}
			i_clt_free(dr_nds[i].i_node); //收集i节点
			printf("deleted '%s'\n", dr_nds[i].file_name);
			//填充末尾目录项到空缺处
			iwrite(dr_i_id, i * sizeof(dir_node), &(dr_nds[dr_i_nd.size / sizeof(dir_node) - 1]), sizeof(dir_node));
			//写回目录i节点
			fseek(fr, dr_i_id * 64, SEEK_SET);
			fread(&dr_i_nd, sizeof(i_node), 1, fr);
			dr_i_nd.size -= sizeof(dir_node);
			fseek(fr, dr_i_id * 64, SEEK_SET);
			fwrite(&dr_i_nd, sizeof(i_node), 1, fr);
			//回收多余块
			if (dr_i_nd.size % 512 == 0)
				clt_block(&dr_i_nd, dr_i_nd.size / 512);
		}
		return del_success;
	}
}

int iread(uint i_id, uint offset, void *buf, uint size) //读取文件指定位置到缓冲区
{
	//读取文件i节点
	i_node i_nd;
	void *buf_pointer = buf;
	fseek(fr, i_id * 64, SEEK_SET);
	fread(&i_nd, sizeof(i_node), 1, fr);

	if (i_nd.size < offset + size)
		return 0; //越界

	uint begin = offset / 512;				 //起始块
	uint begin_offset = offset % 512;		 //起始偏移
	uint end = (offset + size) / 512;		 //终止块
	uint end_offset = (offset + size) % 512; //终止偏移
	uint now_off = 0;						 //总读取数
	if (end_offset == 0)
	{
		end--;
		end_offset = 512;
	}

	for (uint i = begin; i < end + 1; i++)
	{
		if (i == begin)
		{
			if (begin != end)
			{
				fseek(fr, get_block_id(i_id, i) * 512 + begin_offset, SEEK_SET);
				fread(buf_pointer, 512 - begin_offset, 1, fr);
				now_off += 512 - begin_offset;
			}
			else
			{
				fseek(fr, get_block_id(i_id, i) * 512 + begin_offset, SEEK_SET);
				fread(buf_pointer, end_offset - begin_offset, 1, fr);
				now_off += end_offset - begin_offset;
			}
		}
		else if (i == end)
		{
			fseek(fr, get_block_id(i_id, i) * 512, SEEK_SET);
			fread(buf_pointer, end_offset, 1, fr);
			now_off += end_offset;
		}
		else
		{
			fseek(fr, get_block_id(i_id, i) * 512, SEEK_SET);
			fread(buf_pointer, 512, 1, fr);
			now_off += 512;
		}
		buf_pointer = (void *)((unsigned long)buf + now_off);
	}
	return now_off;
}

int iwrite(uint i_id, uint offset, void *buf, uint size) //将缓冲区内容写入到文件指定位置
{
	//读取文件i节点
	i_node i_nd;
	void *buf_pointer = buf;
	fseek(fr, i_id * 64, SEEK_SET);
	fread(&i_nd, sizeof(i_node), 1, fr);
	//验证空闲块是否够
	head_node h_nd;
	fseek(fr, 0, SEEK_SET);
	fread(&h_nd, sizeof(head_node), 1, fr);
	if ((offset + size + 511) / 512 - (i_nd.size + 511) / 512 > h_nd.d_free_num)
	{
		printf("no free node!\n");
		return 0;
	}
	if (i_nd.size < offset)
		return 0; //越界

	uint begin = offset / 512;				 //起始块
	uint begin_offset = offset % 512;		 //起始偏移
	uint end = (offset + size) / 512;		 //终止块
	uint end_offset = (offset + size) % 512; //终止偏移
	uint now_off = 0;						 //总写入数
	if (end_offset == 0)
	{
		end--;
		end_offset = 512;
	}

	for (uint i = begin; i < end + 1; i++)
	{
		if (i >= (i_nd.size + 511) / 512) //大于最大块号，建立新块
			make_block(i_id, i);
		if (i == begin)
		{
			if (begin != end)
			{
				fseek(fr, get_block_id(i_id, i) * 512 + begin_offset, SEEK_SET);
				fwrite(buf_pointer, 512 - begin_offset, 1, fr);
				now_off += 512 - begin_offset;
			}
			else
			{
				fseek(fr, get_block_id(i_id, i) * 512 + begin_offset, SEEK_SET);
				fwrite(buf_pointer, end_offset - begin_offset, 1, fr);
				now_off += end_offset - begin_offset;
			}
		}
		else if (i == end)
		{
			fseek(fr, get_block_id(i_id, i) * 512, SEEK_SET);
			fwrite(buf_pointer, end_offset, 1, fr);
			now_off += end_offset;
		}
		else
		{
			fseek(fr, get_block_id(i_id, i) * 512, SEEK_SET);
			fwrite(buf_pointer, 512, 1, fr);
			now_off += 512;
		}
		buf_pointer = (void *)((unsigned long)buf + now_off);
	}
	//更新文件i节点
	fseek(fr, i_id * 64, SEEK_SET);
	fread(&i_nd, sizeof(i_node), 1, fr);
	i_nd.size = offset + now_off > i_nd.size ? offset + now_off : i_nd.size;
	fseek(fr, i_id * 64, SEEK_SET);
	fwrite(&i_nd, sizeof(i_node), 1, fr);
	return now_off;
}

uint get_block_id(uint i_id, uint block_num) //获取第block_num块的块号
{
	//读取文件i节点
	i_node i_nd;
	fseek(fr, i_id * 64, SEEK_SET);
	fread(&i_nd, 64, 1, fr);

	uint first_seek[128], second_seek[128], third_seek[128];
	if (block_num < 9)
		return i_nd.addr[block_num]; //直接索引
	else if (block_num < 137)		 //一级间接索引
	{
		fseek(fr, i_nd.addr[9] * 512, SEEK_SET);
		fread(&first_seek, 512, 1, fr);
		return first_seek[block_num - 9];
	}
	else if (block_num < 16521) //二级间接索引
	{
		fseek(fr, i_nd.addr[10] * 512, SEEK_SET);
		fread(&first_seek, 512, 1, fr);
		fseek(fr, first_seek[(block_num - 137) / 128] * 512, SEEK_SET);
		fread(&second_seek, 512, 1, fr);
		return second_seek[(block_num - 137) % 128];
	}
	else //三级间接索引
	{
		fseek(fr, i_nd.addr[11] * 512, SEEK_SET);
		fread(&first_seek, 512, 1, fr);
		fseek(fr, first_seek[(block_num - 16521) / 16384] * 512, SEEK_SET);
		fread(&second_seek, 512, 1, fr);
		fseek(fr, second_seek[(block_num - 16521) % 16384 / 128] * 512, SEEK_SET);
		fread(&third_seek, 512, 1, fr);
		return third_seek[(block_num - 16521) % 128];
	}
}

uint make_block(uint i_id, uint block_num) //为对应块序号获取物理块
{
	//读取文件i节点
	i_node i_nd;
	fseek(fr, i_id * 64, SEEK_SET);
	fread(&i_nd, 64, 1, fr);

	uint first_seek[128], second_seek[128], third_seek[128];
	if (block_num < 9) //直接索引
	{
		i_nd.addr[block_num] = d_out_free(fr);
		fseek(fr, i_id * 64, SEEK_SET);
		fwrite(&i_nd, 64, 1, fr);
		return i_nd.addr[block_num];
	}
	else if (block_num < 137) //一级间接索引
	{
		if (block_num == 9)
		{
			i_nd.addr[9] = d_out_free();
			fseek(fr, i_id * 64, SEEK_SET);
			fwrite(&i_nd, 64, 1, fr);
		}
		fseek(fr, i_nd.addr[9] * 512, SEEK_SET);
		fread(&first_seek, 512, 1, fr);
		first_seek[block_num - 9] = d_out_free();
		fseek(fr, i_nd.addr[9] * 512, SEEK_SET);
		fwrite(&first_seek, 512, 1, fr);
		return first_seek[block_num - 9];
	}
	else if (block_num < 16521) //二级间接索引
	{
		if (block_num == 137)
		{
			i_nd.addr[10] = d_out_free();
			fseek(fr, i_id * 64, SEEK_SET);
			fwrite(&i_nd, 64, 1, fr);
		}
		fseek(fr, i_nd.addr[10] * 512, SEEK_SET);
		fread(&first_seek, 512, 1, fr);

		if ((block_num - 137) % 128 == 0)
		{
			first_seek[(block_num - 137) / 128] = d_out_free();
			fseek(fr, i_nd.addr[10] * 512, SEEK_SET);
			fwrite(&first_seek, 512, 1, fr);
		}

		fseek(fr, first_seek[(block_num - 137) / 128] * 512, SEEK_SET);
		fread(&second_seek, 512, 1, fr);
		second_seek[(block_num - 137) % 128] = d_out_free();
		fseek(fr, first_seek[(block_num - 137) / 128] * 512, SEEK_SET);
		fwrite(&second_seek, 512, 1, fr);
		return second_seek[(block_num - 137) % 128];
	}
	else //三级间接索引
	{
		if (block_num == 16521)
		{
			i_nd.addr[11] = d_out_free();
			fseek(fr, i_id * 64, SEEK_SET);
			fwrite(&i_nd, 64, 1, fr);
		}
		fseek(fr, i_nd.addr[11] * 512, SEEK_SET);
		fread(&first_seek, 512, 1, fr);

		if ((block_num - 16521) % 16384 == 0)
		{
			first_seek[(block_num - 16521) / 16384] = d_out_free();
			fseek(fr, i_nd.addr[11] * 512, SEEK_SET);
			fwrite(&first_seek, 512, 1, fr);
		}

		fseek(fr, first_seek[(block_num - 16521) / 16384] * 512, SEEK_SET);
		fread(&second_seek, 512, 1, fr);

		if ((block_num - 16521) % 128 == 0)
		{
			second_seek[(block_num - 16521) % 16384 / 128] = d_out_free();
			fseek(fr, first_seek[(block_num - 16521) / 16384] * 512, SEEK_SET);
			fwrite(&second_seek, 512, 1, fr);
		}

		fseek(fr, second_seek[(block_num - 16521) % 16384 / 128] * 512, SEEK_SET);
		fread(&third_seek, 512, 1, fr);
		third_seek[(block_num - 16521) % 128] = d_out_free();
		fseek(fr, second_seek[(block_num - 16521) % 16384 / 128] * 512, SEEK_SET);
		fwrite(&third_seek, 512, 1, fr);
		return third_seek[(block_num - 16521) % 128];
	}
}

void clt_block(i_node *i_nd, uint block_num) //收集文件物理块，务必从末尾开始
{
	uint first_seek[128], second_seek[128], third_seek[128];
	if (block_num < 9) //直接索引
		d_clt_free(i_nd->addr[block_num]);
	else if (block_num < 137) //一级间接索引
	{
		fseek(fr, i_nd->addr[9] * 512, SEEK_SET);
		fread(&first_seek, 512, 1, fr);
		d_clt_free(first_seek[block_num - 9]);

		if (block_num == 9)
			d_clt_free(i_nd->addr[9]);
	}
	else if (block_num < 16521) //二级间接索引
	{
		fseek(fr, i_nd->addr[10] * 512, SEEK_SET);
		fread(&first_seek, 512, 1, fr);
		fseek(fr, first_seek[(block_num - 137) / 128] * 512, SEEK_SET);
		fread(&second_seek, 512, 1, fr);
		d_clt_free(second_seek[(block_num - 137) % 128]);
		if ((block_num - 137) % 128 == 0)
			d_clt_free(first_seek[(block_num - 137) / 128]);
		if (block_num == 137)
			d_clt_free(i_nd->addr[10]);
	}
	else //三级间接索引
	{
		fseek(fr, i_nd->addr[11] * 512, SEEK_SET);
		fread(&first_seek, 512, 1, fr);
		fseek(fr, first_seek[(block_num - 16521) / 16384] * 512, SEEK_SET);
		fread(&second_seek, 512, 1, fr);
		fseek(fr, second_seek[(block_num - 16521) % 16384 / 128] * 512, SEEK_SET);
		fread(&third_seek, 512, 1, fr);
		d_clt_free(third_seek[(block_num - 16521) % 128]);
		if ((block_num - 16521) % 128 == 0)
			d_clt_free(second_seek[(block_num - 16521) % 16384 / 128]);
		if ((block_num - 16521) % 16384 == 0)
			d_clt_free(first_seek[(block_num - 16521) / 16384]);
		if (block_num == 16521)
			d_clt_free(i_nd->addr[11]);
	}
}

void i_clt_free(uint node_id) //回收i节点
{
	//读取头节点和首空闲块
	head_node head;
	i_free_node first_free;
	fseek(fr, 0, SEEK_SET);
	fread(&head, sizeof(head_node), 1, fr);
	fseek(fr, head.i_first_free * 64, SEEK_SET);
	fread(&first_free, sizeof(i_free_node), 1, fr);

	if (first_free.free_num < 15) //首空闲块未满，添加
	{
		first_free.free_nodes[first_free.free_num++] = node_id;
		head.i_free_num++;

		fseek(fr, head.i_first_free * 64, SEEK_SET);
		fwrite(&first_free, sizeof(i_free_node), 1, fr);
	}
	else //首空闲块满，拓展
	{
		i_free_node new_free;
		new_free.free_num = 1;
		new_free.free_nodes[0] = head.i_first_free;
		head.i_first_free = node_id;
		head.i_free_num++;

		fseek(fr, head.i_first_free * 64, SEEK_SET);
		fwrite(&new_free, sizeof(i_free_node), 1, fr);
	}
	fseek(fr, 0, SEEK_SET);
	fwrite(&head, sizeof(head_node), 1, fr);
}

uint i_out_free() //获取空闲i节点
{
	//读取头节点和首空闲块
	head_node head;
	i_free_node first_free;
	uint node_id;
	fseek(fr, 0, SEEK_SET);
	fread(&head, sizeof(head_node), 1, fr);
	fseek(fr, head.i_first_free * 64, SEEK_SET);
	fread(&first_free, sizeof(i_free_node), 1, fr);

	if (first_free.free_num > 1) //首空闲块有剩余
	{
		node_id = first_free.free_nodes[--first_free.free_num];
		head.i_free_num--;

		fseek(fr, head.i_first_free * 64, SEEK_SET);
		fwrite(&first_free, sizeof(i_free_node), 1, fr);
	}
	else //首空闲块无剩余，拆散
	{
		node_id = head.i_first_free;
		head.i_first_free = first_free.free_nodes[0];
		head.i_free_num--;
	}
	fseek(fr, 0, SEEK_SET);
	fwrite(&head, sizeof(head_node), 1, fr);

	return node_id;
}

void d_clt_free(uint node_id) //回收文件块
{
	//读取头节点和首空闲块
	head_node head;
	d_free_node first_free;
	fseek(fr, 0, SEEK_SET);
	fread(&head, sizeof(head_node), 1, fr);
	fseek(fr, head.d_first_free * 512, SEEK_SET);
	fread(&first_free, sizeof(d_free_node), 1, fr);

	if (first_free.free_num < 127) //首空闲块未满，添加
	{
		first_free.free_nodes[first_free.free_num++] = node_id;
		head.d_free_num++;

		fseek(fr, head.d_first_free * 512, SEEK_SET);
		fwrite(&first_free, sizeof(d_free_node), 1, fr);
	}
	else //首空闲块满，拓展
	{
		d_free_node new_free;
		new_free.free_num = 1;
		new_free.free_nodes[0] = head.d_first_free;
		head.d_first_free = node_id;
		head.d_free_num++;

		fseek(fr, head.d_first_free * 512, SEEK_SET);
		fwrite(&new_free, sizeof(d_free_node), 1, fr);
	}
	fseek(fr, 0, SEEK_SET);
	fwrite(&head, sizeof(head_node), 1, fr);
}

uint d_out_free() //获取空闲文件块
{
	//读取头节点和首空闲块
	head_node head;
	d_free_node first_free;
	uint node_id;
	fseek(fr, 0, SEEK_SET);
	fread(&head, sizeof(head_node), 1, fr);
	fseek(fr, head.d_first_free * 512, SEEK_SET);
	fread(&first_free, sizeof(d_free_node), 1, fr);

	if (first_free.free_num > 1) //首空闲块有剩余
	{
		node_id = first_free.free_nodes[--first_free.free_num];
		head.d_free_num--;

		fseek(fr, head.d_first_free * 512, SEEK_SET);
		fwrite(&first_free, sizeof(d_free_node), 1, fr);
	}
	else //首空闲块无剩余，拆散
	{
		node_id = head.d_first_free;
		head.d_first_free = first_free.free_nodes[0];
		head.d_free_num--;
	}
	fseek(fr, 0, SEEK_SET);
	fwrite(&head, sizeof(head_node), 1, fr);

	return node_id;
}

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		printf("no arg!\n");
		return 0;
	}
	if (strcmp("-i", argv[1]) == 0 && argc == 5) //初始化
	{

		if (init(argv[2], atoi(argv[3]), atoi(argv[4])))
			printf("init successfully!\n");
		else
			printf("init fail!\n");
		return 0;
	}

	else if (strcmp("-s", argv[1]) == 0 && argc == 4) //注册
	{
		int state = sig(argv[2], (ushort)atoi(argv[3]));
		if (state == -2)
			printf("account full!\n");
		else if (state == -1)
			printf("account repeat!\n");
		else if (state == 0)
			printf("no such file!\n");
		else
			printf("id: '%d' sig up successfully!\n", state);
		return 0;
	}
	else if (strcmp("-l", argv[1]) == 0 && argc == 4) //登录并操作
	{
		fr = log_in(argv[2], (ushort)atoi(argv[3]));
		if (fr == NULL)
		{
			printf("log fail!\n");
			return 0;
		}
		else
		{
			//初始化全局变量
			usr_id = (ushort)atoi(argv[3]);
			path_len = 0;
			now_dr_id = 1;
			//定义操作指令空间
			char cmd[30];
			int cmdc = 0;
			char *cmdv[10];
			char *token;
			while (1)
			{
				//显示路径
				printf("%d:/", (int)usr_id);
				for (ushort i = 0; i < path_len; i++)
					printf("%s/", path[i]);
				printf("$ ");

				//读取指令
				fgets(cmd, 30, stdin);
				cmd[strlen(cmd) - 1] = '\0';
				cmdc = 0;
				token = strtok(cmd, " ");
				while (token != NULL)
				{
					cmdv[cmdc++] = token;
					token = strtok(NULL, " ");
				}
				//处理指令
				if (strcmp(cmdv[0], "logout") == 0 && cmdc == 1) //登出
					break;
				else if (strcmp(cmdv[0], "ll") == 0 && cmdc == 1) //模拟ll
					ll();
				else if (strcmp(cmdv[0], "cd") == 0 && cmdc == 2) //模拟cd
					cd(cmdv[1]);
				else if (strcmp(cmdv[0], "mkdir") == 0 && cmdc == 2) //模拟mkdir
					mkdir(cmdv[1]);
				else if (strcmp(cmdv[0], "chmod") == 0 && cmdc == 3) //模拟chmod
					chmod(cmdv[1], cmdv[2]);
				else if (strcmp(cmdv[0], "touch") == 0 && cmdc == 2) //模拟touch
					touch(cmdv[1]);
				else if (strcmp(cmdv[0], "cat") == 0 && cmdc == 2) //模拟cat
					cat(cmdv[1]);
				else if (strcmp(cmdv[0], "rm") == 0 && cmdc == 2) //模拟rm
					rm(cmdv[1]);
				else if (strcmp(cmdv[0], "gedit") == 0 && cmdc == 2) //模拟rm
					gedit(cmdv[1]);
				else
					printf("wrong commend!\n");
			}
			fclose(fr);
			return 0;
		}
	}
	else
		return 0;
}
