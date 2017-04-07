#include <iostream>
#include <cstring>

extern "C" {
#include <stdio.h>   /* Standard input/output definitions */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>  /* String function definitions */
#include <fcntl.h>   /* File control definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <pthread.h>
#include <signal.h>
}

#include <sys/time.h>

#include "lrf.h"

using namespace std;

bool killed = 0;

lrf_device::lrf_device()
{
	fd = 0;
	sample_time = 0;
	stopped = 0;
}

lrf_device::~lrf_device()
{
	if (fd) {
		close(fd);
		fd = 0;
	}
}

int lrf_device::get_sampling_rate(void)
{
	return sample_time;
}

bool lrf_device::setup(void)
{
	int tries;
	char buf[0];
	struct termios tio;

	if (fd)
		return true;

	fd = open(LRF_DEVICE, O_RDWR | O_SYNC);
	if (fd < 0) {
		cerr << "Open device failed: " << LRF_DEVICE << endl;
		return false;
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
		return false;
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

	return true;
}

int lrf_device::stop(void)
{
	stopped = 1;

	return 0;
}

int lrf_device::start(void)
{
	stopped = 0;

	return 0;
}

int lrf_device::is_stopped(void)
{
	return stopped;
}

int lrf_device::get_distance_once(void)
{
	char offset;
	struct timeval begin_t, end_t;

	gettimeofday(&begin_t, NULL);

	write(fd, "R", 1);

	offset = 0;
	memset(lrf_data, 0, BUFSIZE);

	while (read(fd, lrf_data + offset, 1)) {
		if (!strncmp(lrf_data + offset, ":", 1))
			break;

		offset++;

		if (offset >= BUFSIZE)
			offset = 0;
	}

	lrf_data[offset] = 0;

	gettimeofday(&end_t, NULL);

	sample_time = (long)(end_t.tv_sec - begin_t.tv_sec) * 1000 +
		(long)(end_t.tv_usec - begin_t.tv_usec)/1000;

}

int lrf_device::get_distance_loop(void)
{
	return 0;
}

void catcher(int s){
	cout << "Caught signal :" << s << endl;
	killed = true;

	return;
}

int main(int argc, char* argv[])
{
	struct sigaction sigact;

	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = catcher;
	sigaction(SIGINT, &sigact, NULL);

	lrf_device *lrf_dev = new lrf_device();

	if (lrf_dev->setup() == false)
		return -1;

	while (!killed) {
		while (lrf_dev->is_stopped()) {
			usleep(1000);
		}

		lrf_dev->get_distance_once();

		cout << string(lrf_dev->lrf_data) << endl;
		cout << "sample time: " << lrf_dev->get_sampling_rate() << " ms" << endl;
	}

	delete(lrf_dev);

	return 0;
}

