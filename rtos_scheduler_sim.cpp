// rtos_scheduler_sim.cpp
// Fully fixed and error-free version

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <vector>
#include <memory>
#include <atomic>

using namespace std;
using steady_clock_t = chrono::steady_clock;
using ms = chrono::milliseconds;
using TimePoint = chrono::time_point<steady_clock_t>;

// ----------------------------------
// Utility: current time in ms
// ----------------------------------
long long now_ms() {
    return chrono::duration_cast<ms>(
        steady_clock_t::now().time_since_epoch()
    ).count();
}

// ----------------------------------
// Simple Counting Semaphore
// ----------------------------------
class Semaphore {
    mutex m;
    condition_variable cv;
    int count;

public:
    Semaphore(int initial = 0) : count(initial) {}

    void notify(int n = 1) {
        {
            lock_guard<mutex> lock(m);
            count += n;
        }
        cv.notify_all();
    }

    void wait() {
        unique_lock<mutex> lock(m);
        cv.wait(lock, [&] { return count > 0; });
        count--;
    }

    bool try_wait_for(ms timeout) {
        unique_lock<mutex> lock(m);
        if (!cv.wait_for(lock, timeout, [&] { return count > 0; }))
            return false;
        count--;
        return true;
    }
};

// ----------------------------------
// Task structure
// ----------------------------------
enum class TaskState { READY, RUNNING, BLOCKED, FINISHED };

struct Task {
    int id;
    int priority;  
    int total_work_ms;
    int work_done_ms;
    TaskState state;

    TimePoint last_scheduled;
    long long total_wait_time_ms = 0;
    int run_count = 0;

    mutex mtx;
    condition_variable cv;
    bool can_run = false;

    Semaphore* shared_sem = nullptr;

    Task(int id_, int prio, int work)
        : id(id_), priority(prio), total_work_ms(work),
          work_done_ms(0), state(TaskState::READY),
          last_scheduled(steady_clock_t::now()) {}
};

// Priority queue node
struct PQItem {
    int priority;
    TimePoint enqueued;
    shared_ptr<Task> task;
};

// Priority compare
struct PQComp {
    bool operator()(PQItem const& a, PQItem const& b) const {
        if (a.priority == b.priority)
            return a.task->id > b.task->id;
        return a.priority < b.priority;
    }
};

// ----------------------------------
// Scheduler
// ----------------------------------
class Scheduler {
    priority_queue<PQItem, vector<PQItem>, PQComp> ready_q;
    mutex q_m;
    condition_variable q_cv;

    atomic<int> context_switches{0};
    vector<shared_ptr<Task>> all_tasks;

    thread sched_thread;
    bool shutdown_flag = false;

    int time_slice_ms;

public:
    Scheduler(int quantum) : time_slice_ms(quantum) {}

    void start() {
        sched_thread = thread(&Scheduler::scheduler_loop, this);
    }

    void stop() {
        {
            lock_guard<mutex> lock(q_m);
            shutdown_flag = true;
        }
        q_cv.notify_all();

        if (sched_thread.joinable())
            sched_thread.join();
    }

    void add_task(shared_ptr<Task> t) {
        lock_guard<mutex> lock(q_m);
        all_tasks.push_back(t);
        ready_q.push({t->priority, steady_clock_t::now(), t});
        q_cv.notify_one();
    }

    void task_blocked(shared_ptr<Task> t) {
        lock_guard<mutex> lock(q_m);
        t->state = TaskState::BLOCKED;
    }

    void task_unblocked(shared_ptr<Task> t) {
        lock_guard<mutex> lock(q_m);
        if (t->state != TaskState::FINISHED) {
            t->state = TaskState::READY;
            ready_q.push({t->priority, steady_clock_t::now(), t});
            q_cv.notify_one();
        }
    }

    void requeue_task(shared_ptr<Task> t) {
        lock_guard<mutex> lock(q_m);
        if (t->state != TaskState::FINISHED) {
            t->state = TaskState::READY;
            ready_q.push({t->priority, steady_clock_t::now(), t});
            q_cv.notify_one();
        }
    }

    int get_context_switches() const {
        return context_switches.load();
    }

    vector<shared_ptr<Task>> get_all_tasks() {
        lock_guard<mutex> lock(q_m);
        return all_tasks;
    }

private:
    void scheduler_loop() {
        while (true) {
            shared_ptr<Task> next;

            {
                unique_lock<mutex> lock(q_m);
                q_cv.wait(lock, [&] {
                    return shutdown_flag || !ready_q.empty();
                });

                if (shutdown_flag && ready_q.empty())
                    break;

                next = ready_q.top().task;
                ready_q.pop();
            }

            context_switches++;

            {
                unique_lock<mutex> lock(next->mtx);
                next->state = TaskState::RUNNING;

                TimePoint now = steady_clock_t::now();
                if (next->run_count > 0) {
                    long long wait = chrono::duration_cast<ms>(
                        now - next->last_scheduled
                    ).count();
                    next->total_wait_time_ms += wait;
                }

                next->last_scheduled = now;
                next->can_run = true;
                next->run_count++;

                next->cv.notify_one();
            }

            this_thread::sleep_for(ms(time_slice_ms));

            {
                unique_lock<mutex> lock(next->mtx);

                if (next->state == TaskState::RUNNING) {
                    next->can_run = false;

                    if (next->work_done_ms < next->total_work_ms)
                        requeue_task(next);
                    else
                        next->state = TaskState::FINISHED;
                }
            }
        }
    }
};

// ----------------------------------
// Task thread code
// ----------------------------------
void task_thread_fn(shared_ptr<Task> t, Scheduler* sched) {
    while (true) {
        unique_lock<mutex> lock(t->mtx);
        t->cv.wait(lock, [&] {
            return t->can_run || t->state == TaskState::FINISHED;
        });

        if (t->state == TaskState::FINISHED)
            break;

        while (t->can_run && t->work_done_ms < t->total_work_ms) {
            lock.unlock();

            this_thread::sleep_for(ms(10));
            t->work_done_ms += 10;

            // Blocking simulation
            if (t->shared_sem &&
                t->work_done_ms >= t->total_work_ms / 2 &&
                t->work_done_ms < t->total_work_ms / 2 + 10)
            {
                sched->task_blocked(t);

                if (!t->shared_sem->try_wait_for(ms(200))) {
                    lock.lock();
                    t->can_run = false;
                    t->state = TaskState::BLOCKED;
                    lock.unlock();
                    break;
                } else {
                    this_thread::sleep_for(ms(30));
                    t->shared_sem->notify(1);
                }
            }

            lock.lock();
            if (!t->can_run)
                break;
        }

        if (t->work_done_ms >= t->total_work_ms) {
            t->state = TaskState::FINISHED;
            t->can_run = false;
            break;
        }
    }
}

// ----------------------------------
// MAIN
// ----------------------------------
int main() {
    cout << "RTOS Priority-based Scheduler Simulation\n";

    Scheduler sched(50);
    sched.start();

    Semaphore shared_sem(1);

    vector<shared_ptr<Task>> tasks = {
        make_shared<Task>(1, 5, 400),
        make_shared<Task>(2, 3, 600),
        make_shared<Task>(3, 1, 350),
        make_shared<Task>(4, 4, 300),
    };

    tasks[1]->shared_sem = &shared_sem;
    tasks[2]->shared_sem = &shared_sem;

    vector<thread> workers;

    for (auto& t : tasks) {
        sched.add_task(t);
        workers.emplace_back(task_thread_fn, t, &sched);
    }

    thread monitor([&] {
        while (true) {
            bool done = true;
            for (auto& t : tasks)
                if (t->state != TaskState::FINISHED)
                    done = false;

            if (done) break;

            this_thread::sleep_for(ms(150));
            shared_sem.notify();

            for (auto& t : tasks)
                if (t->state == TaskState::BLOCKED)
                    sched.task_unblocked(t);
        }
    });

    for (auto& w : workers)
        if (w.joinable()) w.join();

    if (monitor.joinable()) monitor.join();

    sched.stop();

    cout << "\nFinal Stats:\n";
    for (auto& t : sched.get_all_tasks()) {
        cout << "Task " << t->id
             << " | prio=" << t->priority
             << " | runs=" << t->run_count
             << " | work=" << t->work_done_ms << "/" << t->total_work_ms
             << " | wait=" << t->total_wait_time_ms << "ms\n";
    }

    cout << "Context Switches: " << sched.get_context_switches() << "\n";

    return 0;
}


