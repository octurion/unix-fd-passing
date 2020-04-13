#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_NAME "/tmp/fd-passing-example"

int main(int argc, char** argv)
{
	int exit_code = EXIT_FAILURE;
	int ret;
	if (argc < 2) {
		fprintf(stderr, "Usage: %s filename\n", argv[0]);
		goto out;
	}

	// Open up our file. Change the permissions to just O_RDONLY and observe
	// that the client will not be able modify the file in question
	int file_fd = open(argv[1], O_RDWR | O_APPEND);
	if (file_fd < 0) {
		perror("open()");
		goto out;
	}

	// Unix domain sockets will persist in the file system even after
	// closing them
	unlink(SOCKET_NAME);

	// Open up a Unix domain socket...
	int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		perror("socket()");
		goto out2;
	}

	// ...and then bind it to a specific filesystem path
	struct sockaddr_un addr = {
		.sun_family = AF_UNIX,
		.sun_path = SOCKET_NAME,
	};
	ret = bind(sock_fd, (const struct sockaddr*) &addr, sizeof(addr));
	if (ret < 0) {
		perror("bind()");
		goto out3;
	}

	// Listen for incoming client processes
	ret = listen(sock_fd, 2);
	if (ret < 0) {
		perror("listen()");
		goto out3;
	}

	// Only service 2 clients so that the example server terminates
	for (int count = 0; count < 2; count++) {
		// Accept an incoming connection
		int client_fd = accept(sock_fd, NULL, NULL);
		if (client_fd < 0) {
			perror("accept()");
			continue;
		}

		// Now we will send the client our file descriptor. We can't just send
		// zero bytes of data along with our file descriptors, so just send one
		// byte of data.
		struct iovec iov = {
			.iov_base = "\0",
			.iov_len = 1,
		};

		// Set up the ancillary message header the kernel will use to pass
		// the file descriptor to the client.
		char cmsgbuf[CMSG_SPACE(sizeof(file_fd))] = {0};
		struct msghdr msg = {
			.msg_name = NULL,
			.msg_namelen = 0,
			.msg_iov = &iov,
			.msg_iovlen = 1,
			.msg_control = cmsgbuf,
			.msg_controllen = sizeof(cmsgbuf),
			.msg_flags = 0,
		};
		struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_len = CMSG_LEN(sizeof(file_fd));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		// Write the file descriptor we want to pass...
		memcpy(CMSG_DATA(cmsg), &file_fd, sizeof(file_fd));

		// ...and now send it along
		ret = sendmsg(client_fd, &msg, 0);
		if (ret < 0) {
			perror("sendmsg()");
		}

		// Close the connextion
		close(client_fd);
	}
	exit_code = EXIT_SUCCESS;

out3:
	close(sock_fd);
	// Unix domain sockets will persist in the file system even after
	// closing them
	unlink(SOCKET_NAME);
out2:
	close(file_fd);
out:
	return exit_code;
}
