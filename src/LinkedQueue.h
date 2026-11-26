#ifndef LINKED_QUEUE_H
#define LINKED_QUEUE_H

#include "PrintJob.h"
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
