// ===========================================================================
// DE TAI 13: HE THONG HANG DOI IN AN VAN PHONG (PRINT SPOOLER SIMULATION)
// Phien ban gop 1 file (All-In-One) de bien dich nhanh tren Dev-C++ (F11)
// Sinh vien: Ho Tien Phuoc | MSSV: 6651060046 | Nhom: 4
// ===========================================================================

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

#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

#include <iostream>
#include <iomanip>
#include <stdexcept>

// =====================================================================
// HANG DOI XOAY VONG (CIRCULAR QUEUE) CAI DAT BANG MANG DONG VONG TRON
// - Luu tru bo dem in truc tiep (Active Print Buffer) voi dung luong co dinh
// - Thao tac enqueue/dequeue/peek dat do phuc tap O(1)
// - Co kiem tra isEmpty(), isFull(), xu ly quay vong chi so front/rear
// =====================================================================
class CircularQueue {
private:
    PrintJob* buffer; // Mang dong luu tru cac lenh in
    int capacity;     // Suc chua toi da cua bo dem
    int front;        // Chi so phan tu dau hang doi
    int rear;         // Chi so phan tu cuoi hang doi
    int count;        // So phan tu hien co trong hang doi

public:
    explicit CircularQueue(int cap = 5)
        : buffer(nullptr), capacity(cap > 0 ? cap : 5), front(0), rear(-1), count(0) {
        buffer = new PrintJob[capacity];
    }

    // Copy Constructor (Rule of Three)
    CircularQueue(const CircularQueue& other)
        : buffer(new PrintJob[other.capacity]),
          capacity(other.capacity),
          front(other.front),
          rear(other.rear),
          count(other.count) {
        for (int i = 0; i < capacity; ++i) {
            buffer[i] = other.buffer[i];
        }
    }

    // Copy Assignment Operator (Rule of Three)
    CircularQueue& operator=(const CircularQueue& other) {
        if (this == &other) return *this;
        PrintJob* newBuf = new PrintJob[other.capacity];
        for (int i = 0; i < other.capacity; ++i) {
            newBuf[i] = other.buffer[i];
        }
        delete[] buffer;
        buffer = newBuf;
        capacity = other.capacity;
        front = other.front;
        rear = other.rear;
        count = other.count;
        return *this;
    }

    // Destructor - giai phong mang dong tranh ro ri bo nho
    ~CircularQueue() {
        delete[] buffer;
        buffer = nullptr;
    }

    // Kiem tra hang doi rong - O(1)
    bool isEmpty() const {
        return count == 0;
    }

    // Kiem tra hang doi day (dieu kien phat hien Buffer Overflow) - O(1)
    bool isFull() const {
        return count == capacity;
    }

    int size() const { return count; }
    int getCapacity() const { return capacity; }
    int getFrontIndex() const { return isEmpty() ? -1 : front; }
    int getRearIndex() const { return isEmpty() ? -1 : rear; }

    // Them lenh in vao cuoi hang doi (FIFO) - O(1)
    bool enqueue(const PrintJob& job) {
        if (isFull()) {
            return false; // Tran bo dem (Buffer Overflow)
        }
        rear = (rear + 1) % capacity;
        buffer[rear] = job;
        buffer[rear].status = JobStatus::WAITING_BUFFER;
        count++;
        return true;
    }

    // Lay lenh in dau hang doi de thuc thi (FIFO) - O(1)
    bool dequeue(PrintJob& outJob) {
        if (isEmpty()) {
            return false; // Hang doi rong (Underflow)
        }
        outJob = buffer[front];
        front = (front + 1) % capacity;
        count--;
        if (count == 0) {
            front = 0;
            rear = -1;
        }
        return true;
    }

    // Xem lenh in dau hang doi khong lay ra - O(1)
    bool peek(PrintJob& outJob) const {
        if (isEmpty()) return false;
        outJob = buffer[front];
        return true;
    }

    // Xoa toan bo phan tu - O(1)
    void clear() {
        front = 0;
        rear = -1;
        count = 0;
    }

    // Huy lenh in bat ky theo ma lenh trong Circular Queue (bao toan FIFO) - O(n)
    bool removeById(int jobId, PrintJob& removedJob) {
        if (isEmpty()) return false;

        int foundOffset = -1;
        for (int i = 0; i < count; ++i) {
            int idx = (front + i) % capacity;
            if (buffer[idx].id == jobId) {
                foundOffset = i;
                removedJob = buffer[idx];
                break;
            }
        }
        if (foundOffset == -1) return false;

        // Dich chuyen cac phan tu phia sau len 1 vi tri theo modulo capacity
        for (int i = foundOffset; i < count - 1; ++i) {
            int curIdx = (front + i) % capacity;
            int nextIdx = (front + i + 1) % capacity;
            buffer[curIdx] = buffer[nextIdx];
        }

        count--;
        if (count == 0) {
            front = 0;
            rear = -1;
        } else {
            rear = (front + count - 1) % capacity;
        }
        return true;
    }

    // Tim kiem tuan tu theo ID (tra ve thu tu cho 1..count, hoac -1 neu khong thay) - O(n)
    int linearSearchById(int jobId, PrintJob* foundJob = nullptr) const {
        for (int i = 0; i < count; ++i) {
            int idx = (front + i) % capacity;
            if (buffer[idx].id == jobId) {
                if (foundJob) *foundJob = buffer[idx];
                return i + 1;
            }
        }
        return -1;
    }

    // Tim kiem tuan tu theo tu khoa (ten tai lieu hoac nguoi gui) - O(n)
    int searchByKeyword(const std::string& keyword, std::ostream& os = std::cout) const {
        std::string keyLower = toLowerASCII(keyword);
        int matched = 0;
        for (int i = 0; i < count; ++i) {
            int idx = (front + i) % capacity;
            std::string docLower = toLowerASCII(buffer[idx].documentName);
            std::string senderLower = toLowerASCII(buffer[idx].senderName);
            if (docLower.find(keyLower) != std::string::npos ||
                senderLower.find(keyLower) != std::string::npos) {
                if (matched == 0) PrintJob::printTableHeader(os);
                buffer[idx].printTableRow(os);
                matched++;
            }
        }
        if (matched > 0) PrintJob::printTableFooter(os);
        return matched;
    }

    // Thay doi dung luong bo dem xoay vong (chi cho phep newCap >= count) - O(n)
    bool resize(int newCapacity) {
        if (newCapacity <= 0 || newCapacity < count) return false;
        PrintJob* newBuf = new PrintJob[newCapacity];
        for (int i = 0; i < count; ++i) {
            newBuf[i] = buffer[(front + i) % capacity];
        }
        delete[] buffer;
        buffer = newBuf;
        capacity = newCapacity;
        front = 0;
        rear = (count == 0) ? -1 : (count - 1);
        return true;
    }

    // Xuat danh sach theo dung thu tu FIFO ra mang tinh/dong (can cap phat truoc >= count)
    int exportToArray(PrintJob* outArr) const {
        if (!outArr) return 0;
        for (int i = 0; i < count; ++i) {
            outArr[i] = buffer[(front + i) % capacity];
        }
        return count;
    }

    // Hien thi so do truc quan mang vong tron voi vi tri chi so Front / Rear
    void displayVisualBuffer(std::ostream& os = std::cout) const {
        os << "\n[SO DO MANG XOAY VONG - ACTIVE PRINT BUFFER] "
           << "(Dung luong: " << count << "/" << capacity
           << " | Front = " << getFrontIndex()
           << " | Rear = " << getRearIndex() << ")\n";

        // Dong chi so o mang
        os << "Chi so: ";
        for (int i = 0; i < capacity; ++i) {
            os << "  [" << std::setw(2) << i << "]    ";
        }
        os << "\n";

        // Dong khung tren
        os << "        ";
        for (int i = 0; i < capacity; ++i) {
            os << "+--------+";
        }
        os << "\n";

        // Xac dinh o nao dang chua du lieu hop le trong vong tron
        bool* occupied = new bool[capacity];
        for (int i = 0; i < capacity; ++i) occupied[i] = false;
        for (int i = 0; i < count; ++i) {
            occupied[(front + i) % capacity] = true;
        }

        // Dong gia tri o mang
        os << "Du lieu:";
        for (int i = 0; i < capacity; ++i) {
            if (occupied[i]) {
                std::ostringstream tag;
                tag << "#" << buffer[i].id;
                os << "|" << std::setw(6) << std::right << tag.str() << "  |";
            } else {
                os << "|  ----  |";
            }
        }
        os << "\n";

        // Dong khung duoi
        os << "        ";
        for (int i = 0; i < capacity; ++i) {
            os << "+--------+";
        }
        os << "\n";

        // Dong mui ten danh dau Front / Rear
        os << "Con tro:";
        for (int i = 0; i < capacity; ++i) {
            bool isF = (!isEmpty() && i == front);
            bool isR = (!isEmpty() && i == rear);
            if (isF && isR)      os << "   ^F,R   ";
            else if (isF)        os << "   ^FRONT ";
            else if (isR)        os << "   ^REAR  ";
            else                 os << "          ";
        }
        os << "\n";

        delete[] occupied;
    }

    // Hien thi dang bang theo thu tu FIFO
    void displayTable(std::ostream& os = std::cout) const {
        os << "\n--- DANH SACH LENH CHO TRONG BO DEM XOAY VONG ("
           << count << "/" << capacity << ") ---\n";
        if (isEmpty()) {
            os << "(Bo dem xoay vong dang rong)\n";
            return;
        }
        PrintJob::printTableHeader(os);
        for (int i = 0; i < count; ++i) {
            int idx = (front + i) % capacity;
            buffer[idx].printTableRow(os);
        }
        PrintJob::printTableFooter(os);
    }
};

#endif // CIRCULAR_QUEUE_H

#ifndef LINKED_QUEUE_H
#define LINKED_QUEUE_H

#include <iostream>

// Nut trong Danh sach lien ket don (bam sat bai giang Chuong 3 & Chuong 4)
struct QueueNode {
    PrintJob data;
    QueueNode* next;

    explicit QueueNode(const PrintJob& job) : data(job), next(nullptr) {}
};

// =====================================================================
// HANG DOI CAI DAT BANG DANH SACH LIEN KET DON (LINKED LIST QUEUE)
// - Cap phat dong linh hoat, khong bi gioi han kich thuoc co dinh
// - Dung cho: (1) Hang doi tran (Overflow Spool) khi CircularQueue day
//             (2) Nhat ky lich su in an (Print History Log)
// - Thao tac enqueue/dequeue/peek dat do phuc tap O(1)
// =====================================================================
class LinkedQueue {
private:
    QueueNode* head; // Con tro dau hang doi (Front)
    QueueNode* tail; // Con tro cuoi hang doi (Rear)
    int count;       // So luong nut hien co

public:
    LinkedQueue() : head(nullptr), tail(nullptr), count(0) {}

    // Copy Constructor (Rule of Three)
    LinkedQueue(const LinkedQueue& other) : head(nullptr), tail(nullptr), count(0) {
        QueueNode* cur = other.head;
        while (cur != nullptr) {
            enqueue(cur->data);
            cur = cur->next;
        }
    }

    // Copy Assignment Operator (Rule of Three)
    LinkedQueue& operator=(const LinkedQueue& other) {
        if (this == &other) return *this;
        clear();
        QueueNode* cur = other.head;
        while (cur != nullptr) {
            enqueue(cur->data);
            cur = cur->next;
        }
        return *this;
    }

    // Destructor - giai phong toan bo nut da cap phat dong
    ~LinkedQueue() {
        clear();
    }

    bool isEmpty() const {
        return head == nullptr;
    }

    int size() const {
        return count;
    }

    // Them phan tu vao cuoi DSLK (FIFO) - O(1)
    void enqueue(const PrintJob& job) {
        QueueNode* newNode = new QueueNode(job);
        if (isEmpty()) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
        count++;
    }

    // Lay phan tu o dau DSLK (FIFO) va giai phong nut - O(1)
    bool dequeue(PrintJob& outJob) {
        if (isEmpty()) return false;
        QueueNode* temp = head;
        outJob = temp->data;
        head = head->next;
        if (head == nullptr) {
            tail = nullptr;
        }
        delete temp;
        count--;
        return true;
    }

    // Xem phan tu dau hang doi - O(1)
    bool peek(PrintJob& outJob) const {
        if (isEmpty()) return false;
        outJob = head->data;
        return true;
    }

    // Giai phong toan bo danh sach - O(n)
    void clear() {
        while (head != nullptr) {
            QueueNode* temp = head;
            head = head->next;
            delete temp;
        }
        tail = nullptr;
        count = 0;
    }

    // Huy lenh in bat ky theo ID (xu ly du 3 truong hop: dau, giua, cuoi) - O(n)
    bool removeById(int jobId, PrintJob& removedJob) {
        if (isEmpty()) return false;

        QueueNode* cur = head;
        QueueNode* prev = nullptr;

        while (cur != nullptr) {
            if (cur->data.id == jobId) {
                removedJob = cur->data;
                if (prev == nullptr) {
                    // Xoa nut dau (head)
                    head = cur->next;
                    if (head == nullptr) tail = nullptr;
                } else {
                    // Xoa nut giua hoac nut cuoi (tail)
                    prev->next = cur->next;
                    if (cur == tail) tail = prev;
                }
                delete cur;
                count--;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    // Tim kiem tuan tu theo ID (tra ve thu tu 1..count, hoac -1) - O(n)
    int linearSearchById(int jobId, PrintJob* foundJob = nullptr) const {
        QueueNode* cur = head;
        int pos = 1;
        while (cur != nullptr) {
            if (cur->data.id == jobId) {
                if (foundJob) *foundJob = cur->data;
                return pos;
            }
            cur = cur->next;
            pos++;
        }
        return -1;
    }

    // Tim kiem tuan tu theo tu khoa (ten tai lieu hoac nguoi gui) - O(n)
    int searchByKeyword(const std::string& keyword, std::ostream& os = std::cout) const {
        std::string keyLower = toLowerASCII(keyword);
        int matched = 0;
        QueueNode* cur = head;
        while (cur != nullptr) {
            std::string docLower = toLowerASCII(cur->data.documentName);
            std::string senderLower = toLowerASCII(cur->data.senderName);
            if (docLower.find(keyLower) != std::string::npos ||
                senderLower.find(keyLower) != std::string::npos) {
                if (matched == 0) PrintJob::printTableHeader(os);
                cur->data.printTableRow(os);
                matched++;
            }
            cur = cur->next;
        }
        if (matched > 0) PrintJob::printTableFooter(os);
        return matched;
    }

    // Xuat toan bo phan tu ra mang dong (nguoi goi phai delete[] sau khi dung) - O(n)
    PrintJob* toArray(int& outSize) const {
        outSize = count;
        if (count == 0) return nullptr;
        PrintJob* arr = new PrintJob[count];
        QueueNode* cur = head;
        int i = 0;
        while (cur != nullptr) {
            arr[i++] = cur->data;
            cur = cur->next;
        }
        return arr;
    }

    // Hien thi dang bang
    void displayTable(const std::string& title, std::ostream& os = std::cout) const {
        os << "\n--- " << title << " (" << count << " lenh) ---\n";
        if (isEmpty()) {
            os << "(Danh sach dang rong)\n";
            return;
        }
        PrintJob::printTableHeader(os);
        QueueNode* cur = head;
        while (cur != nullptr) {
            cur->data.printTableRow(os);
            cur = cur->next;
        }
        PrintJob::printTableFooter(os);
    }
};

#endif // LINKED_QUEUE_H

#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <chrono>
#include <iostream>
#include <iomanip>
#include <cstdlib>

// =====================================================================
// CAC THUAT TOAN TIM KIEM, SAP XEP & DO THUC NGHIEM (CHUONG 1, 6, 7)
// - Sap xep chen (Insertion Sort): O(n^2)
// - Sap xep nhanh (QuickSort - Chia de tri): O(n log n) trung binh
// - Tim kiem tuan tu (Linear Search): O(n)
// - Tim kiem nhi phan (Binary Search): O(log2 n)
// =====================================================================

enum class SortCriteria {
    BY_ID_ASC = 1,
    BY_PAGES_DESC = 2,
    BY_SIZE_DESC = 3,
    BY_NAME_ASC = 4
};

inline bool compareJobs(const PrintJob& a, const PrintJob& b, SortCriteria criteria) {
    switch (criteria) {
        case SortCriteria::BY_ID_ASC:
            return a.id < b.id;
        case SortCriteria::BY_PAGES_DESC:
            if (a.pages != b.pages) return a.pages > b.pages;
            return a.id < b.id;
        case SortCriteria::BY_SIZE_DESC:
            if (a.sizeKB != b.sizeKB) return a.sizeKB > b.sizeKB;
            return a.id < b.id;
        case SortCriteria::BY_NAME_ASC:
            if (a.documentName != b.documentName) return a.documentName < b.documentName;
            return a.id < b.id;
        default:
            return a.id < b.id;
    }
}

inline void swapJobs(PrintJob& a, PrintJob& b) {
    PrintJob tmp = a;
    a = b;
    b = tmp;
}

// 1. Sap xep chen (Insertion Sort) - O(n^2)
inline void insertionSort(PrintJob* arr, int n, SortCriteria criteria) {
    for (int i = 1; i < n; ++i) {
        PrintJob key = arr[i];
        int j = i - 1;
        while (j >= 0 && compareJobs(key, arr[j], criteria)) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// 2. Sap xep nhanh (QuickSort) - Chon chot giua tranh suy bien tren mang da sap xep
inline int partitionJobs(PrintJob* arr, int low, int high, SortCriteria criteria) {
    int mid = low + (high - low) / 2;
    swapJobs(arr[mid], arr[high]); // Dua chot ve cuoi doan
    PrintJob pivot = arr[high];
    int i = low - 1;

    for (int j = low; j < high; ++j) {
        if (compareJobs(arr[j], pivot, criteria)) {
            i++;
            swapJobs(arr[i], arr[j]);
        }
    }
    swapJobs(arr[i + 1], arr[high]);
    return i + 1;
}

inline void quickSortRecursive(PrintJob* arr, int low, int high, SortCriteria criteria) {
    if (low < high) {
        int pi = partitionJobs(arr, low, high, criteria);
        quickSortRecursive(arr, low, pi - 1, criteria);
        quickSortRecursive(arr, pi + 1, high, criteria);
    }
}

inline void quickSort(PrintJob* arr, int n, SortCriteria criteria) {
    if (n > 1) {
        quickSortRecursive(arr, 0, n - 1, criteria);
    }
}

// 3. Tim kiem tuan tu (Linear Search) tren mang - O(n)
inline int linearSearchArrayById(const PrintJob* arr, int n, int targetId) {
    for (int i = 0; i < n; ++i) {
        if (arr[i].id == targetId) return i;
    }
    return -1;
}

// 4. Tim kiem nhi phan (Binary Search) tren mang da sap xep tang dan theo ID - O(log2 n)
inline int binarySearchArrayById(const PrintJob* arr, int n, int targetId, int* comparisonsOut = nullptr) {
    int left = 0;
    int right = n - 1;
    int comps = 0;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        comps++;
        if (arr[mid].id == targetId) {
            if (comparisonsOut) *comparisonsOut = comps;
            return mid;
        } else if (arr[mid].id < targetId) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    if (comparisonsOut) *comparisonsOut = comps;
    return -1;
}

// 5. Phan tich thuc nghiem do thoi gian chay (Chuong 1 & Chuong 4 Bao cao)
inline void runPerformanceBenchmark(std::ostream& os = std::cout) {
    using Clock = std::chrono::high_resolution_clock;

    os << "\n=========================================================================\n";
    os << "   PHAN TICH THUC NGHIEM HIEU NANG CAU TRUC DU LIEU & THUAT TOAN\n";
    os << "=========================================================================\n";

    // Phan 1: So sanh CircularQueue (Mang xoay vong) vs LinkedQueue (DSLK don)
    os << "\n[1] So sanh Enqueue + Dequeue (CircularQueue vs LinkedQueue) - Don vi: ms\n";
    os << "+------------+-------------------+-------------------+\n";
    os << "|  So lenh N | CircularQueue(ms) |  LinkedQueue (ms) |\n";
    os << "+------------+-------------------+-------------------+\n";

    const int queueSizes[] = {1000, 10000, 50000, 100000};
    for (int n : queueSizes) {
        PrintJob sample(1, "Doc_Bench.pdf", "User_Test", 10, 250.5, "2026-11-19 10:00:00");
        PrintJob out;

        // Do CircularQueue
        CircularQueue cq(n);
        auto t1 = Clock::now();
        for (int i = 0; i < n; ++i) {
            sample.id = i + 1;
            cq.enqueue(sample);
        }
        for (int i = 0; i < n; ++i) {
            cq.dequeue(out);
        }
        auto t2 = Clock::now();
        double cqMs = std::chrono::duration<double, std::milli>(t2 - t1).count();

        // Do LinkedQueue
        LinkedQueue lq;
        auto t3 = Clock::now();
        for (int i = 0; i < n; ++i) {
            sample.id = i + 1;
            lq.enqueue(sample);
        }
        for (int i = 0; i < n; ++i) {
            lq.dequeue(out);
        }
        auto t4 = Clock::now();
        double lqMs = std::chrono::duration<double, std::milli>(t4 - t3).count();

        os << "| " << std::right << std::setw(10) << n
           << " | " << std::setw(17) << std::fixed << std::setprecision(3) << cqMs
           << " | " << std::setw(17) << std::fixed << std::setprecision(3) << lqMs << " |\n";
    }
    os << "+------------+-------------------+-------------------+\n";

    // Phan 2: So sanh InsertionSort O(n^2) vs QuickSort O(n log n) tren lich su in
    os << "\n[2] So sanh Sap xep lich su (InsertionSort vs QuickSort) - Don vi: ms\n";
    os << "+------------+-------------------+-------------------+\n";
    os << "|  So lenh N | InsertionSort(ms) |   QuickSort (ms)  |\n";
    os << "+------------+-------------------+-------------------+\n";

    const int sortSizes[] = {500, 1000, 2000, 5000};
    std::srand(42);
    for (int n : sortSizes) {
        PrintJob* arr1 = new PrintJob[n];
        PrintJob* arr2 = new PrintJob[n];
        for (int i = 0; i < n; ++i) {
            int pages = (std::rand() % 500) + 1;
            PrintJob j(i + 1, "TaiLieu", "NV", pages, pages * 12.5, "2026-11-19 10:00:00");
            arr1[i] = j;
            arr2[i] = j;
        }

        auto t1 = Clock::now();
        insertionSort(arr1, n, SortCriteria::BY_PAGES_DESC);
        auto t2 = Clock::now();
        double insMs = std::chrono::duration<double, std::milli>(t2 - t1).count();

        auto t3 = Clock::now();
        quickSort(arr2, n, SortCriteria::BY_PAGES_DESC);
        auto t4 = Clock::now();
        double qsMs = std::chrono::duration<double, std::milli>(t4 - t3).count();

        os << "| " << std::right << std::setw(10) << n
           << " | " << std::setw(17) << std::fixed << std::setprecision(3) << insMs
           << " | " << std::setw(17) << std::fixed << std::setprecision(3) << qsMs << " |\n";

        delete[] arr1;
        delete[] arr2;
    }
    os << "+------------+-------------------+-------------------+\n";

    // Phan 3: So sanh Linear Search O(n) vs Binary Search O(log2 n) (1000 truy van)
    os << "\n[3] So sanh Tim kiem (1.000 truy van truong hop xau nhat) - Don vi: ms\n";
    os << "+------------+-------------------+-------------------+\n";
    os << "|  So lenh N |  LinearSearch(ms) |  BinarySearch(ms) |\n";
    os << "+------------+-------------------+-------------------+\n";

    const int searchSizes[] = {1000, 5000, 20000, 50000};
    const int queries = 1000;
    for (int n : searchSizes) {
        PrintJob* arr = new PrintJob[n];
        for (int i = 0; i < n; ++i) {
            arr[i] = PrintJob(i + 1, "Doc", "NV", 10, 100.0, "2026-11-19 10:00:00");
        }
        int target = n; // Phan tu cuoi (truong hop xau cua Linear Search)
        volatile int sink = 0;

        auto t1 = Clock::now();
        for (int q = 0; q < queries; ++q) {
            sink += linearSearchArrayById(arr, n, target);
        }
        auto t2 = Clock::now();
        double linMs = std::chrono::duration<double, std::milli>(t2 - t1).count();

        auto t3 = Clock::now();
        for (int q = 0; q < queries; ++q) {
            sink += binarySearchArrayById(arr, n, target);
        }
        auto t4 = Clock::now();
        double binMs = std::chrono::duration<double, std::milli>(t4 - t3).count();
        (void)sink;

        os << "| " << std::right << std::setw(10) << n
           << " | " << std::setw(17) << std::fixed << std::setprecision(3) << linMs
           << " | " << std::setw(17) << std::fixed << std::setprecision(3) << binMs << " |\n";

        delete[] arr;
    }
    os << "+------------+-------------------+-------------------+\n";
}

#endif // ALGORITHMS_H

#ifndef PRINT_SPOOLER_H
#define PRINT_SPOOLER_H

#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>

// =====================================================================
// HE THONG DIEU PHOI HANG DOI IN AN VAN PHONG (PRINT SPOOLER SYSTEM)
// Kien truc 2 tang:
//   - Tang 1: CircularQueue (Active Buffer co dung luong gioi han)
//   - Tang 2: LinkedQueue   (Overflow Spool tu dong hung lenh khi tran)
//   - Nhat ky: LinkedQueue  (History Log luu lenh da in / da huy)
// =====================================================================
class PrintSpooler {
private:
    CircularQueue activeBuffer;
    LinkedQueue overflowQueue;
    LinkedQueue historyLog;
    OverflowPolicy policy;

    int nextJobId;
    int totalSubmitted;
    int totalPrinted;
    int totalCancelled;
    int totalOverflowEvents;
    int totalRejected;
    int totalPagesPrinted;
    double totalSizePrintedKB;

    // Tu dong don lenh dau tien tu Overflow Queue len Active Buffer khi co cho trong
    bool promoteFromOverflow(std::ostream* logOs = nullptr) {
        if (!activeBuffer.isFull() && !overflowQueue.isEmpty()) {
            PrintJob promoted;
            if (overflowQueue.dequeue(promoted)) {
                promoted.status = JobStatus::WAITING_BUFFER;
                activeBuffer.enqueue(promoted);
                if (logOs) {
                    *logOs << "  -> [TU DONG DON LENH] Chuyen lenh #" << promoted.id
                           << " (\"" << promoted.documentName
                           << "\") tu Hang doi tran vao Bo dem xoay vong.\n";
                }
                return true;
            }
        }
        return false;
    }

public:
    explicit PrintSpooler(int bufferCapacity = 5,
                          OverflowPolicy initialPolicy = OverflowPolicy::SPILL_TO_LINKED_QUEUE)
        : activeBuffer(bufferCapacity),
          overflowQueue(),
          historyLog(),
          policy(initialPolicy),
          nextJobId(1001),
          totalSubmitted(0),
          totalPrinted(0),
          totalCancelled(0),
          totalOverflowEvents(0),
          totalRejected(0),
          totalPagesPrinted(0),
          totalSizePrintedKB(0.0) {}

    // Cac ham getter thong tin trang thai
    int getBufferCapacity() const { return activeBuffer.getCapacity(); }
    int getBufferCount() const { return activeBuffer.size(); }
    int getOverflowCount() const { return overflowQueue.size(); }
    int getPendingCount() const { return activeBuffer.size() + overflowQueue.size(); }
    int getHistoryCount() const { return historyLog.size(); }
    int getTotalSubmitted() const { return totalSubmitted; }
    int getTotalPrinted() const { return totalPrinted; }
    int getTotalCancelled() const { return totalCancelled; }
    int getTotalOverflowEvents() const { return totalOverflowEvents; }
    int getTotalRejected() const { return totalRejected; }
    int getTotalPagesPrinted() const { return totalPagesPrinted; }
    double getTotalSizePrintedKB() const { return totalSizePrintedKB; }
    OverflowPolicy getOverflowPolicy() const { return policy; }

    void setOverflowPolicy(OverflowPolicy newPolicy) {
        policy = newPolicy;
    }

    // Thay doi suc chua bo dem xoay vong (tu dong don them tu overflow neu mo rong)
    bool resizeBuffer(int newCapacity, std::ostream* logOs = nullptr) {
        if (!activeBuffer.resize(newCapacity)) {
            return false;
        }
        while (!activeBuffer.isFull() && !overflowQueue.isEmpty()) {
            promoteFromOverflow(logOs);
        }
        return true;
    }

    // 1. Tiep nhan lenh in moi (co xu ly tran bo dem Buffer Overflow)
    // Tra ve ma lenh (>0) neu tiep nhan thanh cong, -1 neu bi tu choi / du lieu khong hop le
    int submitJob(const std::string& docName, const std::string& sender,
                  int pages, double sizeKB, std::ostream* logOs = &std::cout,
                  const std::string& customTime = "") {
        if (docName.empty() || sender.empty() || pages <= 0 || sizeKB <= 0.0) {
            if (logOs) *logOs << "[LOI] Thong tin lenh in khong hop le!\n";
            return -1;
        }

        int assignedId = nextJobId++;
        PrintJob newJob(assignedId, docName, sender, pages, sizeKB, customTime, JobStatus::WAITING_BUFFER);
        totalSubmitted++;

        if (!activeBuffer.isFull()) {
            activeBuffer.enqueue(newJob);
            if (logOs) {
                *logOs << "[THANH CONG] Da tiep nhan lenh #" << assignedId
                       << " (\"" << newJob.documentName << "\") vao Bo dem xoay vong ("
                       << activeBuffer.size() << "/" << activeBuffer.getCapacity() << ").\n";
            }
            return assignedId;
        }

        // Xu ly tinh huong TRAN BO DEM (BUFFER OVERFLOW)
        totalOverflowEvents++;
        if (logOs) {
            *logOs << "[CANH BAO - BUFFER OVERFLOW] Bo dem xoay vong da day ("
                   << activeBuffer.size() << "/" << activeBuffer.getCapacity() << ")!\n";
        }

        if (policy == OverflowPolicy::SPILL_TO_LINKED_QUEUE) {
            newJob.status = JobStatus::WAITING_OVERFLOW;
            overflowQueue.enqueue(newJob);
            if (logOs) {
                *logOs << "  -> [CHINH SACH: OVERFLOW SPOOL] Chuyen lenh #" << assignedId
                       << " sang Hang doi DSLK mo rong (Vi tri cho Overflow: "
                       << overflowQueue.size() << ").\n";
            }
            return assignedId;
        } else if (policy == OverflowPolicy::REJECT_NEW) {
            totalRejected++;
            if (logOs) {
                *logOs << "  -> [CHINH SACH: REJECT] Tu choi lenh #" << assignedId
                       << " (\"" << newJob.documentName << "\"). Vui long doi may in giai phong bo dem!\n";
            }
            return -1;
        } else { // OVERWRITE_OLDEST
            PrintJob evicted;
            if (activeBuffer.dequeue(evicted)) {
                evicted.status = JobStatus::CANCELLED;
                historyLog.enqueue(evicted);
                totalCancelled++;
                if (logOs) {
                    *logOs << "  -> [CHINH SACH: OVERWRITE] Loai bo lenh cu nhat #" << evicted.id
                           << " (\"" << evicted.documentName << "\") khoi Front de lay cho.\n";
                }
            }
            activeBuffer.enqueue(newJob);
            if (logOs) {
                *logOs << "  -> Da ghi de lenh moi #" << assignedId
                       << " vao Bo dem xoay vong.\n";
            }
            return assignedId;
        }
    }

    // 2. Dieu phoi in 1 lenh tiep theo (FIFO)
    bool processNextJob(PrintJob* printedOut = nullptr, std::ostream* logOs = &std::cout) {
        if (activeBuffer.isEmpty()) {
            // Phong truong hop buffer rong nhung overflow con lenh
            promoteFromOverflow(logOs);
            if (activeBuffer.isEmpty()) {
                if (logOs) *logOs << "[THONG BAO] Khong co tai lieu nao dang cho in!\n";
                return false;
            }
        }

        PrintJob current;
        activeBuffer.dequeue(current);
        current.status = JobStatus::COMPLETED;
        historyLog.enqueue(current);

        totalPrinted++;
        totalPagesPrinted += current.pages;
        totalSizePrintedKB += current.sizeKB;

        if (printedOut) *printedOut = current;

        if (logOs) {
            *logOs << "[DA IN XONG - FIFO] Lenh #" << current.id
                   << " | Tai lieu: \"" << current.documentName << "\""
                   << " | Nguoi gui: " << current.senderName
                   << " | " << current.pages << " trang ("
                   << std::fixed << std::setprecision(1) << current.sizeKB << " KB).\n";
        }

        // Ngay sau khi giai phong 1 o trong CircularQueue, don lenh tu OverflowQueue (neu co)
        promoteFromOverflow(logOs);
        return true;
    }

    // Dieu phoi in hang loat k lenh (hoac tat ca neu k <= 0)
    int processMultipleJobs(int k, std::ostream* logOs = &std::cout) {
        int countDone = 0;
        if (getPendingCount() == 0) {
            if (logOs) *logOs << "[THONG BAO] Tat ca hang doi deu dang rong!\n";
            return 0;
        }
        while (getPendingCount() > 0 && (k <= 0 || countDone < k)) {
            if (processNextJob(nullptr, logOs)) {
                countDone++;
            } else {
                break;
            }
        }
        if (logOs) {
            *logOs << "=> Tong so lenh vua hoan thanh trong dot nay: " << countDone << "\n";
        }
        return countDone;
    }

    // 3. Huy lenh in bat ky theo ID (tim trong ca CircularQueue va LinkedQueue)
    bool cancelJob(int jobId, PrintJob* cancelledOut = nullptr, std::ostream* logOs = &std::cout) {
        PrintJob removed;
        if (activeBuffer.removeById(jobId, removed)) {
            removed.status = JobStatus::CANCELLED;
            historyLog.enqueue(removed);
            totalCancelled++;
            if (cancelledOut) *cancelledOut = removed;
            if (logOs) {
                *logOs << "[DA HUY LENH] Da huy lenh #" << removed.id
                       << " (\"" << removed.documentName << "\") tu Bo dem xoay vong.\n";
            }
            promoteFromOverflow(logOs);
            return true;
        }

        if (overflowQueue.removeById(jobId, removed)) {
            removed.status = JobStatus::CANCELLED;
            historyLog.enqueue(removed);
            totalCancelled++;
            if (cancelledOut) *cancelledOut = removed;
            if (logOs) {
                *logOs << "[DA HUY LENH] Da huy lenh #" << removed.id
                       << " (\"" << removed.documentName << "\") tu Hang doi tran DSLK.\n";
            }
            return true;
        }

        if (logOs) {
            *logOs << "[KHONG TIM THAY] Khong ton tai lenh cho in mang ma #" << jobId << ".\n";
        }
        return false;
    }

    // 4. Hien thi toan bo trang thai hang doi cho in
    void displayPendingQueues(std::ostream& os = std::cout) const {
        os << "\n=========================================================================================\n";
        os << "  TRANG THAI HANG DOI IN AN HIEN TAI (Tong cho: " << getPendingCount() << " lenh)\n";
        os << "  Chinh sach Overflow: " << policyToString(policy) << "\n";
        os << "=========================================================================================\n";
        activeBuffer.displayVisualBuffer(os);
        activeBuffer.displayTable(os);
        overflowQueue.displayTable("DANH SACH LENH CHO TRONG HANG DOI TRAN (LINKED QUEUE)", os);
    }

    // 5. Tim kiem tuan tu (Linear Search) lenh dang cho theo ID
    bool searchPendingById(int jobId, std::ostream& os = std::cout) const {
        PrintJob found;
        int pos = activeBuffer.linearSearchById(jobId, &found);
        if (pos != -1) {
            os << "[TIM THAY - LINEAR SEARCH] Lenh #" << jobId
               << " dang o vi tri cho thu " << pos << " trong Bo dem xoay vong:\n";
            PrintJob::printTableHeader(os);
            found.printTableRow(os);
            PrintJob::printTableFooter(os);
            return true;
        }

        pos = overflowQueue.linearSearchById(jobId, &found);
        if (pos != -1) {
            os << "[TIM THAY - LINEAR SEARCH] Lenh #" << jobId
               << " dang o vi tri thu " << pos << " trong Hang doi tran DSLK"
               << " (tong thu tu cho: " << (activeBuffer.size() + pos) << "):\n";
            PrintJob::printTableHeader(os);
            found.printTableRow(os);
            PrintJob::printTableFooter(os);
            return true;
        }

        os << "[KHONG TIM THAY] Lenh #" << jobId << " khong co trong danh sach dang cho.\n";
        return false;
    }

    // Tim kiem tuan tu theo tu khoa (ten tai lieu / nguoi gui)
    int searchPendingByKeyword(const std::string& keyword, std::ostream& os = std::cout) const {
        os << "\n--- KET QUA TIM KIEM TUAN TU CHO TU KHOA: \"" << keyword << "\" ---\n";
        os << "* Trong Bo dem xoay vong:\n";
        int c1 = activeBuffer.searchByKeyword(keyword, os);
        if (c1 == 0) os << "  (Khong co ket qua)\n";

        os << "* Trong Hang doi tran DSLK:\n";
        int c2 = overflowQueue.searchByKeyword(keyword, os);
        if (c2 == 0) os << "  (Khong co ket qua)\n";

        os << "=> Tong cong tim thay: " << (c1 + c2) << " lenh.\n";
        return c1 + c2;
    }

    // 6. Tra cuu lich su bang Tim kiem nhi phan (Binary Search - O(log2 n))
    bool searchHistoryBinary(int jobId, std::ostream& os = std::cout) const {
        int n = 0;
        PrintJob* arr = historyLog.toArray(n);
        if (n == 0 || !arr) {
            os << "[THONG BAO] Lich su in an dang rong!\n";
            return false;
        }

        // Sap xep mang lich su tang dan theo ID bang QuickSort truoc khi Binary Search
        quickSort(arr, n, SortCriteria::BY_ID_ASC);

        int comps = 0;
        int idx = binarySearchArrayById(arr, n, jobId, &comps);
        if (idx != -1) {
            os << "[TIM THAY - BINARY SEARCH] Tim thay lenh #" << jobId
               << " trong lich su (" << n << " ban ghi) chi sau " << comps << " phep so sanh:\n";
            PrintJob::printTableHeader(os);
            arr[idx].printTableRow(os);
            PrintJob::printTableFooter(os);
            delete[] arr;
            return true;
        }

        os << "[KHONG TIM THAY - BINARY SEARCH] Lenh #" << jobId
           << " khong co trong lich su (da kiem tra " << comps << " phep so sanh).\n";
        delete[] arr;
        return false;
    }

    // 7. Hien thi lich su da sap xep (QuickSort O(n log n))
    void displayHistorySorted(SortCriteria criteria, std::ostream& os = std::cout) const {
        int n = 0;
        PrintJob* arr = historyLog.toArray(n);
        if (n == 0 || !arr) {
            os << "\n--- LICH SU IN AN DANG RONG ---\n";
            return;
        }

        quickSort(arr, n, criteria);
        std::string label;
        switch (criteria) {
            case SortCriteria::BY_ID_ASC:     label = "Ma lenh tang dan"; break;
            case SortCriteria::BY_PAGES_DESC: label = "So trang giam dan"; break;
            case SortCriteria::BY_SIZE_DESC:  label = "Dung luong (KB) giam dan"; break;
            case SortCriteria::BY_NAME_ASC:   label = "Ten tai lieu A-Z"; break;
        }

        os << "\n--- LICH SU IN AN (" << n << " lenh | Sap xep QuickSort: " << label << ") ---\n";
        PrintJob::printTableHeader(os);
        for (int i = 0; i < n; ++i) {
            arr[i].printTableRow(os);
        }
        PrintJob::printTableFooter(os);
        delete[] arr;
    }

    // 8. Hien thi thong ke tong quan he thong
    void displayStatistics(std::ostream& os = std::cout) const {
        os << "\n==========================================================\n";
        os << "          BAO CAO THONG KE HE THONG PRINT SPOOLER\n";
        os << "==========================================================\n";
        os << "  Suc chua Bo dem xoay vong (Capacity) : " << activeBuffer.getCapacity() << " lenh\n";
        os << "  Dang cho trong Bo dem xoay vong      : " << activeBuffer.size() << " lenh\n";
        os << "  Dang cho trong Hang doi tran (DSLK)  : " << overflowQueue.size() << " lenh\n";
        os << "  Chinh sach xu ly Buffer Overflow     : " << policyToString(policy) << "\n";
        os << "----------------------------------------------------------\n";
        os << "  Tong so lenh da tiep nhan            : " << totalSubmitted << "\n";
        os << "  Tong so lenh da in thanh cong        : " << totalPrinted << "\n";
        os << "  Tong so lenh da huy / bi ghi de      : " << totalCancelled << "\n";
        os << "  So lan xay ra tran bo dem (Overflow) : " << totalOverflowEvents << "\n";
        os << "  So lenh bi tu choi do tran bo dem    : " << totalRejected << "\n";
        os << "  Tong so trang giay da in             : " << totalPagesPrinted << " trang\n";
        os << "  Tong dung luong du lieu da in        : " << std::fixed << std::setprecision(2)
           << totalSizePrintedKB << " KB\n";
        os << "==========================================================\n";
    }

    // 9. Luu toan bo trang thai he thong ra file CSV/TXT ben vung
    bool saveStateToFile(const std::string& filename) const {
        std::ofstream fout(filename);
        if (!fout.is_open()) return false;

        fout << "#PRINT_SPOOLER_STATE_V1\n";
        fout << "META," << activeBuffer.getCapacity() << ","
             << static_cast<int>(policy) << ","
             << nextJobId << ","
             << totalSubmitted << ","
             << totalPrinted << ","
             << totalCancelled << ","
             << totalOverflowEvents << ","
             << totalRejected << ","
             << totalPagesPrinted << ","
             << std::fixed << std::setprecision(2) << totalSizePrintedKB << "\n";

        // Ghi CircularQueue
        int nBuf = activeBuffer.size();
        if (nBuf > 0) {
            PrintJob* bufArr = new PrintJob[nBuf];
            activeBuffer.exportToArray(bufArr);
            for (int i = 0; i < nBuf; ++i) {
                fout << "BUFFER," << bufArr[i].toCSV() << "\n";
            }
            delete[] bufArr;
        }

        // Ghi OverflowQueue
        int nOv = 0;
        PrintJob* ovArr = overflowQueue.toArray(nOv);
        for (int i = 0; i < nOv; ++i) {
            fout << "OVERFLOW," << ovArr[i].toCSV() << "\n";
        }
        delete[] ovArr;

        // Ghi HistoryLog
        int nHist = 0;
        PrintJob* histArr = historyLog.toArray(nHist);
        for (int i = 0; i < nHist; ++i) {
            fout << "HISTORY," << histArr[i].toCSV() << "\n";
        }
        delete[] histArr;

        fout.close();
        return true;
    }

    // Khoi phuc toan bo trang thai he thong tu file
    bool loadStateFromFile(const std::string& filename) {
        std::ifstream fin(filename);
        if (!fin.is_open()) return false;

        std::string header;
        if (!std::getline(fin, header)) return false;
        if (!header.empty() && header.back() == '\r') header.pop_back();
        if (header != "#PRINT_SPOOLER_STATE_V1") return false;

        activeBuffer.clear();
        overflowQueue.clear();
        historyLog.clear();

        std::string line;
        while (std::getline(fin, line)) {
            if (line.empty()) continue;
            if (line.back() == '\r') line.pop_back();

            std::size_t commaPos = line.find(',');
            if (commaPos == std::string::npos) continue;
            std::string tag = line.substr(0, commaPos);
            std::string payload = line.substr(commaPos + 1);

            if (tag == "META") {
                std::stringstream ss(payload);
                std::string tok;
                std::string vals[10];
                int k = 0;
                while (std::getline(ss, tok, ',') && k < 10) vals[k++] = tok;
                if (k == 10) {
                    int cap = std::stoi(vals[0]);
                    activeBuffer = CircularQueue(cap > 0 ? cap : 5);
                    int polInt = std::stoi(vals[1]);
                    if (polInt >= 1 && polInt <= 3) policy = static_cast<OverflowPolicy>(polInt);
                    nextJobId = std::stoi(vals[2]);
                    totalSubmitted = std::stoi(vals[3]);
                    totalPrinted = std::stoi(vals[4]);
                    totalCancelled = std::stoi(vals[5]);
                    totalOverflowEvents = std::stoi(vals[6]);
                    totalRejected = std::stoi(vals[7]);
                    totalPagesPrinted = std::stoi(vals[8]);
                    totalSizePrintedKB = std::stod(vals[9]);
                }
            } else if (tag == "BUFFER") {
                PrintJob j;
                if (PrintJob::fromCSV(payload, j)) activeBuffer.enqueue(j);
            } else if (tag == "OVERFLOW") {
                PrintJob j;
                if (PrintJob::fromCSV(payload, j)) overflowQueue.enqueue(j);
            } else if (tag == "HISTORY") {
                PrintJob j;
                if (PrintJob::fromCSV(payload, j)) historyLog.enqueue(j);
            }
        }
        fin.close();
        return true;
    }

    // Nap danh sach lenh in tu file CSV mau (bo qua dong tieu de neu co)
    int importJobsFromCSV(const std::string& filename, std::ostream* logOs = &std::cout) {
        std::ifstream fin(filename);
        if (!fin.is_open()) {
            if (logOs) *logOs << "[LOI] Khong mo duoc file: " << filename << "\n";
            return -1;
        }
        std::string line;
        int imported = 0;
        while (std::getline(fin, line)) {
            if (line.empty()) continue;
            if (line.back() == '\r') line.pop_back();
            // Bo qua dong tieu de hoac chu thich
            if (line[0] == '#' || line.rfind("id,", 0) == 0 || line.rfind("ID,", 0) == 0) continue;

            PrintJob parsed;
            if (PrintJob::fromCSV(line, parsed)) {
                int newId = submitJob(parsed.documentName, parsed.senderName,
                                      parsed.pages, parsed.sizeKB, logOs, parsed.timestamp);
                if (newId != -1) imported++;
            }
        }
        fin.close();
        return imported;
    }

    // Xuat bao cao thong ke va lich su ra file .txt de nop/luu tru
    bool exportReportToTxt(const std::string& filename) const {
        std::ofstream fout(filename);
        if (!fout.is_open()) return false;
        displayStatistics(fout);
        displayPendingQueues(fout);
        displayHistorySorted(SortCriteria::BY_ID_ASC, fout);
        fout.close();
        return true;
    }
};

#endif // PRINT_SPOOLER_H

#include <iostream>
#include <limits>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

// Cac ham ho tro nhap lieu an toan (xu ly ngoai le khi nguoi dung nhap sai kieu)
static int readInt(const std::string& prompt, int minVal, int maxVal) {
    int val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && val >= minVal && val <= maxVal) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return val;
        }
        std::cout << "  [LOI NHAP LIEU] Vui long nhap so nguyen trong khoang ["
                  << minVal << " .. " << maxVal << "].\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

static double readDouble(const std::string& prompt, double minVal, double maxVal) {
    double val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && val >= minVal && val <= maxVal) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return val;
        }
        std::cout << "  [LOI NHAP LIEU] Vui long nhap so thuc trong khoang ["
                  << minVal << " .. " << maxVal << "].\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

static std::string readNonEmptyLine(const std::string& prompt) {
    std::string line;
    while (true) {
        std::cout << prompt;
        std::getline(std::cin, line);
        // Cat khoang trang hai dau
        std::size_t first = line.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            std::size_t last = line.find_last_not_of(" \t\r\n");
            return line.substr(first, last - first + 1);
        }
        std::cout << "  [LOI NHAP LIEU] Khong duoc de trong, vui long nhap lai!\n";
    }
}

static void printBanner() {
    std::cout << "\n=========================================================================\n";
    std::cout << "   DE TAI 13: HE THONG HANG DOI IN AN VAN PHONG (PRINT SPOOLER)\n";
    std::cout << "   Hoc phan : Cau truc du lieu va Giai thuat (KHM05.3) - DH GTVT\n";
    std::cout << "   Sinh vien: Ho Tien Phuoc | MSSV: 6651060046 | Nhom: 4\n";
    std::cout << "=========================================================================\n";
}

static void printMenu(const PrintSpooler& spooler) {
    std::cout << "\n------------------------- MENU CHUC NANG --------------------------------\n";
    std::cout << " [Trang thai] Buffer: " << spooler.getBufferCount() << "/" << spooler.getBufferCapacity()
              << " | Overflow: " << spooler.getOverflowCount()
              << " | Lich su: " << spooler.getHistoryCount() << "\n";
    std::cout << "-------------------------------------------------------------------------\n";
    std::cout << "  1. Tiep nhan lenh in moi (Enqueue / Xu ly Buffer Overflow)\n";
    std::cout << "  2. Dieu phoi in 1 tai lieu tiep theo (Dequeue - FIFO)\n";
    std::cout << "  3. Dieu phoi in hang loat (In K lenh hoac in tat ca)\n";
    std::cout << "  4. Huy lenh in dang cho theo Ma lenh\n";
    std::cout << "  5. Hien thi So do Bo dem xoay vong & Danh sach tai lieu cho\n";
    std::cout << "  6. Tim kiem tai lieu dang cho (Linear Search - Ma / Tu khoa)\n";
    std::cout << "  7. Xem Lich su in an & Sap xep (QuickSort)\n";
    std::cout << "  8. Tra cuu Lich su theo Ma lenh (Binary Search)\n";
    std::cout << "  9. Cau hinh Chinh sach Tran bo dem (Buffer Overflow Policy)\n";
    std::cout << " 10. Thay doi dung luong Bo dem xoay vong (Resize Circular Queue)\n";
    std::cout << " 11. Nap du lieu lenh in mau tu file CSV (data/sample_jobs.csv)\n";
    std::cout << " 12. Luu / Khoi phuc trang thai he thong (File .csv)\n";
    std::cout << " 13. Xem Bao cao thong ke & Xuat bao cao ra file .txt\n";
    std::cout << " 14. Chay Benchmark thuc nghiem do thoi gian (Chuong 4 Bao cao)\n";
    std::cout << "  0. Thoat chuong trinh\n";
    std::cout << "-------------------------------------------------------------------------\n";
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    printBanner();
    std::cout << "Khoi tao he thong Print Spooler mac dinh:\n";
    std::cout << " - Bo dem xoay vong (Circular Queue) suc chua = 5 lenh.\n";
    std::cout << " - Chinh sach tran bo dem: Chuyen sang Hang doi DSLK mo rong.\n";

    PrintSpooler spooler(5, OverflowPolicy::SPILL_TO_LINKED_QUEUE);
    const std::string STATE_FILE = "data/spooler_state.csv";
    const std::string SAMPLE_FILE = "data/sample_jobs.csv";
    const std::string REPORT_FILE = "data/spooler_report.txt";

    int choice = -1;
    while (choice != 0) {
        printMenu(spooler);
        choice = readInt("Chon chuc nang (0-14): ", 0, 14);

        switch (choice) {
            case 1: {
                std::cout << "\n--- TIEP NHAN LENH IN MOI ---\n";
                std::string doc = readNonEmptyLine("  Ten tai lieu (vd: BaoCao_BTL.pdf): ");
                std::string sender = readNonEmptyLine("  Nguoi gui / Phong ban (vd: Ho Tien Phuoc): ");
                int pages = readInt("  So trang can in (1 - 5000): ", 1, 5000);
                double sizeKB = readDouble("  Dung luong tai lieu KB (0.1 - 500000): ", 0.1, 500000.0);
                spooler.submitJob(doc, sender, pages, sizeKB, &std::cout);
                break;
            }
            case 2: {
                std::cout << "\n--- DIEU PHOI IN 1 TAI LIEU (FIFO) ---\n";
                spooler.processNextJob(nullptr, &std::cout);
                break;
            }
            case 3: {
                std::cout << "\n--- DIEU PHOI IN HANG LOAT ---\n";
                int mode = readInt("  Chon (1: In K lenh tiep theo | 2: In tat ca hang doi): ", 1, 2);
                if (mode == 1) {
                    int k = readInt("  Nhap so lenh K can in (1 - 1000): ", 1, 1000);
                    spooler.processMultipleJobs(k, &std::cout);
                } else {
                    spooler.processMultipleJobs(0, &std::cout);
                }
                break;
            }
            case 4: {
                std::cout << "\n--- HUY LENH IN DANG CHO ---\n";
                if (spooler.getPendingCount() == 0) {
                    std::cout << "[THONG BAO] Hien khong co lenh nao dang cho de huy.\n";
                    break;
                }
                spooler.displayPendingQueues(std::cout);
                int id = readInt("  Nhap Ma lenh can huy: ", 1, 10000000);
                spooler.cancelJob(id, nullptr, &std::cout);
                break;
            }
            case 5: {
                spooler.displayPendingQueues(std::cout);
                break;
            }
            case 6: {
                std::cout << "\n--- TIM KIEM TAI LIEU DANG CHO (LINEAR SEARCH) ---\n";
                int mode = readInt("  Tim theo (1: Ma lenh ID | 2: Tu khoa ten tai lieu/nguoi gui): ", 1, 2);
                if (mode == 1) {
                    int id = readInt("  Nhap Ma lenh can tim: ", 1, 10000000);
                    spooler.searchPendingById(id, std::cout);
                } else {
                    std::string kw = readNonEmptyLine("  Nhap tu khoa tim kiem: ");
                    spooler.searchPendingByKeyword(kw, std::cout);
                }
                break;
            }
            case 7: {
                std::cout << "\n--- XEM & SAP XEP LICH SU IN AN (QUICKSORT) ---\n";
                std::cout << "  1. Ma lenh tang dan\n";
                std::cout << "  2. So trang giam dan\n";
                std::cout << "  3. Dung luong (KB) giam dan\n";
                std::cout << "  4. Ten tai lieu (A-Z)\n";
                int crit = readInt("  Chon tieu chi sap xep (1-4): ", 1, 4);
                spooler.displayHistorySorted(static_cast<SortCriteria>(crit), std::cout);
                break;
            }
            case 8: {
                std::cout << "\n--- TRA CUU LICH SU BANG TIM KIEM NHI PHAN (BINARY SEARCH) ---\n";
                int id = readInt("  Nhap Ma lenh can tra cuu trong lich su: ", 1, 10000000);
                spooler.searchHistoryBinary(id, std::cout);
                break;
            }
            case 9: {
                std::cout << "\n--- CAU HINH CHINH SACH TRAN BO DEM (BUFFER OVERFLOW) ---\n";
                std::cout << "  Hien tai: " << policyToString(spooler.getOverflowPolicy()) << "\n";
                std::cout << "  1. Chuyen sang Hang doi DSLK mo rong (Overflow Spool)\n";
                std::cout << "  2. Tu choi lenh moi & bao loi tran bo dem (Reject)\n";
                std::cout << "  3. Ghi de lenh cu nhat tai Front (Ring Overwrite)\n";
                int p = readInt("  Chon chinh sach moi (1-3): ", 1, 3);
                spooler.setOverflowPolicy(static_cast<OverflowPolicy>(p));
                std::cout << "[THANH CONG] Da chuyen chinh sach sang: "
                          << policyToString(spooler.getOverflowPolicy()) << "\n";
                break;
            }
            case 10: {
                std::cout << "\n--- THAY DOI DUNG LUONG BO DEM XOAY VONG ---\n";
                std::cout << "  Dung luong hien tai: " << spooler.getBufferCapacity()
                          << " (Dang chua: " << spooler.getBufferCount() << " lenh)\n";
                int newCap = readInt("  Nhap dung luong moi (1 - 100): ", 1, 100);
                if (spooler.resizeBuffer(newCap, &std::cout)) {
                    std::cout << "[THANH CONG] Da cap nhat suc chua Bo dem xoay vong thanh "
                              << newCap << " lenh.\n";
                } else {
                    std::cout << "[LOI] Dung luong moi khong duoc nho hon so lenh dang nam trong bo dem ("
                              << spooler.getBufferCount() << ")!\n";
                }
                break;
            }
            case 11: {
                std::cout << "\n--- NAP DU LIEU MAU TU FILE CSV ---\n";
                std::cout << "  File mac dinh: " << SAMPLE_FILE << "\n";
                int opt = readInt("  Dung file mac dinh (1) hay nhap duong dan khac (2)? ", 1, 2);
                std::string path = (opt == 1) ? SAMPLE_FILE : readNonEmptyLine("  Nhap duong dan file CSV: ");
                int cnt = spooler.importJobsFromCSV(path, &std::cout);
                if (cnt >= 0) {
                    std::cout << "=> Da nap thanh cong " << cnt << " lenh in tu file.\n";
                }
                break;
            }
            case 12: {
                std::cout << "\n--- LUU / KHOI PHUC TRANG THAI HE THONG ---\n";
                int opt = readInt("  Chon (1: Luu trang thai ra file | 2: Khoi phuc trang thai tu file): ", 1, 2);
                if (opt == 1) {
                    if (spooler.saveStateToFile(STATE_FILE)) {
                        std::cout << "[THANH CONG] Da luu toan bo trang thai vao: " << STATE_FILE << "\n";
                    } else {
                        std::cout << "[LOI] Khong ghi duoc file: " << STATE_FILE << "\n";
                    }
                } else {
                    if (spooler.loadStateFromFile(STATE_FILE)) {
                        std::cout << "[THANH CONG] Da khoi phuc trang thai tu: " << STATE_FILE << "\n";
                        spooler.displayPendingQueues(std::cout);
                    } else {
                        std::cout << "[LOI] Khong doc duoc hoac file chua ton tai: " << STATE_FILE << "\n";
                    }
                }
                break;
            }
            case 13: {
                spooler.displayStatistics(std::cout);
                int exp = readInt("  Ban co muon xuat bao cao nay ra file " + REPORT_FILE + " khong? (1: Co | 0: Khong): ", 0, 1);
                if (exp == 1) {
                    if (spooler.exportReportToTxt(REPORT_FILE)) {
                        std::cout << "[THANH CONG] Da xuat bao cao ra file: " << REPORT_FILE << "\n";
                    } else {
                        std::cout << "[LOI] Khong the ghi file bao cao.\n";
                    }
                }
                break;
            }
            case 14: {
                runPerformanceBenchmark(std::cout);
                break;
            }
            case 0: {
                std::cout << "\nCam on da su dung He thong Print Spooler (DT13 - Ho Tien Phuoc)!\n";
                break;
            }
        }
    }
    return 0;
}
