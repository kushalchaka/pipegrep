#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <cassert>

using namespace std;

// --- Bounded Buffer Class ---
class BoundedBuffer {
private:
    vector<string> buffer;
    int capacity;
    int head, tail, count;
    mutex mtx;
    condition_variable notFull, notEmpty;

public:
    BoundedBuffer(int size) : capacity(size), head(0), tail(0), count(0) {
        buffer.resize(size);
    }

    void add(string item) {
        unique_lock<mutex> lock(mtx);
        while (count == capacity) {
            notFull.wait(lock);
        }
        buffer[tail] = item;
        tail = (tail + 1) % capacity;
        count++;
        notEmpty.notify_one();
    }

    string remove() {
        unique_lock<mutex> lock(mtx);
        while (count == 0) {
            notEmpty.wait(lock);
        }
        string item = buffer[head];
        head = (head + 1) % capacity;
        count--;
        notFull.notify_one();
        return item;
    }
};

// --- Global Buffers ---
// You will create these in main based on buffsize
BoundedBuffer* buf1 = nullptr;

// --- Stage 1: Filename Acquisition ---
void stage1() {
    DIR* dir = opendir(".");
    if (dir == nullptr) {
        perror("Could not open directory");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        struct stat st;
        if (stat(entry->d_name, &st) == 0 && S_ISREG(st.st_mode)) {
            buf1->add(string(entry->d_name));
        }
    }
    closedir(dir);

    // Sentinel to signal Stage 2 that directory scan is complete
    buf1->add("DONE");
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0] << " <buffsize> <filesize> <uid> <gid> <string>" << endl;
        return 1;
    }

    int buffsize = stoi(argv[1]);

    // Initialize buffer 1
    buf1 = new BoundedBuffer(buffsize);

    // Launch Stage 1
    thread t1(stage1);

    // For now, just test that Stage 1 is working by consuming from buf1
    // (You will replace this with Stage 2 once you implement it)
    while (true) {
        string file = buf1->remove();
        if (file == "DONE") break;
        cout << "Stage 1 found: " << file << endl;
    }

    t1.join();
    delete buf1;
    return 0;
}
