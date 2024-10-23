// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <cstdio>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string_view>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

struct EnvironmentInfo {
  std::string Name;
  std::string Description;

  friend void to_json(nlohmann::json& j, const EnvironmentInfo& e) {
    j = nlohmann::json{
        {"Name", e.Name},
        {"Description", e.Description}
    };
  }

  friend void from_json(const nlohmann::json& j, EnvironmentInfo& e) {
    j.at("Name").get_to(e.Name);
    j.at("Description").get_to(e.Description);
  }
};

struct MetricInfo {
  std::string Name;
  std::string TargetBenchmarkName;
  std::string BaselineBenchmarkName;

  friend void to_json(nlohmann::json& j, const MetricInfo& m) {
    j = nlohmann::json{
        {"Name", m.Name},
        {"TargetBenchmarkName", m.TargetBenchmarkName},
        {"BaselineBenchmarkName", m.BaselineBenchmarkName}
    };
  }

  friend void from_json(const nlohmann::json& j, MetricInfo& m) {
    j.at("Name").get_to(m.Name);
    j.at("TargetBenchmarkName").get_to(m.TargetBenchmarkName);
    j.at("BaselineBenchmarkName").get_to(m.BaselineBenchmarkName);
  }
};

struct ReportConfig {
  std::string TargetName;
  double YellowIndicatorThreshold;
  std::vector<EnvironmentInfo> Environments;
  std::vector<MetricInfo> Metrics;

  friend void to_json(nlohmann::json& j, const ReportConfig& rc) {
    j = nlohmann::json{
        {"TargetName", rc.TargetName},
        {"YellowIndicatorThreshold", rc.YellowIndicatorThreshold},
        {"Environments", rc.Environments},
        {"Metrics", rc.Metrics}
    };
  }

  friend void from_json(const nlohmann::json& j, ReportConfig& rc) {
    j.at("TargetName").get_to(rc.TargetName);
    j.at("YellowIndicatorThreshold").get_to(rc.YellowIndicatorThreshold);
    j.at("Environments").get_to(rc.Environments);
    j.at("Metrics").get_to(rc.Metrics);
  }
};

std::unordered_map<std::string, std::vector<double>> Parse(const std::filesystem::path& file) {
  nlohmann::json obj;
  {
    std::ifstream in;
    in.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    in.open(file, std::ios_base::in | std::ios_base::binary);
    in >> obj;
  }
  std::unordered_map<std::string, std::vector<double>> result;
  for (auto& node : obj["benchmarks"]) {
    result[node["name"]].push_back(node["real_time"]);
  }
  for (auto& pair : result) {
    std::ranges::sort(pair.second);
  }
  return result;
}

double CalculateRate(const std::vector<double>& target, const std::vector<double>& baseline) {
  // Pre-condition: target.size() == baseline.size() && !target.empty() && (both `target` and `baseline` are sorted)
  if (target.size() != baseline.size() || target.empty()) {
    throw std::runtime_error{"CalculateRate(): target.size() != baseline.size() || target.empty()"};
  }
  std::vector<double> rates;
  for (std::size_t i = 0; i < target.size(); ++i) {
    rates.push_back((baseline[i] - target[i]) * 100 / target[i]);
  }
  std::ranges::sort(rates);
  if (rates.size() % 2 == 0) {
    return (rates[rates.size() / 2 - 1] + rates[rates.size() / 2]) / 2;
  } else {
    return rates[rates.size() / 2];
  }
}

void GenerateReport(const std::filesystem::path& config_path, const std::string& commit_id, const std::filesystem::path& source, const std::filesystem::path& output) {
  ReportConfig config;
  {
    nlohmann::json obj;
    std::ifstream in;
    in.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    in.open(config_path, std::ios_base::in | std::ios_base::binary);
    in >> obj;
    obj.get_to(config);
  }
  std::vector<std::unordered_map<std::string, std::vector<double>>> benchmarks;
  benchmarks.reserve(config.Environments.size());
  for (auto& environment : config.Environments) {
    benchmarks.push_back(Parse(source / std::format("benchmarking-results-{}", environment.Name) / "benchmarking-results.json"));
  }
  std::ofstream out;
  out.exceptions(std::ios_base::failbit | std::ios_base::badbit);
  out.open(output, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
  out << "## Benchmarking Report\n";
  out << "\n";
  out << "- Generated for: [Microsoft \"Proxy\" library](https://github.com/microsoft/proxy)\n";
  out << "- Commit ID: [" << commit_id << "](https://github.com/microsoft/proxy/commit/" << commit_id << ")\n";
  out << "- Generated at: " << std::format("{:%FT%TZ}", std::chrono::utc_clock::now()) << "\n";
  out << "\n";
  out << "| |";
  for (auto& environment : config.Environments) {
    out << " " << environment.Description << " |";
  }
  out << "\n";
  out << "| - |";
  for (std::size_t i = 0; i < config.Environments.size(); ++i) {
    out << " - |";
  }
  out << "\n";
  for (auto& metric : config.Metrics) {
    out << "| " << metric.Name << " |";
    for (auto& benchmark : benchmarks) {
      double rate = CalculateRate(benchmark.at(metric.TargetBenchmarkName), benchmark.at(metric.BaselineBenchmarkName));
      bool is_negative = rate < 0;
      if (is_negative) {
        rate = -rate;
      }
      out << " ";
      if (rate < config.YellowIndicatorThreshold) {
        out << "\xf0\x9f\x9f\xa1";  // Yellow circle
      } else if (is_negative) {
        out << "\xf0\x9f\x94\xb4";  // Red circle
      } else {
        out << "\xf0\x9f\x9f\xa2";  // Green circle
      }
      auto rate_str = std::format("{:.1f}", rate);
      std::string message;
      if (rate_str == "0.0") {
        out << config.TargetName << " has similar performance";
      } else {
        out << config.TargetName << " is about **" << rate_str << "% " << (is_negative ? "slower" : "faster") << "**";
      }
      out << " |";
    }
    out << "\n";
  }
}

int main(int argc, char** argv) {
  if (argc != 5) {
    puts("Usage: report_generator <config file path> <commit ID> <benchmarking results directory> <output file path>");
    return 0;
  }
  try {
    GenerateReport(argv[1], argv[2], argv[3], argv[4]);
  } catch (const std::exception& e) {
    fprintf(stderr, "An error occurred: %s\n", e.what());
    return 1;
  }
  return 0;
}
