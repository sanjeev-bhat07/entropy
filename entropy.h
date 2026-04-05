#ifndef ENTROPY_H
#define ENTROPY_H

#include <stddef.h>

/*
 * Computes Shannon entropy of a raw byte buffer.
 * Returns a value between 0.0 (no randomness) and 8.0 (maximum randomness).
 *
 * data   : pointer to raw bytes
 * length : number of bytes to analyze
 */
double compute_entropy(const unsigned char *data, size_t length);

/*
 * Classifies an entropy score into a human-readable label.
 * Thresholds are calibrated empirically for file analysis:
 *   < 5.0  : Plaintext / Structured data
 *   < 6.5  : Mixed content
 *   < 7.5  : Compressed
 *  >= 7.5  : Encrypted / Packed
 */
const char *classify_entropy(double entropy);

/*
 * Divides the buffer into fixed-size windows and computes entropy per window.
 * Prints a summary of entropy distribution across the file.
 * If verbose is non-zero, also prints a per-window ASCII bar chart.
 *
 * data        : pointer to raw bytes
 * length      : total number of bytes
 * window_size : bytes per analysis window
 * verbose     : if non-zero, print per-window bar display
 */
void sliding_window_entropy(const unsigned char *data, size_t length, size_t window_size, int verbose);

/*
 * Prints the top 10 most frequent byte values in the buffer.
 * Shows hex value, count, and percentage — useful for understanding
 * why a file scored the entropy value it did.
 *
 * data   : pointer to raw bytes
 * length : number of bytes to analyze
 */
void verbose_analysis(const unsigned char *data, size_t length);

#endif