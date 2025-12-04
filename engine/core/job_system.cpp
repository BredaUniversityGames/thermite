#include "job_system.hpp"

#include <thread>

#include "logger.hpp"

namespace tmt {

JobManager::JobManager() {
    Log::info("Start JobSystem module: creating worker threads");
    // Threads in the pool will be the amount of hardware threads - 1 (to account for main thread).
    const size_t thread_count = std::thread::hardware_concurrency() - 1;

    for (size_t i = 0; i < thread_count; i++) {
        worker_threads.emplace_back(&JobManager::run_jobs, this);
    }

    // We could set the thread affinity here (locking a worker thread to a specific hardware thread), however on more modern PCs it can sometimes be slower.
}

JobManager::~JobManager() {
    // Notify all threads to quit.
    {
        std::scoped_lock lock(mutex);
        exit = true;
        conditional.notify_all();
    }

    // Wait for all threads after we notify them to quit.
    Log::info("Quit JobSystem module: joining worker threads");
    for (std::thread& thread : worker_threads)
    {
        thread.join();
    }
}

void JobManager::run_jobs() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock lock(mutex);
            conditional.wait(lock, [this] { return tasks.empty() == false || exit == true; });

            if (exit) return;

            job = tasks.front();
            tasks.pop();
        }
        job();
    }
}

// Queue a task created by a job
void JobManager::queue_task(const std::function<void()>& task) {
    std::scoped_lock lock(mutex);
    tasks.push(task);
    conditional.notify_one();
}

}  // namespace tmt
