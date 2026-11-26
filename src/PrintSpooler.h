#ifndef PRINT_SPOOLER_H
#define PRINT_SPOOLER_H
#include <sstream>
#include "PrintJob.h"
#include "CircularQueue.h"
#include "LinkedQueue.h"
#include "Algorithms.h"
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
