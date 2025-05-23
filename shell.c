/*
shell.c - Mini shell multiplataforma (Windows/Unix)
Soporta comandos internos y externos.
*/

// ==================== INCLUDES ====================
#include <stdio.h>  // Funciones de entrada/salida: printf, perror, fgets
#include <stdlib.h> // Gestión de memoria dinámica: malloc, free, exit
#include <string.h> // Manipulación de strings: strtok, strcmp, strcspn

// Inclusión de librerías específicas del sistema operativo
#ifdef _WIN32
#include <process.h> // _spawnvp (creación de procesos en Windows)
#include <direct.h>  // _chdir (cambio de directorio en Windows)
#else
#include <unistd.h>    // chdir, fork, execvp (Unix)
#include <sys/types.h> // Tipos de datos para procesos (Unix)
#include <sys/wait.h>  // waitpid (espera de procesos en Unix)
#endif

// ==================== read_line ====================
/*
Lee una línea de entrada desde el teclado.
- Reserva 1024 bytes de memoria.
- Maneja Ctrl+Z (Windows) o Ctrl+D (Unix) para salir.
- Retorna: Puntero a la cadena leída (debe liberarse con free()).
*/
char *read_line(void)
{
    size_t bufsize = 1024;
    char *line = malloc(bufsize);

    // Verificar asignación de memoria
    if (!line)
    {
        fprintf(stderr, "shell: error de asignación de memoria\n");
        exit(EXIT_FAILURE);
    }

    // Leer entrada con fgets
    if (fgets(line, bufsize, stdin) == NULL)
    {
        if (feof(stdin))
        { // Caso: EOF (usuario termina la entrada)
            printf("\n");
            exit(EXIT_SUCCESS);
        }
        perror("fgets"); // Error de lectura
        exit(EXIT_FAILURE);
    }
    return line;
}

// ==================== split_line ====================
/*
Divide la línea en tokens (palabras) usando delimitadores.
- Delimitadores: espacios, tabs, retornos de carro y saltos de línea.
- Retorna: Array de punteros a tokens terminado en NULL (debe liberarse con free()).
*/
#define TOK_BUFSIZE 64      // Tamaño inicial del array de tokens
#define TOK_DELIM " \t\r\n" // Caracteres delimitadores

char **split_line(char *line)
{
    int bufsize = TOK_BUFSIZE;
    int pos = 0;
    char **tokens = malloc(bufsize * sizeof(char *));

    if (!tokens)
    {
        fprintf(stderr, "shell: error de asignación de memoria\n");
        exit(EXIT_FAILURE);
    }

    // Primer token usando strtok
    char *tok = strtok(line, TOK_DELIM);

    while (tok != NULL)
    {
        tokens[pos++] = tok;

        // Redimensionar array si es necesario
        if (pos >= bufsize)
        {
            bufsize += TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));

            if (!tokens)
            {
                fprintf(stderr, "shell: error de asignación de memoria\n");
                exit(EXIT_FAILURE);
            }
        }

        tok = strtok(NULL, TOK_DELIM); // Siguiente token
    }

    tokens[pos] = NULL; // Marca final del array
    return tokens;
}

// ==================== launch ====================
/*
Ejecuta el comando recibido.
- Maneja comandos internos: exit, echo, cd.
- Ejecuta comandos externos según el sistema operativo.
- Retorna: 1 para continuar ejecución, 0 para terminar.
*/
int launch(char **args)
{
    if (!args[0])
        return 1; // Línea vacía

    // ------ Comando: exit ------
    if (strcmp(args[0], "exit") == 0)
    {
        return 0; // Señal para terminar el shell
    }

    // ------ Comando: echo ------
    if (strcmp(args[0], "echo") == 0)
    {
        for (int i = 1; args[i]; i++)
        {
            printf("%s%s", args[i], args[i + 1] ? " " : "");
        }
        printf("\n");
        return 1;
    }

    // ------ Comando: cd ------
    if (strcmp(args[0], "cd") == 0)
    {
        char *dir = args[1] ? args[1] : getenv("HOME");

#ifdef _WIN32
        if (_chdir(dir) != 0)
        {
#else
        if (chdir(dir) != 0)
        {
#endif
            perror("shell");
        }
        return 1;
    }

// ------ Comandos externos ------
#ifdef _WIN32
    // Windows: Usar cmd.exe con /C para ejecutar comandos
    int arg_count = 0;
    while (args[arg_count])
        arg_count++;

    // Crear array: ["cmd.exe", "/C", comando..., NULL]
    char **cmd_args = malloc((arg_count + 3) * sizeof(char *));
    cmd_args[0] = "cmd.exe";
    cmd_args[1] = "/C";

    for (int i = 0; i <= arg_count; i++)
    {
        cmd_args[i + 2] = args[i];
    }

    // Ejecutar y esperar
    if (_spawnvp(_P_WAIT, "cmd.exe", (const char *const *)cmd_args) == -1)
    {
        perror("shell");
    }
    free(cmd_args);

#else
    // Unix: Crear proceso hijo
    pid_t pid = fork();

    if (pid < 0)
    { // Error en fork
        perror("shell");
    }
    else if (pid == 0)
    { // Proceso hijo
        if (execvp(args[0], args) == -1)
        {
            perror("shell");
            exit(EXIT_FAILURE);
        }
    }
    else
    { // Proceso padre
        int status;
        waitpid(pid, &status, WUNTRACED); // Esperar al hijo
    }
#endif

    return 1; // Continuar ejecución
}

// ==================== main ====================
/*
Función principal del shell.
- Bucle infinito: prompt → leer → dividir → ejecutar → liberar memoria.
*/
int main(void)
{
    char *line;
    char **tokens;
    int status;

    do
    {
        printf("shell> "); // Mostrar prompt
        fflush(stdout);    // Asegurar que se imprime

        line = read_line();        // Leer línea
        tokens = split_line(line); // Dividir en tokens
        status = launch(tokens);   // Ejecutar comando

        // Liberar memoria
        free(tokens);
        free(line);

    } while (status); // Continuar hasta recibir 'exit'

    return EXIT_SUCCESS;
}