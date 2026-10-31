#pragma once
#include <string>
struct HwInfo {
    std::wstring cpu, gpu, gpuVendor, gpuDriverVersion, gpuDriverDate, os;
    unsigned cores = 0;
    unsigned long long ramTotal = 0;
};
namespace Hardware {
const HwInfo& Get();   // collected once, cached
}
