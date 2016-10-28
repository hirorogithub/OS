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
	char attribute;//�ļ��������ֶ�
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
	int  bnum;				 //�����̿��
	int  dnum;                  //�����̿��ڵڼ����ֽ�        
}pointer;                    //�Ѵ��ļ����ж���дָ��Ľṹ

typedef struct{
	block* blocks[DISK_SIZE];
	char next[DISK_SIZE];
}FAT;

/*file system datastruct:
	open file table
*/
typedef struct
{
	char  name[20];          //�ļ�����·����
	char  attribute;;          //�ļ������ԣ���1���ֽڱ�ʾ�����Դ���char����
	int number;       //�ļ���ʼ�̿��
	int  length;                //�ļ����ȣ��ļ�ռ�õ��ֽ���
	int  flag;                  //�������ͣ��á�0����ʾ�Զ�������ʽ���ļ����á�1����ʾ��д������ʽ���ļ�
	pointer  read;        //���ļ���λ�ã��ļ���ʱbnumΪ�ļ���ʼ�̿�ţ�dnumΪ��0��
	pointer  write;      //д�ļ���λ�ã��ļ��ս���ʱbnumΪ�ļ���ʼ�̿�ţ�dnumΪ��0�����ļ�ʱbnum��dnumΪ�ļ���ĩβλ��
}openFile;          //�Ѵ��ļ��������Ͷ���

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
	vfile file[MAX_OPEN];    //�Ѵ��ļ��ǼǱ�
	int  length;          //�Ѵ��ļ��ǼǱ��еǼǵ��ļ�����
}openFileTable;              //�Ѵ��ļ��ǼǱ���


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

/*Ĭ��ֵ��dir�ã�˫������file��*/
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