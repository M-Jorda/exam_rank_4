/* ************************************************************************** */
/*                                                                            */
/*                      SANDBOX COMPREHENSIVE TESTER                          */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>

// Your sandbox function
int sandbox(void (*f)(void), unsigned int timeout, bool verbose);

// Color codes
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define CYAN "\033[0;36m"
#define MAGENTA "\033[0;35m"
#define RESET "\033[0m"

typedef struct {
    int passed;
    int failed;
} TestResults;

TestResults results = {0, 0};

void print_header(const char *title) {
    printf("\n%s╔════════════════════════════════════════════════════════════╗%s\n", CYAN, RESET);
    printf("%s║  %-56s  ║%s\n", CYAN, title, RESET);
    printf("%s╚════════════════════════════════════════════════════════════╝%s\n", CYAN, RESET);
}

void print_test_name(const char *name) {
    printf("\n%s🧪 Test: %s%s\n", BLUE, name, RESET);
}

void print_success(const char *msg) {
    printf("%s✅ %s%s\n", GREEN, msg, RESET);
    results.passed++;
}

void print_failure(const char *msg) {
    printf("%s❌ %s%s\n", RED, msg, RESET);
    results.failed++;
}

void print_info(const char *msg) {
    printf("%s💡 %s%s\n", YELLOW, msg, RESET);
}

/* ================================================================ */
/*                       TEST FUNCTIONS                             */
/* ================================================================ */

// 1. Nice function - returns normally
void nice_function(void) {
    // Do nothing, just return
}

// 2. Nice function with some work
void nice_function_with_work(void) {
    int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += i;
    }
    (void)sum;
}

// 3. Bad function - segfault
void bad_segfault(void) {
    int *ptr = NULL;
    *ptr = 42;  // Segmentation fault
}

// 4. Bad function - abort
void bad_abort(void) {
    abort();
}

// 5. Bad function - exit with non-zero
void bad_exit_code(void) {
    exit(42);
}

// 6. Bad function - division by zero (SIGFPE)
void bad_div_by_zero(void) {
    int x = 42;
    int y = 0;
    int z = x / y;
    (void)z;
}

// 7. Bad function - infinite loop (will timeout)
void bad_infinite_loop(void) {
    while (1) {
        // Busy wait
    }
}

// 8. Bad function - raise SIGTERM
void bad_sigterm(void) {
    raise(SIGTERM);
}

// 9. Bad function - raise SIGKILL (can't be caught)
void bad_sigkill(void) {
    raise(SIGKILL);
}

// 10. Bad function - illegal instruction
void bad_illegal_instruction(void) {
    #if defined(__x86_64__) || defined(__i386__)
    __asm__("ud2");  // x86 illegal instruction
    #else
    raise(SIGILL);
    #endif
}

// 11. Bad function - bus error
void bad_bus_error(void) {
    raise(SIGBUS);
}

// 12. Nice function - sleeps but within timeout
void nice_function_sleep(void) {
    sleep(1);  // Sleep for 1 second (less than timeout)
}

// 13. Bad function - timeout with sleep
void bad_timeout_sleep(void) {
    sleep(10);  // Sleep longer than timeout
}

// 14. Bad function - writes to invalid address
void bad_write_invalid(void) {
    char *ptr = (char *)0x12345678;
    *ptr = 'A';
}

// 15. Nice function - calls exit(0) explicitly
void nice_function_exit_zero(void) {
    exit(0);
}

/* ================================================================ */
/*                       HELPER FUNCTIONS                           */
/* ================================================================ */

// Capture output helper
typedef struct {
    char buffer[4096];
    int captured;
} CapturedOutput;

CapturedOutput capture_output(void (*test_func)(void)) {
    CapturedOutput output = {{0}, 0};
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return output;
    }
    
    int saved_stdout = dup(STDOUT_FILENO);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
    
    test_func();
    
    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    
    ssize_t n = read(pipefd[0], output.buffer, sizeof(output.buffer) - 1);
    if (n > 0) {
        output.buffer[n] = '\0';
        output.captured = 1;
    }
    close(pipefd[0]);
    
    return output;
}

// Check for zombie processes
int check_zombies(void) {
    int status;
    pid_t pid;
    int zombie_count = 0;
    
    // Non-blocking wait to check for any zombies
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        zombie_count++;
    }
    
    return zombie_count;
}

/* ================================================================ */
/*                          TESTS                                   */
/* ================================================================ */

void test_nice_function(void) {
    print_test_name("Nice Function - Simple Return");
    
    int ret = sandbox(nice_function, 2, true);
    
    if (ret == 1) {
        print_success("Returned 1 (nice function)");
    } else {
        char msg[256];
        sprintf(msg, "Expected 1, got %d", ret);
        print_failure(msg);
    }
    
    int zombies = check_zombies();
    if (zombies == 0) {
        print_success("No zombie processes");
    } else {
        char msg[256];
        sprintf(msg, "Found %d zombie process(es)", zombies);
        print_failure(msg);
    }
}

void test_nice_function_with_work(void) {
    print_test_name("Nice Function - With Work");
    
    int ret = sandbox(nice_function_with_work, 2, false);
    
    if (ret == 1) {
        print_success("Returned 1 (nice function)");
    } else {
        print_failure("Expected 1 (nice function)");
    }
    
    if (check_zombies() == 0) {
        print_success("No zombie processes");
    } else {
        print_failure("Zombie processes detected");
    }
}

void test_segfault(void) {
    print_test_name("Bad Function - Segmentation Fault");
    print_info("Should detect SIGSEGV and print signal description");
    
    printf("Expected output contains: 'Bad function: Segmentation fault'\n");
    int ret = sandbox(bad_segfault, 2, true);
    
    if (ret == 0) {
        print_success("Returned 0 (bad function)");
    } else {
        char msg[256];
        sprintf(msg, "Expected 0, got %d", ret);
        print_failure(msg);
    }
    
    if (check_zombies() == 0) {
        print_success("No zombie processes");
    } else {
        print_failure("Zombie processes detected");
    }
}

void test_abort(void) {
    print_test_name("Bad Function - Abort (SIGABRT)");
    
    printf("Expected output contains: 'Bad function: Aborted'\n");
    int ret = sandbox(bad_abort, 2, true);
    
    if (ret == 0) {
        print_success("Returned 0 (bad function)");
    } else {
        print_failure("Expected 0 (bad function)");
    }
    
    if (check_zombies() == 0) {
        print_success("No zombie processes");
    } else {
        print_failure("Zombie processes detected");
    }
}

void test_exit_code(void) {
    print_test_name("Bad Function - Exit Code 42");
    
    printf("Expected output: 'Bad function: exited with code 42'\n");
    int ret = sandbox(bad_exit_code, 2, true);
    
    if (ret == 0) {
        print_success("Returned 0 (bad function)");
    } else {
        print_failure("Expected 0 (bad function)");
    }
    
    if (check_zombies() == 0) {
        print_success("No zombie processes");
    } else {
        print_failure("Zombie processes detected");
    }
}

void test_timeout(void) {
    print_test_name("Bad Function - Timeout (Infinite Loop)");
    print_info("Testing with 2 second timeout");
    
    printf("Expected output: 'Bad function: timed out after 2 seconds'\n");
    int ret = sandbox(bad_infinite_loop, 2, true);
    
    if (ret == 0) {
        print_success("Returned 0 (bad function)");
    } else {
        print_failure("Expected 0 (bad function)");
    }
    
    if (check_zombies() == 0) {
        print_success("No zombie processes after timeout");
    } else {
        print_failure("Zombie processes detected after timeout");
    }
}

void test_sigterm(void) {
    print_test_name("Bad Function - SIGTERM");
    
    printf("Expected output contains: 'Bad function: Terminated'\n");
    int ret = sandbox(bad_sigterm, 2, true);
    
    if (ret == 0) {
        print_success("Returned 0 (bad function)");
    } else {
        print_failure("Expected 0 (bad function)");
    }
    
    if (check_zombies() == 0) {
        print_success("No zombie processes");
    } else {
        print_failure("Zombie processes detected");
    }
}

void test_sigkill(void) {
    print_test_name("Bad Function - SIGKILL");
    
    printf("Expected output contains: 'Bad function: Killed'\n");
    int ret = sandbox(bad_sigkill, 2, true);
    
    if (ret == 0) {
        print_success("Returned 0 (bad function)");
    } else {
        print_failure("Expected 0 (bad function)");
    }
    
    if (check_zombies() == 0) {
        print_success("No zombie processes");
    } else {
        print_failure("Zombie processes detected");
    }
}

void test_nice_with_sleep(void) {
    print_test_name("Nice Function - Sleep Within Timeout");
    print_info("Sleeps 1 second with 3 second timeout");
    
    int ret = sandbox(nice_function_sleep, 3, true);
    
    if (ret == 1) {
        print_success("Returned 1 (nice function)");
    } else {
        print_failure("Expected 1 (nice function)");
    }
    
    if (check_zombies() == 0) {
        print_success("No zombie processes");
    } else {
        print_failure("Zombie processes detected");
    }
}

void test_timeout_with_sleep(void) {
    print_test_name("Bad Function - Timeout With Sleep");
    print_info("Sleeps 10 seconds with 2 second timeout");
    
    printf("Expected output: 'Bad function: timed out after 2 seconds'\n");
    int ret = sandbox(bad_timeout_sleep, 2, true);
    
    if (ret == 0) {
        print_success("Returned 0 (bad function)");
    } else {
        print_failure("Expected 0 (bad function)");
    }
    
    if (check_zombies() == 0) {
        print_success("No zombie processes after timeout");
    } else {
        print_failure("Zombie processes detected");
    }
}

void test_verbose_off(void) {
    print_test_name("Verbose Mode Off");
    print_info("Should not print any message");
    
    printf("Running sandbox with verbose=false...\n");
    int ret = sandbox(nice_function, 2, false);
    printf("(No output expected above this line)\n");
    
    if (ret == 1) {
        print_success("Returned 1 correctly");
    } else {
        print_failure("Expected 1");
    }
}

void test_nice_exit_zero(void) {
    print_test_name("Nice Function - Explicit exit(0)");
    
    int ret = sandbox(nice_function_exit_zero, 2, true);
    
    if (ret == 1) {
        print_success("Returned 1 (exit(0) is nice)");
    } else {
        print_failure("Expected 1 (exit(0) should be nice)");
    }
    
    if (check_zombies() == 0) {
        print_success("No zombie processes");
    } else {
        print_failure("Zombie processes detected");
    }
}

void test_div_by_zero(void) {
    print_test_name("Bad Function - Division by Zero (SIGFPE)");
    
    printf("Expected output contains: 'Bad function: Floating point exception'\n");
    int ret = sandbox(bad_div_by_zero, 2, true);
    
    if (ret == 0) {
        print_success("Returned 0 (bad function)");
    } else {
        print_failure("Expected 0 (bad function)");
    }
    
    if (check_zombies() == 0) {
        print_success("No zombie processes");
    } else {
        print_failure("Zombie processes detected");
    }
}

/* ================================================================ */
/*                          MAIN                                    */
/* ================================================================ */

int main(void) {
    printf("%s", CYAN);
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║         SANDBOX COMPREHENSIVE TEST SUITE                   ║\n");
    printf("║       Testing Function Safety Sandbox System              ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("%s\n", RESET);
    
    print_header("CATEGORY 1: Nice Functions");
    test_nice_function();
    test_nice_function_with_work();
    test_nice_with_sleep();
    test_nice_exit_zero();
    
    print_header("CATEGORY 2: Bad Functions - Exit Codes");
    test_exit_code();
    
    print_header("CATEGORY 3: Bad Functions - Signals");
    test_segfault();
    test_abort();
    test_sigterm();
    test_sigkill();
    test_div_by_zero();
    
    print_header("CATEGORY 4: Bad Functions - Timeouts");
    test_timeout();
    test_timeout_with_sleep();
    
    print_header("CATEGORY 5: Verbose Mode");
    test_verbose_off();
    
    /* Final Summary */
    printf("\n%s", CYAN);
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                    TEST SUMMARY                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("%s", RESET);
    
    printf("\n%s✅ Tests Passed: %d%s\n", GREEN, results.passed, RESET);
    printf("%s❌ Tests Failed: %d%s\n", RED, results.failed, RESET);
    
    if (results.failed == 0) {
        printf("\n%s🎉 ALL TESTS PASSED! Your sandbox is exam-ready! 🎉%s\n", 
               GREEN, RESET);
    } else {
        printf("\n%s⚠️  Some tests failed. Review the output above.%s\n", 
               YELLOW, RESET);
    }
    
    printf("\n%s💡 Pro tip: Run with valgrind for complete validation:%s\n", CYAN, RESET);
    printf("   valgrind --leak-check=full --track-fds=yes ./test_sandbox\n\n");
    
    return (results.failed == 0) ? 0 : 1;
}