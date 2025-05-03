// shell.c
#include <stdio.h>  // printf, perror, fgets
#include <stdlib.h> // malloc, realloc, free, exit
#include <string.h> // strtok, strcmp

/* ==================== Función read_line ====================
   Lee una línea completa desde stdin usando fgets().
   - Reserva un buffer de tamaño fijo (puede ampliarse si querés).
   - Detecta EOF (Ctrl+Z + Enter en Windows).
   - En caso de EOF, sale limpiamente con EXIT_SUCCESS.
   - En caso de error, muestra perror() y sale con EXIT_FAILURE.
*/
char *read_line(void)
{
    size_t bufsize = 1024; // tamaño máximo permitido para una línea
    char *line = malloc(bufsize);

    if (!line)
    {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    // fgets lee una línea desde stdin (incluye el '\n')
    if (fgets(line, bufsize, stdin) == NULL)
    {
        if (feof(stdin))
        {
            printf("\n"); // línea vacía con Ctrl+Z
            exit(EXIT_SUCCESS);
        }
        else
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
    }

    return line;
}

/* ==================== Función split_line ====================
   Divide la línea en tokens usando delimitadores (espacio, tab, retorno, newline).
   - Devuelve un array de punteros a cadenas terminado en NULL.
   - Realloca dinámicamente si el número de tokens supera el buffer inicial.
*/
#define TOK_BUFSIZE 64      // tamaño inicial del array de tokens
#define TOK_DELIM " \t\r\n" // delimitadores

char **split_line(char *line)
{
    int bufsize = TOK_BUFSIZE;
    int position = 0;
    // Reservamos espacio para bufsize punteros a char*
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Primera llamada a strtok: pasa la línea y los delimitadores
    token = strtok(line, TOK_DELIM);
    while (token != NULL)
    {
        tokens[position++] = token;

        // Si nos pasamos del buffer, lo expandimos
        if (position >= bufsize)
        {
            bufsize += TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        // Llamadas posteriores a strtok: pasa NULL
        token = strtok(NULL, TOK_DELIM);
    }

    tokens[position] = NULL; // NULL para marcar el final del array
    return tokens;
}

/* ==================== main ====================
   Bucle principal de la shell:
   1. Muestra prompt "shell> ".
   2. Llama a read_line() para capturar la entrada del usuario.
   3. Llama a split_line() para tokenizar la línea.
   4. Imprime cada token como prueba.
   5. Libera la memoria y repite.
*/
int main(void)
{
    char *line;
    char **tokens;

    while (1)
    {
        // 1. Prompt
        printf("shell> ");
        fflush(stdout); // asegura que el prompt se muestre

        // 2. Leer línea
        line = read_line();

        // 3. Tokenizar
        tokens = split_line(line);

        // 4. Mostrar tokens (debug)
        for (int i = 0; tokens[i] != NULL; i++)
        {
            printf("  token[%d] = '%s'\n", i, tokens[i]);
        }

        // 5. Limpiar memoria
        free(tokens);
        free(line);
    }

    return EXIT_SUCCESS;
}
