#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_NAME "/tmp/fd-passing-example"

int main(void)
{
	int exit_code = EXIT_FAILURE;
	int ret;

	// Open up a Unix domain socket...
	int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		perror("socket()");
		goto out;
	}

	// ...and then connect to the server
	struct sockaddr_un addr = {
		.sun_family = AF_UNIX,
		.sun_path = SOCKET_NAME,
	};
	ret = connect(sock_fd, (const struct sockaddr*) &addr, sizeof(addr));
	if (ret < 0) {
		perror("connect()");
		goto out2;
	}

	// We have to receive that one byte the client sent
	char msgbuf = '\0';
	struct iovec iov = {
		.iov_base = &msgbuf,
		.iov_len = 1,
	};

	// Set up the ancillary message header the kernel will use to pass
	// the file descriptor from the client
	char cmsgbuf[CMSG_SPACE(sizeof(int))] = {0};
	struct msghdr msg = {
		.msg_name = NULL,
		.msg_namelen = 0,
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = cmsgbuf,
		.msg_controllen = sizeof(cmsgbuf),
		.msg_flags = 0,
	};
	ret = recvmsg(sock_fd, &msg, 0);
	if (ret < 0) {
		perror("recvmsg()");
		goto out2;
	}

	// Now get the file descriptor from the server
	struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL || cmsg->cmsg_type != SCM_RIGHTS) {
		fprintf(stderr, "No file descriptor rights data\n");
		goto out2;
	}

	int file_fd;
	memcpy(&file_fd, CMSG_DATA(cmsg), sizeof(file_fd));

	// Size of the file
	struct stat st;
	ret = fstat(file_fd, &st);
	if (ret < 0) {
		perror("fstat()");
		goto out3;
	}
	printf("Size of file: %ju\n", (intmax_t) st.st_size);

	// Try to write to the file. If the server opened the file with `O_RDONLY`,
	// `write()` will fail. If the server opened the file with
	// `O_RDWR | O_APPEND`, not only will `write()` succeed, but also our
	// message will be appended to the end of the file.
	ret = write(file_fd, "Hello\n", strlen("Hello\n"));
	if (ret < 0) {
		perror("write()");
		fprintf(stderr, "As you can see, you cannot write to the file\n");
	} else {
		fprintf(stderr, "As you can see, you can write to the file\n");
	}

	exit_code = EXIT_SUCCESS;

out3:
	close(file_fd);
out2:
	close(sock_fd);
out:
	return exit_code;
}
