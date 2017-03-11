#include "report.hpp"
#include "calendar.hpp"
#include <fmo/assert.hpp>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

Report::Report(const Results& results, const Results& baseline, float seconds) {
    mResults = &results;

    std::ostringstream out;
    info(out, results, baseline, seconds);
    mInfo = out.str();
}

void Report::write(std::ostream& out) { out << mInfo; }

void Report::save(const std::string& directory) {
    if (mResults->empty()) return;
    std::string fn = directory + '/' + safeTimestamp() + ".txt";
    std::ofstream out{fn, std::ios_base::out | std::ios_base::binary};

    if (!out) {
        std::cerr << "failed to open '" << fn << "'\n";
        throw std::runtime_error("failed to open file for writing results");
    }

    write(out);
    out << '\n';
    out << "###DATA_START###\n";
    out << mResults->size() << '\n';

    for (auto& entry : *mResults) {
        auto& name = entry.first;
        auto& file = *entry.second;
        out << std::quoted(name) << ' ';
        out << file.size() << '\n';
        for (auto evaluation : file) { out << int(evaluation); }
        out << '\n';
    }
}

void Report::info(std::ostream& out, const Results& results, const Results& baseline,
                  float seconds) {
    std::vector<std::string> fields;
    bool haveBase = false;
    int count[4] = {0, 0, 0, 0};
    int countBase[4] = {0, 0, 0, 0};

    auto precision = [](int* count) {
        return count[int(Evaluation::TP)] /
               double(count[int(Evaluation::TP)] + count[int(Evaluation::FP)]);
    };
    auto recall = [](int* count) {
        return count[int(Evaluation::TP)] /
               double(count[int(Evaluation::TP)] + count[int(Evaluation::FN)]);
    };
    auto percent = [](std::ostream& out, double val) {
        out << std::fixed << std::setprecision(2) << (val * 100) << '%';
    };
    auto countStr = [&](Evaluation eval) {
        std::ostringstream out;
        int val = count[int(eval)];
        out << val;
        if (haveBase) {
            int delta = val - countBase[int(eval)];
            if (delta != 0) { out << " (" << std::showpos << delta << std::noshowpos << ')'; }
        }
        return out.str();
    };
    auto percentStr = [&](double (*calc)(int*)) {
        std::ostringstream out;
        double val = calc(count);
        percent(out, val);
        if (haveBase) {
            double delta = val - calc(countBase);
            if (std::abs(delta) > 0.005) {
                out << " (" << std::showpos;
                percent(out, delta);
                out << std::noshowpos << ')';
            }
        }
        return out.str();
    };

    fields.push_back("name");
    fields.push_back("tp");
    fields.push_back("tn");
    fields.push_back("fp");
    fields.push_back("fn");
    fields.push_back("precision");
    fields.push_back("recall");

    for (auto& entry : results) {
        auto& name = entry.first;
        auto& file = *entry.second;
        if (file.size() == 0) continue;
        auto& baseFile = baseline.getFile(name);
        haveBase = baseFile.size() == file.size();

        for (auto eval : file) { count[int(eval)]++; }
        if (haveBase) {
            for (auto eval : baseFile) { countBase[int(eval)]++; }
        }

        fields.push_back(name);
        fields.push_back(countStr(Evaluation::TP));
        fields.push_back(countStr(Evaluation::TN));
        fields.push_back(countStr(Evaluation::FP));
        fields.push_back(countStr(Evaluation::FN));
        fields.push_back(percentStr(precision));
        fields.push_back(percentStr(recall));
    }

    constexpr int COLS = 7;
    FMO_ASSERT(fields.size() % COLS == 0, "bad number of fields");

    if (fields.size() == COLS) {
        // no entries -- quit
        return;
    }

    int colSize[COLS] = {0, 0, 0, 0, 0, 0, 0};
    for (auto it = fields.begin(); it != fields.end();) {
        for (int col = 0; col < COLS; col++, it++) {
            colSize[col] = std::max(colSize[col], int(it->size()) + 1);
        }
    }

    out << "generated on: " << timestamp() << '\n';
    out << "evaluation time: " << std::fixed << std::setprecision(1) << seconds << " s\n";
    out << '\n';
    int row = 0;
    for (auto it = fields.begin(); it != fields.end();) {
        out << std::setw(colSize[0]) << std::left << *it++ << std::right;
        for (int col = 1; col < COLS; col++, it++) { out << '|' << std::setw(colSize[col]) << *it; }
        out << '\n';
        if (row++ == 0) {
            for (int i = 0; i < colSize[0]; i++) { out << '-'; }
            for (int col = 1; col < COLS; col++) {
                out << '|';
                for (int i = 0; i < colSize[col]; i++) { out << '-'; }
            }
            out << '\n';
        }
    }
}
