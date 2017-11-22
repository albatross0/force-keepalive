#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

int keepalive	= 1;
int keep_idle	= 7200;
int keep_intvl	= 75;
int keep_cnt	= 9;
int (*orig_connect)(int, const struct sockaddr *, socklen_t) = NULL;

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	if (orig_connect == NULL) {
		orig_connect = dlsym(RTLD_NEXT, "connect");
		if (orig_connect == NULL) {
			fprintf(stderr, "Failed to open connect function\n");
			errno = ENOENT;
			return -1;
		}
	}

	int type;
	socklen_t typelen = sizeof(type);

	if (getsockopt(sockfd, SOL_SOCKET, SO_TYPE, &type, &typelen) || type != SOCK_STREAM)
		return orig_connect(sockfd, addr, addrlen);

	char *env_idle, *env_intvl, *env_cnt = NULL;

	env_idle  = getenv("FORCE_KEEPALIVE_IDLE");
	if (env_idle != NULL) {
		int int_env_idle = strtol(env_idle, NULL, 10);
		if (0 < int_env_idle && int_env_idle <= 7200)
			keep_idle = int_env_idle;
	}
	env_intvl = getenv("FORCE_KEEPALIVE_INTVL");
	if (env_intvl != NULL) {
		int int_env_intvl = strtol(env_intvl, NULL, 10);
		if (0 < int_env_intvl && int_env_intvl <= 75)
			keep_intvl = int_env_intvl;
	}
	env_cnt   = getenv("FORCE_KEEPALIVE_CNT");
	if (env_cnt != NULL) {
		int int_env_cnt = strtol(env_cnt, NULL, 10);
		if (0 < int_env_cnt && int_env_cnt <= 9)
			keep_cnt = int_env_cnt;
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *) &keepalive, sizeof(keepalive)))
		fprintf(stderr, "Failed to set SO_KEEPALIVE\n");
	if (setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (void *) &keep_idle, sizeof(keep_idle)))
		fprintf(stderr, "Failed to set TCP_KEEPIDLE\n");
	if (setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (void *) &keep_intvl, sizeof(keep_intvl)))
		fprintf(stderr, "Failed to set TCP_KEEPINTVL\n");
	if (setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, (void *) &keep_cnt, sizeof(keep_cnt)))
		fprintf(stderr, "Failed to set TCP_KEEPCNT\n");

	return orig_connect(sockfd, addr, addrlen);
}
