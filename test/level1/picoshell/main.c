/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jjorda <jjorda@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 13:28:37 by jjorda            #+#    #+#             */
/*   Updated: 2026/01/08 14:21:16 by jjorda           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* ************************************************************************** */
/*                                                                            */
/*                      PICOSHELL COMPREHENSIVE TESTER                        */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// Your picoshell function
int picoshell(char **cmds[]);

// Color codes for output
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define CYAN "\033[0;36m"
#define RESET "\033[0m"

// Test result structure
typedef struct {
    int passed;
    int failed;
} TestResults;

TestResults results = {0, 0};

void print_header(const char *title) {
    printf("\n%s=== %s ===%s\n", CYAN, title, RESET);
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

// Count open file descriptors
int count_fds(void) {
    int count = 0;
    for (int i = 0; i < 1024; i++) {
        if (fcntl(i, F_GETFD) != -1)
            count++;
    }
    return count;
}

// Helper to run a test
void run_test(const char *test_name, char **cmds[], int expected_return) {
    print_test_name(test_name);
    
    int fd_before = count_fds();
    int ret = picoshell(cmds);
    int fd_after = count_fds();
    
    // Check return value
    if (ret == expected_return) {
        print_success("Return value correct");
    } else {
        char msg[256];
        sprintf(msg, "Return value: expected %d, got %d", expected_return, ret);
        print_failure(msg);
    }
    
    // Check FD leaks
    if (fd_after == fd_before) {
        print_success("No FD leaks detected");
    } else {
        char msg[256];
        sprintf(msg, "FD leak detected: %d FDs before, %d after", fd_before, fd_after);
        print_failure(msg);
    }
}

// Test with output capture
void run_test_with_output(const char *test_name, char **cmds[], 
                          const char *expected_output, int expected_return) {
    print_test_name(test_name);
    
    // Create a pipe to capture stdout
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        print_failure("Failed to create pipe for output capture");
        return;
    }
    
    int fd_before = count_fds();
    int saved_stdout = dup(STDOUT_FILENO);
    
    // Redirect stdout to pipe
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
    
    // Run picoshell
    int ret = picoshell(cmds);
    
    // Restore stdout
    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    
    // Read captured output
    char buffer[4096] = {0};
    read(pipefd[0], buffer, sizeof(buffer) - 1);
    close(pipefd[0]);
    
    int fd_after = count_fds();
    
    // Print the actual output for debugging
    printf("Expected output: '%s'\n", expected_output);
    printf("Actual output:   '%s'\n", buffer);
    
    // Check return value
    if (ret == expected_return) {
        print_success("Return value correct");
    } else {
        char msg[256];
        sprintf(msg, "Return value: expected %d, got %d", expected_return, ret);
        print_failure(msg);
    }
    
    // Check output (strip trailing newline for comparison)
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
        buffer[len - 1] = '\0';
    
    if (strcmp(buffer, expected_output) == 0) {
        print_success("Output matches expected result");
    } else {
        print_failure("Output doesn't match expected result");
    }
    
    // Check FD leaks
    if (fd_after == fd_before) {
        print_success("No FD leaks detected");
    } else {
        char msg[256];
        sprintf(msg, "FD leak detected: %d FDs before, %d after", fd_before, fd_after);
        print_failure(msg);
    }
}

int main(void) {
    printf("%s", CYAN);
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║       PICOSHELL COMPREHENSIVE TEST SUITE                   ║\n");
    printf("║           Testing Pipeline Implementation                  ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("%s\n", RESET);
    
    /* ================================================================ */
    /*                     TEST 1: SIMPLE COMMAND                       */
    /* ================================================================ */
    print_header("TEST 1: Single Command (No Pipeline)");
    {
        char *cmd1[] = {"/bin/echo", "hello", NULL};
        char **cmds[] = {cmd1, NULL};
        run_test_with_output("Single echo command", cmds, "hello", 0);
    }
    
    /* ================================================================ */
    /*                TEST 2: TWO-STAGE PIPELINE (ls | grep)            */
    /* ================================================================ */
    print_header("TEST 2: Two-Stage Pipeline");
    {
        char *cmd1[] = {"/bin/ls", NULL};
        char *cmd2[] = {"/usr/bin/grep", "picoshell", NULL};
        char **cmds[] = {cmd1, cmd2, NULL};
        
        print_test_name("ls | grep picoshell");
        print_info("This should show 'picoshell.c' if the file exists");
        
        int fd_before = count_fds();
        int ret = picoshell(cmds);
        int fd_after = count_fds();
        
        if (ret == 0) {
            print_success("Pipeline executed successfully");
        } else {
            print_failure("Pipeline returned error");
        }
        
        if (fd_after == fd_before) {
            print_success("No FD leaks detected");
        } else {
            char msg[256];
            sprintf(msg, "FD leak: %d FDs before, %d after", fd_before, fd_after);
            print_failure(msg);
        }
    }
    
    /* ================================================================ */
    /*             TEST 3: THREE-STAGE PIPELINE (Example 2)             */
    /* ================================================================ */
    print_header("TEST 3: Three-Stage Pipeline");
    {
        char *cmd1[] = {"/bin/echo", "squalala", NULL};
        char *cmd2[] = {"/bin/cat", NULL};
        char *cmd3[] = {"/bin/sed", "s/a/b/g", NULL};
        char **cmds[] = {cmd1, cmd2, cmd3, NULL};
        run_test_with_output("echo 'squalala' | cat | sed 's/a/b/g'", 
                           cmds, "squblblb", 0);
    }
    
    /* ================================================================ */
    /*           TEST 4: FOUR-STAGE PIPELINE (Stress Test)              */
    /* ================================================================ */
    print_header("TEST 4: Four-Stage Pipeline (Stress Test)");
    {
        char *cmd1[] = {"/bin/echo", "abcdefghij", NULL};
        char *cmd2[] = {"/bin/cat", NULL};
        char *cmd3[] = {"/bin/sed", "s/a/X/g", NULL};
        char *cmd4[] = {"/bin/sed", "s/e/Y/g", NULL};
        char **cmds[] = {cmd1, cmd2, cmd3, cmd4, NULL};
        run_test_with_output("Four-stage pipeline with multiple sed", 
                           cmds, "XbcdYfghij", 0);
    }
    
    /* ================================================================ */
    /*                  TEST 5: INVALID COMMAND                         */
    /* ================================================================ */
    print_header("TEST 5: Error Handling - Invalid Command");
    {
        char *cmd1[] = {"/bin/echo", "test", NULL};
        char *cmd2[] = {"/this/does/not/exist", NULL};
        char **cmds[] = {cmd1, cmd2, NULL};
        
        print_test_name("echo test | /invalid/command");
        print_info("Should return 1 and not leak FDs");
        
        int fd_before = count_fds();
        int ret = picoshell(cmds);
        int fd_after = count_fds();
        
        if (ret == 1) {
            print_success("Correctly returned error code 1");
        } else {
            char msg[256];
            sprintf(msg, "Expected return 1, got %d", ret);
            print_failure(msg);
        }
        
        if (fd_after == fd_before) {
            print_success("No FD leaks on error");
        } else {
            char msg[256];
            sprintf(msg, "FD leak on error: %d before, %d after", fd_before, fd_after);
            print_failure(msg);
        }
    }
    
    /* ================================================================ */
    /*         TEST 6: EMPTY OUTPUT (ls nonexistent | grep)             */
    /* ================================================================ */
    print_header("TEST 6: Empty Pipeline Output");
    {
        char *cmd1[] = {"/bin/ls", "/nonexistent_directory_xyz", NULL};
        char *cmd2[] = {"/usr/bin/grep", "anything", NULL};
        char **cmds[] = {cmd1, cmd2, NULL};
        
        print_test_name("ls /nonexistent | grep anything");
        print_info("First command fails but pipeline should handle it");
        
        int fd_before = count_fds();
        int ret = picoshell(cmds);
        int fd_after = count_fds();
        
        if (ret == 1) {
            print_success("Correctly detected error");
        } else {
            print_info("Some implementations may return 0 if second command succeeds");
        }
        
        if (fd_after == fd_before) {
            print_success("No FD leaks detected");
        } else {
            char msg[256];
            sprintf(msg, "FD leak: %d before, %d after", fd_before, fd_after);
            print_failure(msg);
        }
    }
    
    /* ================================================================ */
    /*              TEST 7: COMPLEX GREP PIPELINE                       */
    /* ================================================================ */
    print_header("TEST 7: Complex Grep Pipeline");
    {
        char *cmd1[] = {"/bin/echo", "apple\nbanana\napricot\navocado", NULL};
        char *cmd2[] = {"/usr/bin/grep", "^a", NULL};
        char *cmd3[] = {"/usr/bin/wc", "-l", NULL};
        char **cmds[] = {cmd1, cmd2, cmd3, NULL};
        
        print_test_name("echo lines | grep '^a' | wc -l");
        print_info("Should count lines starting with 'a'");
        
        int fd_before = count_fds();
        int ret = picoshell(cmds);
        int fd_after = count_fds();
        
        if (ret == 0) {
            print_success("Pipeline executed successfully");
        } else {
            print_failure("Pipeline returned error");
        }
        
        if (fd_after == fd_before) {
            print_success("No FD leaks detected");
        } else {
            char msg[256];
            sprintf(msg, "FD leak: %d before, %d after", fd_before, fd_after);
            print_failure(msg);
        }
    }
    
    /* ================================================================ */
    /*                    TEST 8: LARGE DATA TRANSFER                   */
    /* ================================================================ */
    print_header("TEST 8: Large Data Through Pipeline");
    {
        char *cmd1[] = {"/usr/bin/seq", "1", "1000", NULL};
        char *cmd2[] = {"/usr/bin/tail", "-n", "5", NULL};
        char **cmds[] = {cmd1, cmd2, NULL};
        
        print_test_name("seq 1 1000 | tail -n 5");
        print_info("Testing with larger data volume");
        
        int fd_before = count_fds();
        int ret = picoshell(cmds);
        int fd_after = count_fds();
        
        if (ret == 0) {
            print_success("Large data handled successfully");
        } else {
            print_failure("Failed with large data");
        }
        
        if (fd_after == fd_before) {
            print_success("No FD leaks with large data");
        } else {
            char msg[256];
            sprintf(msg, "FD leak: %d before, %d after", fd_before, fd_after);
            print_failure(msg);
        }
    }
    
    /* ================================================================ */
    /*                      FINAL SUMMARY                               */
    /* ================================================================ */
    printf("\n%s", CYAN);
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                    TEST SUMMARY                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("%s", RESET);
    
    printf("\n%s✅ Tests Passed: %d%s\n", GREEN, results.passed, RESET);
    printf("%s❌ Tests Failed: %d%s\n", RED, results.failed, RESET);
    
    if (results.failed == 0) {
        printf("\n%s🎉 ALL TESTS PASSED! Your picoshell is exam-ready! 🎉%s\n", 
               GREEN, RESET);
    } else {
        printf("\n%s⚠️  Some tests failed. Review the output above.%s\n", 
               YELLOW, RESET);
    }
    
    printf("\n%s💡 Pro tip: Run with valgrind for complete validation:%s\n", CYAN, RESET);
    printf("   valgrind --leak-check=full --track-fds=yes ./test_picoshell\n\n");
    
    return (results.failed == 0) ? 0 : 1;
}