#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <fstream>

using namespace std;

// --- Bounded Buffer ---
class BoundedBuffer {
private:
    vector<string> buffer;
    int capacity, head, tail, count;
    mutex mtx;
    condition_variable notFull, notEmpty;
public:
    BoundedBuffer(int size) : capacity(size), head(0), tail(0), count(0) { buffer.resize(size); }
    void add(string item) {
        unique_lock<mutex> lock(mtx);
        while (count == capacity) notFull.wait(lock);
        buffer[tail] = item;
        tail = (tail + 1) % capacity;
        count++;
        notEmpty.notify_one();
    }
    string remove() {
        unique_lock<mutex> lock(mtx);
        while (count == 0) notEmpty.wait(lock);
        string item = buffer[head];
        head = (head + 1) % capacity;
        count--;
        notFull.notify_one();
        return item;
    }
};

BoundedBuffer *buf1, *buf2, *buf3, *buf4;

void stage1() {
    DIR* dir = opendir(".");
    struct dirent* entry;
    if (dir) {
        while ((entry = readdir(dir))) {
            struct stat st;
            if (stat(entry->d_name, &st) == 0 && S_ISREG(st.st_mode))
                buf1->add(string(entry->d_name));
        }
        closedir(dir);
    }
    buf1->add("DONE");
}

void stage2(long fs, int uid, int gid) {
    while (true) {
        string f = buf1->remove();
        if (f == "pipegrep" || f == "pipegrep.o") continue;
        if (f == "DONE") { buf2->add("DONE"); break; }
        struct stat st;
        if (stat(f.c_str(), &st) == 0) {
            if ((fs == -1 || st.st_size > fs) && (uid == -1 || st.st_uid == (uid_t)uid) && (gid == -1 || st.st_gid == (gid_t)gid))
                buf2->add(f);
        }
    }
}

void stage3() {
    while (true) {
        string f = buf2->remove();
        if (f == "DONE") { buf3->add("DONE"); break; }
        ifstream file(f);
        string line;
        while (getline(file, line)) buf3->add(f + ": " + line);
    }
}

void stage4(string target) {
    while (true) {
        string line = buf3->remove();
        if (line == "DONE") { buf4->add("DONE"); break; }
        if (line.find(target) != string::npos) buf4->add(line);
    }
}

void stage5(int& matches) {
    while (true) {
        string line = buf4->remove();
        if (line == "DONE") break;
        cout << line << endl;
        matches++;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 6) { cerr << "Usage: " << argv[0] << " <buffsize> <filesize> <uid> <gid> <string>" << endl; return 1; }
    int b = stoi(argv[1]);
    buf1 = new BoundedBuffer(b); buf2 = new BoundedBuffer(b); buf3 = new BoundedBuffer(b); buf4 = new BoundedBuffer(b);
    int matches = 0;
    thread t1(stage1), t2(stage2, stol(argv[2]), stoi(argv[3]), stoi(argv[4])), t3(stage3), t4(stage4, argv[5]), t5(stage5, ref(matches));
    t1.join(); t2.join(); t3.join(); t4.join(); t5.join();
    cout << "***** You found " << matches << " matches *****" << endl;
    delete buf1;
    delete buf2;
    delete buf3;
    delete buf4;
    return 0;
}
