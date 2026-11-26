#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

#include "PrintJob.h"
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
