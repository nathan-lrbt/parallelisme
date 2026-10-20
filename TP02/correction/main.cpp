#include <iostream>
#include <vector>
#include <chrono> // For perf measure
#include <cstdint> // For bonus, fixed size integer
#include <random> // For bonus, random number
#include <array>

#include <omp.h>

void side_effect_function(float& value, size_t index) {
    value = value + index * 0.1f;
}

int main() {

    // Task 1
    {
        // OpenMP chooses the number of threads
        #pragma omp parallel
        {
            std::cout << "Hello from " << omp_get_thread_num() << std::endl;
        }
        // Hellos come in a different order due to non-deterministic thread execution
    }

    // Task 2
    {
        const std::vector<float> data{ 0., 2., 1., 3., 5., 4. };
        std::vector<float> copy(data.size(), 0.);

        #pragma omp parallel for
        for(size_t i = 0; i < data.size(); ++i) {
            copy[i] = data[i];
        }

        for(const auto v: copy) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
    }


    // Task 3
    {
        std::vector<float> data(10, 0.);
        float seed = 7.f;

        omp_set_num_threads(4);

        #pragma omp parallel for firstprivate(seed)
        for (size_t i = 0; i < data.size(); ++i) {
            side_effect_function(seed, i + 1);
            data[i] = seed;
        }

        for(const auto v: data) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
        std::cout << seed << std::endl;

        // firstprivate(seed) : each thread has its own copy of seed = 7.f, no race condition but
        //                      results vary depending on the number of threads used
        // private(seed) : each thread has its own uninitialised copy of seed, no race condition but
        //                 non-deterministic due to reading an uninitialised variable
        // shared(seed) : seed is shared between each thread, non-deterministic due to race condition

        // i is private to each thread, with a different value for each

        // A variable declared outside of parallel section and used inside is shared by default
        // A variable declared inside a parallel section is thread-local
    }

    // Task 4
    {
        std::vector<float> data{ 0., 2., 1., 3., 5., 4. };

        float sum = 0;

        #pragma omp parallel for reduction(+:sum)
        for(size_t i = 0; i < data.size(); ++i) {
            sum += data[i];
        }

        std::cout << "Sum: " << sum << std::endl;

        float max_value = data[0];

        #pragma omp parallel for reduction(max:max_value)
        for(size_t i = 0; i < data.size(); ++i) {
            if (data[i] > max_value) {
                max_value = data[i];
            }
        }

        std::cout << "Max value: " << max_value << std::endl;
    }

    // Task 5
    {
        const size_t n = 100'000'000;
        const double dz = 1. / n;

        // Parallel
        {
            double quarter_pi = 0;

            const auto start = std::chrono::high_resolution_clock::now();

            #pragma omp parallel for reduction(+:quarter_pi)
            for(size_t i = 0; i < n; ++i) {
                const double z = i * dz;
                quarter_pi += (1. / (1. + z * z)) * dz;
            }

            const auto end = std::chrono::high_resolution_clock::now();
            std::cout << "Elapsed time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                      << " ms" << std::endl;

            std::cout << "Pi: " << quarter_pi * 4. << std::endl;
        }

        // Sequential
        {
            double quarter_pi = 0;

            const auto start = std::chrono::high_resolution_clock::now();

            for(size_t i = 0; i < n; ++i) {
                const double z = i * dz;
                quarter_pi += (1. / (1. + z * z)) * dz;
            }

            const auto end = std::chrono::high_resolution_clock::now();
            std::cout << "Elapsed time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                      << " ms" << std::endl;

            std::cout << "Pi: " << quarter_pi * 4. << std::endl;
        }
    }

    /*
    * ===========
    * BONUS PART
    * ===========
    */

    // Task 2 Bonus
    {
        const uint8_t nb = 19;

        // std::vector<unsigned char>
        {
            std::vector<unsigned char> bits(8, 0);

            #pragma omp parallel for
            for(size_t i = 0; i < bits.size(); ++i) {
                bits[i] = (nb >> i) & 1;
            }

            for(size_t i = 0; i < bits.size(); ++i) {
                // Reverse order for human readability
                // Cast to see 1 or 0 instead of char
                std::cout << static_cast<int>(bits[bits.size() - 1 - i]);
            }
            std::cout << std::endl;
        }

        // std::vector<bool>
        {
            std::vector<bool> bits(8, 0);

            #pragma omp parallel for
            for(size_t i = 0; i < bits.size(); ++i) {
                bits[i] = (nb >> i) & 1;
            }

            for(size_t i = 0; i < bits.size(); ++i) {
                // Reverse order for human readability
                // Cast to see 1 or 0 instead of char
                std::cout << static_cast<int>(bits[bits.size() - 1 - i]);
            }
            std::cout << std::endl;

            // Using std::vector<bool> does not work due to its space-optimized specialisation:
            // multiple elements are stored in a single byte, creating a race condition
        }

        // std::array<bool, N>
        {
            std::array<bool, 8> bits;

            #pragma omp parallel for
            for(size_t i = 0; i < bits.size(); ++i) {
                bits[i] = (nb >> i) & 1;
            }

            for(size_t i = 0; i < bits.size(); ++i) {
                // Reverse order for human readability
                // Cast to see 1 or 0 instead of char
                std::cout << static_cast<int>(bits[bits.size() - 1 - i]);
            }
            std::cout << std::endl;
            // Works: specialisation used for std::vector<bool> does not apply to std::array
        }
    }

    // Task 4 Bonus Sum
    {
        std::vector<float> data(100'000'000, 0.f);
        // Generate random data for real test case
        std::random_device rd;
        const unsigned seed = rd();
        std::mt19937 gen(seed);
        std::uniform_real_distribution<float> distrib(-100.f, 100.f);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = distrib(gen);
        }

        // Sequential
        {
            float sum = 0;

            const auto start = std::chrono::high_resolution_clock::now();
            for(size_t i = 0; i < data.size(); ++i) {
                sum += data[i];
            }
            const auto end = std::chrono::high_resolution_clock::now();
            std::cout << "(Sequential) Elapsed time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                      << " ms, result: " << sum << std::endl;
        }

        // Reduction
        {
            float sum = 0;

            const auto start = std::chrono::high_resolution_clock::now();

            #pragma omp parallel for reduction(+:sum)
            for(size_t i = 0; i < data.size(); ++i) {
                sum += data[i];
            }

            const auto end = std::chrono::high_resolution_clock::now();
            std::cout << "(Reduction) Elapsed time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                      << " ms, result: " << sum << std::endl;
        }

        // Atomic
        {
            float sum = 0;

            const auto start = std::chrono::high_resolution_clock::now();

            #pragma omp parallel for
            for(size_t i = 0; i < data.size(); ++i) {
                #pragma omp atomic
                sum += data[i];
            }

            const auto end = std::chrono::high_resolution_clock::now();
            std::cout << "(Atomic) Elapsed time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                      << " ms, result: " << sum << std::endl;
        }

        // Critical
        {
            float sum = 0;

            const auto start = std::chrono::high_resolution_clock::now();

            #pragma omp parallel for
            for(size_t i = 0; i < data.size(); ++i) {
                #pragma omp critical
                sum += data[i];
            }

            const auto end = std::chrono::high_resolution_clock::now();
            std::cout << "(Critical) Elapsed time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                      << " ms, result: " << sum << std::endl;
        }

        // The performance of reduction is better than critical and atomic as threads do not wait on each other, only final sync is needed
        // Results must be printed (or used), otherwise with optimization the compiler removes the sequential loop entirely (0 ms)
        // Float sums differ slightly between versions: the addition order changes, and float addition is not associative
    }

    // Task 4 Bonus max
    {
        std::vector<float> data(100'000'000, 0.f);
        // Generate random data for real test case
        std::random_device rd;
        const unsigned seed = rd();
        std::mt19937 gen(seed);
        std::uniform_real_distribution<float> distrib(-100.f, 100.f);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = distrib(gen);
        }

        // Sequential
        {
            float max_value = data[0];

            const auto start = std::chrono::high_resolution_clock::now();

            for(size_t i = 0; i < data.size(); ++i) {
                if (data[i] > max_value) {
                    max_value = data[i];
                }
            }

            const auto end = std::chrono::high_resolution_clock::now();
            std::cout << "(Sequential) Elapsed time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                      << " ms, result: " << max_value << std::endl;
        }

        // Reduction
        {
            float max_value = data[0];

            const auto start = std::chrono::high_resolution_clock::now();

            #pragma omp parallel for reduction(max:max_value)
            for(size_t i = 0; i < data.size(); ++i) {
                if (data[i] > max_value) {
                    max_value = data[i];
                }
            }

            const auto end = std::chrono::high_resolution_clock::now();
            std::cout << "(Reduction) Elapsed time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                      << " ms, result: " << max_value << std::endl;
        }

        // Atomic
        {
            float max_value = data[0];

            const auto start = std::chrono::high_resolution_clock::now();

            #pragma omp parallel for
            for(size_t i = 0; i < data.size(); ++i) {
                #pragma omp atomic update, compare
                if (data[i] > max_value) {
                    max_value = data[i];
                }
            }

            const auto end = std::chrono::high_resolution_clock::now();
            std::cout << "(Atomic) Elapsed time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                      << " ms, result: " << max_value << std::endl;
        }

        // Critical
        {
            float max_value = data[0];

            const auto start = std::chrono::high_resolution_clock::now();

            #pragma omp parallel for
            for(size_t i = 0; i < data.size(); ++i) {
                #pragma omp critical
                {
                    if (data[i] > max_value) {
                        max_value = data[i];
                    }
                }
            }

            const auto end = std::chrono::high_resolution_clock::now();
            std::cout << "(Critical) Elapsed time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                      << " ms, result: " << max_value << std::endl;
        }

        // The performance of reduction is better than critical and atomic as threads do not wait on each other, only final sync is needed
        // Results must be printed (or used), otherwise with optimization the compiler removes the sequential loop entirely (0 ms)

    }

    return 0;
}
