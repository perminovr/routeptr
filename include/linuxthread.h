#ifndef LINUXTHREAD_H
#define LINUXTHREAD_H

#include <pthread.h>
#include <exception>
#include <functional>

#ifndef THREAD_STACK_SIZE
#define THREAD_STACK_SIZE (1<<16)
#endif

/*!
 * @class LinuxThread
 * @brief: thread class основанный на pthread linux; fixed stack size
*/
class LinuxThread {
public:
	typedef void *(*thread_func_t)(void *);

	static void exit(void *ret = 0) {
		pthread_exit(ret);
	}
	static void yield() {
		pthread_yield();
	}
	static int cancel(LinuxThread &thr) {
		return pthread_cancel(thr.self);
	}
	int detach() {
		if (joinable) {
			joinable = false;
			return pthread_detach(self);
		}
		return -1;
	}
	int join(void **ret = 0) {
		if (joinable) {
			joinable = false;
			return pthread_join(self, ret);
		}
		return -1;
	}
	template <class Class>
	LinuxThread(Class *obj, void (Class::*func)(void), long stackSize = THREAD_STACK_SIZE) {
		joinable = true;
		helper = new Helper(obj, func);
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, stackSize);
		pthread_create(&self, &attr, &Helper::loop, helper);
	}
	template <typename Callable>
	LinuxThread(Callable &lambdaf, long stackSize = THREAD_STACK_SIZE) {
		joinable = true;
		helper = new Helper(lambdaf);
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, stackSize);
		pthread_create(&self, &attr, &Helper::loop, helper);
	}
	LinuxThread(thread_func_t func, void *arg, long stackSize = THREAD_STACK_SIZE) {
		joinable = true;
		helper = nullptr;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, stackSize);
		pthread_create(&self, &attr, func, arg);
	}
	~LinuxThread() {
		if (joinable) {
			std::terminate();
		}
		delete helper;
	}

protected:
	pthread_t self;
	bool joinable;

	class Helper {
	public:
		std::function <void(void)> func;
		template <class Class>
		Helper(Class *obj, void (Class::*func)(void)) {
			this->func = std::bind(func, obj);
		}
		Helper(std::function <void(void)> func) {
			this->func = func;
		}
		static void *loop(void *arg) {
			Helper *self = (Helper *)arg;
			self->func();
			pthread_exit(0);
		}
	};
	Helper *helper;
};

#endif // LINUXTHREAD_H