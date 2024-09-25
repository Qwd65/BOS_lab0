#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_OPEN_FILES 256

typedef struct {
    int fd;            // файловый дескриптор
    char *filename;    // имя файла
} FileEntry;

FileEntry open_files[MAX_OPEN_FILES];
int open_file_count = 0;

// Функция для вывода ошибки
void print_error(int index) {
    fprintf(stderr, "Ошибка аргумента %d: %d (%s)\n", index, errno, strerror(errno)); // errno - туда пишется код ошибки, strerror - возвращает описание ошибки
}


// Функция для закрытия файлов
void handle_close(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Ошибка аргумента 1: %d (%s)\n", EINVAL, "Недостаточно аргументов"); //код ошибки "Неправильный аргумент"
        return;
    }

    int fd = atoi(argv[1]);// в int
    for (int i = 0; i < open_file_count; ++i) {
        if (open_files[i].fd == fd) {
            close(fd);
            printf("Файл %s (fd %d) закрыт.\n", open_files[i].filename, fd);
            free(open_files[i].filename);
            // Удаляем закрытый файл из списка
            open_files[i] = open_files[open_file_count - 1];//что бы избежать сдвига
            open_file_count--;
            return;
        }
    }
    fprintf(stderr, "Ошибка: файл с fd %d не найден.\n", fd);
}


// Функция записи в файл
void handle_write(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Ошибка аргументов: %d \n", EINVAL);
        return;
    }

    int fd = atoi(argv[1]);
    const char *data = argv[2];
    off_t offset = (argc == 4) ? atoi(argv[3]) : -1; // off_t - используется для представления смещений в файлах.

    if (offset != -1 && lseek(fd, offset, SEEK_SET) == -1) { //lseek перемещает указатель чтения, SEEK_SET указывает, что перемещение должно происходить относительно начала файла.Если lseek возвращает -1, это значит, что произошла ошибка (например, offset за пределами размера файла)
        print_error(1);
        return;
    }

    ssize_t bytes_written = write(fd, data, strlen(data)); //write возвращает количество записанных байт. ssize_t —  используется для представления количества байт 
    if (bytes_written == -1) {
        print_error(1);
    } else {
        printf("%zd байт записано в файловый дескриптор %d.\n", bytes_written, fd);
    }
}

// Функция чтения из файла
void handle_read(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Ошибка аргументов: %d\n", EINVAL);
        return;
    }

    int fd = atoi(argv[1]);
    size_t count = atoi(argv[2]);
    off_t offset = (argc == 4) ? atoi(argv[3]) : -1;

    if (offset != -1 && lseek(fd, offset, SEEK_SET) == -1) {
        print_error(1);
        return;
    }

    char *buffer = malloc(count);
    if (!buffer) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        return;
    }

    ssize_t bytes_read = read(fd, buffer, count);
    if (bytes_read == -1) {
        print_error(1);
        free(buffer);
    } else {
        printf("Прочитано %zd байт: %.*s\n", bytes_read, (int)bytes_read, buffer);
    }

    free(buffer);
}

// Функция создания файлов
void handle_create(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Ошибка аргумента 1: %d (%s)\n", EINVAL, "Недостаточно аргументов");
        return;
    }

    int num_files = atoi(argv[1]);
    if (argc != num_files + 2) {
        fprintf(stderr, "Ошибка аргумента 2: %d (%s)\n", EINVAL, "Несоответствие количества файлов");
        return;
    }

    for (int i = 0; i < num_files; ++i) {
        int fd = open(argv[i + 2], O_RDWR | O_CREAT, 0666);//флаги указывают, что файл будет открыт для чтения и записи и что если файл не существует, он должен быть создан
        if (fd == -1) {
            print_error(i + 1);
            continue;
        }
        printf("Файл %s создан.\n", argv[i + 2]);
        close(fd);
    }
}

// Функция копирования файлов
void handle_copy(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Ошибка аргумента 1: %d (%s)\n", EINVAL, "Недостаточно аргументов");
        return;
    }

    FILE *src = fopen(argv[1], "rb"); //"rb" — это режим открытия файла: r — открыть файл для чтения.b — открыть файл в двоичном режиме (для обработки двоичных файлов)
    if (!src) {
        print_error(1);
        return;
    }

    FILE *dst = fopen(argv[2], "wb");
    if (!dst) {
        print_error(2);
        fclose(src);
        return;
    }

    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) { //2 аргумент -  размер одного элемента, 3 - кол-во элементов
        fwrite(buffer, 1, bytes, dst);
    }

    fclose(src);
    fclose(dst);
    printf("Файл %s скопирован в %s.\n", argv[1], argv[2]);
}

// Функция перемещения файлов
void handle_move(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Ошибка аргумента 1: %d (%s)\n", EINVAL, "Недостаточно аргументов");
        return;
    }

    if (rename(argv[1], argv[2]) == -1) {
        print_error(1);
        return;
    }

    printf("Файл %s перемещен в %s.\n", argv[1], argv[2]);
}

// Функция создания каталога
void handle_cd(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Ошибка аргумента 1: %d (%s)\n", EINVAL, "Недостаточно аргументов");
        return;
    }

    if (mkdir(argv[1], 0755) == -1) {
        print_error(1);
        return;
    }

    printf("Каталог %s создан.\n", argv[1]);
}

// Функция для открытия файлов
void handle_open(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Аргумент 1 ошибка: недостаточно аргументов.\n");
        return;
    }

    int num_files = atoi(argv[1]);
    if (argc != num_files + 2) {
        fprintf(stderr, "Аргумент 2 ошибка: количество файлов не совпадает с аргументами.\n");
        return;
    }

    for (int i = 0; i < num_files; ++i) {
        if (open_file_count >= MAX_OPEN_FILES) {
            fprintf(stderr, "Ошибка: слишком много открытых файлов.\n");
            return;
        }

        int fd = open(argv[i + 2], O_RDWR);
        if (fd == -1) {
            print_error(i + 1);
            continue;
        }

        open_files[open_file_count].fd = fd;
        open_files[open_file_count].filename = strdup(argv[i + 2]);
        printf("Файл %s открыт с fd %d.\n", argv[i + 2], fd);
        open_file_count++;
    }
}

// Функция для вывода списка открытых файлов
void handle_list() {
    if (open_file_count == 0) {
        printf("Нет открытых файлов.\n");
        return;
    }

    printf("Список открытых файлов:\n");
    for (int i = 0; i < open_file_count; ++i) {
        printf("fd: %d, имя файла: %s\n", open_files[i].fd, open_files[i].filename);
    }
}

// Функция для вывода содержимого каталога с размерами файлов
void handle_ls(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Аргумент 1 ошибка: недостаточно аргументов.\n");
        return;
    }

    DIR *dir = opendir(argv[1]);
    if (!dir) {
        print_error(1);
        return;
    }

    struct dirent *entry; // структура используется для представления записи в каталоге
    struct stat file_stat;// структура используется для хранения информации о файле
    char path[512];

    printf("Содержимое каталога %s:\n", argv[1]);
    while ((entry = readdir(dir)) != NULL) {
        snprintf(path, sizeof(path), "%s/%s", argv[1], entry->d_name);// формирует полный путь к текущему файлу
        if (stat(path, &file_stat) == 0) {// Функция stat заполняет структуру file_stat информацией о файле по указанному пути
            if (S_ISDIR(file_stat.st_mode)) { //проверяет, является ли файл каталогом, основываясь на его st_mode
                printf("%s (dir)\n", entry->d_name);
            } else {
                printf("%s (%lld bytes)\n", entry->d_name, (long long)file_stat.st_size);
            }
        } else {
            print_error(1);
        }
    }

    closedir(dir);
}

// Функция удаления файлов или каталогов
void handle_rm(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Ошибка аргумента 1: %d (%s)\n", EINVAL, "Недостаточно аргументов");
        return;
    }

    if (remove(argv[1]) == -1) {
        print_error(1);
        return;
    }

    printf("Файл или каталог %s удален.\n", argv[1]);
}

// Функция помощи (help)
void handle_help() {
    printf("Доступные команды:\n");
    printf("create <количество файлов> <файл1> <файл2> ... - Создать файлы\n");
    printf("copy <исходный файл> <файл назначения>        - Копировать файл\n");
    printf("move <исходный файл> <файл назначения>        - Переместить файл\n");
    printf("cd <имя каталога>                             - Создать каталог\n");
    printf("ls <имя каталога>                             - Вывести содержимое каталога\n");
    printf("rm <имя файла или каталога>                   - Удалить файл или каталог\n");
    printf("write <fd> <строка> [смещение]                - Записать строку в файл\n");
    printf("read <fd> <количество байтов> [смещение]      - Прочитать файл\n");
    printf("open <количество файлов> <файл1> <файл2> ...  - Открыть файлы\n");
    printf("close <fd>                                    - Закрыть файл\n");
    printf("list                                          - Показать список открытых файлов\n");
    printf("exit                                          - Выйти из программы\n");
}


int main() {
    char command[256];

    while (1) {
        printf("Введите команду: ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            perror("fgets"); //выводит сообщение об ошибке в stderr
            exit(EXIT_FAILURE);// EXIT_FAILURE - 1(выход с ошибкой)
        }

        size_t len = strlen(command); // беззнаковый целый тип
        if (len > 0 && command[len - 1] == '\n') {
            command[len - 1] = '\0';
        }

        char *tokens[6];
        int token_count = 0;
        char *token = strtok(command, " ");//разделяем аргументы
        while (token != NULL && token_count < 6) {
            tokens[token_count++] = token;
            token = strtok(NULL, " ");// продолжаем разбиение
        }
        tokens[token_count] = NULL;//последний элемент в NULL

        if (token_count > 0) {
            if (strcmp(tokens[0], "create") == 0) {
                handle_create(token_count, tokens);
            } else if (strcmp(tokens[0], "copy") == 0) {
                handle_copy(token_count, tokens);
            } else if (strcmp(tokens[0], "move") == 0) {
                handle_move(token_count, tokens);
            } else if (strcmp(tokens[0], "cd") == 0) {
                handle_cd(token_count, tokens);
            } else if (strcmp(tokens[0], "ls") == 0) {
                handle_ls(token_count, tokens);
            } else if (strcmp(tokens[0], "rm") == 0) {
                handle_rm(token_count, tokens);
            } else if (strcmp(tokens[0], "help") == 0) {
                handle_help();
            } else if (strcmp(tokens[0], "write") == 0) {
                handle_write(token_count, tokens);
            } else if (strcmp(tokens[0], "read") == 0) {
                handle_read(token_count, tokens);
            } else if (strcmp(tokens[0], "open") == 0) {
                handle_open(token_count, tokens);
            } else if (strcmp(tokens[0], "close") == 0) {
                handle_close(token_count, tokens);
            } else if (strcmp(tokens[0], "list") == 0) {
                handle_list();
            } else if (strcmp(tokens[0], "exit") == 0) {
                for (int i = 0; i < open_file_count; ++i) {
                    close(open_files[i].fd);
                    free(open_files[i].filename);
                }
                printf("Выход из программы...\n");
                exit(EXIT_SUCCESS);// 0
            } else {
                fprintf(stderr, "Неизвестная команда: %s\n", tokens[0]);
            }
        }
    }

    return 0;
}
