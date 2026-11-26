# ĐỀ TÀI 13: HỆ THỐNG HÀNG ĐỢI IN ẤN VĂN PHÒNG (PRINT SPOOLER SIMULATION)

- **Học phần:** Cấu trúc dữ liệu và Giải thuật (`KHM05.3`) – Trường Đại học Giao thông Vận tải
- **Sinh viên thực hiện:** Hồ Tiến Phước
- **MSSV:** `6651060046`
- **Nhóm:** Nhóm 4 (Thực hiện cá nhân)
- **Ngôn ngữ lập trình:** C++ (Chuẩn C++17, không phụ thuộc thư viện ngoài)

---

## 1. Giới thiệu & Kiến trúc hệ thống

Hệ thống mô phỏng bộ điều phối hàng đợi in ấn văn phòng (**Print Spooler**) theo kiến trúc hàng đợi 2 tầng nhằm giải quyết đồng thời hai bài toán cốt lõi là **điều phối FIFO (First-In First-Out)** và **xử lý tràn bộ đệm (Buffer Overflow)**:

1. **Tầng 1 – `CircularQueue` (Hàng đợi xoay vòng bằng mảng động):**
   - Lưu trữ bộ đệm in trực tiếp (`Active Print Buffer`) với dung lượng giới hạn `capacity`.
   - Tự cài đặt đầy đủ biến chỉ số `front`, `rear`, `count`; phép toán `isEmpty()`, `isFull()`, `enqueue()`, `dequeue()`, `peek()` đạt độ phức tạp $O(1)$.
   - Hỗ trợ hủy lệnh ở vị trí bất kỳ (bảo toàn FIFO), thay đổi kích thước bộ đệm (`resize`), và vẽ sơ đồ mảng vòng tròn trực quan chỉ rõ vị trí `FRONT` / `REAR`.
2. **Tầng 2 – `LinkedQueue` (Hàng đợi danh sách liên kết đơn):**
   - Cấp phát động bằng `struct QueueNode`, quản lý con trỏ `head`/`tail`, giải phóng bộ nhớ chặt chẽ bằng Destructor.
   - Đóng vai trò **Hàng đợi tràn mở rộng (`Overflow Spool`)** tự động hứng lệnh khi `CircularQueue` đầy, đồng thời lưu **Nhật ký lịch sử in ấn (`History Log`)**.
   - Ngay khi `CircularQueue` trống chỗ (sau khi in xong hoặc hủy lệnh), hệ thống tự động đôn lệnh đầu của `Overflow Spool` lên theo đúng thứ tự FIFO.
3. **3 Chính sách xử lý tràn bộ đệm (Buffer Overflow):**
   - `SPILL_TO_LINKED_QUEUE`: Chuyển sang hàng đợi DSLK mở rộng (mặc định).
   - `REJECT_NEW`: Từ chối lệnh mới và phát cảnh báo tràn bộ đệm.
   - `OVERWRITE_OLDEST`: Ghi đè lệnh cũ nhất tại `front` (mô phỏng Ring Buffer).
4. **Thuật toán tìm kiếm, sắp xếp & phân tích thực nghiệm:**
   - **Tìm kiếm tuần tự (`Linear Search` - $O(n)$):** Tra cứu lệnh đang chờ theo mã lệnh hoặc từ khóa.
   - **Sắp xếp nhanh (`QuickSort` - $O(n \log n)$):** Sắp xếp lịch sử in theo mã lệnh, số trang, dung lượng, tên tài liệu.
   - **Tìm kiếm nhị phân (`Binary Search` - $O(\log_2 n)$):** Tra cứu nhanh lịch sử in đã sắp xếp.
   - **Benchmark thực nghiệm:** Đo thời gian chạy thực tế giữa `CircularQueue` vs `LinkedQueue`, `InsertionSort` vs `QuickSort`, và `LinearSearch` vs `BinarySearch`.

---

## 2. Cấu trúc thư mục

```text
DT13_PrintSpooler_HoTienPhuoc/
├── src/
│   ├── PrintJob.h          # Cấu trúc lệnh in, trạng thái, CSV I/O
│   ├── CircularQueue.h     # Tự cài đặt Hàng đợi xoay vòng (mảng động)
│   ├── LinkedQueue.h       # Tự cài đặt Hàng đợi DSLK đơn (con trỏ head/tail)
│   ├── Algorithms.h        # InsertionSort, QuickSort, Linear/Binary Search & Benchmark
│   ├── PrintSpooler.h      # Lớp điều phối Print Spooler 2 tầng & File Persistence
│   └── main.cpp            # Giao diện Console tương tác & Input Validation
├── single_file/
│   └── PrintSpooler_AllInOne.cpp  # Phiên bản gộp 1 file (mở Dev-C++ bấm F11 chạy ngay)
├── data/
│   └── sample_jobs.csv     # Bộ 8 lệnh in mẫu để nạp kiểm thử nhanh
├── tests/
│   └── test_spooler.cpp    # Bộ kiểm thử tự động (Unit & Integration Tests)
├── Makefile                # Biên dịch nhanh trên Linux/macOS/MinGW
├── build_windows.bat       # Biên dịch và chạy nhanh trên Windows
├── .gitignore
└── README.md
```

---

## 3. Hướng dẫn biên dịch và chạy chương trình

### Cách 1: Dùng trình biên dịch `g++` (VS Code / Terminal / MinGW / Linux / macOS)
```bash
# Biên dịch chương trình chính
g++ -std=c++17 -Wall -Wextra -O2 src/main.cpp -o print_spooler

# Chạy chương trình (Linux/macOS)
./print_spooler

# Chạy chương trình (Windows CMD/PowerShell)
print_spooler.exe
```

### Cách 2: Chạy nhanh trên Windows bằng file `.bat`
Nhấp đúp vào file `build_windows.bat` (yêu cầu máy đã cài MinGW/g++ trong PATH).

### Cách 3: Mở bằng Dev-C++ (Không cần tạo Project)
Mở trực tiếp file `single_file/PrintSpooler_AllInOne.cpp` trong Dev-C++, bật chuẩn C++11/C++17 (`Tools -> Compiler Options -> -std=c++17`) rồi nhấn **F11 (Compile & Run)**.

### Cách 4: Chạy bộ kiểm thử tự động (Unit Tests & Benchmark)
```bash
g++ -std=c++17 -Wall -Wextra -O2 tests/test_spooler.cpp -o test_spooler
./test_spooler
```

### 5. Kết quả kiểm thử và Benchmark

**Kết quả kiểm thử tự động:**
- TC01: CircularQueue FIFO, Wrap-Around và Remove-Middle: PASS.
- TC02: LinkedQueue cấp phát động, FIFO và Remove: PASS.
- TC03: Buffer Overflow với 3 chính sách và Auto-Promotion FIFO: PASS.
- TC04: CSV Import, Save/Load, Search và QuickSort: PASS.

**Tổng kết: 4/4 kiểm thử thành công.**

**Kết quả Benchmark:**
- CircularQueue có tốc độ Enqueue/Dequeue tốt hơn LinkedQueue trong các lần đo.
- QuickSort nhanh hơn InsertionSort rõ rệt khi số lượng phần tử tăng.
- BinarySearch nhanh hơn LinearSearch trong các truy vấn trên dữ liệu đã sắp xếp.

**Kết luận:** Hệ thống đáp ứng các chức năng hàng đợi FIFO, xử lý tràn bộ đệm, hủy lệnh, lưu trữ dữ liệu và tìm kiếm, sắp xếp.
