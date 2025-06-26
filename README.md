# SSD Simulator – OS Project

A simplified **Solid-State Drive (SSD) simulator** implemented in **C**, inspired by the classic [`ssd.py`](https://github.com/remzi-arpacidusseau/ostep-projects/blob/master/file-ssd/ssd.py) from the book ***Operating Systems: Three Easy Pieces (OSTEP)***.
This project models SSD behaviors at a low level and is ideal for educational purposes in **Operating Systems**.

---

## 📌 Overview

This simulator replicates and extends the functionality of `ssd.py`, offering three SSD types:

* ✅ **Ideal SSD** – behaves like perfect memory
* 📦 **Direct SSD** – includes block erasure and simple write logic
* 🔁 **Log-structured SSD** – mimics real-world SSDs with **write logs** and **garbage collection**

### ✨ Features

* Supports basic I/O operations: **read**, **write**, **trim**
* Implements a **page-mapped FTL (Flash Translation Layer)**
* Allows user-defined configuration: blocks, pages, workload
* Provides detailed simulation logs and visual insights

---

## ⚙️ Build Instructions

Compile the simulator using:

```bash
gcc ssd.c -o ssd
```

---

## ▶️ Usage

### Run Ideal SSD:

```bash
ssd -T ideal
```

### Write Example:

```bash
ssd -T ideal -L w10:a -l 30 -B 3 -p 10
```

This command:

* Writes `'a'` to logical page `10`
* Sets up SSD with:

  * 30 logical pages
  * 3 physical blocks
  * 10 pages per block

📸 **Example Output:**

![Ideal SSD Write Output](https://github.com/user-attachments/assets/eace0aed-dfa3-43f9-996a-889b4c9051b1)

---

## 🧾 Command-Line Flags

| Flag       | Description                                        |
| ---------- | -------------------------------------------------- |
| `-T`       | SSD type: `ideal`, `direct`, or `log`              |
| `-L`       | Workload: `wX:Y` (write), `rX` (read), `tX` (trim) |
| `-l`       | Number of logical pages                            |
| `-B`       | Number of physical blocks                          |
| `-p`       | Pages per block                                    |
| `-C`       | Show command log and results                       |
| `-F`       | Show Flash state after each operation              |
| `-n`       | Generate `n` random operations                     |
| `-s`       | Seed for random workload                           |
| `-q`       | Quiz mode – guess operations from state            |
| `-S`       | Show stats (operation counts, time estimates)      |
| `-G`, `-g` | Garbage collection thresholds                      |
| `-J`       | Show GC’s low-level commands                       |
| `-r`       | Read % that may target invalid data                |
| `-P`       | Probabilities for read/write/trim ops              |
| `-K`, `-k` | Skewed access pattern simulation                   |

---

## 📋 Example Workloads

### Basic Write, Read, Trim:

```bash
ssd -T ideal -L w10:a,r10,t10 -l 30 -B 3 -p 10 -C -F
```

![Workload Example](https://github.com/user-attachments/assets/1f11140e-a580-407b-ae44-b0fee9dbb99e)

---

### Random Workload on Log SSD:

```bash
ssd -T log -l 30 -B 3 -p 10 -s 10 -n 5 -C
```

![Random Workload Output](https://github.com/user-attachments/assets/00ac7f38-c23c-456c-b1f2-9dd51c1f491b)

---

### Enable Garbage Collection:

```bash
ssd -T log -l 30 -B 3 -p 10 -s 10 -n 60 -G 3 -g 2 -C -F -J
```

Here, **Live** indicates if the data is still referenced by the FTL.

---

## 🎯 Learning Objectives

Using this simulator, you’ll gain insight into:

* The inner workings of **Flash memory**
* How **FTLs** manage logical-to-physical mapping
* **Erase-before-write** constraints and page invalidation
* **Garbage collection** mechanisms
* Realistic workload patterns, including skewed access

---

## 📚 References

* 📖 *Operating Systems: Three Easy Pieces* by Remzi & Andrea Arpaci-Dusseau
  [Book Website](https://pages.cs.wisc.edu/~remzi/OSTEP/)
* 💻 [Original `ssd.py` simulator](https://github.com/remzi-arpacidusseau/ostep-projects/blob/master/file-ssd/ssd.py)

---

## 🚀 Future Enhancements

* Visual output or web-based GUI
* Implement **wear leveling**
* Integration with **file system simulators**
* Better workload generation based on trace logs

---
Team Members:
--
- Hemil Patel (202303005)
- Rom Tala (202303051)
