#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_THREADS 16
#define LINE_SIZE 1024

typedef struct {
    char **lines;
    size_t start;
    size_t end;
    long info;
    long warning;
    long error;
    long other;
} WorkerData;

static void *count_lines(void *argument) {
    WorkerData *data = argument;
    for (size_t i = data->start; i < data->end; i++) {
        const char *line = data->lines[i];
        if (strstr(line, "INFO")) data->info++;
        else if (strstr(line, "WARNING")) data->warning++;
        else if (strstr(line, "ERROR")) data->error++;
        else data->other++;
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <log-file> <threads 1-16>\n", argv[0]);
        return 1;
    }

    char *endptr = NULL;
    long requested = strtol(argv[2], &endptr, 10);
    if (*argv[2] == '\0' || *endptr != '\0' ||
        requested < 1 || requested > MAX_THREADS) {
        fprintf(stderr, "Thread count must be between 1 and %d.\n", MAX_THREADS);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Could not open log file");
        return 1;
    }

    char **lines = NULL;
    size_t count = 0, capacity = 0;
    char buffer[LINE_SIZE];

    while (fgets(buffer, sizeof buffer, file)) {
        if (count == capacity) {
            size_t new_capacity = capacity == 0 ? 64 : capacity * 2;
            char **grown = realloc(lines, new_capacity * sizeof *lines);
            if (!grown) {
                fprintf(stderr, "Out of memory.\n");
                fclose(file);
                for (size_t i = 0; i < count; i++) free(lines[i]);
                free(lines);
                return 1;
            }
            lines = grown;
            capacity = new_capacity;
        }
        lines[count] = malloc(strlen(buffer) + 1);
        if (!lines[count]) {
            fprintf(stderr, "Out of memory.\n");
            fclose(file);
            for (size_t i = 0; i < count; i++) free(lines[i]);
            free(lines);
            return 1;
        }
        strcpy(lines[count], buffer);
        count++;
    }
    fclose(file);

    size_t thread_count = (size_t)requested;
    pthread_t threads[MAX_THREADS];
    WorkerData workers[MAX_THREADS] = {0};
    size_t chunk = (count + thread_count - 1) / thread_count;
    size_t created = 0;

    for (size_t i = 0; i < thread_count; i++) {
        workers[i].lines = lines;
        workers[i].start = i * chunk;
        workers[i].end = workers[i].start + chunk;
        if (workers[i].end > count) workers[i].end = count;
        if (workers[i].start > count) workers[i].start = count;

        if (pthread_create(&threads[i], NULL, count_lines, &workers[i]) != 0) {
            fprintf(stderr, "Could not create worker thread.\n");
            break;
        }
        created++;
    }

    for (size_t i = 0; i < created; i++) pthread_join(threads[i], NULL);

    long info = 0, warning = 0, error = 0, other = 0;
    for (size_t i = 0; i < created; i++) {
        info += workers[i].info;
        warning += workers[i].warning;
        error += workers[i].error;
        other += workers[i].other;
    }

    printf("Lines read: %zu\nINFO: %ld\nWARNING: %ld\nERROR: %ld\nOTHER: %ld\n",
           count, info, warning, error, other);

    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
    return created == thread_count ? 0 : 1;
}
