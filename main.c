#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entropy.h"

/*
 * Stores per-file analysis results for the final summary.
 * entropy = -1.0 signals a file that failed to load.
 */
typedef struct {
    const char *filename;
    double      entropy;
    const char *label;
    size_t      size;
} FileResult;

/*
 * Reads an entire file into a heap-allocated buffer.
 * Returns a pointer to the buffer and sets *out_size to the byte count.
 * Returns NULL on any error — caller must free the buffer when done.
 *
 * We read in binary mode ("rb") to prevent the runtime from
 * translating newlines or filtering bytes — we need raw bytes.
 */
static unsigned char *read_file(const char *path, size_t *out_size) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open '%s'\n", path);
        return NULL;
    }

    /* Standard C idiom for getting file size without a system call */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size <= 0) {
        fprintf(stderr, "Error: '%s' is empty or unreadable.\n", path);
        fclose(fp);
        return NULL;
    }

    /*
     * Allocate on the heap — file could be large, stack memory is limited.
     * Cast to size_t: ftell returns long (signed), malloc takes size_t (unsigned).
     */
    unsigned char *buffer = malloc((size_t)file_size);
    if (buffer == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for '%s'.\n", path);
        fclose(fp);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, (size_t)file_size, fp);
    fclose(fp); /* Close as soon as we're done reading — don't hold handles open */

    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "Error: Could not fully read '%s'.\n", path);
        free(buffer);
        return NULL;
    }

    *out_size = bytes_read;
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [--verbose] <file1> <file2> ...\n", argv[0]);
        return 1;
    }

    /*
     * Parse arguments — flags and filenames can appear in any order.
     * --verbose triggers byte frequency breakdown and per-window bars.
     * All other arguments are treated as file paths.
     */
    int         verbose    = 0;
    const char *files[64];
    int         file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0)
            verbose = 1;
        else
            files[file_count++] = argv[i];
    }

    if (file_count == 0) {
        fprintf(stderr, "Error: No files specified.\n");
        return 1;
    }

    /* Store results for each file to build the comparison summary */
    FileResult results[64];

    for (int i = 0; i < file_count; i++) {
        size_t         size   = 0;
        unsigned char *buffer = read_file(files[i], &size);

        if (buffer == NULL) {
            /* Mark as failed — skip in summary */
            results[i] = (FileResult){ files[i], -1.0, "Error", 0 };
            continue;
        }

        double      entropy = compute_entropy(buffer, size);
        const char *label   = classify_entropy(entropy);

        results[i] = (FileResult){ files[i], entropy, label, size };

        /* Per-file output block */
        printf("\n----------------------------------------\n");
        printf("File     : %s\n",            files[i]);
        printf("Size     : %zu bytes\n",     size);
        printf("Entropy  : %.4f / 8.0000\n", entropy);
        printf("Class    : %s\n",            label);

        if (verbose) {
            printf("\n");
            verbose_analysis(buffer, size);
        }

        /*
         * Use larger windows for large files to keep output readable.
         * Small files get 256-byte windows for finer granularity.
         */
        size_t window_size = (size > 10000) ? 1024 : 256;
        sliding_window_entropy(buffer, size, window_size, verbose);

        free(buffer); /* Free immediately after processing each file */
    }

    /* Print comparison summary only when multiple files were analyzed */
    if (file_count > 1) {
        printf("\n========== Summary ==========\n\n");
        printf("  %-30s  %-10s  %s\n", "File", "Entropy", "Class");
        printf("  %-30s  %-10s  %s\n", "----", "-------", "-----");

        int    max_i = 0, min_i = 0;
        double max_e = -1.0, min_e = 9.0;

        for (int i = 0; i < file_count; i++) {
            if (results[i].entropy < 0) continue; /* Skip failed files */
            printf("  %-30s  %-10.4f  %s\n",
                results[i].filename,
                results[i].entropy,
                results[i].label);

            if (results[i].entropy > max_e) { max_e = results[i].entropy; max_i = i; }
            if (results[i].entropy < min_e) { min_e = results[i].entropy; min_i = i; }
        }

        printf("\n  Highest entropy : %s (%.4f)\n", results[max_i].filename, max_e);
        printf("  Lowest entropy  : %s (%.4f)\n\n", results[min_i].filename, min_e);
    }

    return 0;
}