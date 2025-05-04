// shell.c
// Un “mini shell” para Windows y Unix que soporta:
//   - Comandos internos: exit, echo, cd
//   - Comandos externos de Windows: mkdir, dir, copy, etc.
//   - Comandos externos de Unix: ls, cat, grep, etc.

#include <stdio.h>  // printf, perror, fgets
#include <stdlib.h> // malloc, free, exit
#include <string.h> // strtok, strcmp, strcspn

#ifdef _WIN32
#include <process.h> // _spawnvp, _P_WAIT
#include <direct.h>  // _chdir
#else
#include <unistd.h> // chdir, fork, execvp
#include <sys/types.h>
#include <sys/wait.h> // waitpid, WIFEXITED, WIFSIGNALED
#endif

/* ==================== read_line ====================
   - Reserva 1024 bytes (máx) para leer del teclado.
   - fgets() lee hasta ‘\n’ o EOF.
   - Si llega EOF (Ctrl+Z/Enter en Windows, Ctrl+D en Unix):
       imprime un salto de línea limpio y sale con éxito.
   - Si hay error de lectura, llama a perror y sale con error.
*/
char *read_line(void)
{
    size_t bufsize = 1024;
    char *line = malloc(bufsize);
    if (!line)
    { // comprobar malloc
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }
    if (fgets(line, bufsize, stdin) == NULL)
    {
        if (feof(stdin))
        { // Ctrl+Z/Enter o Ctrl+D
            printf("\n");
            exit(EXIT_SUCCESS);
        }
        perror("fgets"); // otro fallo
        exit(EXIT_FAILURE);
    }
    return line; // cliente libera con free()
}

/* ==================== split_line ====================
   - Define delimitadores: espacio/tab/\r/\n.
   - Reserva un array inicial de 64 punteros (tokens).
   - strtok() recorre la línea y devuelve cada palabra.
   - Si superamos ese array, hacemos realloc() para crecerlo.
   - Terminamos el array poniendo tokens[pos] = NULL.
*/
#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n"

char **split_line(char *line)
{
    int bufsize = TOK_BUFSIZE, pos = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *tok;
    if (!tokens)
    {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }
    tok = strtok(line, TOK_DELIM);
    while (tok)
    {
        tokens[pos++] = tok; // guardar puntero al token
        if (pos >= bufsize)
        {                           // si llenamos el array
            bufsize += TOK_BUFSIZE; // ampliarlo
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        tok = strtok(NULL, TOK_DELIM); // siguiente token
    }
    tokens[pos] = NULL; // marca final de lista
    return tokens;      // cliente libera con free()
}

/* ==================== launch ====================
   Ejecuta un comando dado:
   1. Si es “exit” → devolver 0 (señal para salir).
   2. Si es “echo” → imprimir manualmente tokens[1..].
   3. Si es “cd”   → cambiar directorio con _chdir()/chdir().
   4. Si no, ejecutar externamente:
        - En Windows: construir ["cmd.exe","/C",args...,NULL]
          y llamar a _spawnvp(_P_WAIT,...).
        - En Unix: fork()/execvp()/waitpid().
   Devuelve 1 para continuar el bucle en main().
*/
int launch(char **args)
{
    if (!args[0])
        return 1; // línea vacía

    // --- 1) exit interno ---
    if (strcmp(args[0], "exit") == 0)
    {
        return 0;
    }
    // --- 2) echo interno ---
    if (strcmp(args[0], "echo") == 0)
    {
        for (int i = 1; args[i]; i++)
        {
            printf("%s", args[i]);
            if (args[i + 1])
                printf(" ");
        }
        printf("\n");
        return 1;
    }
    // --- 3) cd interno ---
    if (strcmp(args[0], "cd") == 0)
    {
#ifdef _WIN32
        if (_chdir(args[1] ? args[1] : "") != 0)
            perror("shell");
#else
        if (chdir(args[1] ? args[1] : "") != 0)
            perror("shell");
#endif
        return 1;
    }

    // --- 4) comando externo ---
#ifdef _WIN32
    // 4.a) contar tokens
    int count = 0;
    while (args[count])
        count++;
    // 4.b) preparar array ["cmd.exe","/C", arg0, arg1, ..., NULL]
    char **cmd_args = malloc((count + 3) * sizeof(char *));
    if (!cmd_args)
    {
        fprintf(stderr, "shell: allocation error\n");
        return 1;
    }
    cmd_args[0] = "cmd.exe";
    cmd_args[1] = "/C";
    for (int i = 0; i <= count; i++)
    {
        cmd_args[i + 2] = args[i]; // incluye el NULL final
    }
    // 4.c) lanzar y esperar
    if (_spawnvp(_P_WAIT, "cmd.exe", (const char *const *)cmd_args) == -1)
    {
        perror("shell");
    }
    free(cmd_args);
#else
    // 4.a) fork
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("shell");
    }
    else if (pid == 0)
    {
        // 4.b) hijo → execvp
        if (execvp(args[0], args) == -1)
            perror("shell");
        exit(EXIT_FAILURE);
    }
    else
    {
        // 4.c) padre → esperar hijo
        int status;
        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
#endif

    return 1; // seguimos en el bucle
}

/* ==================== main ====================
   1. Bucle do-while que llama a:
      - read_line()
      - split_line()
      - launch()
   2. Libera memoria (tokens y línea).
   3. Repite mientras launch devuelva 1.
*/
int main(void)
{
    char *line;
    char **tokens;
    int status;

    do
    {
        printf("shell> "); // prompt
        fflush(stdout);    // asegurar que se muestre correctamente

        line = read_line();
        tokens = split_line(line);
        status = launch(tokens);

        free(tokens); // liberar array de punteros
        free(line);   // liberar buffer de lectura
    } while (status);

    return EXIT_SUCCESS;
}
