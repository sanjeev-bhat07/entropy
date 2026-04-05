#include "entropy.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double compute_entropy(const unsigned char *data, size_t length) {
    if (data == NULL || length == 0)
        return 0.0;

    /*
     * Count how many times each byte value (0–255) appears.
     * Each byte value is used directly as an array index.
     * = {0} zero-initializes the entire array — required for
     * local arrays in C which are otherwise uninitialized.
     */
    size_t freq[256] = {0};
    for (size_t i = 0; i < length; i++)
        freq[data[i]]++;

    /*
     * Shannon entropy formula: H = -sum(p * log2(p))
     * where p is the probability of each byte value appearing.
     *
     * We skip zero-frequency bytes because:
     *   - mathematically, 0 * log2(0) = 0 by convention
     *   - but log2(0) in C returns -inf, causing NaN
     */
    double entropy = 0.0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] == 0)
            continue;

        /* Cast to double before dividing — integer division would truncate */
        double p = (double)freq[i] / length;

        /*
         * log2(p) is always negative for p in (0,1).
         * Subtracting a negative adds a positive value to entropy.
         * This is equivalent to writing: entropy += -p * log2(p)
         */
        entropy -= p * log2(p);
    }

    return entropy;
}

const char *classify_entropy(double entropy) {
    /*
     * Thresholds based on empirical observation across file types.
     * Note: plain English text scores ~4.5–5.0 at the byte level
     * due to character variety — higher than the linguistic entropy
     * of ~1.0 bits/character measured at the word/pattern level.
     */
    if (entropy < 5.0)
        return "Plaintext / Structured data";
    else if (entropy < 6.5)
        return "Mixed content";
    else if (entropy < 7.5)
        return "Compressed";
    else
        return "Encrypted / Packed";
}

void sliding_window_entropy(const unsigned char *data, size_t length, size_t window_size, int verbose) {
    if (data == NULL || length == 0 || window_size == 0)
        return;

    printf("\n--- Sliding Window Analysis (window: %zu bytes) ---\n\n", window_size);

    size_t offset        = 0;
    size_t window_count  = 0;
    double min_entropy   = 8.0, max_entropy = 0.0;
    size_t min_offset    = 0,   max_offset  = 0;

    /* Counters for how many windows fall into each class */
    size_t count_plain = 0, count_mixed = 0, count_compressed = 0, count_encrypted = 0;

    /* Store up to 512 windows for optional bar display */
    size_t  max_display  = 64;
    size_t  display_count = 0;
    double *entropies = malloc(sizeof(double) * (length / window_size + 1));
    size_t *offsets   = malloc(sizeof(size_t) * (length / window_size + 1));

    while (offset < length) {
        /*
         * Clamp the last window to remaining bytes.
         * Without this, the last chunk could read past the buffer end
         * if the file size isn't evenly divisible by window_size.
         */
        size_t chunk_size = window_size;
        if (offset + chunk_size > length)
            chunk_size = length - offset;

        /*
         * Pointer arithmetic: data + offset points to byte number
         * 'offset' inside the buffer. We pass this mid-buffer pointer
         * directly to compute_entropy — no copying needed.
         */
        double entropy = compute_entropy(data + offset, chunk_size);

        /* Track min/max entropy positions across all windows */
        if (entropy < min_entropy) { min_entropy = entropy; min_offset = offset; }
        if (entropy > max_entropy) { max_entropy = entropy; max_offset = offset; }

        /* Classify and count this window */
        if      (entropy < 5.0) count_plain++;
        else if (entropy < 6.5) count_mixed++;
        else if (entropy < 7.5) count_compressed++;
        else                    count_encrypted++;

        /* Store for optional bar display, up to max_display windows */
        if (entropies && offsets && display_count < max_display) {
            entropies[display_count] = entropy;
            offsets[display_count]   = offset;
            display_count++;
        }

        window_count++;
        offset += window_size;
    }

    /* Print summary — always shown regardless of verbose flag */
    printf("  Total windows   : %zu\n",      window_count);
    printf("  Plaintext       : %zu windows\n", count_plain);
    printf("  Mixed           : %zu windows\n", count_mixed);
    printf("  Compressed      : %zu windows\n", count_compressed);
    printf("  Encrypted/Packed: %zu windows\n\n", count_encrypted);
    printf("  Lowest entropy  : %.4f at offset 0x%zx\n", min_entropy, min_offset);
    printf("  Highest entropy : %.4f at offset 0x%zx\n\n", max_entropy, max_offset);

    /*
     * Per-window bar chart — only shown in verbose mode.
     * Each bar is scaled: entropy 0–8 mapped to 0–40 characters.
     * Visually highlights where entropy spikes or drops across the file.
     */
    if (verbose && display_count > 0) {
        printf("  First %zu windows:\n\n", display_count);
        printf("  %-12s  %-10s  %s\n", "Offset", "Entropy", "Bar");
        printf("  %-12s  %-10s  %s\n", "------", "-------", "---");
        for (size_t i = 0; i < display_count; i++) {
            int  bar_len = (int)((entropies[i] / 8.0) * 40);
            char bar[41] = {0}; /* 40 chars + null terminator, zero-initialized */
            for (int j = 0; j < bar_len; j++) bar[j] = '|';
            printf("  0x%-10zx  %-10.4f  %s\n", offsets[i], entropies[i], bar);
        }
        printf("\n");
    }

    free(entropies);
    free(offsets);
}

void verbose_analysis(const unsigned char *data, size_t length) {
    if (data == NULL || length == 0)
        return;

    /* Build frequency table — same approach as compute_entropy */
    size_t freq[256] = {0};
    for (size_t i = 0; i < length; i++)
        freq[data[i]]++;

    /*
     * Sort indices by frequency descending to find top 10.
     * We only need the top 10, so we run only 10 passes of
     * bubble sort rather than sorting the full 256 entries.
     */
    int indices[256];
    for (int i = 0; i < 256; i++)
        indices[i] = i;

    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 256; j++) {
            if (freq[indices[j]] > freq[indices[i]]) {
                int tmp    = indices[i];
                indices[i] = indices[j];
                indices[j] = tmp;
            }
        }
    }

    printf("  Top 10 byte frequencies:\n\n");
    printf("  %-8s  %-6s  %-10s  %s\n", "Byte", "Hex", "Count", "Percentage");
    printf("  %-8s  %-6s  %-10s  %s\n", "----", "---", "-----", "----------");

    for (int i = 0; i < 10; i++) {
        int    idx = indices[i];
        double pct = (double)freq[idx] / length * 100.0;

        /*
         * Assign human-readable labels to common control/whitespace bytes.
         * Printable ASCII (0x20–0x7E) are shown as their actual character.
         * Everything else gets "---".
         */
        char label[8] = {0};
        if      (idx == 0x20) snprintf(label, sizeof(label), "SPACE");
        else if (idx == 0x0A) snprintf(label, sizeof(label), "LF");
        else if (idx == 0x0D) snprintf(label, sizeof(label), "CR");
        else if (idx == 0x00) snprintf(label, sizeof(label), "NULL");
        else if (idx == 0x09) snprintf(label, sizeof(label), "TAB");
        else if (idx >= 0x20 && idx <= 0x7E)
            snprintf(label, sizeof(label), "'%c'", idx);
        else
            snprintf(label, sizeof(label), "---");

        printf("  %-8s  0x%-4x  %-10zu  %.2f%%\n", label, idx, freq[idx], pct);
    }
    printf("\n");
}