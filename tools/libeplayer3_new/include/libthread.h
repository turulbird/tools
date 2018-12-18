#ifndef _LIBTHREAD_H
#define _LIBTHREAD_H

#include <pthread.h>

class Mutex
{
	friend class Condition;

	pthread_mutex_t mMutex;

	Mutex(const Mutex&);
	const Mutex& operator=(const Mutex&);

	protected:
		explicit Mutex(int);

	public:
		Mutex();
		virtual ~Mutex();
		virtual void lock();
		virtual void unlock();
};

class ScopedLock
{
	Mutex& mMutex;

	ScopedLock(const ScopedLock&);
	const ScopedLock& operator=(const ScopedLock&);

	public:
		ScopedLock(Mutex&);
		~ScopedLock();
};

#endif
// vim:ts=4
