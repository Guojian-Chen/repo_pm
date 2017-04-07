#ifndef __PARALLAX_LRF_H__
#define __PARALLAX_LRF_H__

#define LRF_DEVICE "/dev/ttyUSB0"
#define BAUTRATE B9600
#define BUFSIZE 16

class lrf_device {

		int fd;
		long sample_time;
		int stopped;

	public:
		char lrf_data[BUFSIZE];

		lrf_device();
		~lrf_device();

		bool setup(void);

		int start(void);
		int stop(void);
		int is_stopped(void);

		int get_sampling_rate(void);
		int get_distance_once(void);
		int get_distance_loop(void);
};

#endif
