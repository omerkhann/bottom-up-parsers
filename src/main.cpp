#include "grammar.h"
#include "items.h"
#include "lr1_parser.h"
#include "slr_parser.h"
#include "stack.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>

using cc::Grammar;

static void WriteFile(const std::string& path, const std::string& content) {
  std::ofstream out(path, std::ios::binary);
  if (!out) throw std::runtime_error("Failed to open output file for writing: " + path);
  out << content;
  if (!out) throw std::runtime_error("Failed to write output file: " + path);
}

static std::string ItemSetsToString(const cc::Grammar& g, const cc::ItemsBuilder::CanonicalCollection& col) {
  std::ostringstream out;
  for (int i = 0; i < static_cast<int>(col.states.size()); ++i) {
    out << "I" << i << ":\n";
    for (const auto& it : col.states[i].Items()) out << "  " << cc::ItemToString(g, it) << "\n";
    out << "\n";
  }
  return out.str();
}

static std::string TraceToString(const std::vector<cc::TraceRow>& trace) {
  std::ostringstream out;

  auto trunc = [](const std::string& s, size_t maxLen) -> std::string {
    if (s.size() <= maxLen) return s;
    if (maxLen <= 3) return s.substr(0, maxLen);
    return s.substr(0, maxLen - 3) + "...";
  };

  int stepW = 4;
  int stackW = 40;
  int inputW = 28;
  int actionW = 28;
  for (const auto& r : trace) {
    stepW = std::max<int>(stepW, static_cast<int>(std::to_string(r.step).size()));
    stackW = std::max<int>(stackW, static_cast<int>(std::min<size_t>(80, r.stack.size())));
    inputW = std::max<int>(inputW, static_cast<int>(std::min<size_t>(60, r.input.size())));
    actionW = std::max<int>(actionW, static_cast<int>(std::min<size_t>(60, r.action.size())));
  }
  stackW = std::min(stackW, 80);
  inputW = std::min(inputW, 60);
  actionW = std::min(actionW, 60);

  out << std::left << std::setw(stepW) << "Step"
      << " | " << std::left << std::setw(stackW) << "Stack"
      << " | " << std::left << std::setw(inputW) << "Input"
      << " | " << std::left << std::setw(actionW) << "Action"
      << "\n";
  out << std::string(stepW, '-') << "-+-" << std::string(stackW, '-') << "-+-" << std::string(inputW, '-')
      << "-+-" << std::string(actionW, '-') << "\n";

  for (const auto& r : trace) {
    out << std::left << std::setw(stepW) << r.step << " | " << std::left << std::setw(stackW) << trunc(r.stack, stackW)
        << " | " << std::left << std::setw(inputW) << trunc(r.input, inputW) << " | " << std::left << std::setw(actionW)
        << trunc(r.action, actionW) << "\n";
  }
  return out.str();
}

static std::vector<std::string> SplitWS(const std::string& s) {
  std::istringstream in(s);
  std::vector<std::string> out;
  std::string tok;
  while (in >> tok) out.push_back(tok);
  return out;
}

static void PrintComparison(const cc::ParserBuildResult& slr, const cc::ParserBuildResult& lr1) {
  const int metricW = 18;
  const int colW = 10;
  std::cout << std::left << std::setw(metricW) << "Metric"
            << " | " << std::left << std::setw(colW) << "SLR(1)"
            << " | " << std::left << std::setw(colW) << "LR(1)"
            << "\n";
  std::cout << std::string(metricW, '-') << "-+-" << std::string(colW, '-') << "-+-" << std::string(colW, '-') << "\n";
  std::cout << std::left << std::setw(metricW) << "States"
            << " | " << std::left << std::setw(colW) << slr.collection.states.size()
            << " | " << std::left << std::setw(colW) << lr1.collection.states.size() << "\n";
  std::cout << std::left << std::setw(metricW) << "Conflicts"
            << " | " << std::left << std::setw(colW) << slr.table.Conflicts().size()
            << " | " << std::left << std::setw(colW) << lr1.table.Conflicts().size() << "\n";
}

struct PerfRow {
  size_t states = 0;
  size_t conflicts = 0;
  size_t actionEntries = 0;
  size_t gotoEntries = 0;
  size_t approxBytes = 0;
  double buildMs = 0.0;
  double parseMs = 0.0;
  double parseIters = 0.0;
};

static std::string ComparisonTxt(const std::string& grammarPath, const std::string& tokensStr, const PerfRow& slr,
                                 const PerfRow& lr1) {
  auto row = [&](std::ostringstream& out, const std::string& label, const std::string& a, const std::string& b) {
    out << std::left << std::setw(28) << label << " | " << std::right << std::setw(14) << a << " | " << std::right
        << std::setw(14) << b << "\n";
  };

  std::ostringstream out;
  out << "Bottom-Up Parser Suite Performance Comparison\n";
  out << "Grammar: " << grammarPath << "\n";
  out << "Input tokens: " << tokensStr << "\n\n";

  out << std::left << std::setw(28) << "Metric"
      << " | " << std::right << std::setw(14) << "SLR(1)"
      << " | " << std::right << std::setw(14) << "LR(1)" << "\n";
  out << std::string(28, '-') << "-+-" << std::string(14, '-') << "-+-" << std::string(14, '-') << "\n";

  row(out, "Number of states", std::to_string(slr.states), std::to_string(lr1.states));
  row(out, "Conflicts", std::to_string(slr.conflicts), std::to_string(lr1.conflicts));
  row(out, "ACTION entries", std::to_string(slr.actionEntries), std::to_string(lr1.actionEntries));
  row(out, "GOTO entries", std::to_string(slr.gotoEntries), std::to_string(lr1.gotoEntries));
  row(out, "Total table entries", std::to_string(slr.actionEntries + slr.gotoEntries),
      std::to_string(lr1.actionEntries + lr1.gotoEntries));
  row(out, "Approx table bytes", std::to_string(slr.approxBytes), std::to_string(lr1.approxBytes));

  {
    std::ostringstream a, b;
    a << std::fixed << std::setprecision(3) << slr.buildMs;
    b << std::fixed << std::setprecision(3) << lr1.buildMs;
    row(out, "Table build time (ms)", a.str(), b.str());
  }

  {
    std::ostringstream a, b;
    a << std::fixed << std::setprecision(3) << slr.parseMs;
    b << std::fixed << std::setprecision(3) << lr1.parseMs;
    row(out, "Parse time (ms)", a.str(), b.str());
  }

  if (slr.parseIters > 0 && lr1.parseIters > 0) {
    std::ostringstream a, b;
    const double slrPerSec = (slr.parseIters * 1000.0) / std::max(0.001, slr.parseMs);
    const double lr1PerSec = (lr1.parseIters * 1000.0) / std::max(0.001, lr1.parseMs);
    a << std::fixed << std::setprecision(1) << slrPerSec;
    b << std::fixed << std::setprecision(1) << lr1PerSec;
    row(out, "Parse throughput (iters/s)", a.str(), b.str());
  }

  out << "\nNotes:\n";
  out << "- \"Approx table bytes\" is a rough estimate (excludes map/node overhead).\n";
  out << "- Parse timing runs multiple iterations for stability.\n";
  return out.str();
}

static void Usage() {
  std::cout << "Usage:\n"
            << "  ./parser --grammar input/<file>.txt --tokens \"tok1 tok2 ...\" [--outdir output]\n"
            << "\n"
            << "Notes:\n"
            << "- Tokens must be space-separated (e.g. \"id = id\"). '$' is appended automatically.\n"
            << "- Outputs written: augmented grammar, item sets, parsing tables, traces, parse tree.\n";
}

int main(int argc, char** argv) {
  try {
    std::string grammarPath = "input/grammar_with_conflict.txt";
    std::string tokensStr = "id = id";
    std::string outDir = "output";

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      auto needValue = [&](const std::string& flag) -> std::string {
        if (i + 1 >= argc) throw std::runtime_error("Missing value for " + flag);
        return argv[++i];
      };

      if (arg == "--help" || arg == "-h") {
        Usage();
        return 0;
      } else if (arg == "--grammar") {
        grammarPath = needValue("--grammar");
      } else if (arg == "--tokens") {
        tokensStr = needValue("--tokens");
      } else if (arg == "--outdir") {
        outDir = needValue("--outdir");
      } else {
        throw std::runtime_error("Unknown argument: " + arg);
      }
    }

    cc::Grammar g = cc::Grammar::FromFile(grammarPath);

    g.Augment();
    g.ComputeFirst();
    g.ComputeFollow();

    WriteFile(outDir + "/augmented_grammar.txt", g.ToString());

    // Build SLR(1)
    cc::SLRParserBuilder slr(g);
    const auto t0 = std::chrono::steady_clock::now();
    auto slrRes = slr.Build();
    const auto t1 = std::chrono::steady_clock::now();
    WriteFile(outDir + "/slr_items.txt", ItemSetsToString(g, slrRes.collection));
    WriteFile(outDir + "/slr_parsing_table.txt", slrRes.table.ToString());
    // WriteFile(outDir + "/slr_parsing_table_pretty.txt", slrRes.table.ToString());

    // Build LR(1)
    cc::LR1ParserBuilder lr1(g);
    const auto t2 = std::chrono::steady_clock::now();
    auto lr1Res = lr1.Build();
    const auto t3 = std::chrono::steady_clock::now();
    WriteFile(outDir + "/lr1_items.txt", ItemSetsToString(g, lr1Res.collection));
    WriteFile(outDir + "/lr1_parsing_table.txt", lr1Res.table.ToString());
    // WriteFile(outDir + "/lr1_parsing_table_pretty.txt", lr1Res.table.ToString());

    PrintComparison(slrRes, lr1Res);

    const std::vector<std::string> tokens = SplitWS(tokensStr);

    cc::ShiftReduceParser parserLR1(g, lr1Res.table);
    auto parseLR1 = parserLR1.Parse(tokens);
    WriteFile(outDir + "/lr1_trace.txt", TraceToString(parseLR1.trace));

    if (parseLR1.accepted) {
      WriteFile(outDir + "/parse_trees.txt", parseLR1.tree.ToString());
    } else {
      WriteFile(outDir + "/parse_trees.txt", "LR(1) parse error: " + parseLR1.error + "\n");
    }

    // For completeness, attempt SLR parse too (may fail due to conflicts / incorrect table).
    cc::ShiftReduceParser parserSLR(g, slrRes.table);
    auto parseSLR = parserSLR.Parse(tokens);
    WriteFile(outDir + "/slr_trace.txt", TraceToString(parseSLR.trace));

    // Performance comparison (timing + sizes) to output/comparison.txt
    // Parse timing uses multiple iterations for a stable wall-clock.
    auto timeParse = [&](const cc::ShiftReduceParser& p, int iters) -> double {
      const auto a = std::chrono::steady_clock::now();
      for (int i = 0; i < iters; ++i) (void)p.Parse(tokens);
      const auto b = std::chrono::steady_clock::now();
      return std::chrono::duration<double, std::milli>(b - a).count();
    };

    const int parseIters = 300;
    PerfRow slrPerf;
    slrPerf.states = slrRes.collection.states.size();
    slrPerf.conflicts = slrRes.table.Conflicts().size();
    slrPerf.actionEntries = slrRes.table.ActionEntryCount();
    slrPerf.gotoEntries = slrRes.table.GotoEntryCount();
    slrPerf.approxBytes = slrRes.table.ApproxBytes();
    slrPerf.buildMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    slrPerf.parseMs = timeParse(parserSLR, parseIters);
    slrPerf.parseIters = parseIters;

    PerfRow lr1Perf;
    lr1Perf.states = lr1Res.collection.states.size();
    lr1Perf.conflicts = lr1Res.table.Conflicts().size();
    lr1Perf.actionEntries = lr1Res.table.ActionEntryCount();
    lr1Perf.gotoEntries = lr1Res.table.GotoEntryCount();
    lr1Perf.approxBytes = lr1Res.table.ApproxBytes();
    lr1Perf.buildMs = std::chrono::duration<double, std::milli>(t3 - t2).count();
    lr1Perf.parseMs = timeParse(parserLR1, parseIters);
    lr1Perf.parseIters = parseIters;

    WriteFile(outDir + "/comparison.txt", ComparisonTxt(grammarPath, tokensStr, slrPerf, lr1Perf));

    std::cout << "\nOutputs written to " << outDir << "/\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "Fatal: " << ex.what() << "\n";
    Usage();
    return 1;
  }
}

