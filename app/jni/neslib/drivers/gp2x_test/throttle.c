#include <sys/time.h>
#include <unistd.h>

#include "../gp2x/main.h"
#include "../gp2x/gp2x.h"
#include "../gp2x/throttle.h"


extern uint8 PAL;
extern int FSkip;
static int usec_aim = 0, usec_done = 0;
static int skip_count = 0;

void RefreshThrottleFPS(void)
{
	usec_aim = usec_done = skip_count = 0;
}

void SpeedThrottle(void)
{
	static struct timeval tv_prev;
	struct timeval tv_now;
	int delta_nom = PAL ? 19997 : 16639; // ~50.007, 19.997 ms/frame : ~60.1, 16.639 ms/frame


	if (usec_done == 0) { // first time
		usec_done = 1;
		gettimeofday(&tv_prev, 0);
		return;
	}

	gettimeofday(&tv_now, 0);

	usec_aim += delta_nom;
	if (tv_now.tv_sec != tv_prev.tv_sec)
		usec_done += 1000000;
	usec_done += tv_now.tv_usec - tv_prev.tv_usec;

#ifdef FRAMESKIP
	if (Settings.frameskip >= 0)
	{
		if (skip_count >= Settings.frameskip)
			skip_count = 0;
		else {
			skip_count++;
			FSkip = 1;
		}
	}
	else if (usec_done > usec_aim + 1024*4)
	{
		/* auto frameskip */
		if (usec_done - usec_aim > 1024*32)
			usec_done = usec_aim = 1; // too much behind, try to recover..
		else
			FSkip = 1;
		tv_prev = tv_now;
		return;
	}
#endif

	tv_prev = tv_now;
	while (usec_done < usec_aim)
	{
		usleep(300);
		gettimeofday(&tv_now, 0);

		if (tv_now.tv_sec != tv_prev.tv_sec)
			usec_done += 1000000;
		usec_done += tv_now.tv_usec - tv_prev.tv_usec;
		tv_prev = tv_now;
	}
	usec_done = usec_done - usec_aim + 1; // reset to prevent overflows
	usec_aim = 0;
}

