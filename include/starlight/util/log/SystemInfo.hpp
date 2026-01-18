#pragma once

#include <string>
#include <vector>
#include <optional>
#include <sstream>
#include <iostream>
#include <thread>
#include <algorithm>

#if defined(_WIN32)
    #include <windows.h>
    #include <winreg.h>
    #include <versionhelpers.h>
    #include <intrin.h>
#elif defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <sys/utsname.h>
    #include <unistd.h>
#else // assume Linux/Unix
    #include <sys/utsname.h>
    #include <unistd.h>
    #include <fstream>
    #include <regex>
    #include <filesystem>
#endif

#include "core/logging/LoggingFactory.hpp" // include your provided logger header

namespace star::log {

// -------------------------
// Types
// -------------------------
struct CpuInfo {
    std::string vendor;
    std::string model;
    std::string architecture;            // e.g., x86_64, ARM64
    int logicalProcessors = 0;           // online logical CPUs
    std::optional<int> physicalCores;    // if detectable
    std::optional<int> sockets;          // Linux: physical sockets
    std::optional<double> baseClockMHz;  // reported/base
    std::optional<double> maxClockMHz;   // max turbo if available
    std::string featuresSummary;         // flags/features string (may be truncated)
};

struct HostInfo {
    std::string hostname;
    std::string osName;      // "Linux", "Windows", "Darwin/macOS"
    std::string osVersion;   // kernel version, build, etc.
};

// -------------------------
// Helpers
// -------------------------
inline std::string trim(std::string s) {
    auto notSpace = [](int ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

inline std::string join(const std::vector<std::string>& parts, const char* sep = " ") {
    std::ostringstream oss;
    for (size_t i = 0; i < parts.size(); ++i) {
        oss << parts[i];
        if (i + 1 < parts.size()) oss << sep;
    }
    return oss.str();
}

inline std::string detectArchMacro() {
#if defined(_M_X64) || defined(__x86_64__)
    return "x86_64";
#elif defined(_M_IX86) || defined(__i386__)
    return "x86";
#elif defined(_M_ARM64) || defined(__aarch64__)
    return "ARM64";
#elif defined(_M_ARM) || defined(__arm__)
    return "ARM";
#else
    return "unknown";
#endif
}

// -------------------------
// Host info (portable)
// -------------------------
inline HostInfo queryHostInfo() {
    HostInfo hi;
    // Hostname
#if defined(_WIN32)
    WCHAR wname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(wname, &size)) {
        // Convert to UTF-8
        int len = WideCharToMultiByte(CP_UTF8, 0, wname, -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wname, -1, utf8.data(), len, nullptr, nullptr);
        hi.hostname = trim(utf8);
    } else {
        hi.hostname = "unknown";
    }
    hi.osName = "Windows";

    OSVERSIONINFOEXW osvi{};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    // Version (build number via GetVersionEx is deprecated; keep minimal)
    if (::GetVersionExW(reinterpret_cast<LPOSVERSIONINFOW>(&osvi)))
    {
        std::ostringstream oss;
        oss << "Win " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << " (build " << osvi.dwBuildNumber << ")";
        hi.osVersion = oss.str();
    }
    else
    {
        hi.osVersion = "unknown";
    }

#elif defined(__APPLE__)
    char buf[256];
    if (gethostname(buf, sizeof(buf)) == 0) {
        hi.hostname = trim(std::string(buf));
    } else {
        hi.hostname = "unknown";
    }
    struct utsname u{};
    if (uname(&u) == 0) {
        hi.osName = "macOS";
        std::ostringstream oss;
        oss << u.sysname << " " << u.release << " (" << u.version << ")";
        hi.osVersion = oss.str();
    } else {
        hi.osName = "macOS";
        hi.osVersion = "unknown";
    }
#else
    char buf[256];
    if (gethostname(buf, sizeof(buf)) == 0) {
        hi.hostname = trim(std::string(buf));
    } else {
        hi.hostname = "unknown";
    }
    struct utsname u{};
    if (uname(&u) == 0) {
        hi.osName = u.sysname; // "Linux"
        std::ostringstream oss;
        oss << u.release << " (" << u.version << ")";
        hi.osVersion = oss.str();
    } else {
        hi.osName = "Linux";
        hi.osVersion = "unknown";
    }
#endif
    return hi;
}

// -------------------------
// CPU info per platform
// -------------------------
#if defined(_WIN32)

// Count physical cores using Windows API
inline std::optional<int> windowsPhysicalCoreCount() {
    DWORD len = 0;
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &len) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        return std::nullopt;
    }
    std::vector<uint8_t> buffer(len);
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX p =
        reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data());
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, p, &len)) {
        return std::nullopt;
    }
    int cores = 0;
    BYTE* cur = buffer.data();
    BYTE* end = buffer.data() + len;
    while (cur < end) {
        auto info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(cur);
        if (info->Relationship == RelationProcessorCore) {
            ++cores;
        }
        cur += info->Size;
    }
    return cores;
}

inline std::string windowsCpuNameFromRegistry() {
    HKEY hKey;
    std::string name = "unknown";
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char data[256];
        DWORD type = 0;
        DWORD size = sizeof(data);
        if (RegQueryValueExA(hKey, "ProcessorNameString", nullptr, &type,
                             reinterpret_cast<LPBYTE>(data), &size) == ERROR_SUCCESS) {
            if (type == REG_SZ) {
                name = trim(std::string(data));
            }
        }
        RegCloseKey(hKey);
    }
    return name;
}

inline std::optional<double> windowsCpuMHzFromRegistry() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD mhz = 0;
        DWORD type = 0;
        DWORD size = sizeof(mhz);
        if (RegQueryValueExA(hKey, "~MHz", nullptr, &type,
                             reinterpret_cast<LPBYTE>(&mhz), &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return static_cast<double>(mhz);
        }
        RegCloseKey(hKey);
    }
    return std::nullopt;
}

inline std::string windowsVendorViaCPUID() {
    int regs[4]{};
    __cpuid(regs, 0);
    char vendor[13];
    *reinterpret_cast<int*>(vendor)      = regs[1]; // EBX
    *reinterpret_cast<int*>(vendor + 4)  = regs[2]; // EDX
    *reinterpret_cast<int*>(vendor + 8)  = regs[3]; // ECX
    vendor[12] = '\0';
    return std::string(vendor);
}

inline CpuInfo queryCpuInfo() {
    CpuInfo ci;
    ci.architecture = detectArchMacro();

    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    ci.logicalProcessors = static_cast<int>(si.dwNumberOfProcessors);

    ci.model = windowsCpuNameFromRegistry();
    ci.vendor = windowsVendorViaCPUID();

    ci.physicalCores = windowsPhysicalCoreCount();
    ci.baseClockMHz = windowsCpuMHzFromRegistry();
    ci.maxClockMHz = std::nullopt; // WMI would be needed; keep simple

    ci.featuresSummary = ""; // Optional: could use CPUID feature bits to populate
    return ci;
}

#elif defined(__APPLE__)

inline bool sysctlString(const char* name, std::string& out) {
    size_t len = 0;
    if (sysctlbyname(name, nullptr, &len, nullptr, 0) != 0 || len == 0) return false;
    std::vector<char> buf(len);
    if (sysctlbyname(name, buf.data(), &len, nullptr, 0) != 0) return false;
    out.assign(buf.data(), buf.size());
    out = trim(out);
    return true;
}

template <typename T>
inline bool sysctlValue(const char* name, T& out) {
    size_t len = sizeof(T);
    return sysctlbyname(name, &out, &len, nullptr, 0) == 0;
}

inline CpuInfo queryCpuInfo() {
    CpuInfo ci;
    ci.architecture = detectArchMacro();
    int ncpu = 0;
    if (sysctlValue("hw.ncpu", ncpu)) {
        ci.logicalProcessors = ncpu;
    } else {
        ci.logicalProcessors = static_cast<int>(std::thread::hardware_concurrency());
    }

    int phys = 0;
    if (sysctlValue("hw.physicalcpu", phys)) {
        ci.physicalCores = phys;
    }

    uint64_t hz = 0, hzMax = 0;
    if (sysctlValue("hw.cpufrequency", hz)) {
        ci.baseClockMHz = static_cast<double>(hz) / 1'000'000.0;
    }
    if (sysctlValue("hw.cpufrequency_max", hzMax)) {
        ci.maxClockMHz = static_cast<double>(hzMax) / 1'000'000.0;
    }

    std::string brand, vendor, feats;
    if (sysctlString("machdep.cpu.brand_string", brand)) ci.model = brand;
    if (sysctlString("machdep.cpu.vendor", vendor)) ci.vendor = vendor;
    if (sysctlString("machdep.cpu.features", feats)) ci.featuresSummary = feats;

    return ci;
}

#else // Linux / Unix

inline std::optional<double> readCpuinfoMaxMHz() {
    // Try common path: /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq (kHz)
    const char* path = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq";
    std::ifstream ifs(path);
    if (!ifs) return std::nullopt;
    double khz = 0.0;
    ifs >> khz;
    if (khz <= 0.0) return std::nullopt;
    return khz / 1000.0; // to MHz
}

inline CpuInfo queryCpuInfo() {
    CpuInfo ci;
    ci.architecture = detectArchMacro();

    // Logical processors (online)
#ifdef _SC_NPROCESSORS_ONLN
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    ci.logicalProcessors = (n > 0 ? static_cast<int>(n)
                                  : static_cast<int>(std::thread::hardware_concurrency()));
#else
    ci.logicalProcessors = static_cast<int>(std::thread::hardware_concurrency());
#endif

    // Parse /proc/cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo) {
        std::string line;
        int sockets = 0;
        std::optional<int> cpuCoresPerSocket;
        std::string flags;
        std::string vendor, model;
        bool gotVendor = false, gotModel = false, gotMhz = false;
        double mhzCurrent = 0.0;

        std::set<int> physicalIds;
        while (std::getline(cpuinfo, line)) {
            auto pos = line.find(':');
            std::string key = trim(line.substr(0, pos));
            std::string val = pos != std::string::npos ? trim(line.substr(pos + 1)) : "";

            if (!gotVendor && key == "vendor_id") { vendor = val; gotVendor = true; }
            if (!gotModel && key == "model name") { model = val; gotModel = true; }
            if (!gotMhz && key == "cpu MHz") {
                try { mhzCurrent = std::stod(val); gotMhz = true; } catch (...) {}
            }
            if (key == "physical id") {
                try { physicalIds.insert(std::stoi(val)); } catch (...) {}
            }
            if (key == "cpu cores") {
                try { cpuCoresPerSocket = std::stoi(val); } catch (...) {}
            }
            if (key == "flags") {
                flags = val;
            }
        }

        ci.vendor = vendor.empty() ? "unknown" : vendor;
        ci.model = model.empty() ? "unknown" : model;
        ci.baseClockMHz = gotMhz ? std::optional<double>(mhzCurrent) : std::nullopt;
        ci.maxClockMHz = readCpuinfoMaxMHz();
        if (!physicalIds.empty()) ci.sockets = static_cast<int>(physicalIds.size());
        ci.physicalCores = (ci.sockets && cpuCoresPerSocket)
                         ? std::optional<int>(*ci.sockets * *cpuCoresPerSocket)
                         : cpuCoresPerSocket;
        ci.featuresSummary = flags;
    }

    return ci;
}
#endif

// -------------------------
// Logging utilities
// -------------------------
inline void logHostOverview() {
    using star::core::logging::info;
    using star::core::logging::debug;

    auto host = queryHostInfo();
    info("[Host] Name: ", host.hostname);
    info("[Host] OS: ", host.osName, " | Version: ", host.osVersion);
}

inline void logCpuOverview() {
    using star::core::logging::info;
    using star::core::logging::debug;
    using star::core::logging::warning;

    auto ci = queryCpuInfo();

    info("[CPU] Architecture: ", ci.architecture);
    info("[CPU] Vendor: ", ci.vendor, " | Model: ", ci.model);
    info("[CPU] Logical processors: ", ci.logicalProcessors);

    if (ci.physicalCores) {
        info("[CPU] Physical cores: ", *ci.physicalCores);
    } else {
        warning("[CPU] Physical core count not available on this platform/config.");
    }

    if (ci.sockets) {
        info("[CPU] Sockets: ", *ci.sockets);
    }

    if (ci.baseClockMHz) {
        info("[CPU] Base/Reported clock: ", *ci.baseClockMHz, " MHz");
    } else {
        warning("[CPU] Base clock not reported.");
    }

    if (ci.maxClockMHz) {
        info("[CPU] Max (turbo) clock: ", *ci.maxClockMHz, " MHz");
    }

    if (!ci.featuresSummary.empty()) {
        // For very long flags lists, shorten to keep logs readable.
        const size_t MAX_LEN = 512;
        std::string flags = ci.featuresSummary.size() > MAX_LEN
                                ? (ci.featuresSummary.substr(0, MAX_LEN) + " ...")
                                : ci.featuresSummary;
        debug("[CPU] Features/flags: ", flags);
    }
}

// Convenience: log both
inline void logSystemOverview() {
    logHostOverview();
    logCpuOverview();
}

} // namespace star::core::sysinfo
