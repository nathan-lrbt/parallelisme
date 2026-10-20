#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>
#include <execution>
#include <random>

// Task 4 a)
float transformer(float x) {
    const float ratio = 0.1f;
    return std::sqrt(x) * ratio;
}

// Task 4 a)
struct Transformer {
    float operator()(float x) const { // call operator
        const float ratio = 0.1f;
        return std::sqrt(x) * ratio;
    }
};

// Task 4 b)
float transformer_with_ratio(float x, float ratio) {
    return std::sqrt(x) * ratio;
}

// Task 4 b)
struct TransformerWithRatio {
    const float ratio;

    TransformerWithRatio(float ratio)
        : ratio(ratio) 
    {}

    float operator()(float x) const { // call operator
        return std::sqrt(x) * ratio;
    }
};

int main() {

    // Task 1
    {
        std::cout << "=== Task 1 ===" << std::endl;
        // Same seed, so both sorts get the same shuffled data
        constexpr unsigned seed = 42;
        auto shuffled_vector = [] {
            std::vector<int> v(100'000'000);
            std::iota(v.begin(), v.end(), 1);
            std::shuffle(v.begin(), v.end(), std::mt19937{seed});
            return v;
        };

        long long t_seq = 0;
        {
            auto v = shuffled_vector();

            const auto start = std::chrono::steady_clock::now();
            std::sort(v.begin(), v.end());
            const auto end = std::chrono::steady_clock::now();
            t_seq = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cout << "Elapsed time: " << t_seq << " ms" << std::endl;
        }

        long long t_par = 0;
        {
            auto v = shuffled_vector();

            const auto start = std::chrono::steady_clock::now();
            std::sort(std::execution::par, v.begin(), v.end());
            const auto end = std::chrono::steady_clock::now();
            t_par = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cout << "Elapsed time: " << t_par << " ms" << std::endl;
        }

        std::cout << "Speedup: " << static_cast<double>(t_seq) / static_cast<double>(t_par) << std::endl;
    }

    // Task 2
    {
        std::cout << "\n=== Task 2 ===" << std::endl;
        const std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!?,. ";
        const std::vector<uint8_t> msg_keys = {
            48, 4, 11, 2, 14, 12, 4, 56, 8, 13, 56, 15,
            0, 17, 0, 11, 11, 4, 11, 8, 18, 12, 56, 2,
            11, 0, 18, 18, 56, 52
        };

        // Transform each key to its character, directly into a string of the right size
        // The transform lambda capture the alphabet as a reference to not copy
        // NOTE: Concatenation of std::string/std::vector is not commutative, as require per std::reduce, so it can't be used here
        std::string msg(msg_keys.size(), ' ');
        std::transform(
            std::execution::par,
            msg_keys.cbegin(), msg_keys.cend(), msg.begin(),
            [&](uint8_t key) {
                return alphabet[key];
            }
        );

        std::cout << "Message was: " << msg << std::endl;
    }

    // Task 3 a)
    {
        std::cout << "\n=== Task 3 a) ===" << std::endl;
        std::vector<float> data(1'000'000, 1.f);
        data[0] = 1.0e8f;
        // back() is a reference to the last element (undefined on an empty vector)
        data.back() = -1.0e8f;

        // accumulate is a left fold with a guaranteed order, unlike std::reduce
        // which is allowed to reorder the additions, even without execution policy
        const auto seq_sum = std::accumulate(data.cbegin(), data.cend(), 0.f);
        // Since the first float is huge, the smaller values are ignored afterward
        // It reduces to 1.0e8f - 1.0e8f = 0
        std::cout << "Sequential sum (float): " << std::setprecision(10) << seq_sum << std::endl;

        // The same sequential sum with a double accumulator gives the exact value
        // Floats are converted to double without loss, and near 1.0e8 the gap between two doubles
        // is about 1.5e-8 (53-bit significand) instead of 8 for a float (24 bits),
        // so the ones are not absorbed by the huge value
        const auto exact_sum = std::accumulate(data.cbegin(), data.cend(), 0.0);
        std::cout << "Sequential sum (double): " << exact_sum << std::endl;

        // Non-deterministic, since the addition order is not guaranteed,
        // a certain amount of smaller values are discarded by the huge one before canceling each other
        // Only visible with at least 2 cores: with a single one (using taskset)
        // the sum is always the sequential one
        std::cout << "Parallel sum: ";
        for(size_t i = 0; i < 10; ++i) {
            const auto par_sum = std::reduce(std::execution::par, data.cbegin(), data.cend());
            std::cout << std::setprecision(10) << par_sum << " ";
        }
        std::cout << std::endl;
    }

    // Task 3 b)
    {
        std::cout << "\n=== Task 3 b) ===" << std::endl;
        std::vector<float> data(1'000'000, 1.f);
        data[0] = 1.0e8f;
        data[1] = -1.0e8f;

        // Correct, both huge values first cancel each other, then the sum is exact
        const auto seq_sum = std::accumulate(data.cbegin(), data.cend(), 0.f);
        std::cout << "Sequential sum: " << std::setprecision(10) << seq_sum << std::endl;

        // The order is still not guaranteed, but the result is stable in practice:
        // both huge values are adjacent, so they end up in the same chunk and cancel each other first
        std::cout << "Parallel sum: ";
        for(size_t i = 0; i < 10; ++i) {
            const auto par_sum = std::reduce(std::execution::par, data.cbegin(), data.cend());
            std::cout << std::setprecision(10) << par_sum << " ";
        }
        std::cout << std::endl;
    }

    // Print the sums of Task 4 in plain digits (instead of scientific notation)
    std::cout << std::fixed << std::setprecision(0);

    // Task 4 a)
    {
        std::cout << "\n=== Task 4 a) ===" << std::endl;
        auto lambda_transform = [](float x){
            const float ratio = 0.1f;
            return std::sqrt(x) * ratio;
        };

        constexpr size_t N = 1'000'000;
        std::vector<float> data(N);
        std::iota(data.begin(), data.end(), 1.f);

        // Free function
        std::cout << "Free function: "
                  << std::transform_reduce(
                        data.cbegin(), data.cend(), 0.f, std::plus{}, transformer
                     )
                  << std::endl;
        // Functor
        std::cout << "Functor: " << std::transform_reduce(data.cbegin(), data.cend(), 0.f, std::plus{}, Transformer{}) << std::endl;
        // Lambda
        std::cout << "Lambda 1: " << std::transform_reduce(data.cbegin(), data.cend(), 0.f, std::plus{}, lambda_transform) << std::endl;
        // Or define it in the call :
        const auto sum = std::transform_reduce(data.cbegin(), data.cend(), 0.f, std::plus{}, [](float x){
            const float ratio = 0.1f;
            return std::sqrt(x) * ratio;
        });
        std::cout << "Lambda 2: " << sum << std::endl;
    }

    // Task 4 b)
    // Advantages and disadvantages of each approach to bind the ratio:
    // - Free function: the ratio can only be an extra parameter. The algorithm calls
    //   the operation with a single argument, so a wrapper (lambda or std::bind) is needed.
    // - Lambda: defined where it is used, the ratio is captured by value or by reference.
    //   Compact, but the type has no name and the code is not reusable unless stored in a variable
    // - Functor: the ratio is an explicit member set by the constructor. Reusable, testable and able
    //   to hold more state, but it is the most verbose
    {
        std::cout << "\n=== Task 4 b) ===" << std::endl;

        constexpr size_t N = 1'000'000;
        std::vector<float> data(N);
        std::iota(data.begin(), data.end(), 1.f);

        const float ratio = 0.15f;

        // Capture ratio by copy with [=]
        auto lambda_transformer_with_ratio = [=](float x){
            return std::sqrt(x) * ratio;
        };

        // Functor
        std::cout << "Functor: " << std::transform_reduce(data.cbegin(), data.cend(), 0.f, std::plus{}, TransformerWithRatio{ratio}) << std::endl;
        // Lambda
        std::cout << "Lambda 1: " << std::transform_reduce(data.cbegin(), data.cend(), 0.f, std::plus{}, lambda_transformer_with_ratio) << std::endl;
        // Or define it in the call :
        const auto sum1 = std::transform_reduce(data.cbegin(), data.cend(), 0.f, std::plus{}, [=](float x){
            return std::sqrt(x) * ratio;
        });
        std::cout << "Lambda 2: " << sum1 << std::endl;

        // Free function require to wrap the call in a lambda
        const auto sum2 = std::transform_reduce(data.cbegin(), data.cend(), 0.f, std::plus{}, [=](float x) {
            return transformer_with_ratio(x, ratio);
        });
        std::cout << "Free function: " << sum2 << std::endl;
    }

    // Back to the default format
    std::cout << std::defaultfloat << std::setprecision(6);

    // Task 5
    {
        std::cout << "\n=== Task 5 ===" << std::endl;
        constexpr size_t dim_x = 3;
        constexpr size_t dim_y = 2;
        constexpr size_t dim_z = 4;
        // Indexed as tensor[z][y][x]
        const std::vector<std::vector<std::vector<unsigned>>> tensor {
            { {1, 2, 3}, {3, 2, 1} },
            { {2, 3, 4}, {4, 3, 2} },
            { {6, 7, 8}, {8, 7, 6} },
            { {7, 8, 9}, {9, 8, 7} },
        };

        std::vector<unsigned> data(dim_x * dim_y * dim_z);

        // Use a second vector to store the indices of the loop
        // NOTE: It is possible to replace it by a iota-view
        std::vector<size_t> indices(data.size());
        std::iota(indices.begin(), indices.end(), 0);

        std::for_each(std::execution::par, indices.cbegin(), indices.cend(), [&](size_t i) {
            const size_t idx = i;
            // Compute the 3D indices
            const size_t z = i / (dim_x * dim_y);
            i -= z * dim_x * dim_y;
            const size_t y = i / dim_x;
            const size_t x = i % dim_x;
            data[idx] = tensor[z][y][x];
        });

        for(const auto v: data) {
            std::cout << v << ", ";
        }
        std::cout << std::endl;
    }

    // Task 5 (alternative)
    {
        std::cout << "\n=== Task 5 (alternative) ===" << std::endl;
        constexpr size_t dim_x = 3;
        constexpr size_t dim_y = 2;
        constexpr size_t dim_z = 4;
        // Indexed as tensor[z][y][x]
        const std::vector<std::vector<std::vector<unsigned>>> tensor {
            { {1, 2, 3}, {3, 2, 1} },
            { {2, 3, 4}, {4, 3, 2} },
            { {6, 7, 8}, {8, 7, 6} },
            { {7, 8, 9}, {9, 8, 7} },
        };

        std::vector<unsigned> data(dim_x * dim_y * dim_z);
        // Parallelize over the rows (one row = one (z, y) pair), not over the elements
        // NOTE: It is possible to replace it by a iota-view
        std::vector<size_t> indices(dim_z * dim_y);
        std::iota(indices.begin(), indices.end(), 0);

        std::for_each(std::execution::par, indices.cbegin(), indices.cend(), [&](size_t row) {
            const size_t z = row / dim_y;
            const size_t y = row % dim_y;
            // Short rows, copy is sequential
            std::copy(tensor[z][y].cbegin(), tensor[z][y].cend(), data.begin() + row * dim_x);
        });

        for(const auto v: data) {
            std::cout << v << ", ";
        }
        std::cout << std::endl;
    }

    // Task 6
    {
        std::cout << "\n=== Task 6 ===" << std::endl;
        const std::vector<std::vector<unsigned>> jagged {
            { 0, 0 },
            { 1 },
            { 2, 2, 2, 2 },
            { 3, 3, 3 },
        };

        // Get the size of each segment
        std::vector<size_t> sizes(jagged.size(), 0);
        std::transform(
            std::execution::par,
            jagged.cbegin(), jagged.cend(), sizes.begin(),
            [](const std::vector<unsigned>& v) {
                return v.size();
            }
        );

        // Compute the exclusive sum of the sizes, giving at which index each segment start
        // NOTE: It is possible to reuse "sizes" there, but a bug in gcc prevent correct in-place parallelized exclusive scan
        // Fixed in gcc 13.4 and 14.4
        std::vector<size_t> offsets(jagged.size(), 0);
        std::exclusive_scan(
            std::execution::par,
            sizes.begin(), sizes.end(),
            offsets.begin(), size_t{0} 
        );

        const size_t nb_element = offsets.back() + sizes.back();
        std::vector<unsigned> result(nb_element, 0u);

        // Copy each segment to the right place
        std::for_each(
            std::execution::par,
            jagged.cbegin(), jagged.cend(),
            [&](const std::vector<unsigned>& v) {
                // Other method to get the current index
                const size_t idx = &v - jagged.data(); 
                // May not run in parallel
                std::copy(std::execution::par, v.cbegin(), v.cend(), result.begin() + offsets[idx]);
            }
        );

        for(const auto v: result) {
            std::cout << v << ", ";
        }
        std::cout << std::endl;
    }

    // Task 6 (alternative)
    {
        std::cout << "\n=== Task 6 (alternative) ===" << std::endl;
        const std::vector<std::vector<unsigned>> jagged {
            { 0, 0 },
            { 1 },
            { 2, 2, 2, 2 },
            { 3, 3, 3 },
        };

        // Get the size of each segment
        std::vector<size_t> sizes(jagged.size(), 0);
        std::transform(
            std::execution::par,
            jagged.cbegin(), jagged.cend(), sizes.begin(),
            [](const std::vector<unsigned>& v) {
                return v.size();
            }
        );

        // Compute the exclusive sum of the sizes, giving at which index each segment start
        std::vector<size_t> offsets(jagged.size(), 0);
        std::exclusive_scan(
            std::execution::par,
            sizes.cbegin(), sizes.cend(),
            offsets.begin(), size_t{}
        );

        std::vector<unsigned> result(offsets.back() + sizes.back());

        // Parallelize over the output elements instead of the segments:
        // the load is balanced whatever the size of the segments
        std::vector<size_t> indices(result.size());
        std::iota(indices.begin(), indices.end(), 0);

        std::transform(
            std::execution::par,
            indices.cbegin(), indices.cend(), result.begin(),
            [&](size_t i) {
                // Compute the index of the segment containing the future value of this position i
                const size_t seg = std::upper_bound(
                    offsets.cbegin(), offsets.cend(), i
                ) - offsets.cbegin() - 1;
                return jagged[seg][i - offsets[seg]];
            }
        );

        for(const auto v: result) {
            std::cout << v << ", ";
        }
        std::cout << std::endl;
    }

    // Task 7
    {
        // See mandelbrot_par.cpp (algorithms), mandelbrot_omp.cpp (OpenMP, from the previous TP)
        // and mandelbrot_omp_dynamic.cpp (same with schedule(dynamic))
        // Using the parameters of run.sh: 4000 2000 300 -2.2 1.0 -1.5 1.5
        // Average over 5 runs, default number of threads (28)
        // Algo par       : 122.2ms
        // OpenMP         : 232.6ms
        // OpenMP dynamic : 121.6ms

        // The difference is explained by the scheduling: load-balancing is a futur chapter
        // With a dynamic schedule OpenMP reaches the same time as the algorithms
        // For better benchmark, a warmup section would be needed
    }

    // Bonus Task
    {
        std::cout << "\n=== Bonus task ===" << std::endl;
        const size_t N = 100'000;
        const size_t key_max = N / 4;
        std::vector<std::tuple<size_t, float>> data(N, { 0, 0.f });

        // Random generator
        std::random_device rd;
        std::mt19937 engine(rd());
        std::uniform_real_distribution<float> dist_f(0.f, 1.f);
        std::uniform_int_distribution<size_t> dist_u(0, key_max - 1);

        for(size_t i = 0; i < N; ++i) {
            data[i] = { dist_u(engine), dist_f(engine) };
        }

        // The result is a vector of size key_max, indexed by key (a key without any tuple gives 0).
        // The obvious solution, a parallel for_each doing result[key] += value, is a data race:
        // two tuples with the same key can be processed by two threads at the same time, and the
        // read-modify-write of result[key] loses updates (the result changes from run to run).

        // Reference: sequential sum with a double accumulator
        // Floating-point addition is not associative, so the parallel results are compared with a
        // tolerance and not with ==
        std::vector<double> reference(key_max, 0.0);
        std::for_each(data.cbegin(), data.cend(), [&](const std::tuple<size_t, float>& p) {
            reference[std::get<0>(p)] += std::get<1>(p);
        });

        // Maximum absolute difference with the reference
        const auto max_error = [&](const std::vector<float>& result) {
            return std::transform_reduce(
                std::execution::par,
                result.cbegin(), result.cend(), reference.cbegin(),
                0.0,
                [](double a, double b) { return std::max(a, b); },
                [](float r, double ref) { return std::abs(r - ref); }
            );
        };

        // The race, for demonstration (undefined behavior, do not use): lost updates, the error is large and varies.
        // It must run on the unsorted data: once sorted, the tuples of a key are in the same chunk, so
        // the race rarely happens
        {
            std::vector<float> result(key_max, 0.f);
            std::for_each(
                std::execution::par,
                data.cbegin(), data.cend(),
                [&](const std::tuple<size_t, float>& p) {
                    result[std::get<0>(p)] += std::get<1>(p);
                }
            );
            std::cout << "Bonus, data race: max error " << max_error(result) << std::endl;
        }

        // Solution 1: sort by key, then one thread per key
        // After the sort the tuples of a key are contiguous, so each key owns a range of data and
        // writes only to its own result[key].
        {
            std::sort(
                std::execution::par,
                data.begin(), data.end(),
                [](const std::tuple<size_t, float>& a, const std::tuple<size_t, float>& b) {
                    return std::get<0>(a) < std::get<0>(b);
                }
            );

            // Use impossible values to set unused range (keys without any tuple)
            const size_t none = N;
            std::vector<std::array<size_t, 2>> ranges(key_max, {{none, none}});

            // Find the first and the last index of each key. Each index writes to a different
            // element of ranges, since only one tuple is the beginning (or the end) of a key.
            std::for_each(
                std::execution::par,
                data.cbegin(), data.cend(),
                [&](const std::tuple<size_t, float>& p) {
                    // Use special trick to get indices
                    const size_t idx = &p - data.data();
                    if (idx == 0 || std::get<0>(p) != std::get<0>(data[idx - 1])) { // Beginning of a segment
                        ranges[std::get<0>(p)][0] = idx;
                    }
                    if (idx + 1 == data.size() || std::get<0>(p) != std::get<0>(data[idx + 1])) { // End of a segment
                        ranges[std::get<0>(p)][1] = idx;
                    }
                }
            );

            // Sum each segment and store the result
            std::vector<float> result(key_max, 0.f);
            std::for_each(
                std::execution::par,
                ranges.cbegin(), ranges.cend(),
                [&](const std::array<size_t, 2>& range) {
                    if (range[0] == none) {
                        return;
                    }
                    const size_t key = &range - ranges.data();
                    for (size_t i = range[0]; i <= range[1]; ++i) { // End included according to range creation
                        result[key] += std::get<1>(data[i]);
                    }
                }
            );
            std::cout << "Bonus, sort + ranges: max error " << max_error(result) << std::endl;
        }

        // Solution 2: atomic accumulation
        // std::atomic_ref (C++20) makes the addition on result[key] atomic, there is no race anymore.
        // Simple, no sort and no extra memory. The order of the additions is not fixed, so the last
        // bits of the sums change from run to run.
        {
            std::vector<float> result(key_max, 0.f);
            std::for_each(
                std::execution::par,
                data.cbegin(), data.cend(),
                [&](const std::tuple<size_t, float>& p) {
                    std::atomic_ref<float>(result[std::get<0>(p)]).fetch_add(
                        std::get<1>(p), std::memory_order_relaxed
                    );
                }
            );
            std::cout << "Bonus, atomic: max error " << max_error(result) << std::endl;
        }

    }

    return 0;
}
