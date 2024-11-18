#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
        return 1;
    }

    struct timeval start_time, end_time;

    // Obtener el tiempo inicial
    if (gettimeofday(&start_time, NULL) != 0) {
        perror("Error al obtener el tiempo inicial");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Error al crear el proceso hijo");
        return 1;
    } else if (pid == 0) {
        // Proceso hijo: Ejecutar el comando
        execvp(argv[1], &argv[1]);
        // Si exec falla
        perror("Error al ejecutar el comando");
        exit(1);
    } else {
        // Proceso padre: Esperar al hijo
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("Error al esperar al proceso hijo");
            return 1;
        }

        // Obtener el tiempo final
        if (gettimeofday(&end_time, NULL) != 0) {
            perror("Error al obtener el tiempo final");
            return 1;
        }

        // Calcular el tiempo transcurrido
        double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                              (end_time.tv_usec - start_time.tv_usec) / 1e6;

        printf("Tiempo transcurrido: %.6f segundos\n", elapsed_time);

        // Verificar el estado del proceso hijo
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            fprintf(stderr, "El proceso hijo terminó de manera anormal.\n");
            return 1;
        }
    }
}