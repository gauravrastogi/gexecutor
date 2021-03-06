/**
 * gasync_executor.cpp
 *  Copyright Admin Cppexecutor, 2013
 *  Created on: Jun 9, 2013
 *      Author: cppexecutor@gmail.com
 */

#include "gasync_executor.h"
#include <event2/event.h>
#include <assert.h>
#include <iostream>
#include <glog/logging.h>

GAsyncExecutor::GAsyncExecutor(struct event_base *event_base,
                               GTaskQSharedPtr taskq)
    : GExecutor(GExecutorType::ASYNC, taskq), event_base_(event_base),
      p_taskq_ev_(NULL), p_timer_ev_(NULL), taskq_timeout_(),
      timer_timeout_() {
    /**
     *  When a async executor is created then following is done
     *  1. setup a queue for receiving events
     *  2. setup an event for these queues
     *  3. Ensure there are no thundering herd issues.
     */
    taskq_timeout_ = { 2, 0 };
    timer_timeout_ = { 1, 0 };
}

GAsyncExecutor::~GAsyncExecutor() {
    if (p_taskq_ev_) {
        event_del(p_taskq_ev_);
        event_free(p_taskq_ev_);
    }
}

gerror_code_t GAsyncExecutor::Shutdown() {
    if (p_taskq_ev_) {
        event_del(p_taskq_ev_);
        event_free(p_taskq_ev_);
        p_taskq_ev_ = 0;
    }
    StopTimer();
    return 0;
}

void GAsyncExecutor::StopTimer() {
    if (!p_timer_ev_) {
        return;
    }
    event_del(p_timer_ev_);
    //event_free(p_timer_ev_);
    p_timer_ev_ = 0;
}

gerror_code_t GAsyncExecutor::EnQueueTask(GTaskSharedPtr task) {
    return p_taskq_->EnqueueGTask(task);
}

static void taskq_cb(evutil_socket_t fd, short what, void *arg) {
    char msg[4096];
    ssize_t num_bytes = 0;
    GExecutor* executor = static_cast<GExecutor *>(arg);
    GTaskQSharedPtr p_taskq = executor->taskq();
    snprintf(msg, 128, "Got an event on socket %d:%s%s%s%s Taskq[%p]",
             (int) fd,
             (what&EV_TIMEOUT) ? " timeout" : "",
             (what&EV_READ)    ? " read" : "",
             (what&EV_WRITE)   ? " write" : "",
             (what&EV_SIGNAL)  ? " signal" : "",
             arg);

    GEXECUTOR_LOG(GEXECUTOR_TRACE) << msg << std::endl;
    if (!(what & EV_READ)) {
        return;
    }

    do {
        num_bytes = read(fd, msg, 4096);
        if (num_bytes == -1) {
            GEXECUTOR_LOG(GEXECUTOR_TRACE)
                    << "Read: "<< num_bytes << " from fd: " << fd
                    << " errno: " << errno
                    << "strerr: " << strerror(errno) << std::endl;
            return;
        }
        //GEXECUTOR_LOG(GEXECUTOR_TRACE)
        //           << "Read: "<< num_bytes << " from fd: " << fd << std::endl;
        for (int num_tasks = 0; num_tasks < num_bytes; num_tasks++) {
            GTaskSharedPtr p_task = p_taskq->DequeueGTask();
            GEXECUTOR_LOG(GEXECUTOR_TRACE)
                << "Executing task " << p_task->DebugString() << std::endl;
            if (p_task) {
                p_task->set_executor(executor);
                p_task->Execute();
            }
        }
    } while (num_bytes > 0);
    return;
}


class GTaskCheckTaskQ : public GTask {
public:
    GTaskCheckTaskQ(GTaskQSharedPtr taskq)
    : GTask(taskq) {
    }
    virtual ~GTaskCheckTaskQ() {
        return;
    }
protected:
    virtual gerror_code_t Execute() {
        //VLOG(GEXECUTOR_TRACE) << "Task Q working fine" << std::endl;
        return 0;
    }
};


static void check_taskq_cb(evutil_socket_t fd, short what, void *arg) {
    GAsyncExecutor *executor = static_cast<GAsyncExecutor *>(arg);
    VLOG(GEXECUTOR_TRACE) << __FUNCTION__ << ": checking task callback"
            << executor->taskq() << arg << std::endl;
    GTaskSharedPtr check_task(new GTaskCheckTaskQ(executor->taskq()));
    executor->taskq()->EnqueueGTask(check_task);
}


gerror_code_t GAsyncExecutor::Initialize() {
    // p_taskq_->Initialize();
    //executor_shared_ptr_ = shared_from_this();
    /**
     * Add event for reading from the queue
     */

    p_taskq_ev_ = event_new(event_base_,
                            p_taskq_->read_fd(),
                            (EV_READ|EV_PERSIST),
                            taskq_cb,
                            static_cast<void *>(this));
    assert(p_taskq_ev_);
    VLOG(GEXECUTOR_TRACE) << "Setup Read event for pipe \n";

    event_add(p_taskq_ev_, &taskq_timeout_);

    void *timer_arg = static_cast<void *>(this);

    p_timer_ev_ = evtimer_new(event_base_,
                              check_taskq_cb,
                              timer_arg);
    // evtimer_add(p_timer_ev_, &timer_timeout_);

    VLOG(GEXECUTOR_TRACE) << "Setup Timer event for async engine "
            << taskq() << "\n";

    return 0;
}


