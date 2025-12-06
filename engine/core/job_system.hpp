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
    // The number of tasks that are still active OR have not yet returned their result value using get() or try_get().
    [[nodiscard]] size_t pending_task_count() const { return futures.size(); }

   protected:
    JobBase(Function function) : function(std::move(function)) {}

    Function function;
    std::vector<std::future<ReturnType>> futures;
};
}  // namespace internal

template <typename ReturnType, typename Function>
class Job final : public internal::JobBase<ReturnType, Function> {
   public:
    // Create a task for the worker threads to perform with the provided arguments.
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
    void dispatch_to_queue(Container container, const size_t task_count_hint, Args... args);

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

    // A multithreaded parallel for loop, this version works best on a small amount of longer tasks.
    void parallel_for(size_t task_count, const std::function<void(size_t)>& function);

    // A multithreaded parallel for loop, this version works best on a large amount of short tasks by grouping those tasks together.
    void grouped_parallel_for(size_t task_count, const std::function<void(size_t)>& function, size_t group_count = engine.job_manager.get_thread_count());

    // A multithreaded parallel foreach loop.
    template <typename Container>
    void parallel_foreach(Container& container, const std::function<void(typename Container::value_type&)>& function);
    // A multithreaded parallel foreach loop.
    template <typename Container>
    void parallel_foreach(const Container& container, const std::function<void(typename Container::value_type&)>& function);

    /* 
    - A multithreaded parallel foreach loop, this overload is meant specifically for entt::view.each().
    - When using a lambda, due to type deduction, you need to wrap the lambda around std::function() to correctly deduce typing, e.g: parallel_foreach(view.each(), std::function([](entt:entity, Transform&){}));
    - The function arguments should be entt::entity followed by all component types int the view (as references).
    */
    template <typename Container, typename... Args>
        requires(std::is_same_v<std::tuple<Args...>, typename Container::value_type>)
    void parallel_foreach(const Container& container, const std::function<void(Args...)>& function);

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

template <typename Container>
void JobManager::parallel_foreach(Container& container, const std::function<void(typename Container::value_type&)>& function) {
    std::atomic<bool> finished {false};
    std::atomic<size_t> finished_count {0};
    std::atomic<size_t> task_count {0};

    size_t local_task_count = 0;
    for (auto&& element : container) {
        queue_task([&function, &finished_count, &task_count, &container, &element, &finished] {
            function(element);
            ++finished_count;

            // Task count is set later once so is either 0 or the total task count, this means the values can't be accidentally the same, since finished_count is incremented right before check
            // and thus can never be 0.
            if (finished_count == task_count) {
                finished = true;
                finished.notify_one();
            }
        });
        ++local_task_count;
    }
    // We store the value instead of incrementing the atomic to avoid finished_count and task_count being the same but not equal to the total task count.
    task_count = local_task_count;
    // This check is to avoid the unlucky event that all threads have already finished when we start waiting, therefor never notifying the atomic variable to stop waiting.
    if (finished_count == task_count) return;

    finished.wait(false);
}

template <typename Container>
void JobManager::parallel_foreach(const Container& container, const std::function<void(typename Container::value_type&)>& function) {
    std::atomic<bool> finished {false};
    std::atomic<size_t> finished_count {0};
    std::atomic<size_t> task_count {0};

    size_t local_task_count = 0;
    for (auto&& element : container) {
        queue_task([&function, &finished_count, &task_count, &element, &finished] {
            function(element);
            ++finished_count;

            // Task count is set later once so is either 0 or the total task count, this means the values can't be accidentally the same, since finished_count is incremented right before check
            // and thus can never be 0.
            if (finished_count == task_count) {
                finished = true;
                finished.notify_one();
            }
        });
        ++local_task_count;
    }
    // We store the value instead of incrementing the atomic to avoid finished_count and task_count being the same but not equal to the total task count.
    task_count = local_task_count;
    // This check is to avoid the unlucky event that all threads have already finished when we start waiting, therefor never notifying the atomic variable to stop waiting.
    if (finished_count == task_count) return;

    finished.wait(false);
}

template <typename Container, typename... Args>
    requires(std::is_same_v<std::tuple<Args...>, typename Container::value_type>)
void JobManager::parallel_foreach(const Container& container, const std::function<void(Args...)>& function) {
    std::atomic finished {false};
    std::atomic<size_t> finished_count {0};
    std::atomic<size_t> task_count {0};

    size_t local_task_count = 0;
    for (auto&& element : container) {
        queue_task([&function, &finished_count, &task_count, &element, &finished] {
            function(std::get<Args>(element)...);
            ++finished_count;

            // Task count is set later once so is either 0 or the total task count, this means the values can't be accidentally the same, since finished_count is incremented right before check
            // and thus can never be 0.
            if (finished_count == task_count) {
                finished = true;
                finished.notify_one();
            }
        });
        ++local_task_count;
    }
    // We store the value instead of incrementing the atomic to avoid finished_count and task_count being the same but not equal to the total task count.
    task_count = local_task_count;
    // This check is to avoid the unlucky event that all threads have already finished when we start waiting, therefor never notifying the atomic variable to stop waiting.
    if (finished_count == task_count) return;

    finished.wait(false);
}

// Function used to deduce the return-type of a function without its arguments, source:
// https://medium.com/@matthieu.dorier/deducing-argument-and-return-types-of-a-callable-in-c-834598f6a385
template <typename Function, typename ReturnType, typename... Args>
Job<ReturnType, Function> JobManager::deduce_return_type(Function function, const std::function<ReturnType(Args...)>&) {
    return Job<ReturnType, Function>(function);
}
#pragma endregion

#pragma region Job function definitions
template <typename ReturnType, typename Function>
template <typename... Args>
void Job<ReturnType, Function>::add_to_queue(Args... args) {
    auto promise = std::make_shared<std::promise<ReturnType>>();  // Make the promise a shared pointer in order to capture it in
                                                                  // the lambda task
    this->futures.push_back(promise->get_future());

    auto task = [this, args..., promise] { promise->set_value(this->function(args...)); };

    engine.job_manager.queue_task(task);
}

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

// Create a large number of tasks in groups to more efficiently perform a lot of small tasks.
// The task count can be a hint while still handling all tasks correct, as long as the hint is not smaller than the real task count.
template <typename Function>
template <typename Container, typename... Args>
void Job<void, Function>::dispatch_to_queue(Container container, const size_t task_count_hint, Args... args) {
    if (task_count_hint <= 0) return;

    const size_t group_count = engine.job_manager.get_thread_count();
    const auto group_size = static_cast<size_t>(std::ceil(static_cast<float>(task_count_hint) / static_cast<float>(group_count)));

    for (size_t group_index = 0; group_index < group_count; group_index++) {
        const size_t start_index = group_index * group_size;  // Index of the first element in the current group.
        if (start_index >= task_count_hint) continue;         // Skip scheduling a task if we have exceeded the job count prematurely.

        auto promise = std::make_shared<std::promise<void>>();
        this->futures.push_back(promise->get_future());

        const size_t max_local_task_count = task_count_hint - start_index;

        // Size of the current group, either group_size or a smaller size depending on the amount of elements left.
        const size_t local_group_size = std::min<size_t>(group_size, max_local_task_count);
        const size_t group_end = start_index + local_group_size;

        auto task_grouping = [this, start_index, group_end, container, promise, args...] {
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

        engine.job_manager.queue_task(task_grouping);
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
// After all tasks have been executed, doesn't clear all tasks until get() or try_get() has been used
template <typename ReturnType, typename Function>
void Job<ReturnType, Function>::wait() {
    for (auto& future : this->futures) future.wait();
}

// Wait for all tasks created with this job to be complete.
// After all tasks have been executed, clears all tasks from the job.
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