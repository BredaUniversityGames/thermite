#include "job_system.hpp"

#include <latch>
#include <thread>
#include <windows.h>

#include "logger.hpp"

namespace tmt {

JobManager::JobManager() {
    Log::info("Start JobSystem module: creating worker threads");
    // Threads in the pool will be the amount of hardware threads - 2 (to account for main thread and background tasks).
    const size_t thread_count = std::thread::hardware_concurrency() - 2;
    for (size_t i = 0; i < thread_count; i++) {
        std::thread& worker = worker_threads.emplace_back(&JobManager::run_worker, this);
        SetThreadAffinityMask(worker.native_handle(), 0b1 << (i + 2));
    }
    worker_threads_active.clear();

    // We could set the thread affinity here (locking a worker thread to a specific hardware thread), however on more modern PCs it can sometimes be slower.
}

JobManager::~JobManager() {
    // Notify all threads to quit.
    activate_workers();
    exit.test_and_set();

    // Wait for all threads after we notify them to quit.
    Log::info("Quiting job_manager module: joining worker threads");
    for (std::thread& thread : worker_threads) {
        thread.join();
    }
    deactivate_workers();
}

void JobManager::parallel_for(const size_t count, const std::function<void(size_t)>& function) {
    task_function = function;

    next_task_index = 0;
    total_task_count = count;

    finished_working.clear();

    bool workers_automatically_woken = false;
    if (finished_working.test()) {
        workers_automatically_woken = true;
        Log::warn("Worker threads were not woken up manually! Threads are now waking up too late!");
        activate_workers();
    }

    execute_work();

    finished_working.wait(false);

    if (workers_automatically_woken) deactivate_workers();
}

void JobManager::run_worker() {
    while (true) {
        worker_threads_active.wait(false);

        if (exit.test()) return;

        if (!finished_working.test()) {
            execute_work();
        } else {
            std::this_thread::yield();
        }
    }
}

void JobManager::execute_work() {
    if (next_task_index >= total_task_count) return;
    ++threads_finished_working;

    size_t i = next_task_index.fetch_add(1);
    while (i < total_task_count) {
        task_function(i);

        i = next_task_index.fetch_add(1);
    }

    if (threads_finished_working.fetch_sub(1) == 1) {
        finished_working.test_and_set();
        finished_working.notify_one();
    }
}
}  // namespace tmt
