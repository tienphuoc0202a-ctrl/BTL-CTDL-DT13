#include <iostream>
#include <limits>
#include <string>
#include "PrintSpooler.h"

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
