#include "ThreadPool.h"
using namespace std;

int main() {
    ThreadPool pool(4);
    vector<future<string>> results;

    for (int i = 0; i < 8; ++i) {
        results.emplace_back(pool.enqueue([i]{
                    cout << "hello " << i << endl;
                    this_thread::sleep_for(chrono::seconds(1));
                    cout << "world" << i << endl;
                    return "---thread " + to_string(i) + "finished.---";
                    }));
    }

    for (auto &&result: results) {
        cout << result.get() << ' ' << endl;
    }
    return 0;
}
