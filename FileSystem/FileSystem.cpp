#include"FileSystem.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
/* TODO
void toB(char n, char s[]);
//TODO
*/
_hd hardDisk;
_cmd CMD;
_hfs	HFS;

bool out(int id, char buffer[]){

	return out(id, 0, BLOCK_SIZE, buffer);
}

bool out(int id, int begin, int len,char buffer[]){
	if (id<DISK_SIZE){
		memcpy(hardDisk.blocks[id].data+begin, buffer, len);
		return true;
	}
	else
		return false;
}



bool in(int id, int begin, int len,char buffer[]){
	if (id<DISK_SIZE){
		memcpy(buffer, hardDisk.blocks[id].data+begin, len);
		return true;
	}
	else
		return false;
}

bool in(int id, char buffer[]){

	return in(id, 0, BLOCK_SIZE, buffer);

}

void initVfile(vfile* file){

}

void initVdir(vdir* dir){

}
bool hardDisk_init(){
	//TODO 
	return true;
}

bool HFS_install(){
	 
	/*init fat in hardisk*/
	//char *temp = (char*)malloc(sizeof(char)*BLOCK_SIZE);
	char temp[BLOCK_SIZE];
	//io test:
	//memset(temp, -1, BLOCK_SIZE);
	//out(0, temp);
	memset(temp, 0, BLOCK_SIZE);
	out(1, temp);
	temp[0] = temp[1] =temp[2]= FILE_END;
	out(0, temp);

	/*init root direction*/
	((inode*)temp)[0].attribute = DIRECTION;
	((inode*)temp)[0].blockId = ROOT_DIR;
	((inode*)temp)[0].fileLength = 1;
	memcpy(((inode*)temp)[0].fileName, "Rt:",3);
	memcpy(((inode*)temp)[0].fileType, DIR_TYPE,2);
	out(ROOT_DIR, temp);

	return true;
}

void save_FAT(){

	out(0, HFS.fat.next);
	out(1, HFS.fat.next + BLOCK_SIZE);

}

bool HFS_init(){
	hardDisk_init();
	FAT_init();
	OPT_init();
	return true;
}

bool FAT_init(){

	for (int j = 0; j < FAT_SIZE; j++){
		in(j, &(HFS.fat.next[j*BUF_SIZE]));
	}
	for (int i = 0; i < DISK_SIZE; i++){
		HFS.fat.blocks[i] = &(hardDisk.blocks[i]);
	}
	

	return true;
}

bool OPT_init(){
	
	HFS.OPT.length = 0;
	return true;
}

bool CMD_init(){

	CMD.cur_dir.data = (inode *)malloc(BLOCK_SIZE);
	in(ROOT_DIR, (char*)(CMD.cur_dir.data));
	CMD.cur_dir.cnt = CMD.cur_dir.data[0].fileLength;

	CMD.cur_dir.fileInfo.attribute = DIRECTION;
	CMD.cur_dir.fileInfo.flag = WRITE;
	CMD.cur_dir.fileInfo.length = CMD.cur_dir.cnt;
	CMD.cur_dir.fileInfo.number = ROOT_DIR;
	memcpy(CMD.cur_dir.fileInfo.name, CMD.cur_dir.data[0].fileName, LEN_FILE_NAME);
	CMD.cur_dir.fileInfo.name[LEN_FILE_NAME ] = '/';
	CMD.cur_dir.fileInfo.name[LEN_FILE_NAME +1] = '\0';
	CMD.cur_dir.fileInfo.write = CMD.cur_dir.fileInfo.read = { ROOT_DIR, 0 };
	return true;
}
/*file system API:
for file
*/
bool HFS_create_file(char name[], char attribute){

	if (attribute&READ_ONLY)
		return false;
	/*if dir is full*/
	if (CMD.cur_dir.cnt >= 8)
		return false;

	/*if this name is exist*/
	if (checkExist(name, DIRECTION) != NO_EXIST)
		return false;

	/*check disk is full or not*/
	int blockid = getFreeBlock();


	HFS.fat.next[blockid] = FILE_END;

	/*init vfile info*/
	vfile* temp = (vfile*)malloc(sizeof(vfile));

	if (!addName(temp->fileInfo.name, CMD.cur_dir.fileInfo.name, name, FILE_TYPE))
		return false;
	temp->fileInfo.attribute = attribute&NORMAL_FILE;
	temp->fileInfo.flag = 0;
	temp->fileInfo.length = BLOCK_SIZE;
	temp->fileInfo.number = blockid;
	temp->data = (char*)malloc(BLOCK_SIZE);

	/*pointer tempp = *(pointer*)malloc(sizeof(pointer));
	tempp.bnum = blockid;
	tempp.dnum = 0;*/
	temp->fileInfo.read = { blockid, 0 };
	temp->fileInfo.write = { blockid, 0 };


	/*update CMD*/
	memcpy(CMD.cur_dir.data[CMD.cur_dir.cnt].fileName, name, nameLen(name));
	CMD.cur_dir.data[CMD.cur_dir.cnt].fileLength = BLOCK_SIZE;
	memcpy(CMD.cur_dir.data[CMD.cur_dir.cnt].fileType, FILE_TYPE, 2);
	CMD.cur_dir.data[CMD.cur_dir.cnt].blockId = blockid;
	CMD.cur_dir.data[CMD.cur_dir.cnt].attribute = attribute;
	CMD.cur_dir.data[0].fileLength++;
	CMD.cur_dir.cnt++;
	//clearEmptyFlag(CMD.cur_dir.fileInfo.name);

	/*save in disk*/
	pushBuf((char*)(CMD.cur_dir.data), BLOCK_SIZE, CMD.cur_dir.fileInfo.write);
	pushBuf(temp->data, BLOCK_SIZE, temp->fileInfo.write);
	save_FAT();

	//free(&tempp);
	free(temp->data);
	free(temp);
	return true;

}

vfile* HFS_open_file(char name[], char opt_type){

	/* if READ_ONLY or not*/
	if (opt_type&READ_ONLY)
		return NULL;

	/*if this name exsit ?*/
	int dirIndex = checkExist(name, NORMAL_FILE);

	if (dirIndex == NO_EXIST)
		return NULL;

	/*if have open 5 files?*/
	if (HFS.OPT.length >= MAX_OPEN)
		return NULL;

	/*if have opened?*/
	for (int i = 0; i < MAX_OPEN; i++){
		char* temp = getFileName(HFS.OPT.file[i].fileInfo.name);
		if (!memcmp(temp, name,nameLen(temp)))
			return NULL;
	}

	/*create a vfile and fill openfile table*/
	vfile* temp = &(HFS.OPT.file[HFS.OPT.length]);
	if(addName(temp->fileInfo.name, CMD.cur_dir.fileInfo.name, name, FILE_TYPE)){
		return false;
	}
	//temp->data = (char*)malloc(temp->fileInfo.length);
	temp->fileInfo.length = CMD.cur_dir.data[dirIndex].fileLength;
	temp->fileInfo.number = CMD.cur_dir.data[dirIndex].blockId;
	temp->fileInfo.attribute = CMD.cur_dir.data[dirIndex].attribute;
	temp->fileInfo.flag = (int)(opt_type);
	/*
	pointer tempp = *(pointer*)malloc(sizeof(pointer));
	tempp.bnum = CMD.cur_dir.data[dirIndex].blockId;
	tempp.dnum = 0;	
	*/
	temp->fileInfo.read = { CMD.cur_dir.data[dirIndex].blockId,0 };
	temp->fileInfo.write = { CMD.cur_dir.data[dirIndex].blockId, 0 };
															
	
	HFS.OPT.length++;

	return temp;


}
vfile* HFS_read_file(char name[], int length){

	vfile * file = checkOpen(name,READ);

	if (file){
		popBuf(file->data, length, file->fileInfo.read);
	}
	
	return file;

}

vfile*  HFS_write_file(char name[], char buf[], int length){

	vfile * file=checkOpen(name, WRITE);
	if (!file)
		return NULL;
	memcpy(file->data, buf, length);
	file->fileInfo.length += length;
	return file;
}
bool HFS_close_file(char name[]){

	/*check if opened*/
	int fileIndex = 0;
	char* temp = getFileName(HFS.OPT.file[fileIndex].fileInfo.name);
	for (; fileIndex < HFS.OPT.length; fileIndex++){
		 if (!memcmp(temp, name,nameLen(temp)))
			break;
	}

	vfile *file;
	if (fileIndex == HFS.OPT.length)
		return true;
	else
		file =&( HFS.OPT.file[fileIndex]);

	if (file->fileInfo.flag == WRITE){
		file->data = (char*)realloc(file->data, file->fileInfo.length + 1);
		file->data[file->fileInfo.length] = '#';
		file->fileInfo.length++;
	}

	
	/*save to disk when close*/
	pushBuf(file->data, file->fileInfo.length,file->fileInfo.write);

	/*delete info in OPT*/
	for (int i = fileIndex; i < HFS.OPT.length; i++)
		HFS.OPT.file[i] = HFS.OPT.file[i + 1];
	HFS.OPT.length--;
	return true;
}
bool HFS_delete_file(char name[]){

	/*if this name is exist*/
	int dirIndex=checkExist(name,NORMAL_FILE);

	if (dirIndex == NO_EXIST)
		return false;

	/*if haved open?*/
	int fileIndex = 0;
	char* temp = getFileName(HFS.OPT.file[fileIndex].fileInfo.name);
	for (; fileIndex < HFS.OPT.length; fileIndex++){
		if (!memcmp(temp, name,nameLen(temp)))
			break;
	}


	if (fileIndex == HFS.OPT.length)
		return true;


	/*delect disk*/
	int father = CMD.cur_dir.data[dirIndex].blockId;
	int blockId;
	while (HFS.fat.next[father] != FILE_END){
		blockId = HFS.fat.next[father];
		HFS.fat.next[father] = FREE;
		father = blockId;
	}
	HFS.fat.next[father] = FREE;

	/*delect dir info*/
	for (int i = dirIndex; i < CMD.cur_dir.cnt; i++){
		CMD.cur_dir.data[i] = CMD.cur_dir.data[i + 1];
	}
	CMD.cur_dir.cnt--;
	CMD.cur_dir.fileInfo.length --;

	

	/*save Fat in Disk*/
	save_FAT();
	return true;
	
}
bool HFS_typefile(char name[]){
	/*if this name is exist*/
	int dirIndex=checkExist(name,NORMAL_FILE);

	if (dirIndex == NO_EXIST)
		return false;

	/*if haved open?*/
	int fileIndex = 0;
	char* tempName = getFileName(HFS.OPT.file[fileIndex].fileInfo.name);
	for (; fileIndex < HFS.OPT.length; fileIndex++){
		if (!memcmp(tempName, name,nameLen(tempName)))
			break;
	}
	if (fileIndex != HFS.OPT.length)
		return false;

	/*load from disk*/
	char temp[BLOCK_SIZE+1];
	temp[BLOCK_SIZE] = '\0';
	int blockId = CMD.cur_dir.data[CMD.cur_dir.cnt].blockId;
	pointer p ;
	while (HFS.fat.next[blockId] != FILE_END){
		p = { blockId, 0 };
		popBuf(temp, BLOCK_SIZE, p);
		blockId = HFS.fat.next[blockId];
		printf("%s\n", temp);/*show*/
	}
	
	return true;
}
bool HFS_change(char name[], char attribute){

	/*if this name is exist*/
	int dirIndex=checkExist(name,NORMAL_FILE);

	if (dirIndex == NO_EXIST)
		return false;

	/*if haved open?*/
	int fileIndex = 0;
	char* temp = getFileName(HFS.OPT.file[fileIndex].fileInfo.name);
	for (; fileIndex < HFS.OPT.length; fileIndex++){
		if (!memcmp(temp, name,nameLen(temp)))
			break;
	}
	if (fileIndex != HFS.OPT.length)
		return false;

	CMD.cur_dir.data[dirIndex].attribute = attribute;

	return true;
}

/*file system API:
for dir
*/

bool HFS_create_dir(char name[]){

	/*if dir is exist add print*/
	if (checkExist(name, DIRECTION) != NO_EXIST)
		return false;

	/*if dir is full*/
	if (CMD.cur_dir.cnt >= BLOCK_SIZE / (sizeof(inode)))
	return false;/*maybe enlarge*/

	
	/*check disk is full or not*/
	int blockid = getFreeBlock();
	if (blockid == NO_EXIST)
		return false;
	HFS.fat.next[blockid] = FILE_END;

	/*update current direction and save in disk*/
	CMD.cur_dir.fileInfo.length ++;
	memcpy(CMD.cur_dir.data[CMD.cur_dir.cnt].fileName ,name,3);
	for (int i = strlen(name); i < 3; i++){
		CMD.cur_dir.data[CMD.cur_dir.cnt].fileName[i] = ' ';
	}//fill with space;
	memcpy(CMD.cur_dir.data[CMD.cur_dir.cnt].fileType , DIR_TYPE,2);
	CMD.cur_dir.data[CMD.cur_dir.cnt].fileLength = 1;
	CMD.cur_dir.data[CMD.cur_dir.cnt].blockId = blockid;
	CMD.cur_dir.data[CMD.cur_dir.cnt].attribute = DIRECTION;

	CMD.cur_dir.cnt++;
	CMD.cur_dir.data[0].fileLength++;

	pushBuf((char*)(CMD.cur_dir.data), BLOCK_SIZE, CMD.cur_dir.fileInfo.write);

	/*save new dir in disk*/
	inode *temp=(inode*)malloc(BLOCK_SIZE );
	temp[0] = { "", "", DIRECTION,
		CMD.cur_dir.data[0].blockId,
		CMD.cur_dir.cnt};
	memcpy(temp[0].fileName,".. ",3);
	memcpy(temp[0].fileType, DIR_TYPE, 2);

	pointer p = { blockid, 0 };
	pushBuf((char*)temp, BLOCK_SIZE, p);
	free(temp);
	/*save fat in disk*/
	save_FAT();
	return true;
}
void HFS_show_dir(){


	printf("\n"); 
	char string[9];
	string[8] = '\0';
	for (int i = 0; i < CMD.cur_dir.cnt; i++){	

		/*example:
		FILE    LENGTH  NAM.TY  00000101*/
		toB(CMD.cur_dir.data[i].attribute, string);
		if (CMD.cur_dir.data[i].attribute == NORMAL_FILE){
			printf("FILE");
		}
		else {
			printf("DIR");	
		}

		printf("\t%d\t%c%c%c %c%c\t%s\n",
			CMD.cur_dir.data[i].fileLength,
			CMD.cur_dir.data[i].fileName[0],
			CMD.cur_dir.data[i].fileName[1],
			CMD.cur_dir.data[i].fileName[2],
			CMD.cur_dir.data[i].fileType[0],
			CMD.cur_dir.data[i].fileType[1],
			string);		

	}
	printf("\n");
}

bool HFS_delete_dir(char name[]){

	/*if dir exist*/
	int dirIndex = checkExist(name, DIRECTION);
	if (dirIndex == NO_EXIST)
		return false;

	if (HFS_DFS(name, CMD.cur_dir.data[dirIndex].blockId, CMD.cur_dir.data[dirIndex].fileLength,0)){
		return HFS_DFS(name, CMD.cur_dir.data[dirIndex].blockId, CMD.cur_dir.data[dirIndex].fileLength,1);
	}
	return true;
}

bool HFS_DFS(char name[], int blockId,int length,bool flag){


	bool b = true;
	inode temp [BLOCK_SIZE/sizeof(inode)];
	pointer p = { blockId, 0 };
	popBuf((char*)(temp), length, p);

	for (int i = 0; i < length; i++){
		if (temp[i].attribute == NORMAL_FILE){
			/*have open?*/
			int fileIndex = 0;
			char* tempName = getFileName(HFS.OPT.file[fileIndex].fileInfo.name);
			for (; fileIndex < HFS.OPT.length; fileIndex++){
				if (!memcmp(tempName,temp[i].fileName,nameLen(tempName)))
					return false;
			}
			if (flag == 1)
				HFS.fat.next[temp[i].blockId] = FREE;
		}
		else if (temp[i].attribute == DIRECTION&&
						strcmp(temp[i].fileName, ".. /")){
			b &= HFS_DFS(temp[i].fileName, temp[i].blockId, temp[i].fileLength,flag);
			if (flag == 1)
				HFS.fat.next[temp[i].blockId]=FREE;
		}
	}

	/*save fat in disk*/
	save_FAT();
	return b;

}


bool HFS_change_dir(char name[]){

	/*if dir exist*/
	int dirIndex = checkExist(name, DIRECTION);
	if (dirIndex == NO_EXIST)
		return false;

	/*change current dir */
	CMD.cur_dir.cnt=CMD.cur_dir.fileInfo.length = CMD.cur_dir.data[dirIndex].fileLength;
	CMD.cur_dir.fileInfo.number = CMD.cur_dir.data[dirIndex].blockId;
	CMD.cur_dir.fileInfo.write=CMD.cur_dir.fileInfo.read = { CMD.cur_dir.fileInfo.number, 0 };

	if (!strcmp(name, "..")){
		/*move back to father*/
		int len=strlen(CMD.cur_dir.fileInfo.name);
		CMD.cur_dir.fileInfo.name[len - nameLen(CMD.cur_dir.data[0].fileName)] = '\0';	
	}
	else{
		if (!addName(CMD.cur_dir.fileInfo.name, CMD.cur_dir.fileInfo.name, name, DIR_TYPE)){
			return false;
		}
	}

	popBuf((char*)CMD.cur_dir.data, BLOCK_SIZE,CMD.cur_dir.fileInfo.read);

	return true;
}



vfile* checkOpen(char name[],int opt_type){
	/*haved opened?*/
	int fileIndex = 0;
	char* temp = getFileName(HFS.OPT.file[fileIndex].fileInfo.name);
	for (; fileIndex < HFS.OPT.length; fileIndex++){
		
		if (!memcmp(temp, name, nameLen(temp)))
			break;
	}

	vfile *file;
	if (fileIndex == HFS.OPT.length)
		file = HFS_open_file(name, opt_type);
	else
		file = &(HFS.OPT.file[fileIndex]);
	if (file == NULL)
		return NULL;
	return file;
}

int getFreeBlock(){
	/*RR*/
	static int id=0;
	int end = (id - 1 + DISK_SIZE - ROOT_DIR - 1) % (DISK_SIZE - ROOT_DIR - 1) ;
	for (; id != end; id = (id + 1) % (DISK_SIZE - ROOT_DIR - 1)){
		if (HFS.fat.next[id+BLOCK_BEGIN+1] == FREE)
			break;
	}
	if (id == end)
		return FILE_END;
	else
		return id + BLOCK_BEGIN + 1;
}

int checkExist(char* name,int attribute){
	/*if this name is exist*/
	for (int i = 0; i < CMD.cur_dir.cnt; i++){
		int temp = nameLen(CMD.cur_dir.data[i].fileName);
		if (!memcmp(name, CMD.cur_dir.data[i].fileName, nameLen(CMD.cur_dir.data[i].fileName)) &&
						(CMD.cur_dir.data[i].attribute&attribute))
			return i;
	}
	return NO_EXIST;
}

void console(){
	char args1[10]="\0";
	char args2[BLOCK_SIZE] = "\0";
	char args3=0;
	printf("Welcome to Hiro_File_System:\n");
	
	while (1){
		printf("%s>", CMD.cur_dir.fileInfo.name);
		scanf("%s", args1);
		switch (ins_judge(args1))
		{
			case	MD:
				scanf("%s", args1);
				CMD_MD(args1);
				break;
			case DIR:
				CMD_DIR();
				break;
			case RD:
				scanf("%s", args1);
				CMD_RD(args1);
				break;
			case CD:
				scanf("%s", args1);
				CMD_CD(args1);
				break;
			case HELP:
				CMD_HELP();
				break; 
			case Make_File:
				scanf("%s", args1);
				CMD_MakeFile(args1);
				break;
			case	Read_File:
				scanf("%s %d", args1,args3);
				CMD_ReadFile(args1, args3);
				break;
			case	Change:
				scanf("%s %d", args1,args3);
				CMD_Change(args1, args3);
				break;
			case	Write_File	:
				scanf("%s %s", args1, args2);
				CMD_WriteFile(args1, args2);
				break;
			case	DEL:
				scanf("%s", args1);
				CMD_DEL(args1);
				break;
			case	ERR:
				CMD_ERR();
		default:
			break;
		}
	}
}

void CMD_MD(char name[]){

	if (checkValid(name)){
		HFS_create_dir(name);
	}
}

void CMD_DIR(){

	HFS_show_dir();

}

void CMD_RD(char name[]){

	if (checkValid(name)){
		HFS_delete_dir(name);
	}

}

void CMD_CD(char name[]){

	if (checkValid(name)){
		HFS_change_dir(name);
	}
}

void CMD_HELP(){
	/*#define	MD						0
#define	DIR						1
#define	RD						2
#define	CD						3
#define	HELP					4
#define	Make_File				5//make file
#define	Read_File				6//read file
#define	Change				7
#define	Write_File				8
#define	DEL						9
#define	ERR						-1*/
	printf("Welcome to use Hiro_File_System\n");
	printf("Instructions are as follow:\n\n");
	printf("md [name]\t\tmake a new direction by[name]\n");
	printf("dir\t\t\tshow content in current direction\n");
	printf("cd [name]\t\tchange current direction to destinational direction by [name]\n");
	printf("rd [name]\t\trecursively remove a direction and EVERY files OR directions\n \t\t\twhoese root direction is as input by[name]\n");
	printf("help\t\t\tshow all the instructions\n");
	printf("mf [name]\t\tmake a new file by [name]\n");
	printf("rf [name]\t\tread file in current direction by [name]\n");
	printf("cg [name]\t\tchange a file to new type by [name][type]\n");
	printf("wf [name] [buf]\t\twrite to file by[name] and [buf]\n");
	printf("del [name]\t\tdelete a file by[name]\n");
	printf("for some instruction you should input  parameter to make them done,for example:\n");
	printf("cd doc \n");
	printf("this instruction try to change current direction to 'doc'\n");

}

void CMD_MakeFile(char name[]){

	if (checkValid(name)){
		HFS_create_file(name,NORMAL_FILE);
	}
}

void CMD_Change(char name[], char attribute){

	if (checkValid(name)){
		HFS_change(name, attribute);
	}
}

void CMD_ReadFile(char name[],int length){

	if (checkValid(name)){
		if (length > BLOCK_SIZE){
			printf("parameter:length is too large,please input less then%d", BLOCK_SIZE);
			return;
		}
		vfile *file;
		if (!(file=HFS_read_file(name, length)))
			printf("Error: file :%s does not exist",name);
		else{
			printf("%s", file->data);
		}
	}
}

void CMD_WriteFile(char name[], char buf[]){

	if (checkValid(name)){
		vfile* file = HFS_write_file(name, buf, strlen(buf));
		if (!file){
			printf("Error:file %s does not exist", name);
			return;
		}
		else{
			printf("%s", file->data);
		}
	}
}

void CMD_DEL(char name[]){

	if (checkValid(name)){
		HFS_delete_file(name);
	}
}

void CMD_ERR(){
	printf("NO SUCH INSTRUCTION!!!\n");
}

bool checkValid(char name[]){

	if (strlen(name) > LEN_FILE_NAME ){
		printf("Please input less than 3 char\n");
		return false;
	}
	for (int i = 0; i < strlen(name); i++){
		if(! (name[i] == '$' || name[i] == '.' || name[i] == '/' ||
			('0' <= name[i] && name[i] <= '9') ||
			('a' <= name[i] && name[i] <= 'z') ||
			('A' <= name[i] && name[i] <= 'Z'))){
			printf("Please input letter,number ,'$', '/' or '/'\n");
			return false;
		}
	}
	return true;
}

bool addName(char dst[], char dir[], char name[], char type[]){
	
	int end = strlen(dir);
	if (end + 6 >= 20)
		return false;
	strcpy(dst, dir);
	memcpy(dst + end, name,strlen(name));
	end += strlen(name);
	if (!memcmp(type, FILE_TYPE, 2)){
		/*for file*/
		dst[end] = '.';
		end++;
		memcpy(dst + end, type, 2);
		end += 2;
		dst[end] = '\0';
	}
	else{
		/*for dir*/
		dst[end] = '/';
		dst[end + 1] = '\0';
	}
	
	return true;
}

char ins_judge(char args[]){

	if (!strcmp(args, "md"))
		return MD;
	if (!strcmp(args, "dir"))
		return DIR;
	if (!strcmp(args, "rd"))
		return RD;
	if (!strcmp(args, "cd"))
		return CD;
	if (!strcmp(args, "help"))
		return HELP;
	if (!strcmp(args, "makefile"))
		return Make_File;
	if (!strcmp(args, "read"))
		return Read_File;
	if (!strcmp(args, "write"))
		return Write_File;
	if (!strcmp(args, "del"))
		return DEL;
	return ERR;
}

void toB(char n, char s[]){

	for (int i = 0; i < 8; i++){
		if ((1 << (7 - i)&n) != 0){
			s[i] = '1';
		}
		else
			s[i] = '0';
	}
}

char *getFileName(char name[]){
	int len = strlen(name);
	return name + len - 6;
}

void pushBuf(char* val, int len, pointer p){

	out(p.bnum, p.dnum, len,val);
	return;
}
void popBuf(char* val, int len, pointer p){

	in(p.bnum,p.dnum,len,val);
	return;
}
int nameLen(char name[]){
	int i = 0;
	for (; i < LEN_FILE_NAME; i++){
		if (name[i] == ' ')
			return i;
	}
	return  LEN_FILE_NAME;
}

