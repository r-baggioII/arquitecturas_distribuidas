#include <iostream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>
#include <future>
#include <atomic>
#include <algorithm>
#include <mutex>

using namespace std;

// Global mutex for thread-safe console output
mutex cout_mutex;

// Structure to pass parameters to each thread
struct ThreadParams {
    long double x;
    int start;
    int end;
    int thread_id;
};

// ==================== IMPROVED LOGARITHM CALCULATION ====================
// Uses the identity: ln(x) = ln(2^k * m) = k*ln(2) + ln(m)
// where m is in [1, 2) and we use Taylor series for ln(m)

long double calculate_ln_range_reduction(long double x, int num_terms) {
    if (x <= 0) {
        cerr << "Error: Logarithm is not defined for numbers <= 0" << endl;
        return 0.0L;
    }
    
    // Range reduction: express x = 2^k * m where 1 <= m < 2
    int k = 0;
    long double m = x;
    
    // Reduce to range [1, 2)
    while (m >= 2.0L) {
        m /= 2.0L;
        k++;
    }
    while (m < 1.0L) {
        m *= 2.0L;
        k--;
    }
    
    // Now use Taylor series for ln(m) around m=1
    // ln(1+t) = t - t²/2 + t³/3 - t⁴/4 + ... where t = m-1
    long double t = m - 1.0L;
    long double result = 0.0L;
    long double term = t;
    
    for (int n = 1; n <= num_terms; n++) {
        if (n % 2 == 1) {
            result += term / n;
        } else {
            result -= term / n;
        }
        term *= t;
        
        // Early termination if term becomes negligible
        if (fabsl(term / n) < 1e-18L) {
            break;
        }
    }
    
    // ln(x) = k*ln(2) + ln(m)
    const long double ln2 = 0.6931471805599453094172321214581766L;
    return k * ln2 + result;
}

// ==================== THREADED VERSION WITH RANGE REDUCTION ====================
long double calculate_ln_part_threaded(const ThreadParams& params) {
    long double x = params.x;
    int start = params.start;
    int end = params.end;
    
    // Range reduction
    int k = 0;
    long double m = x;
    
    while (m >= 2.0L) {
        m /= 2.0L;
        k++;
    }
    while (m < 1.0L) {
        m *= 2.0L;
        k--;
    }
    
    long double t = m - 1.0L;
    long double local_result = 0.0L;
    long double term = powl(t, start + 1); // t^(start+1)
    
    // Calculate terms from start to end-1
    for (int n = start + 1; n <= end; n++) {
        if (n % 2 == 1) {
            local_result += term / n;
        } else {
            local_result -= term / n;
        }
        term *= t;
        
        if (fabsl(term / (n + 1)) < 1e-20L) {
            break;
        }
    }
    
    return local_result;
}

long double calculate_ln_with_threads_improved(long double x, int num_threads) {
    int TOTAL_TERMS = 1000; // Much fewer terms needed with range reduction
    
    // Adjust terms to make them divisible by num_threads
    if (TOTAL_TERMS % num_threads != 0) {
        TOTAL_TERMS = ((TOTAL_TERMS / num_threads) + 1) * num_threads;
    }
    
    int terms_per_thread = TOTAL_TERMS / num_threads;
    
    // Range reduction (same for all threads)
    int k = 0;
    long double m = x;
    
    while (m >= 2.0L) {
        m /= 2.0L;
        k++;
    }
    while (m < 1.0L) {
        m *= 2.0L;
        k--;
    }
    
    cout << "Calculating ln(" << fixed << setprecision(6) << x << ") with range reduction..." << endl;
    cout << "x = 2^" << k << " * " << setprecision(10) << m << endl;
    cout << "Using " << TOTAL_TERMS << " terms with " << num_threads << " threads..." << endl;
    
    vector<future<long double>> futures;
    futures.reserve(num_threads);
    
    // Create threads to calculate different parts of the series
    for (int i = 0; i < num_threads; i++) {
        ThreadParams params = {
            x,
            i * terms_per_thread,
            (i + 1) * terms_per_thread - 1,
            i + 1
        };
        
        futures.push_back(async(launch::async, calculate_ln_part_threaded, params));
    }
    
    // Collect results
    long double series_result = 0.0L;
    int completed = 0;
    
    cout << "Progress: ";
    for (auto& future : futures) {
        series_result += future.get();
        completed++;
        cout << completed << "/" << num_threads << " ";
        cout.flush();
    }
    cout << "Done!" << endl;
    
    // Add k*ln(2) + first term (t)
    const long double ln2 = 0.6931471805599453094172321214581766L;
    long double t = m - 1.0L;
    return k * ln2 + t + series_result;
}



// ==================== VERSION WITHOUT THREADS (IMPROVED) ====================
long double calculate_ln_without_threads_improved(long double x) {
    if (x <= 0) {
        cerr << "Error: Logarithm is not defined for numbers <= 0" << endl;
        return 0.0L;
    }
    
    // Range reduction
    int k = 0;
    long double m = x;
    
    while (m >= 2.0L) {
        m /= 2.0L;
        k++;
    }
    while (m < 1.0L) {
        m *= 2.0L;
        k--;
    }
    
    cout << "Calculating ln(" << fixed << setprecision(6) << x << ") with range reduction (WITHOUT THREADS)..." << endl;
    cout << "x = 2^" << k << " * " << setprecision(10) << m << endl;
    
    // Taylor series for ln(1+t) where t = m-1
    long double t = m - 1.0L;
    long double result = 0.0L;
    long double term = t;
    
    const int MAX_TERMS = 1000;
    int terms_used = 0;
    
    for (int n = 1; n <= MAX_TERMS; n++) {
        long double current_term = term / n;
        
        if (n % 2 == 1) {
            result += current_term;
        } else {
            result -= current_term;
        }
        
        terms_used = n;
        
        // Early termination if term becomes negligible
        if (fabsl(current_term) < 1e-18L) {
            break;
        }
        
        term *= t;
    }
    
    cout << "Terms used: " << terms_used << " (converged)" << endl;
    
    // ln(x) = k*ln(2) + ln(m)
    const long double ln2 = 0.6931471805599453094172321214581766L;
    return k * ln2 + result;
}

// ==================== AUXILIARY FUNCTIONS ====================
int get_num_cores() {
    int cores = thread::hardware_concurrency();
    return (cores > 0) ? cores : 4;
}

bool is_input_valid(long double x) {
    return x > 0.0L;
}

double calculate_speedup(double time_without_threads, double time_with_threads) {
    if (time_with_threads <= 0) return 0.0;
    return time_without_threads / time_with_threads;
}

// ==================== MAIN MENU FUNCTIONS ====================
void execute_without_threads(long double x) {
    cout << "\n=== EXECUTION WITHOUT THREADS ===" << endl;
    
    auto start = chrono::high_resolution_clock::now();
    long double result = calculate_ln_without_threads_improved(x);
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    
    cout << "\n--- RESULTS WITHOUT THREADS ---" << endl;
    cout << "Number: " << fixed << setprecision(6) << x << endl;
    cout << "Result ln(x): " << setprecision(15) << result << endl;
    cout << "Library ln(x): " << setprecision(15) << logl(x) << endl;
    cout << "Execution time: " << duration.count() << " µs" << endl;
    
    // Calculate error
    long double library_result = logl(x);
    long double error = fabsl(result - library_result);
    cout << "Absolute error: " << scientific << error << endl;
    cout << "Relative error: " << setprecision(2) << fixed 
         << (error / library_result * 100) << "%" << endl;
}

void execute_with_threads(long double x) {
    cout << "\n=== EXECUTION WITH THREADS ===" << endl;
    
    int cores = get_num_cores();
    cout << "Detected CPU cores: " << cores << endl;
    
    int num_threads;
    cout << "Enter number of threads to use (1-" << cores * 2 << "): ";
    
    if (!(cin >> num_threads)) {
        cout << "Error: Invalid input" << endl;
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }
    
    if (num_threads <= 0) {
        cout << "Error: Number of threads must be positive" << endl;
        return;
    }
    
    auto start = chrono::high_resolution_clock::now();
    long double result = calculate_ln_with_threads_improved(x, num_threads);
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    
    cout << "\n--- RESULTS WITH THREADS ---" << endl;
    cout << "Number: " << fixed << setprecision(6) << x << endl;
    cout << "Threads used: " << num_threads << endl;
    cout << "Result ln(x): " << setprecision(15) << result << endl;
    cout << "Library ln(x): " << setprecision(15) << logl(x) << endl;
    cout << "Execution time: " << duration.count() << " µs" << endl;
    
    // Calculate error
    long double library_result = logl(x);
    long double error = fabsl(result - library_result);
    cout << "Absolute error: " << scientific << error << endl;
    cout << "Relative error: " << setprecision(2) << fixed 
         << (error / library_result * 100) << "%" << endl;
}

void execute_comparison(long double x) {
    cout << "\n=== PERFORMANCE COMPARISON ===" << endl;
    
    // Execute without threads
    cout << "\n1. Executing WITHOUT THREADS version..." << endl;
    auto start_without = chrono::high_resolution_clock::now();
    long double result_without = calculate_ln_without_threads_improved(x);
    auto end_without = chrono::high_resolution_clock::now();
    auto duration_without = chrono::duration_cast<chrono::microseconds>(end_without - start_without);
    
    // Request number of threads
    int cores = get_num_cores();
    cout << "\nDetected CPU cores: " << cores << endl;
    
    int num_threads;
    cout << "Enter number of threads for comparison: ";
    
    if (!(cin >> num_threads)) {
        cout << "Error: Invalid input" << endl;
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }
    
    if (num_threads <= 0) {
        cout << "Error: Number of threads must be positive" << endl;
        return;
    }
    
    // Execute with threads
    cout << "\n2. Executing WITH THREADS version (" << num_threads << " threads)..." << endl;
    auto start_with = chrono::high_resolution_clock::now();
    long double result_with = calculate_ln_with_threads_improved(x, num_threads);
    auto end_with = chrono::high_resolution_clock::now();
    auto duration_with = chrono::duration_cast<chrono::microseconds>(end_with - start_with);
    
    // Calculate metrics
    double speedup = calculate_speedup(duration_without.count(), duration_with.count());
    double efficiency = speedup / num_threads * 100.0;
    long double library_result = logl(x);
    
    // Show comparative results
    cout << "\n=== COMPARISON RESULTS ===" << endl;
    cout << "Number analyzed: " << fixed << setprecision(6) << x << endl;
    cout << "Library result: " << setprecision(15) << library_result << endl;
    
    cout << "\n--- Without Threads Version ---" << endl;
    cout << "Result: " << setprecision(15) << result_without << endl;
    cout << "Time: " << duration_without.count() << " µs" << endl;
    cout << "Error: " << scientific << fabsl(result_without - library_result) << endl;
    
    cout << "\n--- With Threads Version ---" << endl;
    cout << "Threads used: " << num_threads << endl;
    cout << "Result: " << setprecision(15) << result_with << endl;
    cout << "Time: " << duration_with.count() << " µs" << endl;
    cout << "Error: " << scientific << fabsl(result_with - library_result) << endl;
    
    cout << "\n--- Performance Analysis ---" << endl;
    cout << "Speedup: " << fixed << setprecision(2) << speedup << "x" << endl;
    cout << "Efficiency: " << setprecision(1) << efficiency << "%" << endl;
    
    if (speedup > 1.0) {
        cout << "Time improvement: " << setprecision(1) 
             << (1.0 - (double)duration_with.count() / duration_without.count()) * 100 << "%" << endl;
    } else {
        cout << "Time loss: " << setprecision(1) 
             << ((double)duration_with.count() / duration_without.count() - 1.0) * 100 << "%" << endl;
    }
    
    // Difference between results
    long double difference = fabsl(result_without - result_with);
    cout << "Difference between results: " << scientific << difference << endl;
    
    // Recommendation
    cout << "\n--- Recommendation ---" << endl;
    if (speedup > 1.0) {
        cout << "✓ Parallelization is beneficial for this case." << endl;
    } else {
        cout << "⚠ Parallelization overhead exceeds benefits. Use sequential version." << endl;
    }
}

void execute_benchmark(long double x) {
    cout << "\n=== AUTOMATIC BENCHMARK ===" << endl;
    cout << "Testing different numbers of threads..." << endl;
    
    int cores = get_num_cores();
    vector<int> threads_to_test = {1, 2, 4, 8, 16};
    
    cout << "Detected cores: " << cores << endl;
    cout << "Testing with: ";
    for (int h : threads_to_test) cout << h << " ";
    cout << "threads" << endl;
    
    vector<pair<int, double>> results;
    long double library_result = logl(x);
    
    for (int num_threads : threads_to_test) {
        cout << "\nTesting with " << num_threads << " thread(s)..." << endl;
        
        auto start = chrono::high_resolution_clock::now();
        long double result = (num_threads == 1) ? 
            calculate_ln_without_threads_improved(x) : 
            calculate_ln_with_threads_improved(x, num_threads);
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
        
        double time_us = duration.count();
        results.push_back({num_threads, time_us});
        
        cout << "Time: " << fixed << setprecision(0) << time_us << " µs" << endl;
        cout << "Error: " << scientific << fabsl(result - library_result) << endl;
    }
    
    // Show benchmark results
    cout << "\n=== BENCHMARK RESULTS ===" << endl;
    cout << "Threads\tTime(µs)\tSpeedup\tEfficiency" << endl;
    cout << "-------\t--------\t-------\t----------" << endl;
    
    double base_time = results[0].second;
    
    for (auto& result : results) {
        int threads = result.first;
        double time_us = result.second;
        double speedup = base_time / time_us;
        double efficiency = speedup / threads * 100.0;
        
        cout << threads << "\t" << fixed << setprecision(0) << time_us 
             << "\t\t" << setprecision(2) << speedup 
             << "\t" << setprecision(1) << efficiency << "%" << endl;
    }
    
    // Find best configuration
    auto best = *min_element(results.begin(), results.end(), 
        [](const pair<int,double>& a, const pair<int,double>& b) {
            return a.second < b.second;
        });
    
    cout << "\n--- Recommendation ---" << endl;
    cout << "Optimal configuration: " << best.first << " threads" << endl;
    cout << "Minimum time: " << fixed << setprecision(0) << best.second << " µs" << endl;
}

int main() {
    cout << "=================================================================" << endl;
    cout << "    NATURAL LOGARITHM CALCULATION SYSTEM WITH TAYLOR SERIES" << endl;
    cout << "                    CORRECTED VERSION                          " << endl;
    cout << "=================================================================" << endl;
    cout << "Method: Range reduction + Taylor series ln(1+t)" << endl;
    cout << "Precision: long double" << endl;
    cout << "Detected CPU cores: " << get_num_cores() << endl;
    cout << "=================================================================" << endl;
    
    long double x;
    int option;
    
    while (true) {
        cout << "\n=== MAIN MENU ===" << endl;
        cout << "1. Execution WITHOUT threads" << endl;
        cout << "2. Execution WITH threads" << endl;
        cout << "3. Performance comparison (both versions)" << endl;
        cout << "4. Automatic benchmark (test multiple configurations)" << endl;
        cout << "5. Exit" << endl;
        cout << "Select an option (1-5): ";
        
        if (!(cin >> option)) {
            cout << "Invalid input. Please enter a number." << endl;
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }
        
        if (option == 5) {
            cout << "\nThank you for using the system!" << endl;
            break;
        }
        
        if (option < 1 || option > 5) {
            cout << "Invalid option. Please try again." << endl;
            continue;
        }
        
        cout << "\nEnter a positive number: ";
        if (!(cin >> x)) {
            cout << "Error: Invalid number format" << endl;
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }
        
        if (!is_input_valid(x)) {
            cout << "Error: Number must be positive" << endl;
            continue;
        }
        
        switch (option) {
            case 1:
                execute_without_threads(x);
                break;
            case 2:
                execute_with_threads(x);
                break;
            case 3:
                execute_comparison(x);
                break;
            case 4:
                execute_benchmark(x);
                break;
        }
        
        cout << "\n=================================================================" << endl;
    }
    
    return 0;
}