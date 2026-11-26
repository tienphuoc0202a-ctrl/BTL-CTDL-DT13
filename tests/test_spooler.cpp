#include <iostream>
#include <cassert>
#include <sstream>
#include "../src/PrintSpooler.h"

static void testCircularQueueBasicsAndWrapAround() {
    CircularQueue cq(3);
    assert(cq.isEmpty());
    assert(!cq.isFull());

    PrintJob j1(1, "Doc1.pdf", "A", 5, 100.0);
    PrintJob j2(2, "Doc2.pdf", "B", 10, 200.0);
    PrintJob j3(3, "Doc3.pdf", "C", 15, 300.0);
    PrintJob j4(4, "Doc4.pdf", "D", 20, 400.0);

    assert(cq.enqueue(j1));
    assert(cq.enqueue(j2));
    assert(cq.enqueue(j3));
    assert(cq.isFull());
    assert(!cq.enqueue(j4)); // Buffer Overflow!

    // Kiem tra xoay vong chi so (wrap-around)
    PrintJob out;
    assert(cq.dequeue(out) && out.id == 1); // front = 1, rear = 2
    assert(cq.enqueue(j4));                 // rear quay vong ve 0!
    assert(cq.getFrontIndex() == 1);
    assert(cq.getRearIndex() == 0);
    assert(cq.isFull());

    // Huy phan tu o giua (id = 3 tai chi so 2) trong trang thai da wrap-around
    PrintJob removed;
    assert(cq.removeById(3, removed) && removed.id == 3);
    assert(cq.size() == 2);

    // Kiem tra thu tu FIFO con lai: 2 -> 4
    assert(cq.dequeue(out) && out.id == 2);
    assert(cq.dequeue(out) && out.id == 4);
    assert(cq.isEmpty());
    std::cout << "[PASS] TC01: CircularQueue FIFO, Wrap-Around & Remove-Middle\n";
}

static void testLinkedQueueOperations() {
    LinkedQueue lq;
    assert(lq.isEmpty());

    for (int i = 1; i <= 5; ++i) {
        lq.enqueue(PrintJob(i, "File" + std::to_string(i), "User", i * 2, i * 50.0));
    }
    assert(lq.size() == 5);

    // Xoa dau (1), xoa giua (3), xoa cuoi (5)
    PrintJob rem;
    assert(lq.removeById(1, rem) && rem.id == 1);
    assert(lq.removeById(3, rem) && rem.id == 3);
    assert(lq.removeById(5, rem) && rem.id == 5);
    assert(!lq.removeById(99, rem));
    assert(lq.size() == 2);

    // Thu tu con lai phai la 2 -> 4
    PrintJob out;
    assert(lq.dequeue(out) && out.id == 2);
    assert(lq.dequeue(out) && out.id == 4);
    assert(lq.isEmpty());
    std::cout << "[PASS] TC02: LinkedQueue Dynamic Allocation, FIFO & 3-Case Remove\n";
}

static void testSpoolerOverflowPoliciesAndPromotion() {
    std::ostringstream nullLog;

    // 1. Chinh sach SPILL_TO_LINKED_QUEUE
    PrintSpooler sp(3, OverflowPolicy::SPILL_TO_LINKED_QUEUE);
    int id1 = sp.submitJob("A.pdf", "Phuoc", 5, 100.0, &nullLog);
    int id2 = sp.submitJob("B.pdf", "Phuoc", 10, 200.0, &nullLog);
    int id3 = sp.submitJob("C.pdf", "Phuoc", 15, 300.0, &nullLog);
    int id4 = sp.submitJob("D.pdf", "Phuoc", 20, 400.0, &nullLog); // Tran -> Overflow Queue
    int id5 = sp.submitJob("E.pdf", "Phuoc", 25, 500.0, &nullLog); // Tran -> Overflow Queue

    assert(sp.getBufferCount() == 3);
    assert(sp.getOverflowCount() == 2);
    assert(sp.getTotalOverflowEvents() == 2);

    // In 1 lenh -> id1 xong, tu dong don id4 tu Overflow len Active Buffer
    PrintJob printed;
    assert(sp.processNextJob(&printed, &nullLog) && printed.id == id1);
    assert(sp.getBufferCount() == 3);
    assert(sp.getOverflowCount() == 1);

    // Huy id2 trong Active Buffer -> tu dong don id5 len Active Buffer
    assert(sp.cancelJob(id2, nullptr, &nullLog));
    assert(sp.getBufferCount() == 3);
    assert(sp.getOverflowCount() == 0);

    // 2. Chinh sach REJECT_NEW
    sp.setOverflowPolicy(OverflowPolicy::REJECT_NEW);
    int idReject = sp.submitJob("Reject.pdf", "User", 2, 50.0, &nullLog);
    assert(idReject == -1);
    assert(sp.getTotalRejected() == 1);

    // 3. Chinh sach OVERWRITE_OLDEST
    sp.setOverflowPolicy(OverflowPolicy::OVERWRITE_OLDEST);
    int idOverwrite = sp.submitJob("Overwrite.pdf", "User", 9, 180.0, &nullLog);
    assert(idOverwrite > 0);
    assert(sp.getBufferCount() == 3);
    // Kiem tra id3 (cu nhat tai Front) da bi ghi de va dua vao lich su
    assert(sp.searchHistoryBinary(id3, nullLog));

    // In not tat ca con lai: id4 -> id5 -> idOverwrite
    assert(sp.processNextJob(&printed, &nullLog) && printed.id == id4);
    assert(sp.processNextJob(&printed, &nullLog) && printed.id == id5);
    assert(sp.processNextJob(&printed, &nullLog) && printed.id == idOverwrite);
    assert(sp.getPendingCount() == 0);
    assert(sp.getTotalPrinted() == 4);
    assert(sp.getTotalCancelled() == 2); // id2 (user huy) + id3 (bi ghi de)
    std::cout << "[PASS] TC03: Buffer Overflow (3 Policies) & Auto-Promotion FIFO\n";
}

static void testFilePersistenceAndSearchSort() {
    std::ostringstream nullLog;
    PrintSpooler sp1(4, OverflowPolicy::SPILL_TO_LINKED_QUEUE);
    int loaded = sp1.importJobsFromCSV("data/sample_jobs.csv", &nullLog);
    assert(loaded == 8);
    assert(sp1.getBufferCount() == 4);
    assert(sp1.getOverflowCount() == 4);

    // In 3 lenh de co lich su
    assert(sp1.processMultipleJobs(3, &nullLog) == 3);
    assert(sp1.saveStateToFile("data/test_state.csv"));

    // Khoi phuc sang doi tuong moi va kiem tra khop 100%
    PrintSpooler sp2;
    assert(sp2.loadStateFromFile("data/test_state.csv"));
    assert(sp2.getBufferCapacity() == 4);
    assert(sp2.getBufferCount() == 4);
    assert(sp2.getOverflowCount() == 1);
    assert(sp2.getHistoryCount() == 3);
    assert(sp2.getTotalPrinted() == 3);

    // Kiem tra tim kiem tuan tu & nhi phan
    assert(sp2.searchPendingById(1004, nullLog));
    assert(sp2.searchHistoryBinary(1002, nullLog));
    assert(!sp2.searchHistoryBinary(9999, nullLog));

    // Kiem tra xuat bao cao txt
    assert(sp2.exportReportToTxt("data/test_report.txt"));
    std::cout << "[PASS] TC04: CSV Import, State Save/Load, Linear/Binary Search & QuickSort\n";
}

int main() {
    std::cout << "=== BAT DAU BO KIEM THU TU DONG DT13 (PRINT SPOOLER) ===\n";
    testCircularQueueBasicsAndWrapAround();
    testLinkedQueueOperations();
    testSpoolerOverflowPoliciesAndPromotion();
    testFilePersistenceAndSearchSort();
    std::cout << "=== TAT CA 4/4 BO KIEM THU DEU VUOT QUA 100%! ===\n";

    // Chay benchmark thuc nghiem de lay so lieu chinh xac cho Bao cao Chuong 4
    runPerformanceBenchmark(std::cout);
    return 0;
}
