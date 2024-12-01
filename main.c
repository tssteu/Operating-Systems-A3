#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>

#define NUM_ITERATIONS 100

void* thread_function(void* arg) {
    int* pipe_fd = (int*)arg;
    char buffer = 'x';

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        if (read(pipe_fd[0], &buffer, 1) <= 0) {
            perror("Thread: read fehlgeschlagen");
            break;
        }
        printf("Thread: Nachricht %d empfangen\n", i);
        if (write(pipe_fd[1], &buffer, 1) <= 0) {
            perror("Thread: write fehlgeschlagen");
            break;
        }
        printf("Thread: Nachricht %d gesendet\n", i);
    }

    printf("Thread beendet\n");
    return NULL;
}

uint64_t time_diff_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
}

int main() {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Pipe fehlgeschlagen");
        exit(EXIT_FAILURE);
    }

    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_function, (void*)pipe_fd) != 0) {
        perror("Thread konnte nicht erstellt werden");
        exit(EXIT_FAILURE);
    }

    char buffer = 'x';
    struct timespec start, end;
    uint64_t total_time = 0;

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        if (write(pipe_fd[1], &buffer, 1) <= 0) {
            perror("Main: write fehlgeschlagen");
            break;
        }
        printf("Main: Nachricht %d gesendet\n", i);
        if (read(pipe_fd[0], &buffer, 1) <= 0) {
            perror("Main: read fehlgeschlagen");
            break;
        }
        printf("Main: Nachricht %d empfangen\n", i);
        clock_gettime(CLOCK_MONOTONIC, &end);

        total_time += time_diff_ns(start, end);
    }

    printf("Warte auf Thread mit pthread_join\n");
    pthread_join(thread, NULL);
    printf("Thread erfolgreich beendet\n");

    close(pipe_fd[0]);
    close(pipe_fd[1]);

    double avg_context_switch_time = (double)total_time / (NUM_ITERATIONS * 2);
    printf("Durchschnittliche Kontextwechselzeit: %.2f ns\n", avg_context_switch_time);

    return 0;
}