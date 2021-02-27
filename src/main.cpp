#include "routeptrdrv.h"
#include <iostream>
#include <initializer_list>
#include <signal.h>
#include <unistd.h>

static RoutePrtDrv *worker = nullptr;

int main(int argc, char **argv)
{
	worker = new RoutePrtDrv();
	worker->start();
	delete worker;
    return 0;
}

/* extern using */ void Sleep(long ms)
{
	long sec = ms / 1000;
	ms = ms % 1000;
	timespec_t to_sleep = { sec, ms * 1000000L };
	while ((nanosleep(&to_sleep, &to_sleep) == -1) && (errno == EINTR));
}
