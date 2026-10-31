#pragma once
#include <windows.h>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <vector>

struct ProcInfo {
    DWORD pid = 0;
    std::wstring name;
    double cpu = 0;        // % of total CPU
    unsigned long long memBytes = 0;
};

struct Finding {
    int severity = 0;      // 0 = ok/info, 1 = warning, 2 = problem
    std::wstring text;
};

class Monitor {
public:
    static Monitor& I();
    void Update();                                   // call about once per second (UI thread)
    double Cpu() const { return cpu_; }              // 0..100
    double RamPercent() const { return ramPct_; }
    unsigned long long RamUsed() const { return ramUsed_; }
    unsigned long long RamTotal() const { return ramTotal_; }
    const std::deque<double>& CpuHistory() const { return hist_; }
    std::vector<ProcInfo> Processes(bool sortByCpu) const;
    std::vector<Finding> FindLosses() const;         // "Find FPS Losses"
private:
    Monitor() = default;
    double cpu_ = 0, ramPct_ = 0;
    unsigned long long ramUsed_ = 0, ramTotal_ = 0;
    unsigned long long prevIdle_ = 0, prevKernel_ = 0, prevUser_ = 0;
    bool havePrev_ = false;
    std::deque<double> hist_;
    std::vector<ProcInfo> procs_;
    std::map<DWORD, unsigned long long> prevProc_;
    mutable std::mutex mu_;
};
