#include <iostream>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <cstring>
#include <cstdint>    // <<<<< for uint64_t
#include <time.h>

#define SYS_GET_TIMES 454      // my getter syscall
#define SYS_CUSTOM    451      // my custom syscall

struct syscall_times {
    unsigned long long enter_time;
    unsigned long long exit_time;
};

// Helper function to get nanoseconds since boot using CLOCK_MONOTONIC
uint64_t monotonic_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Helper to measure a syscall
template<typename Func>
void measure_syscall(const std::string &name, Func f) {
    syscall_times times;

    uint64_t A = monotonic_ns(); // user-space enter
    f();                         // perform syscall
    uint64_t D = monotonic_ns(); // user-space exit

    // Get kernel times
    if (syscall(SYS_GET_TIMES, &times) != 0) {
        perror("SYS_GET_TIMES failed");
        return;
    }

    uint64_t B = times.enter_time;
    uint64_t C = times.exit_time;

    std::cout << "=== " << name << " ===\n";
    std::cout << "A (user enter)  : " << A << " ns\n";
    std::cout << "B (kernel enter): " << B << " ns\n";
    std::cout << "C (kernel exit) : " << C << " ns\n";
    std::cout << "D (user exit)   : " << D << " ns\n";

    std::cout << "User->Kernel    : " << (B - A) << " ns\n";
    std::cout << "Kernel Time     : " << (C - B) << " ns\n";
    std::cout << "Kernel->User    : " << (D - C) << " ns\n";
    std::cout << "Total Syscall   : " << (D - A) << " ns\n\n";
}

int main() {
    char buf[1] = {0};

    // Measure read(0, buf, 1)
    measure_syscall("read", [&]() {
        ssize_t ret = read(0, buf, 1);
        if (ret < 0) perror("read failed");
    });

    // Measure write(1, buf, 1)
    measure_syscall("write", [&]() {
        ssize_t ret = write(1, buf, 1);
        if (ret < 0) perror("write failed");
    });

    // Measure getpid()
    measure_syscall("getpid", [&]() {
        pid_t pid = getpid();
        (void)pid;
    });

    // Measure custom syscall 451
    measure_syscall("custom 451", [&]() {
        if (syscall(SYS_CUSTOM) != 0) {
           // perror("custom syscall failed");
        }
    });

    return 0;
}
