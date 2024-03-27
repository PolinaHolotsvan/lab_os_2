#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

using namespace std;

class Fork {
public:
    int owner;
    condition_variable cv;
    bool clean = false;
    bool requested = false;

    bool give_away(int phil_id) {
        if (phil_id == owner) {
            return true;
        }

        if (clean) {
            requested = true;
            return false;
        } else {
            clean = true;
            owner = phil_id;
            return true;
        }
    }
};

const int N = 10;
enum {
    THINKING, HUNGRY, EATING
} states[N];
mutex mtx;
mutex c;
vector<Fork> forks(N);

void start_eating(int phil_id) {
    unique_lock<mutex> lock(mtx);
    states[phil_id] = HUNGRY;
    {
        unique_lock<mutex> l(c);
        cout << "Philosopher " << phil_id << " is hungry." << endl;
    }


    int left_fork = phil_id;
    int right_fork = (phil_id + 1) % N;

    bool obtained_left;
    bool obtained_right;
    do {
        obtained_left = forks[left_fork].give_away(phil_id);
        if (!obtained_left) forks[left_fork].cv.wait(lock);
        obtained_right = forks[right_fork].give_away(phil_id);
        if (!obtained_right) forks[right_fork].cv.wait(lock);
    } while (!(obtained_left && obtained_right));

    states[phil_id] = EATING;
}

void done_eating(int phil_id) {
    unique_lock<mutex> lock(mtx);
    {
        unique_lock<mutex> l(c);
        cout << "Philosopher " << phil_id << " has finished eating." << endl;
    }
    int left_fork = phil_id;
    int right_fork = (phil_id + 1) % N;

    forks[left_fork].clean = false;
    forks[right_fork].clean = false;

    if (forks[left_fork].requested) {
        forks[left_fork].requested = false;
        forks[left_fork].cv.notify_all();
    }

    if (forks[right_fork].requested) {
        forks[right_fork].requested = false;
        forks[right_fork].cv.notify_all();
    }

    states[phil_id] = THINKING;
}

void display_status() {
    system("cls");
    cout << "id \t states \n";
    for (int i = 0; i < N; i++) {
        cout << i << "\t" << to_string(states[i]) << endl;
    }
}

void philosopher(int phil_id) {
    while (true) {
        {
            unique_lock<mutex> l(c);
            cout << "Philosopher " << phil_id << " is thinking." << endl;
        }

        this_thread::sleep_for(chrono::milliseconds(100));
        start_eating(phil_id);
        {
            unique_lock<mutex> l(c);
            cout << "Philosopher " << phil_id << " is eating." << endl;
        }

        this_thread::sleep_for(chrono::milliseconds(100));
        done_eating(phil_id);
    }
}

int main() {
    vector<thread> philosophers;
    for (int i = 0; i < N; ++i) {
        forks[i].owner = i;
        philosophers.push_back(thread(philosopher, i));
    }
    for (int i = 0; i < N; ++i) {
        philosophers[i].join();
    }
    display_status();
    return 0;
}
