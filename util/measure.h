#pragma once

#include <algorithm>
#include <numeric>
#include <array>
#include <chrono>

// Measures how fast running a function is
template<typename F>
double measure(F f)
{
    using namespace std::chrono;
    static const int num_trials = 10;
    static const milliseconds min_time_per_trial(200);
    std::array<double, num_trials> trials;
    volatile decltype(f()) res; // to avoid optimizing f() away

    for(int i = 0; i < num_trials; ++i)
    {
        int runs = 0;
        high_resolution_clock::time_point end;
        auto start = high_resolution_clock::now();
        do
        {
            res = f();
            ++ runs;
            end = high_resolution_clock::now();
        } while(end - start < min_time_per_trial);

        trials[i] = duration_cast<duration<double>>(end - start).count() / runs;
    }
    (void)(res); // var not used warn

    std::sort(trials.begin(), trials.end());
    return std::accumulate(trials.begin() + 2, trials.end() - 2, 0.0) / (trials.size() - 4) * 1E6;
}
