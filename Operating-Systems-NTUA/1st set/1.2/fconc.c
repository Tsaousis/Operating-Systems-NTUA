#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void doWrite(int fd, const char *buff, int len){
        size_t idx=0;
        ssize_t wcnt; /*writecount*/
        do {
                wcnt=write(fd, buff+idx, len-idx);
                if (wcnt==-1) {/*error*/
                        perror("write");
                        exit(1);
                }
                idx+=wcnt;
        } while (idx<len); /*while index<length do that, cause random prob might                                come up*/
}

void write_file(int fd, const char *infile){
        char buff[1024];
        ssize_t rcnt;
        int fdopen=open(infile,O_RDONLY); /*kanonika einai open("filename")*/
        if (fdopen==-1){ /*error*/
                perror(infile);
                exit(1);
        }
        while ((rcnt=read(fdopen, buff, sizeof(buff)-1))>0){
                if (rcnt==0) /*EOF*/
                         exit(0);
                if (rcnt==-1){ /*error*/
                        perror("read");
                        exit(1);
                }
                buff[rcnt]='\0'; /*diavazei mexri edw h strlen(buff)*/
                doWrite(fd, buff, strlen(buff));
        }
        close(fdopen);
}

int main(int argc, char **argv){
        int fd, oflags=O_CREAT | O_WRONLY | O_TRUNC, mode = S_IRUSR | S_IWUSR;
        if ((argc<3) || (argc>4)) {
                perror("Error-> Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]");
                exit(1);
        }
        else if (argc==3){
                fd=open("fconc.out", oflags, mode);
                if (fd==-1){
                        perror("fconc.out");
                        exit(1);
                }
        }
        else { /*if argc==4*/
                fd=open(argv[3], oflags, mode);
                if (fd==-1){
                        perror(argv[3]);
                        exit(1);
                }
        }
        write_file(fd,argv[1]);
        write_file(fd,argv[2]);
        close(fd);

        return 0;
}
