#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#define	ROOT_DIR		2
#define	FAT_SIZE			2
#define	BLOCK_SIZE		64
#define	DISK_SIZE		128
#define	BLOCK_BEGIN		2
#define	FILE_END				-1
#define	DESTORY			254
#define	FREE					0

#define	LEN_FILE_NAME	3
#define	LEN_FILE_TYPE		2
#define	LEN_FILE_LENGTH	1

/*flag*/
#define	WRITE					0
#define	READ					1
#define	NO_EXIST				-1

/*type*/
#define	FILE_TYPE				"fl"
#define	DIR_TYPE				"  "

/*attribute*/
#define	READ_ONLY			1
#define	SYS_FILE				2
#define	NORMAL_FILE		4
#define	DIRECTION			8


#define	MAX_OPEN			5
#define	BUF_SIZE				64
#define	DIR_SIZE				64

/*instructions for cmd*/
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
#define	RESTART				12
#define	ERR						-1


/*hardware	data*/
typedef struct{
	char fileName[LEN_FILE_NAME];
	char fileType[LEN_FILE_TYPE];
	char attribute;//文件的属性字段
	char blockId;
	char fileLength;
}inode;

typedef struct {
	inode data[BLOCK_SIZE/sizeof(inode)];
}block;

typedef struct  {
	block blocks[DISK_SIZE];
} _hd ;


/*hardware	API*/
void saveDisk();
bool hardDisk_init();
bool out(int id, char buffer[]);
bool out(int id, int begin ,int len,char buffer[]);
bool in(int id, char buffer[]);
bool in(int id, int begin, int len,char buffer[]);



/*file system datastruct:
	FAT
*/
typedef struct
{
	int  bnum;				 //磁盘盘块号
	int  dnum;                  //磁盘盘块内第几个字节        
}pointer;                    //已打开文件表中读、写指针的结构

typedef struct{
	block* blocks[DISK_SIZE];
	char next[DISK_SIZE];
}FAT;

/*file system datastruct:
	open file table
*/
typedef struct
{
	char  name[20];          //文件绝对路径名
	char  attribute;;          //文件的属性，用1个字节表示，所以此用char类型
	int number;       //文件起始盘块号
	int  length;                //文件长度，文件占用的字节数
	int  flag;                  //操作类型，用“0”表示以读操作方式打开文件，用“1”表示以写操作方式打开文件
	pointer  read;        //读文件的位置，文件打开时bnum为文件起始盘块号，dnum为“0”
	pointer  write;      //写文件的位置，文件刚建立时bnum为文件起始盘块号，dnum为“0，打开文件时bnum和dnum为文件的末尾位置
}openFile;          //已打开文件表项类型定义

/*file system datastruct
vfile,vdir
*/
typedef struct{
	openFile fileInfo;
	char* data;

}vfile;



void initVfile(vfile* file);

typedef struct{
	openFile fileInfo;
	inode* data;
	int cnt;

}vdir;

void initVdir(vdir* dir);


typedef struct
{
	vfile file[MAX_OPEN];    //已打开文件登记表
	int  length;          //已打开文件登记表中登记的文件数量
}openFileTable;              //已打开文件登记表定义


/*file system datastruct
	my file system
*/

typedef struct{
	char write_buf[BUF_SIZE];
	char read_buf[BUF_SIZE];
	FAT fat;
	openFileTable OPT;
} _hfs;

/*file system API:
	for file
*/
bool HFS_install();
bool HFS_init();
bool FAT_init();
bool OPT_init();
void save_Fat();
void showFat();

void HFS_restart();
bool HFS_create_file(char name[], char attribute);
vfile* HFS_open_file(char name[], char type);
vfile*  HFS_read_file(char name[], int length);
vfile*  HFS_write_file(char name[], char buf[], int length);
bool HFS_close_file(char name[]);
bool HFS_delete_file(char name[]);
bool HFS_typefile(char name[]);
bool HFS_change(char name[], char attribute);
vfile* checkOpen(char name[], int opt_type);
char *getFileName(char name[]);
bool  addName(char dst[],char dir[], char name[], char type[]);

/*file system API:
	for dir
*/

void HFS_show_dir();
bool HFS_delete_dir(char name[]);
bool HFS_DFS(char name[], int blockId,int length,bool flag);// flag :0 for check ,1for delete
bool HFS_change_dir(char name[]);

/*默认值给dir用，双参数给file用*/
//void increaseFileLength(int cur_blockid,int len=1);
//void decreaseFileLength(int cur_blockid,int len=-1);
void saveFileLength(int cur_blockid);
//void clearEmptyFlag(char name[]);

void saveCur_dir();
int getFatherFileLength();

/*HFS API*/
void pushBuf(char* val, int len,pointer p);
void popBuf(char* val, int len, pointer p);

/*console datasturct*/

typedef struct {

	vdir cur_dir;

}_cmd;

void console();
bool CMD_init();
void CMD_MD(char name[]);
void CMD_DIR();
void CMD_RD(char name[]);
void CMD_CD(char name[]);
void CMD_HELP();
void CMD_MakeFile(char name[]);
void CMD_Change(char name[], char attribute[]);
void CMD_ReadFile(char name[], int length);
void CMD_WriteFile(char name[], char buf[]);
void CMD_DEL(char name[]);
void CMD_HELP();
void CMD_showFat();
void CMD_ERR();
void CMD_ReStart();


bool checkValid(char name[]);
char ins_judge(char args[]);
int checkExist(char* name, int attribute);
int getFreeBlock();
int nameLen(char name[]);
void nameEndSpace(char name[]);
/* change n to binary string*/
void toB(char n, char s[]);
void saveInput(char* s, int len);
//TODO

#endif