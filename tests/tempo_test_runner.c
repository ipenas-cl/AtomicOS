/*
 * Tempo Compiler Test Suite
 * Comprehensive testing framework for AtomicOS Tempo language
 * Version: 0.7.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>

typedef struct {
    char* name;
    char* input_file;
    char* expected_output;
    int expected_exit_code;
    int expected_wcet_cycles;
    int expected_security_level;
    int should_compile_successfully;
} test_case_t;

static int tests_passed = 0;
static int tests_failed = 0;
static int total_tests = 0;

// Test output colors
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_RESET "\033[0m"

void print_test_header() {
    printf("\n");
    printf("=================================================\n");
    printf("  AtomicOS Tempo Compiler Test Suite v0.7.0\n");
    printf("=================================================\n\n");
}

void print_test_result(const char* test_name, int passed, const char* message) {
    total_tests++;
    if (passed) {
        tests_passed++;
        printf("[" COLOR_GREEN "PASS" COLOR_RESET "] %s\n", test_name);
    } else {
        tests_failed++;
        printf("[" COLOR_RED "FAIL" COLOR_RESET "] %s: %s\n", test_name, message);
    }
}

void print_test_summary() {
    printf("\n=================================================\n");
    printf("Test Summary:\n");
    printf("  Total: %d\n", total_tests);
    printf("  " COLOR_GREEN "Passed: %d" COLOR_RESET "\n", tests_passed);
    printf("  " COLOR_RED "Failed: %d" COLOR_RESET "\n", tests_failed);
    printf("  Success Rate: %.1f%%\n", (float)tests_passed / total_tests * 100);
    printf("=================================================\n");
}

int run_compiler_test(const char* input_file, const char* output_file) {
    char command[512];
    snprintf(command, sizeof(command), "./build/tempo_compiler %s %s 2>/dev/null", input_file, output_file);
    
    int exit_code = system(command);
    return WEXITSTATUS(exit_code);
}

int check_file_exists(const char* filename) {
    return access(filename, F_OK) == 0;
}

int check_output_contains(const char* filename, const char* expected_text) {
    FILE* file = fopen(filename, "r");
    if (!file) return 0;
    
    char buffer[4096];
    int found = 0;
    
    while (fgets(buffer, sizeof(buffer), file)) {
        if (strstr(buffer, expected_text)) {
            found = 1;
            break;
        }
    }
    
    fclose(file);
    return found;
}

int extract_wcet_from_output(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return -1;
    
    char buffer[256];
    int wcet = -1;
    
    while (fgets(buffer, sizeof(buffer), file)) {
        if (strstr(buffer, "Total WCET:")) {
            sscanf(buffer, "; Total WCET: %d cycles", &wcet);
            break;
        }
    }
    
    fclose(file);
    return wcet;
}

void test_basic_compilation() {
    printf(COLOR_BLUE "Running Basic Compilation Tests...\n" COLOR_RESET);
    
    // Test 1: Simple function compilation
    int exit_code = run_compiler_test("tests/fixtures/simple_function.tempo", "tests/output/simple_function.s");
    print_test_result("Simple Function Compilation", exit_code == 0, "Compiler should succeed");
    
    // Test 2: Check output file exists
    print_test_result("Output File Creation", check_file_exists("tests/output/simple_function.s"), "Output file should be created");
    
    // Test 3: Check assembly structure
    print_test_result("Assembly Structure", check_output_contains("tests/output/simple_function.s", "section .text"), "Should contain proper assembly sections");
}

void test_wcet_analysis() {
    printf(COLOR_BLUE "Running WCET Analysis Tests...\n" COLOR_RESET);
    
    // Test WCET calculation
    run_compiler_test("tests/fixtures/wcet_test.tempo", "tests/output/wcet_test.s");
    int wcet = extract_wcet_from_output("tests/output/wcet_test.s");
    
    print_test_result("WCET Calculation", wcet > 0, "Should calculate positive WCET");
    print_test_result("WCET Reasonable Range", wcet < 1000, "WCET should be in reasonable range");
}

void test_error_handling() {
    printf(COLOR_BLUE "Running Error Handling Tests...\n" COLOR_RESET);
    
    // Test syntax error handling
    int exit_code = run_compiler_test("tests/fixtures/syntax_error.tempo", "tests/output/syntax_error.s");
    print_test_result("Syntax Error Detection", exit_code != 0, "Should fail on syntax errors");
    
    // Test missing file handling
    exit_code = run_compiler_test("tests/fixtures/nonexistent.tempo", "tests/output/nonexistent.s");
    print_test_result("Missing File Handling", exit_code != 0, "Should fail on missing input file");
}

void test_security_features() {
    printf(COLOR_BLUE "Running Security Feature Tests...\n" COLOR_RESET);
    
    // Test security level compilation
    run_compiler_test("tests/fixtures/security_test.tempo", "tests/output/security_test.s");
    print_test_result("Security Feature Compilation", check_file_exists("tests/output/security_test.s"), "Security features should compile");
}

void test_realtime_features() {
    printf(COLOR_BLUE "Running Real-Time Feature Tests...\n" COLOR_RESET);
    
    // Test real-time scheduling compilation
    run_compiler_test("tests/fixtures/realtime_test.tempo", "tests/output/realtime_test.s");
    print_test_result("Real-Time Feature Compilation", check_file_exists("tests/output/realtime_test.s"), "Real-time features should compile");
}

void create_test_fixtures() {
    // Create test fixtures directory
    system("mkdir -p tests/fixtures tests/output");
    
    // Simple function test
    FILE* f = fopen("tests/fixtures/simple_function.tempo", "w");
    fprintf(f, "function test_function(): int32 {\n");
    fprintf(f, "    return 42\n");
    fprintf(f, "}\n");
    fclose(f);
    
    // WCET test
    f = fopen("tests/fixtures/wcet_test.tempo", "w");
    fprintf(f, "function wcet_test(): int32 {\n");
    fprintf(f, "    let a = 10\n");
    fprintf(f, "    let b = 20\n");
    fprintf(f, "    let c = a + b\n");
    fprintf(f, "    return c\n");
    fprintf(f, "}\n");
    fclose(f);
    
    // Syntax error test
    f = fopen("tests/fixtures/syntax_error.tempo", "w");
    fprintf(f, "function broken_function(\n");  // Missing closing parenthesis
    fprintf(f, "    return 42\n");
    fprintf(f, "}\n");
    fclose(f);
    
    // Security test
    f = fopen("tests/fixtures/security_test.tempo", "w");
    fprintf(f, "function security_check(): int32 {\n");
    fprintf(f, "    let level = SECURITY_LEVEL_SYSTEM()\n");
    fprintf(f, "    return level\n");
    fprintf(f, "}\n");
    fclose(f);
    
    // Real-time test
    f = fopen("tests/fixtures/realtime_test.tempo", "w");
    fprintf(f, "function rt_task(): int32 {\n");
    fprintf(f, "    let priority = RT_PRIORITY_HIGH()\n");
    fprintf(f, "    return priority\n");
    fprintf(f, "}\n");
    fclose(f);
}

void test_regression_suite() {
    printf(COLOR_BLUE "Running Regression Tests...\n" COLOR_RESET);
    
    // Test all example files still compile
    DIR* dir = opendir("examples");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, ".tempo")) {
                char input_path[256];
                char output_path[256];
                snprintf(input_path, sizeof(input_path), "examples/%s", entry->d_name);
                snprintf(output_path, sizeof(output_path), "tests/output/regression_%s.s", entry->d_name);
                
                int exit_code = run_compiler_test(input_path, output_path);
                char test_name[256];
                snprintf(test_name, sizeof(test_name), "Regression: %s", entry->d_name);
                print_test_result(test_name, exit_code == 0, "Example file should still compile");
            }
        }
        closedir(dir);
    }
}

int main(int argc, char* argv[]) {
    print_test_header();
    
    // Check if compiler exists
    if (!check_file_exists("build/tempo_compiler")) {
        printf(COLOR_RED "Error: Tempo compiler not found. Run 'make all' first.\n" COLOR_RESET);
        return 1;
    }
    
    // Create test fixtures
    create_test_fixtures();
    
    // Run test suites
    test_basic_compilation();
    test_wcet_analysis();
    test_error_handling();
    test_security_features();
    test_realtime_features();
    test_regression_suite();
    
    // Print summary
    print_test_summary();
    
    // Cleanup
    system("rm -rf tests/fixtures tests/output");
    
    return tests_failed > 0 ? 1 : 0;
}