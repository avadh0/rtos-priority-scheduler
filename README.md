# 🚀 RTOS Priority-Based Task Scheduler (C++ Simulation)

This project simulates a **pre-emptive, priority-based Real-Time Operating System (RTOS) scheduler** using C++.
It demonstrates core OS concepts such as:

* Task scheduling
* Preemption
* Priority queues
* Thread synchronization (mutex, semaphore)
* Task blocking & unblocking
* Context switching
* Latency measurement

This is a **user-space simulation**, built for learning operating-system fundamentals.

---

## 📌 Features

### ✅ Pre-emptive Priority Scheduling

A central scheduler gives CPU time to the highest-priority READY task.
When the time quantum expires, the scheduler **preempts** the task.

### ✅ Task Simulation (Threads)

Each task runs in its own thread and performs simulated work using small time slices.

### ✅ Semaphores & Blocking

Tasks can block when trying to access a shared resource using a counting **Semaphore**.
Blocked tasks are re-scheduled once the resource becomes available.

### ✅ Context Switching

The scheduler counts how many times it switches between tasks to measure scheduling overhead.

### ✅ Task Latency Measurement

Each task measures approximate wait time between being ready and actually running.

---

## 🛠️ Tech Used

* **C++17**
* **std::thread**
* **std::mutex**
* **std::condition_variable**
* **Priority Queue**
* **Semaphores (Custom Implementation)**

---

## 📂 Project Structure

```
rtos-priority-scheduler/
│
├── rtos_scheduler_sim.cpp   # Main RTOS simulation code
└── README.md                 # Documentation
```

---

## ▶️ How to Run

### **1. Compile**

Use g++ with threading support:

```
g++ -std=c++17 -pthread rtos_scheduler_sim.cpp -O2 -o rtos_sim
```

### **2. Run**

```
./rtos_sim
```

You will see:

* Task run counts
* Wait times
* Context switches
* Total execution time

---

## 📘 How it Works (Simple Explanation)

### ⭐ Scheduler Thread

* Picks the highest priority ready task
* Gives it a time slice (e.g., 50ms)
* After time slice → preempts the task
* Requeues unfinished tasks

### ⭐ Task Threads

* Wait until scheduler allows them (`can_run = true`)
* Simulate work in 10ms chunks
* Check if preempted
* May block on semaphore

### ⭐ Semaphore

Used to simulate resource locking.
If a task cannot get the semaphore, it becomes **BLOCKED**.

---

## 📊 Example Output

```
All tasks finished. Stats:
Task 1 prio=5 work=400/400 runs=12 wait(ms)=180
Task 2 prio=3 work=600/600 runs=16 wait(ms)=260
Task 3 prio=1 work=350/350 runs=10 wait(ms)=320
Task 4 prio=4 work=300/300 runs=11 wait(ms)=140
Context switches (approx): 50
```

---

## 🧠 Concepts Learned

✔ Preemptive Scheduling
✔ Priority-based execution
✔ Context switching
✔ Task states (READY, RUNNING, BLOCKED, FINISHED)
✔ Semaphores & synchronization
✔ Multi-threading (C++17)
✔ Condition variables

Perfect for students learning Operating Systems & Real-Time Systems.

---

## 🤝 Contributing

Feel free to fork and improve:

* Implement **aging** (avoid starvation)
* Add **round robin** within same priority
* Add detailed event logging



## 🏁 Author

**GitHub:** [@avadh0](https://github.com/avadh0)


