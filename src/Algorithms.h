#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "PrintJob.h"
#include "CircularQueue.h"
#include "LinkedQueue.h"
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
