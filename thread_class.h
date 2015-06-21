#ifndef THREAD_CLASS_INCLUDED
#define THREAD_CLASS_INCLUDED

#include <pthread.h>

class Thread
{
private:
    pthread_t thread;
 
    Thread(const Thread& copy);         // copy constructor denied
    static void *thread_func(void *d) {
        ((Thread *)d)->run();
        return NULL;
    }
public:
    Thread() {}
    virtual ~Thread() {}
 
    virtual void run() = 0;
 
    int start() {
        return pthread_create(&thread, NULL, Thread::thread_func, (void*)this);
    }
    int wait () {
        return pthread_join(thread, NULL);
    }
};
 
//typedef std::auto_ptr<Thread> ThreadPtr;

#endif