#ifndef FMO_DESKTOP_REPORT_HPP
#define FMO_DESKTOP_REPORT_HPP

#include "args.hpp"
#include "evaluator.hpp"
#include <iosfwd>

struct Report {
    Report(const Results& results, const Results& baseline, const Args& args, float seconds);

    void write(std::ostream& out) const;

    void save(const std::string& directory) const;

    void saveScore(const std::string& file) const;

private:
    struct Stats {
        static constexpr int NUM_STATS = 5;
        double avg[NUM_STATS];       // average precision, recall, f_0.5, f_1.0, f_2.0
        double total[NUM_STATS];     // overall precision, recall, f_0.5, f_1.0, f_2.0
        double avgBase[NUM_STATS];   // ditto for baseline
        double totalBase[NUM_STATS]; // ditto for baseline
    };

    static void info(std::ostream& out, Stats& stats, const Results& results,
                     const Results& baseline, const Args& args, float seconds);

    const Results* mResults;
    std::string mInfo;
    Stats mStats;
};

#endif // FMO_DESKTOP_REPORT_HPP
