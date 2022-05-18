#include <sys/time.h>
#include "main.h"
#include "gp2x.h"
#include "minimal.h"
#include "throttle.h"


extern uint8 PAL;
extern int FSkip;
static int skip_count = 0;
static struct timeval tv_prev;

#define tvdiff(tv1, tv2) \
	((tv1.tv_sec - tv2.tv_sec) * 1000000 + tv1.tv_usec - tv2.tv_usec)

#define tvadd(tv, usec) { \
	tv.tv_usec += usec; \
	if (tv.tv_usec >= 1000000) { \
		tv.tv_sec += 1; \
		tv.tv_usec -= 1000000; \
	} \
}

#define tvsub(tv, usec) { \
	tv.tv_usec -= usec; \
	if (tv.tv_usec < 0) { \
		tv.tv_sec -= 1; \
		tv.tv_usec += 1000000; \
	} \
}

void RefreshThrottleFPS(void)
{
	skip_count = 0;
	if (Settings.perfect_vsync)
	{
		gp2x_video_wait_vsync();
	}
	gettimeofday(&tv_prev, 0);
	tvsub(tv_prev, PAL ? 19997 : 16639);
}

static void wait_to(struct timeval *tv_aim)
{
	struct timeval tv_now;
	int diff;

	do
	{
		gettimeofday(&tv_now, 0);
		diff = tvdiff((*tv_aim), tv_now);
	}
	while (diff > 0);
}

#include <stdio.h>
void SpeedThrottle(void)
{
	struct timeval tv_now, tv_aim;
	int frame_time = PAL ? 19997 : 16639; // ~50.007, 19.997 ms/frame : ~60.1, 16.639 ms/frame
	int tdiff;

	tv_aim = tv_prev;
	tvadd(tv_aim, frame_time);

	gettimeofday(&tv_now, 0);
	tdiff = tvdiff(tv_now, tv_aim);

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
	else if (tdiff >= frame_time)
	{
		/* auto frameskip */
		if (/*tdiff < 36*1024 &&*/ skip_count < 6) {	// limit frameskip
			FSkip = 1;
			skip_count++;
		} else
			skip_count = 0;

		if (tdiff < 92*1024)
			tv_prev = tv_aim;
		else
			tv_prev = tv_now; // something went wrong, try to recover
		return;
	}
	else
		skip_count = 0;
#endif

	/* throttle */
	if (tdiff < 0)
	{
		if (Settings.perfect_vsync)
		{
			if (tdiff <= (PAL ? 19997/2 : 16639/2))
			{
				struct timeval tv_tmp = tv_aim;
				tvsub(tv_tmp, 5000);
				wait_to(&tv_tmp);
			}
			gp2x_video_wait_vsync();
			gettimeofday(&tv_prev, 0);
			return;
		}
		else
		{
			wait_to(&tv_aim);
		}
	}

	tv_prev = tv_aim;
}

