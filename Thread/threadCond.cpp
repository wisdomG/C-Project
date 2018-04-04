#include <condition_variable>
#include <mutex>
#include <thread>
#include <iostream>
#include <queue>
#include <chrono>

const int MOD = 5;

int main() {
    std::queue<int> produced_nums;
    std::mutex m;
    std::condition_variable cond_var;
    bool done = false;

    srand(time(NULL));
    std::cout << "begin" << std::endl;
    // 生产者线程
    std::thread producer([&]() {
            for (int i = 0; i < 5; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(rand() % MOD));
                std::unique_lock<std::mutex> lock(m);
                std::cout << "producing " << i << std::endl;
                produced_nums.push(i);
                cond_var.notify_one();
            }
            // 等待队列中的元素都被取走后，该线程结束
            while(!produced_nums.empty());
            // 此时消费者线程会阻塞在等待信号量上，因此需要再通知一次
            done = true;
            std::this_thread::sleep_for(std::chrono::seconds(MOD));
            cond_var.notify_one();
            std::cout << "producer over" << std::endl;
            });

    // 消费者线程
    std::thread consumer([&]() {
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(rand() % MOD));
                std::unique_lock<std::mutex> lock(m);
                if (produced_nums.empty()) {
                    cond_var.wait(lock);
                }
                if (done) {
                    std::cout << "done == true" << std::endl;
                    break;
                }
                std::cout << "consuming " << produced_nums.front() << std::endl;
                produced_nums.pop();
            }
            std::cout << "consumer over" << std::endl;
            });

    producer.join();
    consumer.join();
    std::cout << "over" << std::endl;
    return 0;
}

