#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>

#define MAX_NAME_LEN	(256)
#define MAX_FILE	(1024*1024*10)
#define MAX_DIR		(128*1024)
#define MAX_FILE_SIZE	(160*1024*1024)
#define LEN_REC		(320)
#define MAX_PARTITION	(2048)
#define MAX_DIR_ENTRY_LEN	(64)
#define MAX_ENTRY_PER_DIR	(65536)

int nFile=0, nFile_BCast=0, nPartition=1;
int nDir=0;


char szDirName[MAX_DIR][MAX_NAME_LEN];
char szFileName[MAX_FILE][MAX_NAME_LEN];
struct stat stat_list[MAX_FILE];
int nFile_in_Partition[MAX_PARTITION];
unsigned char szData[MAX_FILE_SIZE];
char szEntryList[MAX_ENTRY_PER_DIR][MAX_DIR_ENTRY_LEN];

int main(int argc, char *argv[])
{
	int i, j, fd, fd_rd, fd_bcast, nLen, nLen_stat, nLen_File_Name, nFile_per_Partition, Partition_Max, offset, idx, nFileLeft;
	FILE *fIn, *fDir;
	char szLine[512], *ReadLine, *szBuff, szOutputName[256], szInput[256], szBCastTag[64];
	struct stat file_stat;

	if( (argc < 3) || (argc > 4) )	{
		printf("Usage: prep n_partition list [bcast_dir]\n");
		exit(1);
	}
	szBCastTag[0] = 0;

	nPartition = atoi(argv[1]);
	strncpy(szInput, argv[2], 256);
	if(argc == 4)	strncpy(szBCastTag, argv[3], 63);

	nLen_stat = sizeof(struct stat);
	nLen_File_Name = LEN_REC - nLen_stat;
	printf("LenStat = %d MAX_LEN_FILE = %d\n", nLen_stat, nLen_File_Name);

	fIn = fopen(szInput, "r");
	if(fIn == NULL)	{
		printf("Fail to open file %s\n", szInput);
		exit(1);
	}

	if(szBCastTag[0])	{
		fd_bcast = open("fs_bcast", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
		if(fd_bcast == -1)	{
			printf("Fail to open file %s to write.\nQuit\n", "fs_bcast");
			exit(1);
		}
		write(fd_bcast, &nFile_BCast, sizeof(int));	// will be revised later
	}

	fDir = fopen("dir.list", "w");

	while(1)	{
		if(feof(fIn))	{
			break;
		}
		ReadLine = fgets(szLine, 256, fIn);
		if(ReadLine == NULL)	{
			break;
		}
		nLen = strlen(szLine);
		if(szLine[nLen-1] == 0xA)	{
			szLine[nLen-1] = 0;
			nLen--;
		}
		if(szLine[nLen-2] == 0xD)	{
			szLine[nLen-2] = 0;
			nLen--;
		}
		if(strcmp(szLine, ".") == 0)	{	// ignore
		}
		else if(strcmp(szLine, "dir.list") == 0)	{	// this file is generated by my code.
		}
		else {
			stat(szLine, &file_stat);
			if( (file_stat.st_mode & S_IFMT) == S_IFDIR )	{	// directory
				if( strncmp(szLine, "./", 2) == 0 )	{
					fprintf(fDir, "%s\n", szLine+2);
					strcpy(szDirName[nDir], szLine+2);
				}
				else	{
					fprintf(fDir, "%s\n", szLine);
					strcpy(szDirName[nDir], szLine);
				}
				nDir++;
			}
//			else	{
				if(szBCastTag[0] && strstr(szLine, szBCastTag) )	{	// a file will be broadcasted to all nodes. Now dir is accounted as a file too!!!
					fd_rd = open(szLine, O_RDONLY);
					if(fd_rd == -1)	{
						printf("Fail to open file: %s\nQuit\n", szLine);
					}
					read(fd_rd, szData, file_stat.st_size);
					close(fd_rd);

					if( strncmp(szLine, "./", 2) == 0 ) 
						write(fd_bcast, szLine+2, nLen_File_Name);
					else
						write(fd_bcast, szLine, nLen_File_Name);

					write(fd_bcast, &(file_stat.st_size), sizeof(long int));
					write(fd_bcast, szData, file_stat.st_size);

					nFile_BCast++;
				}
				else	{
					if( strncmp(szLine, "./", 2) == 0 ) 
						strcpy(szFileName[nFile], szLine+2);
					else
						strcpy(szFileName[nFile], szLine);
					memcpy(&(stat_list[nFile]), &file_stat, nLen_stat);
					nFile++;
				}
//			}
		}
	}

	lseek(fd_bcast, 0, SEEK_SET);
	write(fd_bcast, &nFile_BCast, sizeof(int));

	fclose(fDir);
	fclose(fIn);
	close(fd_bcast);

	printf("Found %d files to scatter and %d files to broadcast.\n", nFile, nFile_BCast);

	if(nFile%nPartition == 0)	{
		nFile_per_Partition = nFile/nPartition;
	}
	else	{
		nFile_per_Partition = nFile/nPartition + 1;
	}

	Partition_Max = nFile/nFile_per_Partition;
	for(i=0; i<Partition_Max; i++)	{
		offset = nFile_per_Partition * i;
		sprintf(szOutputName, "fs_%d", i);
		fd = open(szOutputName, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
		write(fd, &nFile_per_Partition, sizeof(int));	// number of files in this partition

		for(j=0; j<nFile_per_Partition; j++)	{
			idx = offset + j;

			fd_rd = open(szFileName[idx], O_RDONLY);
			if(fd_rd == -1)	{
				printf("Fail to open file: %s\nQuit\n", szFileName[idx]);
			}

			read(fd_rd, szData, stat_list[idx].st_size);
			close(fd_rd);

			write(fd, szFileName[idx], nLen_File_Name);
			write(fd, &(stat_list[idx]), nLen_stat);
			write(fd, szData, stat_list[idx].st_size);
		}

		close(fd);

		printf("Finished partition %d.\n", i);
	}

	if(nFile%nFile_per_Partition)	{
		offset = nFile_per_Partition * Partition_Max;
		nFileLeft = nFile%nFile_per_Partition;

		sprintf(szOutputName, "fs_%d", Partition_Max);
		fd = open(szOutputName, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
		write(fd, &nFileLeft, sizeof(int));	// number of files in this partition

		for(j=0; j<nFileLeft; j++)	{
			idx = offset + j;

			fd_rd = open(szFileName[idx], O_RDONLY);
			if(fd_rd == -1)	{
				printf("Fail to open file: %s\nQuit\n", szFileName[idx]);
			}

			read(fd_rd, szData, stat_list[idx].st_size);
			close(fd_rd);

			write(fd, szFileName[idx], nLen_File_Name);
			write(fd, &(stat_list[idx]), nLen_stat);
			write(fd, szData, stat_list[idx].st_size);
		}

		close(fd);
		printf("Finished partition %d.\n", Partition_Max);
	}

	int nEntry;
	DIR *dp;
	struct dirent *ep;

	fd = open("fs_dirinfo", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
	if(fd == -1)	{
		printf("Fail to open file fs_dirinfo\nQuit\n");
		exit(1);
	}
	write(fd, &nDir, sizeof(int));
	for(i=0; i<nDir; i++)	{
		nEntry = 0;
		
		dp = opendir(szDirName[i]);
		if (dp != NULL)	{
			while (ep = readdir (dp))	{
				if( (strcmp(ep->d_name, ".")==0) || (strcmp(ep->d_name, "..")==0) )	{	// skip
				}
				else	{
					strcpy(szEntryList[nEntry], ep->d_name);
					nEntry++;
				}
//				printf("%s\n", ep->d_name);
			}
			closedir(dp);
			write(fd, szDirName[i], nLen_File_Name);	// full dir name
			write(fd, &nEntry, sizeof(int));
			write(fd, szEntryList[0], nEntry*MAX_DIR_ENTRY_LEN);
		}
		else
			perror ("Couldn't open the directory");
	}
	close(fd);
}
