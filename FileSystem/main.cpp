#include "FileSystem.h"
#include <stdio.h>

int main(){

	HFS_install();
	HFS_init();
	CMD_init();
	console();
}