# Contributing to DariVN

Thank you for your interest in contributing to **DariVN**! We welcome bug reports, documentation improvements, optimizations, and feature contributions.

---

## Contributor License Agreement (CLA)

Because DariVN is distributed under a **dual-licensing model** (AGPLv3 for open source and a Commercial License for proprietary closed-source games), **all contributors must agree to the Contributor License Agreement before their pull requests can be merged**.

* Please review the full agreement here: [`CLA.md`](CLA.md).
* When you submit a Pull Request, an automated CLA check (via GitHub Actions / CLA Assistant) will prompt you to agree with a single click, or you can state your agreement in the PR discussion.
* **Key takeaway:** You retain full copyright ownership of your contribution, while granting DariVN the right to distribute it under both AGPLv3 and commercial licenses.

---

## How to Contribute

### 1. Reporting Bugs
* Check the existing issues to ensure the bug hasn't already been reported.
* Open an issue describing the problem, reproduction steps, your OS, and engine configuration.

### 2. Proposing Features
* For significant architectural or feature additions, please open an issue first to discuss the design with the maintainers.

### 3. Submitting Code (Pull Requests)
1. Fork the repository and create your feature branch:
   ```bash
   git checkout -b feature/my-new-feature
   ```
2. Keep code style consistent with the existing C++20 codebase:
   * 4 spaces indentation.
   * Modern C++ idioms, RAII, and memory safety.
   * Ensure third-party dependencies are not added unless strictly necessary and permissive-licensed (MIT / Apache / zlib).
3. Verify that the project compiles cleanly on your platform:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```
4. Commit your changes with clear, descriptive commit messages.
5. Push to your branch and open a Pull Request against the `dev` branch.
