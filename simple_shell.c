/*
 *  Copyright (c) 2017 Hiroshi Yamada <hiroshiy@cc.tuat.ac.jp>
 *
 *  a simple shell
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define BUFSIZE  1024
#define ARGVSIZE 100
#define FILENAME 20

const char rightbrank[] = ">";
const char bar[] = "|";
const char whitespace[] = " \t\r\n\v";

int parsecmd(char *argv[][ARGVSIZE], char **filename, int *length, char *buf, char *ebuf)
{
	char *s;
	int i = 0, j = 0;
	int redirect = 0;
	int pipeline = 0;

	*filename = "";
	*length = 0;
	s = buf;

	while (s < ebuf) {

		while (s < ebuf && strchr(bar, *s)) {
			s++;
			pipeline = 1;
			(*length)++;
		}
		while (s < ebuf && strchr(rightbrank, *s)) {
			s++;
			redirect = 1;
		}
		while (s < ebuf && strchr(whitespace, *s)) s++;
		if (ebuf <= s) return -1;

		if (redirect) {
			*filename = s;
		}
		else {
			if (pipeline) {
				i++;
				j = 0;
				pipeline = 0;
			}
			argv[i][j++] = s;
		}
		while (s < ebuf && !strchr(whitespace, *s)) s++;
		*s = '\0'; 
		s++;
	}
	return 1;
}

void runcmd(char *buf)
{
	char *argv[ARGVSIZE][ARGVSIZE];
	char* filename;
	int length = 0;
	int fd = 0, pp[2];

	memset(argv, 0, sizeof(argv));
	if (parsecmd(argv, &filename, &length, buf, &buf[strlen(buf)]) > 0)
	{
		if (length > 0) {
			for (int i = 0; i < length; ++i) {
				if (pipe(pp) < 0) {
					perror("pipe");
					exit(-1);
				}
				if (fork() == 0) {
					//printf("child\n");
					close(0);
					close(pp[1]);
					dup(pp[0]);
					close(pp[0]);
					if (strcmp(filename, "") != 0) {
						fd = open(filename, O_CREAT | O_WRONLY, 0666);
						close(1);
						dup(fd);
						close(fd);
					}
					if (execvp(argv[i+1][0], argv[i+1]) < 0)
						perror("execvp");
				}
				else {
					//printf("parent\n");
					close(1);
					close(pp[0]);
					dup(pp[1]);
					close(pp[1]);
					if (execvp(argv[i][0], argv[i]) < 0)
						perror("execvp");
				}
			}
		}
		else {
			if (strcmp(filename, "") != 0) {
				fd = open(filename, O_CREAT | O_WRONLY, 0666);
				if (fd == -1) perror("open");
				close(1);
				dup(fd);
				close(fd);
			}
			if (execvp(argv[0][0], argv[0]) < 0)
				perror("execvp");
		}
	}
	exit(-1);
}

int getcmd(char *buf, int len)
{
	printf("> ");
	fflush(stdin);

	memset(buf, 0, len);
	fgets(buf, len, stdin);

	if (buf[0] == 0)
		return -1;
	return 0;
}

int main(int argc, char**argv)
{
	static char buf[BUFSIZE];

	while(getcmd(buf, BUFSIZE) >= 0) {
		if (fork() == 0)
			runcmd(buf);
		wait(NULL);
	}

	exit(0);
}
