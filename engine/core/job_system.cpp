#include "job_system.hpp"

#include <latch>
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
    for (std::thread& thread : worker_threads) {
        thread.join();
    }
}

void JobManager::parallel_for(const size_t task_count, const std::function<void(size_t)>& function) {
    std::latch latch {static_cast<ptrdiff_t>(task_count)};

    for (size_t i = 0; i < task_count; i++) {
        queue_task([&function, &latch, i] {
            function(i);
            latch.count_down();
        });
    }

    latch.wait();
}

void JobManager::grouped_parallel_for(const size_t task_count, const std::function<void(size_t)>& function, const size_t group_count) {
    if (task_count <= 0) return;

    std::latch latch {static_cast<ptrdiff_t>(task_count)};
    const auto group_size = static_cast<size_t>(std::ceil(static_cast<float>(task_count) / static_cast<float>(group_count)));

    for (size_t group_index = 0; group_index < group_count; group_index++) {
        const size_t start_index = group_index * group_size;  // Index of the first element in the current group
        if (start_index >= task_count) continue;              // Skip scheduling a task if we have exceeded the job count prematurely

        const size_t max_local_task_count = task_count - start_index;

        // Size of the current group, either group_size or a smaller size depending on the amount of elements left
        const size_t local_group_size = std::min<size_t>(group_size, max_local_task_count);
        const size_t group_end = start_index + local_group_size;

        auto task_grouping = [&function, &latch, start_index, group_end] {
            for (size_t i = start_index; i < group_end; i++) {
                function(i);
                latch.count_down();
            }
        };

        queue_task(task_grouping);
    }

    latch.wait();
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
