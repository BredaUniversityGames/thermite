#pragma once

#include <vector>
#include <future>
#include <queue>

#include "engine.hpp"

namespace std {
class thread;
}

namespace tmt {

#pragma region Job declerations
namespace internal {

template <typename ReturnType, typename Function>
class JobBase {
   public:
    [[nodiscard]] bool finished() const;
    [[nodiscard]] size_t pending_tasks() const { return futures.size(); }

   protected:
    JobBase(Function function) : function(std::move(function)) {}

    Function function;
    std::vector<std::future<ReturnType>> futures;
};
}  // namespace internal

template <typename ReturnType, typename Function>
class Job final : public internal::JobBase<ReturnType, Function> {
   public:
    template <typename... Args>
    void add_to_queue(Args... args);

    void wait();

    std::vector<ReturnType> get();
    std::vector<ReturnType> try_get();

   private:
    friend class JobManager;

    Job(Function function) : internal::JobBase<ReturnType, Function>(std::move(function)) {}
};

template <typename Function>
class Job<void, Function> final : public internal::JobBase<void, Function> {
   public:
    template <typename... Args>
    void add_to_queue(Args... args);

    template <typename Container, typename... Args>
    void dispatch(Container container, const size_t taskCount, Args... args);

    void wait();

   private:
    friend class JobManager;

    Job(Function function) : internal::JobBase<void, Function>(std::move(function)) {}
};
#pragma endregion

class JobManager {
   public:
    JobManager();
    ~JobManager();

    [[nodiscard]] size_t get_thread_count() const { return worker_threads.size(); }

    template <typename Function>
    auto create_job(Function&& function);

   private:
    template <typename, typename>
    friend class JobBase;
    template <typename, typename>
    friend class Job;

    void run_jobs();

    void queue_task(const std::function<void()>& task);

    template <typename Function, typename ReturnType, typename... Args>
    Job<ReturnType, Function> deduce_return_type(Function function, const std::function<ReturnType(Args...)>&);

    bool exit = false;
    std::mutex mutex;
    std::condition_variable conditional;

    std::vector<std::thread> worker_threads;
    std::queue<std::function<void()>> tasks;
};

#pragma region JobManager function definitions
template <typename Function>
auto JobManager::create_job(Function&& function) {
    using FunctionTypeSTD = decltype(std::function {std::forward<Function>(function)});
    return deduce_return_type(std::forward<Function>(function), FunctionTypeSTD {});
}

// Function used to deduce the return-type of a function without its arguments, source:
// https://medium.com/@matthieu.dorier/deducing-argument-and-return-types-of-a-callable-in-c-834598f6a385
template <typename Function, typename ReturnType, typename... Args>
Job<ReturnType, Function> JobManager::deduce_return_type(Function function, const std::function<ReturnType(Args...)>&) {
    return Job<ReturnType, Function>(function);
}
#pragma endregion

#pragma region Job function definitions
// Create a task for the worker threads to perform with the provided arguments
template <typename ReturnType, typename Function>
template <typename... Args>
void Job<ReturnType, Function>::add_to_queue(Args... args) {
    auto promise = std::make_shared<std::promise<ReturnType>>();  // Make the promise a shared pointer in order to capture it in
                                                                  // the lambda task
    this->futures.push_back(promise->get_future());

    auto task = [this, args..., promise] { promise->set_value(this->function(args...)); };

    engine.job_manager.queue_task(task);
}

// Create a task for the worker threads to perform with the provided arguments
template <typename Function>
template <typename... Args>
void Job<void, Function>::add_to_queue(Args... args) {
    auto promise = std::make_shared<std::promise<void>>();  // Make the promise a shared pointer in order to capture it in
                                                            // the lambda task
    this->futures.push_back(promise->get_future());

    auto task = [this, args..., promise] {
        this->function(args...);
        promise->set_value();
    };

    engine.job_manager.queue_task(task);
}

// Create a large number of tasks in groups to more efficiently perform a lot of small tasks
template <typename Function>
template <typename Container, typename... Args>
void Job<void, Function>::dispatch(Container container, const size_t taskCount, Args... args) {
    if (taskCount <= 0) return;

    const size_t group_count = engine.job_manager.get_thread_count();
    const auto group_size = static_cast<size_t>(std::ceil(static_cast<float>(taskCount) / static_cast<float>(group_count)));

    for (size_t groupIndex = 0; groupIndex < group_count; groupIndex++) {
        const size_t start_index = groupIndex * group_size;  // Index of the first element in the current group
        if (start_index >= taskCount) continue;              // Skip scheduling a task if we have exceeded the job count prematurely

        auto promise = std::make_shared<std::promise<void>>();
        this->futures.push_back(promise->get_future());

        const size_t maxLocalTaskCount = taskCount - start_index;

        // Size of the current group, either groupSize or a smaller size depending on the amount of elements left
        const size_t local_group_size = std::min<size_t>(group_size, maxLocalTaskCount);
        const size_t group_end = start_index + local_group_size;

        auto jobGrouping = [this, start_index, group_end, container, promise, args...] {
            size_t i = 0;

            for (auto&& item : container) {
                if (i >= start_index) {
                    if (i >= group_end) break;

                    this->function(item, args...);
                }
                i++;
            }

            promise->set_value();
        };

        engine.job_manager.queue_task(jobGrouping);
    }
}

// Check if all tasks created with this job have been executed
template <typename ReturnType, typename Function>
bool internal::JobBase<ReturnType, Function>::finished() const {
    for (auto& future : futures)
        if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return false;

    return true;
}

// Wait for all tasks created with this job to be complete
// After all tasks have been executed, doesn't clear all tasks until Get() or TryGet() has been used
template <typename ReturnType, typename Function>
void Job<ReturnType, Function>::wait() {
    for (auto& future : this->futures) future.wait();
}

// Wait for all tasks created with this job to be complete
// After all tasks have been executed, clears all tasks from the job
template <typename Function>
void Job<void, Function>::wait() {
    for (auto& future : this->futures) future.wait();

    this->futures.clear();
}

// Wait for all tasks created with this job to execute and returns all their values
template <typename ReturnType, typename Function>
std::vector<ReturnType> Job<ReturnType, Function>::get() {
    std::vector<ReturnType> values;
    values.reserve(this->futures.size());

    for (auto& future : this->futures) values.push_back(future.get());
    this->futures.clear();

    return values;
}

// Return the values of the tasks that have finished executing, then remove them from the job
template <typename ReturnType, typename Function>
std::vector<ReturnType> Job<ReturnType, Function>::try_get() {
    std::vector<ReturnType> values;
    values.reserve(this->futures.size());

    for (auto& future : this->futures) {
        if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            values.push_back(future.get());
        }
    }

    this->futures.erase(std::remove_if(this->futures.begin(), this->futures.end(), [](const auto& future) { return !future.valid(); }), this->futures.end());

    return values;
}

#pragma endregion

}  // namespace tmt