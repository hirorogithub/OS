#include"FileSystem.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
/* TODO
����ĸ�Դ��Ŀ¼�ṹ��Ϣ�ķֲ����
��Ҫ��ƺ����ݽṹ������Ŀ¼��Ϣ��Ψһ��
Ŀǰ��׼ȷ��Ŀ¼��Ϣ�Ǵ���ڸ�inode�ĸ��ڵ��ϵģ�����inode�����
���ڵ���Ϣ�ǹ��ڵģ�
���������
Ŀ¼��Ϣ�����ڵ��⣬ֻ�����ڸ��ڵ��ϣ�����ʱ��ȡ���ڵ��ϵ�����
������һ����Ϣ���
��CMD��������Ϣ�Ļ��棬cd��ʱ��Űѵ�ǰĿ¼��Ϣ���浽������disk��


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
	
	FILE* fp;
	if ((fp = fopen("disk.img", "rb")) == NULL){
		return true;
	}
	fread(&hardDisk, sizeof(hardDisk), 1, fp);
	fclose(fp);
	return true;
}

void saveDisk(){

	FILE* fp;
	if ((fp = fopen("disk.img", "wb")) == NULL){
		printf("Error:save disk faile!\n");
		fclose(fp);
		return;
	}
	fwrite(&hardDisk, sizeof(hardDisk), 1, fp);
	fclose(fp);
	return;

}

void showFat(){

	printf("0:FREE\t1:OCCUPY\n");
	printf("  ");
	for (int i = 0; i < 16; ++i)
		printf("%x ", i);
	printf("\n");
	for (int i = 0; i < DISK_SIZE / 16; ++i){
		printf("%x ", i);
		for (int j = 0; j < 16; ++j){
			printf("%d ", HFS.fat.next[i * 16 + j] == FREE ? 0 : 1);
		}
		printf("\n");
	}
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

void save_Fat(){

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
	for (int i = 0; i < MAX_OPEN; ++i){
		HFS.OPT.file[i].data = (char*)malloc(sizeof(BLOCK_SIZE));
		//memset(HFS.OPT.file[i].data, 0, BLOCK_SIZE);
	}
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
	if (checkExist(name, attribute) != NO_EXIST)
		return false;

	/*check disk is full or not*/
	int blockid = getFreeBlock();
	if (blockid == NO_EXIST)
		return false;

	HFS.fat.next[blockid] = FILE_END;

	/*init vfile info*/
	/*��ͷ���������Ƕ������Ϣ���
	����ע�͵�
	vfile temp ;

	if (!addName(temp.fileInfo.name, CMD.cur_dir.fileInfo.name, name, FILE_TYPE))
		return false;
	temp.fileInfo.attribute = attribute&NORMAL_FILE;
	temp.fileInfo.flag = 0;
	temp.fileInfo.length = BLOCK_SIZE;
	temp.fileInfo.number = blockid;
	*/
	char data [BLOCK_SIZE];
	memset(data, 0, BLOCK_SIZE);
	memcpy(data, name, nameLen(name));
	memcpy(data + nameLen(name),".fl\n",4);
	

	/*pointer tempp = *(pointer*)malloc(sizeof(pointer));
	tempp.bnum = blockid;
	tempp.dnum = 0;*/


	pointer write = { blockid, 0 };


	/*update CMD*/
	/*update current direction */
	memcpy(CMD.cur_dir.data[CMD.cur_dir.cnt].fileName, name, LEN_FILE_NAME);
	CMD.cur_dir.data[CMD.cur_dir.cnt].fileLength = nameLen(name)+4;
	memcpy(CMD.cur_dir.data[CMD.cur_dir.cnt].fileType, FILE_TYPE, 2);
	CMD.cur_dir.data[CMD.cur_dir.cnt].blockId = blockid;
	CMD.cur_dir.data[CMD.cur_dir.cnt].attribute = attribute;
	//increaseFileLength(CMD.cur_dir.fileInfo.number);
	CMD.cur_dir.cnt++;
	CMD.cur_dir.fileInfo.length++;
	CMD.cur_dir.data[0].fileLength++;
	//clearEmptyFlag(CMD.cur_dir.fileInfo.name);

	/*no more save CMD.cur_dir.data[] in disk*/
	//pushBuf((char*)(CMD.cur_dir.data), BLOCK_SIZE, CMD.cur_dir.fileInfo.write);
	pushBuf(data, BLOCK_SIZE, write);
	save_Fat();

	//free(&tempp);
	saveDisk();
	return true;

}

vfile* HFS_open_file(char name[], char opt_type){

	/* if READ_ONLY or not*/
	//if (opt_type&READ)
	//	return NULL;

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
		if (!memcmp(temp, name,nameLen(name)))
			return NULL;
	}

	/*create a vfile and fill openfile table*/
	vfile* temp = &(HFS.OPT.file[HFS.OPT.length]);
	if(!addName(temp->fileInfo.name, CMD.cur_dir.fileInfo.name, name, FILE_TYPE)){
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
										
	popBuf(temp->data, BLOCK_SIZE, temp->fileInfo.read);
	
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
	memcpy(file->data + file->fileInfo.length, buf, length + 1);
	file->fileInfo.length += length;
	int dirIndex = checkExist(name, NORMAL_FILE);
	CMD.cur_dir.data[dirIndex].fileLength += length;

	pushBuf(file->data, file->fileInfo.length, file->fileInfo.write);
	//increaseFileLength(CMD.cur_dir.fileInfo.number,length);
	//CMD.cur_dir.fileInfo.length++;
	return file;
}

bool HFS_close_file(char name[]){

	/*check if opened*/
	int fileIndex = 0;
	char* temp = getFileName(HFS.OPT.file[fileIndex].fileInfo.name);
	for (; fileIndex < HFS.OPT.length; fileIndex++){
		 if (!memcmp(temp, name,nameLen(name)))
			break;
	}

	vfile *file;
	if (fileIndex == HFS.OPT.length)
		return true;
	else
		file =&( HFS.OPT.file[fileIndex]);

	/*����Ŀǰû�����������ļ����������´�����ʱ����
	if (file->fileInfo.flag == WRITE){
		file->data = (char*)realloc(file->data, file->fileInfo.length + 1);
		file->data[file->fileInfo.length] = '#';
		file->fileInfo.length++;
	}
	*/
	
	/*save to disk when close*/
	//pushBuf(file->data, file->fileInfo.length,file->fileInfo.write);

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
	char* temp;
	for (; fileIndex < HFS.OPT.length; fileIndex++){
	    temp = getFileName(HFS.OPT.file[fileIndex].fileInfo.name);
		if (!memcmp(temp, name,nameLen(name)))
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
	for (int i = dirIndex; i < BLOCK_SIZE/sizeof(inode)-1; i++)
		CMD.cur_dir.data[i] = CMD.cur_dir.data[i + 1];
	
	CMD.cur_dir.cnt--;
	CMD.cur_dir.fileInfo.length--;
	CMD.cur_dir.data[0].fileLength--;
	//decreaseFileLength(CMD.cur_dir.fileInfo.number);

	/*save Fat in Disk*/
	saveDisk();
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
		if (!memcmp(tempName, name,nameLen(name)))
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
		if (!memcmp(temp, name,nameLen(name)))
			break;
	}
	if (fileIndex != HFS.OPT.length)
		return false;

	CMD.cur_dir.data[dirIndex].attribute |= attribute;
	saveDisk();
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

	/*update current direction */
	memcpy(CMD.cur_dir.data[CMD.cur_dir.cnt].fileName ,name,LEN_FILE_NAME);
	memcpy(CMD.cur_dir.data[CMD.cur_dir.cnt].fileType , DIR_TYPE,2);
	CMD.cur_dir.data[CMD.cur_dir.cnt].fileLength = 1;
	CMD.cur_dir.data[CMD.cur_dir.cnt].blockId = blockid;
	CMD.cur_dir.data[CMD.cur_dir.cnt].attribute = DIRECTION;

	CMD.cur_dir.cnt++;
	CMD.cur_dir.fileInfo.length++;
	CMD.cur_dir.data[0].fileLength++;
	//increaseFileLength(CMD.cur_dir.fileInfo.number);
	//pushBuf((char*)(CMD.cur_dir.data), BLOCK_SIZE, CMD.cur_dir.fileInfo.write);

	/*save new dir in disk*/
	inode temp[BLOCK_SIZE/sizeof(inode)];
	temp[0] = { "", "", DIRECTION,
		CMD.cur_dir.fileInfo.number,//������д���˸�Ŀ¼�Ŀ�ţ�������
		1};
	memcpy(temp[0].fileName,".. ",3);
	memcpy(temp[0].fileType, DIR_TYPE, 2);

	pointer p = { blockid, 0 };
	pushBuf((char*)temp, BLOCK_SIZE, p);


	/*save fat in disk*/
	saveDisk();
	return true;
}

//��ʱ�䣬������ʾ��ǰĿ¼��"."��
void HFS_show_dir(){


	printf("\n"); 
	char string[9];
	string[8] = '\0';
	for (int i = 0; i < CMD.cur_dir.cnt; i++){	

		/*example:
		FILE    LENGTH  NAM.TY  00000101*/
		toB(CMD.cur_dir.data[i].attribute, string);
		if (CMD.cur_dir.data[i].attribute & NORMAL_FILE){
			printf("FILE");
			printf("\t%d\t%c%c%c.%c%c\t%s\n",
			    CMD.cur_dir.data[i].fileLength ,
				CMD.cur_dir.data[i].fileName[0],
				CMD.cur_dir.data[i].fileName[1],
				CMD.cur_dir.data[i].fileName[2],
				CMD.cur_dir.data[i].fileType[0],
				CMD.cur_dir.data[i].fileType[1],
				string);
		}
		else {
			printf("DIR");	
			printf("\t%c\t%c%c%c %c%c\t%s\n",
				i>0 || ((CMD.cur_dir.data[0].blockId == ROOT_DIR) &&
				(CMD.cur_dir.fileInfo.number == ROOT_DIR)) ? CMD.cur_dir.data[i].fileLength + '0' : ' ',
				CMD.cur_dir.data[i].fileName[0],
				CMD.cur_dir.data[i].fileName[1],
				CMD.cur_dir.data[i].fileName[2],
				CMD.cur_dir.data[i].fileType[0],
				CMD.cur_dir.data[i].fileType[1],
				string);
		}

	}
	printf("\n");
}

bool HFS_delete_dir(char name[]){

	/*if dir exist*/
	int dirIndex = checkExist(name, DIRECTION);
	if (dirIndex == NO_EXIST)
		return false;

	if (HFS_DFS(name, CMD.cur_dir.data[dirIndex].blockId, CMD.cur_dir.data[dirIndex].fileLength,false)){
		HFS.fat.next[CMD.cur_dir.data[dirIndex].blockId] = FREE;
	    HFS_DFS(name, CMD.cur_dir.data[dirIndex].blockId, CMD.cur_dir.data[dirIndex].fileLength,true);
	}
	

	/*delect dir info*/
	for (int i = dirIndex; i < BLOCK_SIZE/sizeof(inode)-1; ++i)
		CMD.cur_dir.data[i] = CMD.cur_dir.data[i + 1];

	CMD.cur_dir.cnt--;
	CMD.cur_dir.fileInfo.length--;
	CMD.cur_dir.data[0].fileLength--;
	//decreaseFileLength(CMD.cur_dir.fileInfo.number);
	saveDisk();
	return true;
}

/*
����HFS_delect_dir()�����flag��
true:ɾ�����������ļ�
false����ɾ��
��DFSֻ�����ø��ڵ�������ӽڵ㣬�������Լ�
������Ҫ�ϲ����ɾ������
*/

bool HFS_DFS(char name[], int blockId,int length,bool flag){

	bool b = true;
	inode temp [BLOCK_SIZE/sizeof(inode)];
	pointer p = { blockId, 0 };
	popBuf((char*)(temp), BLOCK_SIZE, p);

	for (int i = 0; i < length; i++){
		if (temp[i].attribute & NORMAL_FILE){
			/*have open?*/
			int fileIndex = 0;
			char* tempName = getFileName(HFS.OPT.file[fileIndex].fileInfo.name);
			for (; fileIndex < HFS.OPT.length; fileIndex++){
				if (!memcmp(tempName, temp[i].fileName, nameLen(temp[i].fileName)))
					return false;
			}
			if (flag == true)
				HFS.fat.next[temp[i].blockId] = FREE;
		}
		else if ((temp[i].attribute & DIRECTION)&&
						memcmp(temp[i].fileName, ".. ",LEN_FILE_NAME)){
			b &= HFS_DFS(temp[i].fileName, temp[i].blockId, temp[i].fileLength,flag);
			if (flag == true)
				HFS.fat.next[temp[i].blockId]=FREE;
		}
	}

	/*save fat in disk*/
	if (flag=true)
		save_Fat();
	return b;

}

bool HFS_change_dir(char name[]){ 
	/*1.save cur.dir.fileInfo to disk*/
	/*2.load dir message from disk*/
	/*2.5 from grandfather get father message*/
	/*3.switch*/
	
	if (!memcmp(name, ".  ",LEN_FILE_NAME)) return true;
	/*if dir exist*/
	int dirIndex = checkExist(name, DIRECTION);
	if (dirIndex == NO_EXIST)
		return false;

	saveCur_dir();

	/*change current dir */
	//CMD.cur_dir.cnt=CMD.cur_dir.fileInfo.length = CMD.cur_dir.data[dirIndex].fileLength;


	if (!memcmp(name, ".. ", LEN_FILE_NAME)){
		/*move back to father*/
		int fileLength = getFatherFileLength();
		CMD.cur_dir.cnt = CMD.cur_dir.fileInfo.length = fileLength;
		int len=strlen(CMD.cur_dir.fileInfo.name);
		CMD.cur_dir.fileInfo.name[len - nameLen(CMD.cur_dir.data[0].fileName)-1] = '\0';	//����Ӧ��ΪnameLen+1�����޸�
	}
	else{
		CMD.cur_dir.cnt = CMD.cur_dir.fileInfo.length = CMD.cur_dir.data[dirIndex].fileLength;
		if (!addName(CMD.cur_dir.fileInfo.name, CMD.cur_dir.fileInfo.name, name, DIR_TYPE)){
			return false;
		}
	}
	CMD.cur_dir.fileInfo.number = CMD.cur_dir.data[dirIndex].blockId;
	CMD.cur_dir.fileInfo.write = CMD.cur_dir.fileInfo.read = { CMD.cur_dir.fileInfo.number, 0 };
	popBuf((char*)CMD.cur_dir.data, BLOCK_SIZE,CMD.cur_dir.fileInfo.read);
	saveDisk();
	return true;
}

vfile* checkOpen(char name[],int opt_type){


	/*haved opened?*/
	int fileIndex = 0;
	char* temp ;
	for (; fileIndex < HFS.OPT.length; fileIndex++){
		temp = getFileName(HFS.OPT.file[fileIndex].fileInfo.name);
		if (!memcmp(temp, name, nameLen(name)))
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
	static int id = 0;
	int temp_len = (DISK_SIZE - ROOT_DIR - 1);
	for (int i = 0; i < temp_len; id = (id + 1) % (DISK_SIZE - ROOT_DIR - 1), i++) {
		if (HFS.fat.next[id + BLOCK_BEGIN + 1] == FREE)
			return id + BLOCK_BEGIN + 1;
	}

	return FILE_END;
}

int checkExist(char* name,int attribute){
	/*if this name is exist*/
	for (int i = 0; i < CMD.cur_dir.cnt; i++){
		//int temp = nameLen(CMD.cur_dir.data[i].fileName);
		if (!memcmp(name, CMD.cur_dir.data[i].fileName, nameLen(CMD.cur_dir.data[i].fileName)) &&
						(CMD.cur_dir.data[i].attribute&attribute))
			return i;
	}
	return NO_EXIST;
}

void console(){
	/*��Ӱ�ȫ������ƣ��޶����볤��*/
#define inputName(x)		scanf_s("%3s",args1,4);fflush(stdin)
	//�ö��嵼�º��������������
	char args0[10] = "\0";
	char args1[10]="\0";
	char args2[BLOCK_SIZE+1] = "\0";
	char args3=0;
	bool exitFlag = false;
	printf("Welcome to Hiro_File_System:\n");
	while (true){
		printf("%s>", CMD.cur_dir.fileInfo.name); //������Ŀ¼�ٻ��˵���Ŀ¼ʱ����������������⣬���޸�����������
		/*2016��10��18�� 22:27:39�������޸�*/
		scanf_s("%4s", args0,5); //��������bug�����޸� 
		/*
			bug1��������ַ�����������䵱�������Դ�
			bug2������ǰ���������й�������ַ������޶����ȣ������ʹ�á�exit���˳�ʱϵͳ�������bug.
				���ִ����ԭ������������ַ���������ĳ��ȣ�ջ����� ͬ�������Ҳ������args3�ϣ�����ת�������뵼�´���
		*/
		switch (ins_judge(args0))
		{
			case	MD:
				inputName(args1); //��������ʹ�ÿո���������
				CMD_MD(args1);
				break;
			case DIR:
				CMD_DIR();
				break;
			case RD:
				inputName(args1);
				CMD_RD(args1);
				break;
			case CD:
				inputName(args1);
				CMD_CD(args1);
				break;
			case HELP:
				CMD_HELP();
				break; 
			case Make_File:
				inputName(args1);
				CMD_MakeFile(args1);
				break;
			case	Read_File:
				inputName(args1);
				//scanf("%d",&args3);
				CMD_ReadFile(args1, BLOCK_SIZE); //�ú��������⣬���ڵ��ļ�����ʾ�����ڡ�����������
				break;
			case	Change:
				inputName(args1);
				scanf_s("%s", &args2,16);
				CMD_Change(args1, args2); //�ú��������⣬�޸��ļ�����ʱ������ļ���ΪĿ¼����������
				break;
			case	Write_File	:
				inputName(args1);
				scanf("%64s",args2,65);
				CMD_WriteFile(args1, args2);
				break;
			case	DEL:
				inputName(args1);
				CMD_DEL(args1);
				break;
			case	EXIT:
				exitFlag = true;
				break;
			case SHOW_FAT:
				CMD_showFat();
				break;
			case	ERR:
				CMD_ERR();
		default:
			break;
		}
		if (exitFlag) break;
	}

}

/*
����Ŀ¼��md��
����Ŀ¼����Ҫ�ҵ�����Ŀ¼��λ�ã���Ŀ¼����Ȼ����Ҹ�Ŀ¼�Ƿ���ڣ������Ŀ
¼�����ڣ����ܽ�����������ڣ������Ƿ����ͬ��Ŀ¼�����ڣ����ܽ����������ڣ����
��һ����Ŀ¼�Ϊ��Ŀ¼����һ���̿飬����дĿ¼���ݡ�
*/
void CMD_MD(char name[]){

	if (checkValid(name)){
		nameEndSpace(name);
		HFS_create_dir(name);
	}
}

/*
��ʾĿ¼���ݣ�dir��
��ʾĿ¼��������Ҫ�ҵ���Ŀ¼�����Ŀ¼�����ڣ�ָ��ִ��ʧ�ܣ�������ڣ�һ��һ
����ʾĿ¼���ݡ�
*/
void CMD_DIR(){

	HFS_show_dir();

}

/*
ɾ����Ŀ¼��rd��
ɾ����Ŀ¼����Ҫ�ҵ���Ŀ¼�����Ŀ¼�����ڣ�ָ��ִ��ʧ�ܣ�������ڣ����Ǹ�Ŀ
¼��ǿ�Ŀ¼����ʾ����ɾ��������ʧ�ܣ����Ƿǿ���Ŀ¼����ɾ����Ŀ¼����ն�Ӧ��
�䡣
*/
void CMD_RD(char name[]){ //����bug�����޸�

	if (checkValid(name)){
		nameEndSpace(name);
		HFS_delete_dir(name);
	}

}

/*
�ı䵱ǰĿ¼
*/
void CMD_CD(char name[]){

	if (!strcmp(name, "..") || !strcmp(name, ".") || checkValid(name)){
		nameEndSpace(name);
		HFS_change_dir(name);
	}
}

void CMD_HELP(){
/*
#define	MD						0
#define	DIR						1
#define	RD						2
#define	CD						3
#define	HELP					4
#define	Make_File				5//make file
#define	Read_File				6//read file
#define	Change				7
#define	Write_File				8
#define	DEL						9
#define	EXIT						10
#define	SHOW_FAT			11
#define	ERR						-1
*/
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
	printf("exit\t\t\texit the cmd\n");
	printf("fat\t\t\tshow the fat\n");
	printf("for some instruction you should input  parameter to make them done,for example:\n");
	printf("cd doc \n");
	printf("this instruction try to change current direction to 'doc'\n");

}

/*
�����ļ�����Ҫ�������Ǽ���ļ�Ŀ¼��ȷ���������ļ���Ѱ�ҿ��еǼ�����еǼǣ�
Ѱ�ҿ��д洢�飨����һ�飩�Ա��洢�ļ���Ϣ�������������Ӧ����д�Ѵ��ļ���
*/
void CMD_MakeFile(char name[]){

	if (checkValid(name)){
		nameEndSpace(name);
		HFS_create_file(name,NORMAL_FILE);
	}
}

/*
�ı��ļ����ԣ����Ȳ��Ҹ��ļ�����������ڣ�������������ڣ�����ļ��Ƿ�򿪣�
�򿪲��ܸı����ԣ�û�д򿪣�����Ҫ��ı�Ŀ¼��������ֵ��
����������ƣ�ֻ�������ֻ����ϵͳ�ļ����ԡ�
*/
void CMD_Change(char name[], char attribute[]){
	if (strcmp(attribute,"read_only") &&strcmp(attribute,"systerm_file")){
		printf("Please input read_only or systerm_file\n");
		return;
	}
	if (checkValid(name)){	
		nameEndSpace(name);
		char temp_attribute = strcmp(attribute, "read_only") ? SYS_FILE : READ_ONLY;
		HFS_change(name, temp_attribute);
	}
}

/*
���ļ���������Ҫ�����ǲ����Ѵ��ļ������Ƿ���ڸ��ļ�����������ڣ���򿪺��ٶ���
Ȼ�����Ƿ����Զ���ʽ���ļ����������д��ʽ���ļ������������
�����Ѵ��ļ����ж�����ָ�룬�����λ���϶�������Ҫ���ȣ������賤��û�ж�����
�������ļ�������������ֹ������ʵ�����á�#����ʾ�ļ�������
*/
void CMD_ReadFile(char name[],int length){
	if (checkValid(name)){
		nameEndSpace(name);
		if (length > BLOCK_SIZE||(length<0)){
			printf("parameter:length is too large or less than 0,please input less then%d", BLOCK_SIZE);
			return;
		}
		vfile *file;
		if (!(file = HFS_read_file(name, length))){
			printf("Error: file :");
			for (int i = 0; i < nameLen(name); ++i)
				printf("%c", name[i]);
			printf(" does not exist\n");
		}
		else{
			printf("%s\n", file->data);
		}
	}
}

void CMD_WriteFile(char name[], char buf[]){

	if (checkValid(name)){
		nameEndSpace(name);
		vfile* file = HFS_write_file(name, buf, strlen(buf));
		if (!file){
			printf("Error:file ");
			for (int i = 0; i < nameLen(name); i++)
				printf("%c", name[i]); 
			printf("does not exist\n");
			return;
		}
		else{
			printf("%s\n", file->data);
		}
	}
}

void CMD_DEL(char name[]){

	if (checkValid(name)){
		nameEndSpace(name);
		HFS_delete_file(name);
	}
}

void CMD_showFat(){
	showFat();
}

void CMD_ERR(){
	printf("NO SUCH INSTRUCTION!!!\t(try \"help\")\n");
}

bool checkValid(char name[]){
	int nameLen = strlen(name);
	if (nameLen > LEN_FILE_NAME ){
		printf("Please input no more than 3 char\n");
		return false;
	}
	//ʹ����ĸ�����ֺͳ���$������.������/���Լ��ո�������ַ�
	for (int i = 0; i < nameLen; i++){
		if ((name[i] == '$' || name[i] == '.' || name[i] == '/' || name[i] == ' ') || (!isascii(name[i]))){
			printf("Please input letter,number,or characters except '$', '.' ,' 'Or '/'\n");
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
	memcpy(dst + end, name,nameLen(name));
	end += nameLen(name);
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
	if (!strcmp(args, "mf"))
		return Make_File;
	if (!strcmp(args, "rf"))
		return Read_File;
	if (!strcmp(args, "cg"))
		return Change;
	if (!strcmp(args, "wf"))
		return Write_File;
	if (!strcmp(args, "del"))
		return DEL;
	if (!strcmp(args, "exit"))
		return EXIT;
	if (!strcmp(args, "fat"))
		return SHOW_FAT;
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
	int i = strlen(name) - 1;
	while (name[i] != '/')
		--i;
	return name+i+1;
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

//void increaseFileLength(int cur_blockid,int len){
//	if (len == 1)
//		++CMD.cur_dir.fileInfo.length;
//	_changeFileLength(cur_blockid, len);
//}
//
//void decreaseFileLength(int cur_blockid,int len){
//	if (len ==- 1)
//		--CMD.cur_dir.fileInfo.length;
//	_changeFileLength(cur_blockid, len);
//}

void saveFileLength(int cur_blockid){

	/*if is not root ,save the info in father dir*/
	if (cur_blockid != ROOT_DIR){
		int fatherId = CMD.cur_dir.data[0].blockId;
		inode temp[BLOCK_SIZE/sizeof(inode)] ;
		pointer p = { fatherId, 0 };
		popBuf((char*)(&temp), BLOCK_SIZE, p);
		for (int i = 1; i < temp[0].fileLength; i++){
			if (temp[i].blockId == cur_blockid){
				temp[i].fileLength=CMD.cur_dir.fileInfo.length;
				break;
			}
		}
		pushBuf((char*)temp, BLOCK_SIZE, p);

	}
}

void saveCur_dir(){

	saveFileLength(CMD.cur_dir.fileInfo.number);
	pushBuf((char*)CMD.cur_dir.data, BLOCK_SIZE, CMD.cur_dir.fileInfo.write);

}

int getFatherFileLength(){

	inode father[BLOCK_SIZE / sizeof(inode)];
	pointer pf = { CMD.cur_dir.data[0].blockId, 0 };
	popBuf((char*)father, BLOCK_SIZE, pf);


	inode grandFather[BLOCK_SIZE / sizeof(inode)];
	pointer pg = { father[0].blockId, 0 };
	popBuf((char*)grandFather, BLOCK_SIZE, pg);

	for (int i = 0; i < (BLOCK_SIZE / sizeof(inode)); i++){
		if (CMD.cur_dir.data[0].blockId== grandFather[i].blockId){
			return grandFather[i].fileLength;
		}
	}

	/*should never go here*/
	return 0;

}

void nameEndSpace(char name[]){
	for (int i = strlen(name); i < LEN_FILE_NAME; ++i){
		name[i] = ' ';
	}
}
