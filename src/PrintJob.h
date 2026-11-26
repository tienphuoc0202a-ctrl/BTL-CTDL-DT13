#ifndef PRINT_JOB_H
#define PRINT_JOB_H

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <ctime>
#include <algorithm>

// Trang thai cua mot lenh in trong he thong Print Spooler
enum class JobStatus {
    WAITING_BUFFER,    // Dang cho trong bo dem xoay vong (Circular Queue)
    WAITING_OVERFLOW,  // Dang cho trong hang doi tran (Linked Queue)
    COMPLETED,         // Da in thanh cong
    CANCELLED          // Da bi huy (do nguoi dung huy hoac bi ghi de khi tran)
};

// Chinh sach xu ly khi bo dem xoay vong bi tran (Buffer Overflow)
enum class OverflowPolicy {
    SPILL_TO_LINKED_QUEUE = 1, // Chuyen sang hang doi DSLK mo rong (Khuyen nghi)
    REJECT_NEW = 2,            // Tu choi lenh moi va bao loi tran bo dem
    OVERWRITE_OLDEST = 3       // Ghi de lenh cu nhat tai vi tri Front (Ring Buffer)
};

inline std::string statusToString(JobStatus st) {
    switch (st) {
        case JobStatus::WAITING_BUFFER:   return "Cho in (Buffer)";
        case JobStatus::WAITING_OVERFLOW: return "Cho (Overflow)";
        case JobStatus::COMPLETED:        return "Hoan thanh";
        case JobStatus::CANCELLED:        return "Da huy";
        default:                          return "Khong xac dinh";
    }
}

inline std::string statusToCode(JobStatus st) {
    switch (st) {
        case JobStatus::WAITING_BUFFER:   return "WAITING_BUFFER";
        case JobStatus::WAITING_OVERFLOW: return "WAITING_OVERFLOW";
        case JobStatus::COMPLETED:        return "COMPLETED";
        case JobStatus::CANCELLED:        return "CANCELLED";
        default:                          return "UNKNOWN";
    }
}

inline JobStatus codeToStatus(const std::string& code) {
    if (code == "WAITING_BUFFER")   return JobStatus::WAITING_BUFFER;
    if (code == "WAITING_OVERFLOW") return JobStatus::WAITING_OVERFLOW;
    if (code == "COMPLETED")        return JobStatus::COMPLETED;
    if (code == "CANCELLED")        return JobStatus::CANCELLED;
    return JobStatus::WAITING_BUFFER;
}

inline std::string policyToString(OverflowPolicy p) {
    switch (p) {
        case OverflowPolicy::SPILL_TO_LINKED_QUEUE:
            return "Chuyen sang Hang doi DSLK mo rong (Overflow Spool)";
        case OverflowPolicy::REJECT_NEW:
            return "Tu choi lenh moi (Strict Buffer Overflow)";
        case OverflowPolicy::OVERWRITE_OLDEST:
            return "Ghi de lenh cu nhat tai Front (Ring Overwrite)";
        default:
            return "Khong xac dinh";
    }
}

inline std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* localTm = std::localtime(&now);
    char buf[32];
    if (localTm && std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localTm)) {
        return std::string(buf);
    }
    return "2026-01-01 00:00:00";
}

// Chuan hoa chuoi tranh ki tu phay hoac xuong dong lam vo dinh dang CSV
inline std::string sanitizeCSVField(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        if (c == ',' || c == '\n' || c == '\r') c = ';';
    }
    return out;
}

inline std::string toLowerASCII(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

inline std::string truncateText(const std::string& s, std::size_t maxLen) {
    if (s.size() <= maxLen) return s;
    if (maxLen <= 3) return s.substr(0, maxLen);
    return s.substr(0, maxLen - 3) + "...";
}

struct PrintJob {
    int id;
    std::string documentName;
    std::string senderName;
    int pages;
    double sizeKB;
    std::string timestamp;
    JobStatus status;

    PrintJob()
        : id(0), documentName(""), senderName(""),
          pages(0), sizeKB(0.0), timestamp(""),
          status(JobStatus::WAITING_BUFFER) {}

    PrintJob(int _id, const std::string& _doc, const std::string& _sender,
             int _pages, double _sizeKB,
             const std::string& _time = "",
             JobStatus _status = JobStatus::WAITING_BUFFER)
        : id(_id),
          documentName(sanitizeCSVField(_doc)),
          senderName(sanitizeCSVField(_sender)),
          pages(_pages),
          sizeKB(_sizeKB),
          timestamp(_time.empty() ? getCurrentTimestamp() : sanitizeCSVField(_time)),
          status(_status) {}

    std::string toCSV() const {
        std::ostringstream oss;
        oss << id << ","
            << sanitizeCSVField(documentName) << ","
            << sanitizeCSVField(senderName) << ","
            << pages << ","
            << std::fixed << std::setprecision(2) << sizeKB << ","
            << sanitizeCSVField(timestamp) << ","
            << statusToCode(status);
        return oss.str();
    }

    static bool fromCSV(const std::string& line, PrintJob& outJob) {
        if (line.empty()) return false;
        std::stringstream ss(line);
        std::string token;
        std::string fields[7];
        int idx = 0;
        while (std::getline(ss, token, ',') && idx < 7) {
            if (!token.empty() && token.back() == '\r') token.pop_back();
            fields[idx++] = token;
        }
        if (idx < 5) return false;
        try {
            outJob.id = std::stoi(fields[0]);
            outJob.documentName = fields[1];
            outJob.senderName = fields[2];
            outJob.pages = std::stoi(fields[3]);
            outJob.sizeKB = std::stod(fields[4]);
            outJob.timestamp = (idx >= 6 && !fields[5].empty()) ? fields[5] : getCurrentTimestamp();
            outJob.status = (idx >= 7 && !fields[6].empty()) ? codeToStatus(fields[6]) : JobStatus::WAITING_BUFFER;
            if (outJob.pages <= 0 || outJob.sizeKB <= 0.0) return false;
            return true;
        } catch (...) {
            return false;
        }
    }

    static void printTableHeader(std::ostream& os = std::cout) {
        os << "+------+----------------------------+--------------------+-------+----------+---------------------+------------------+\n";
        os << "| " << std::left << std::setw(4)  << "Ma"
           << " | " << std::setw(26) << "Ten tai lieu"
           << " | " << std::setw(18) << "Nguoi gui"
           << " | " << std::right << std::setw(5)  << "Trang"
           << " | " << std::setw(8)  << "Size(KB)"
           << " | " << std::left  << std::setw(19) << "Thoi gian gui"
           << " | " << std::setw(16) << "Trang thai" << " |\n";
        os << "+------+----------------------------+--------------------+-------+----------+---------------------+------------------+\n";
    }

    static void printTableFooter(std::ostream& os = std::cout) {
        os << "+------+----------------------------+--------------------+-------+----------+---------------------+------------------+\n";
    }

    void printTableRow(std::ostream& os = std::cout) const {
        os << "| " << std::left  << std::setw(4)  << id
           << " | " << std::setw(26) << truncateText(documentName, 26)
           << " | " << std::setw(18) << truncateText(senderName, 18)
           << " | " << std::right << std::setw(5)  << pages
           << " | " << std::setw(8)  << std::fixed << std::setprecision(1) << sizeKB
           << " | " << std::left  << std::setw(19) << truncateText(timestamp, 19)
           << " | " << std::setw(16) << truncateText(statusToString(status), 16) << " |\n";
    }
};

#endif // PRINT_JOB_H
