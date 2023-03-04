#include <iostream>
#include <thread>

void do_some_work() { std::cout << "do some work" << std::endl; }
void do_something() { std::cout << "do something" << std::endl; }
void do_something(int i) { std::cout << "do something: " << i << std::endl; }
void do_something_else() { std::cout << "do something else" << std::endl; }
struct func
{
    int &i_;
    func(int &i) : i_(i) {}
    void operator()()
    {
        for (size_t i = 0; i < 100; i++)
        {
            // 潜在访问隐患
            do_something(i_);
        }
    }
};
namespace Start_thread
{
    class background_task
    {
    public:
        void operator()()
        {
            do_something();
            do_something_else();
        }
    };

    int main(void)
    {
        // 1、传入函数指针作为线程入口函数
        std::thread my_thread1(do_some_work);
        // 2、传入仿函数作为线程入口函数
        background_task f;
        std::thread my_thread2(f);
        // 其它两种方法
        // std::thread my_thread2((background_task()));
        // std::thread my_thread2{background_task()};

        // 3、传入lambda表达式作为线程入口函数
        std::thread my_thread3(
            []() -> void
            {
                do_something();
                do_something_else();
            });

        my_thread1.join();
        my_thread2.join();
        my_thread3.join();
        return 0;
    }

    /**
     * @brief 函数已经结束，但是线程依旧访问局部变量
     * @note
     * some_local_state 内存被释放后
     * 分离的线程my_thread 可能还在运行
     */
    void oops()
    {
        int some_local_state = 0;
        func my_func(some_local_state);
        std::thread my_thread(my_func);

        // 分离线程，当前线程结束后my_thread可能还在运行
        my_thread.detach();
        std::cout << "function oops() end." << std::endl;
    }

    int main1(void)
    {
        std::thread t1(oops);
        t1.join();
        getchar();

        return 0;
    }

    void foo1()
    {
        std::thread my_thread1(
            []() -> void
            {
                for (int i = 0; i < 100; i++)
                {
                    do_something();
                }
            });
        std::thread my_thread2(do_some_work);
        // detach() 是分离线程，但要注意当前函数结束后，潜在的访问隐患
        my_thread1.detach();
        // join()是简单粗暴的等待线程完成或不等待
        my_thread2.join();
        std::cout << "function foo1() end." << std::endl;
    }

    int main2(void)
    {
        std::thread t1(foo1);
        t1.join();
        getchar();
        return 0;
    }
} // namespace Start_thread

void do_something(std::string const &str) { std::cout << "[const & " << str << "]"
                                                      << "do something. " << std::endl; }
void do_something(std::string &str) { std::cout << "[& " << str << "]"
                                                << "do something. " << std::endl; }

namespace Parameter_passing
{
    class Base
    {
    public: /*公共*/
        void run() { do_some_work(); }
    };
    int main(void)
    {
        /*
            如果该线程使用detach() 分离时，函数有很大的可能，会在字面值转化成 std::string 对象之前崩溃。
            解决方案就是在传递到 std::thread 构造函数之前就将字面值转化为 std::string 对象
         */
        std::thread my_thread1((void (*)(std::string const &))do_something, "A");

        std::string str("B");
        std::thread my_thread2((void (*)(std::string &))do_something, std::ref(str));

        Base b;
        std::thread my_thread3(&Base::run, &b);

        my_thread1.join();
        my_thread2.join();
        my_thread3.join();
        return 0;
    }

} // namespace Parameter_passing

void some_function()
{
    for (int i = 0; i < 25; ++i)
    {
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "[ID:" << std::this_thread::get_id()
                  << "]:some_function()" << i << std::endl;
    }
}
void some_other_function()
{
    for (int i = 0; i < 25; ++i)
    {
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "[ID:" << std::this_thread::get_id()
                  << "]:some_other_function()" << i << std::endl;
    }
}
#include <vector>
#include <algorithm>
#include <functional>

namespace Transfer_thread_ownership
{
    int main(void)
    {

        std::thread t1(some_function); // 1

        std::thread t2 = std::move(t1); // 2

        t1 = std::thread(some_other_function); // 3

        std::thread t3;     // 4
        t3 = std::move(t2); // 5
        // t1 = std::move(t3);                    // 6 赋值操作将使程序崩溃

        t1.join();
        // t2.join();
        t3.join();
        return 0;
    }

    class scoped_thread
    {
    public: /*公共*/
        scoped_thread(std::thread t) : t_(std::move(t))
        {
            if (!t_.joinable())
            {
                throw std::logic_error("No thread");
            }
        }
        ~scoped_thread()
        {
            t_.join();
        }

        // 删除拷贝
        scoped_thread(scoped_thread const &) = delete;
        scoped_thread &operator=(scoped_thread const &) = delete;

    private: /*私有*/
        std::thread t_;
    };

    int main1(void)
    {
        int some_local_state = 0;
        std::thread my_thread{func(some_local_state)};
        scoped_thread t(std::move(my_thread));
        do_something();

        return 0;
    }
    int main2(void)
    {
        std::vector<std::thread> threads;
        for (unsigned i = 0; i < 20; ++i)
        {
            threads.push_back(std::thread((void (*)(int))do_something, i)); // 产生线程
        }
        std::for_each(threads.begin(), threads.end(),
                      std::mem_fn(&std::thread::join)); // 对每个线程调用join()
        return 0;
    }
} // namespace Transfer_thread_ownership

#include <numeric>
#include "run_time.hpp"

namespace The_runtime_determines_the_number_of_threads
{
    /**
     * @brief 累加块
     * @tparam Iterator
     * @tparam T
     */
    template <typename Iterator, typename T>
    struct accumulate_block
    {
        void operator()(Iterator first, Iterator last, T &result)
        {
            // 累加[first,last)中的值。初始值是result。返回最终的总和。
            result = std::accumulate(first, last, result);
        }
    };

    /**
     * @brief 并行累加
     * @tparam Iterator
     * @tparam T
     * @param first
     * @param last
     * @param init  初始值
     * @return
     */
    template <typename Iterator, typename T>
    T parallel_accumulate(Iterator first, Iterator last, T init)
    {
        // 获取 [first,last) 范围内包含元素的个数
        unsigned long const length = std::distance(first, last);
        // length为0直接返回
        if (!length)
            return init;
        // 每个线程的最小值
        unsigned long const min_per_thread = 25;
        // 需要多少线程
        unsigned long const max_threads =
            (length + min_per_thread - 1) / min_per_thread; // 2
        // 硬件支持的线程数量
        unsigned long const hardware_threads =
            std::thread::hardware_concurrency();
        // 选择线程数
        unsigned long const num_threads = // 3
            std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
        unsigned long const block_size = length / num_threads; // 4
        printf("[num_threads]:%lu\t[block_size]:%lu\t[hardware_threads]:%lu\n", num_threads, block_size, hardware_threads);

        std::vector<T> results(num_threads);
        std::vector<std::thread> threads(num_threads - 1); // 5
        Iterator block_start = first;
        for (unsigned long i = 0; i < (num_threads - 1); ++i)
        {
            Iterator block_end = block_start;
            // block_end+=block_size
            std::advance(block_end, block_size); // 6
            threads[i] = std::thread(            // 7
                accumulate_block<Iterator, T>(),
                block_start, block_end, std::ref(results[i]));
            block_start = block_end; // 8
        }
        // 对剩下的进行计算
        accumulate_block<Iterator, T>()(
            block_start, last, results[num_threads - 1]); // 9

        std::for_each(threads.begin(), threads.end(),
                      std::mem_fn(&std::thread::join));               // 10
        return std::accumulate(results.begin(), results.end(), init); // 11
    }

    int main(void)
    {
        std::vector<int> nums;
        for (size_t i = 0; i < 100000000; i++)
        {
            nums.emplace_back(1);
        }

        int result = 0;
        run_time("parallel_accumulate", [&]
                 { result = parallel_accumulate(nums.begin(), nums.end(), result); });
        std::cout << "result=" << result << std::endl;
        result = 0;
        run_time("accumulate", [&]
                 { result = std::accumulate(nums.begin(), nums.end(), result); });

        std::cout << "result=" << result << std::endl;
        return 0;
    }
} // namespace The_runtime_determines_the_number_of_threads

namespace Identification_thread
{
    int main(void)
    {
        // 获取当前线程的ID
        std::thread::id master_thread = std::this_thread::get_id();
        std::cout << "master_thread ID:" << master_thread << std::endl;

        std::thread my_thread(do_some_work);
        std::cout << "my_thread ID:" << my_thread.get_id() << std::endl;
        my_thread.join();
        return 0;
    }
} // namespace Identification_thread
