#pragma once

#include <vector>
#include <future>
#include <queue>

#include "engine.hpp"

namespace std {
class thread;
}

namespace tmt {

class JobManager {
   public:
    JobManager();
    ~JobManager();

    [[nodiscard]] size_t get_thread_count() const { return worker_threads.size(); }

    // A multithreaded parallel for loop.
    void parallel_for(size_t count, const std::function<void(size_t)>& function);

    // A multithreaded parallel foreach loop, the function its takes has to have a single parameter which matches the type the container holds.
    template <typename Container, typename Element = std::invoke_result_t<typename Container::operator[], typename Container::size_type>>
    void parallel_foreach(Container& container, const std::function<void(Element)>& function) {
        parallel_for(std::size(container), [&container, &function](size_t index) { function(container[index]); });
    }

   private:
    friend class ScopedSpinLock;

    void run_worker();
    void execute_work();

    std::atomic_flag exit;
    std::vector<std::thread> worker_threads;

    std::function<void(size_t)> task_function;
    std::atomic_flag finished_working;
    std::atomic<size_t> threads_finished_working;
    size_t total_task_count;
    std::atomic<size_t> next_task_index;
    std::atomic_flag worker_threads_active;
};

class ScopedSpinLock {
   public:
    ScopedSpinLock(JobManager& job_manager) : job_manager(job_manager) {
        job_manager.worker_threads_active.test_and_set();
        job_manager.worker_threads_active.notify_all();
    }
    ~ScopedSpinLock() { job_manager.worker_threads_active.clear(); }

   private:
    JobManager& job_manager;
};

}  // namespace tmt