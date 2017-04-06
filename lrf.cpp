#include <iostream>
#include <cstring>

extern "C" {
#include <stdio.h>   /* Standard input/output definitions */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>  /* String function definitions */
#include <fcntl.h>   /* File control definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <errno.h>
}

#include <sys/time.h>

using namespace std;

#define LRF_DEVICE "/dev/ttyUSB0"
#define BAUTRATE B9600
#define BUFSIZE 16

int fd = 0;

int setup()
{
	int tries;
	char buf[0];

	struct termios tio;

	fd = open(LRF_DEVICE, O_RDWR | O_SYNC);
	if (fd < 0) {
		cerr << "Open device failed: " << LRF_DEVICE << endl;
		return -EIO;
	}

	tcflush(fd, TCIOFLUSH);
	tcgetattr(fd, &tio);

	tio.c_cflag &= ~(CSIZE | PARENB | CRTSCTS);
	tio.c_cflag |= (CS8 | CLOCAL | CREAD);

	cfsetispeed(&tio, BAUTRATE);
	cfsetospeed(&tio, BAUTRATE);

	tio.c_oflag = 0;
	tio.c_lflag = 0;
	//tio.c_lflag &= ~(ICANON);

	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 0;

	tcsetattr(fd, TCSANOW, &tio);
	tcflush(fd, TCIOFLUSH);

	cout << "Waiting for the LRF..." << endl;
	usleep(1000000);

	for (tries = 0; tries < 5; tries++) { 
		usleep(1000000);
		if (write(fd, "U", 1) != 1)
			continue;
		else {
			cout << "Successfully send \"U\" to LRF to start..." << endl;
			break;
		}
	}

	if (tries == 5)	{
		cout << "Parallax LRF is not ready ?" << endl;
		return -EIO;
	}

	/* read characters into our buffer until we get a ":" */
	cout << "Waiting a \":\" from LRF ..." << endl;

	memset(buf, 0, 1);
	while (strncmp(buf, ":", 1)) {
		read(fd, buf, 1);
		cout << "Read back a \"" << buf[0] << "\"" << endl;
	}

	usleep(5000);
	cout << "Parallax LRF is Ready !" << endl;

	return 0;
}

int main(int argc, char* argv[])
{
	char lrfData[BUFSIZE];
	char offset;
	struct timeval begin_t, end_t;
	long elapse;

	if (setup())
		return -EIO;

	while(1) {
		gettimeofday(&begin_t, NULL);

		write(fd, "R", 1);

		offset = 0;
		memset(lrfData, 0, BUFSIZE);
		while (read(fd, lrfData + offset, 1)) {
			if (!strncmp(lrfData + offset, ":", 1))
				break;

			offset++;

			if (offset >= BUFSIZE)
				offset = 0;
		}

		lrfData[offset] = 0;
		
		gettimeofday(&end_t, NULL);
		elapse = (long)(end_t.tv_sec - begin_t.tv_sec) * 1000000 +
					(long)(end_t.tv_usec - begin_t.tv_usec);

		cout << string(lrfData) << endl;
		cout << "time in one loop: " << elapse/1000 << " ms" << endl;
	}

	return 0;
}

