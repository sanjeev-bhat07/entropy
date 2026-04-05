# entropy

A command-line tool written in C that analyzes files using Shannon entropy to classify them as plaintext, compressed, or encrypted/packed. Built as a practical exploration of an entropy-based heuristic used in real malware analysis workflows.

---

## Background

Shannon entropy measures randomness in data. Applied to a file's raw bytes, it produces a score between 0.0 and 8.0 — the more uniformly distributed the byte values, the higher the score.

This is a heuristic used by malware analysts because packers and encryptors produce output that is statistically close to random. A high entropy score doesn't confirm malicious intent, but it is a meaningful signal worth investigating.

| Entropy Range | Likely File Type |
|---|---|
| 0.0 – 5.0 | Plaintext, source code, structured data |
| 5.0 – 6.5 | Mixed content |
| 6.5 – 7.5 | Compressed (ZIP, PNG, etc.) |
| 7.5 – 8.0 | Encrypted or packed |

> These thresholds were calibrated empirically. Plain English text scores higher than you might expect (~4.5–5.0) because byte-level entropy accounts for character variety, not linguistic patterns.

---

## Features

- **Whole-file entropy** — single score and classification label per file
- **Sliding window analysis** — divides the file into chunks and reports how entropy shifts across sections. Useful for detecting files with low-entropy headers and high-entropy packed payloads
- **Multi-file support** — analyze multiple files in one run with a comparison summary at the end
- **Verbose mode** — shows the top 10 most frequent byte values and their percentage share, helping explain why a file scored the way it did

---

## Build

Requires GCC and Make.

```bash
make
```

To clean compiled output:

```bash
make clean
```

---

## Usage

```bash
# Single file
./entropy <file>

# Multiple files
./entropy <file1> <file2> <file3>

# Verbose mode (byte frequency breakdown + per-window bars)
./entropy --verbose <file>

# Verbose with multiple files
./entropy --verbose <file1> <file2>
```

`--verbose` can appear anywhere in the argument list.

---

## Example Output

```
----------------------------------------
File     : sample.exe
Size     : 413696 bytes
Entropy  : 7.8901 / 8.0000
Class    : Encrypted / Packed

--- Sliding Window Analysis (window: 1024 bytes) ---

  Total windows   : 404
  Plaintext       : 2 windows
  Mixed           : 1 windows
  Compressed      : 8 windows
  Encrypted/Packed: 393 windows

  Lowest entropy  : 3.1245 at offset 0x0
  Highest entropy : 7.9981 at offset 0x1400
```

The low-entropy section at offset `0x0` corresponds to the PE header — structured and readable. The sharp jump at `0x1400` is where the packed payload begins. Whole-file entropy alone would not reveal this boundary.

---

## Project Structure

```
entropy/
├── main.c       — argument parsing, file I/O, output formatting
├── entropy.c    — entropy computation, classification, analysis functions
├── entropy.h    — public interface declarations
└── Makefile     — build configuration
```

`entropy.c` has no file I/O — it only operates on byte buffers passed to it. This keeps the logic reusable and the separation clean.

---

## Limitations

- Entropy is a heuristic, not a classifier. A high score could mean encryption, compression, or a media file. Context matters.
- Thresholds are empirical and may not generalize perfectly across all file types.
- Sliding window summary caps display at 64 windows in verbose mode. Very large files will show a summary rather than every window.
- Maximum of 64 files per invocation.

---

## What I Learned

- How Shannon entropy applies to binary data at the byte level
- Why whole-file entropy misses structural variation — and how sliding windows address that
- Raw file I/O in C, pointer arithmetic on byte buffers, heap memory management
- Why English text scores higher than expected at the byte level versus the linguistic level
- The threshold calibration problem: initial thresholds misclassified plain text, which led to understanding the difference between byte-level and character-level entropy

---

## Possible Extensions

- Output results as JSON for integration with other tools
- Histogram visualization of full byte frequency distribution
- Entropy comparison against a known-clean baseline file
- Recursive directory scanning
