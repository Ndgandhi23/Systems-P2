#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

typedef struct WordNode {
    char *word;
    int count;
    double frequency;
    struct WordNode *next;
} WordNode;

typedef struct {
    WordNode *head;
    int total_words;
} WFD;

typedef struct {
    char **paths;
    int count;
    int size;
} FileArr;

typedef struct {
    char *file1;
    char *file2;
    int combined_word_count;
    double jsd;
} Comparison;

int has_suffix(const char *full_str, const char *suffix){
    int string_len = strlen(full_str);
    int suffix_len = strlen(suffix);

    if(suffix_len > string_len){
        return 0;
    }
    return strcmp(full_str + (string_len - suffix_len), suffix) == 0;
}

void search_directory(const char* path, FileArr *files, const char* suffix){
    DIR *dp = opendir(path);
    if(dp == NULL){
        perror("opendir");
        return;
    }
    struct dirent *entry;
    char full[4096];

    while ((entry = readdir(dp)) != NULL){
        if (entry->d_name[0] == '.'){
            continue;
        }
        snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);

        struct stat st;
        //switched to stat for robustness instead of using d_type
        if(stat(full, &st) == -1){
            perror(full);
            continue;
        }
        if(S_ISDIR(st.st_mode)){
            search_directory(full, files, suffix);
        } else if(S_ISREG(st.st_mode)){
            if(has_suffix(entry->d_name,suffix)){
                if((files->count) == files->size){
                    char** temp = realloc(files->paths, (files->size) * 2 * (sizeof(char *)));
                    if (temp == NULL){
                        free(files->paths);
                        exit(1);
                    }

                    files->paths = temp;
                    files->size = files->size * 2;
                }
                files->paths[files->count] = strdup(full);
                files->count = files->count+1;
            }
        }
    }

    closedir(dp);
}

void find_file(const char *path, FileArr *files, const char* suffix){
    struct stat st;
    if(stat(path, &st) == -1){
        perror("stat");
        return;
    }

    if(S_ISREG(st.st_mode)){
        // if(!has_suffix(path, suffix)){
        //     return;
        // }
        if((files->count) == files->size){
            char** temp = realloc(files->paths, (files->size) * 2 * (sizeof(char *)));
            if (temp == NULL){
                free(files->paths);
                exit(1);
            }

            files->paths = temp;
            files->size = files->size * 2;
        }
        files->paths[files->count] = strdup(path);
        files->count = files->count+1;
        
    }
    else if (S_ISDIR(st.st_mode)){
    //    printf("LOG: in find_file and have found a dir\n");
        search_directory(path, files, suffix);
    }

}

void insert_wordNode(WFD *dist, char* word){
    WordNode *current = dist->head;
    WordNode *previous = NULL;
    WordNode *node = malloc(sizeof(WordNode));
    if(node == NULL){
        perror("Out of memory");
        return;
    }

    while(current){
        int cmp = strcmp(current->word, word);
        if(cmp == 0){
            current->count++;
            dist->total_words++;
            free(node);
            return;
        }
        else if(cmp > 0){
            node->count = 1;
            node->word = strdup(word);
            if(node->word == NULL){
                perror("strdup");
                free(node);
                return;
            }
            node->next = current;
            dist->total_words++;
            if(previous == NULL){
                dist->head = node;
                return;
            }
            else{
                previous->next = node;
                return;
            }
        }
        previous = current;
        current = current->next;
    }

    node->count = 1;
    node->word = strdup(word);
    if(node->word == NULL){
        perror("strdup");
        free(node);
        return;
    }
    node->next = NULL;
    if(previous == NULL){
        dist->head = node;
    }
    else{
        previous->next = node;
    } 
    dist->total_words++;
}

void compute_frequency(WFD *dist){
    int total_words = dist->total_words;
    if(total_words == 0){
        return;
    } 
    WordNode *current = dist->head;
    while(current){
        current->frequency = (double)current->count / total_words;
        current = current->next;
    }
}

void tokenize_file(WFD *dist, char* path){
    int fd = open(path, O_RDONLY);
    if(fd == -1){
        perror(path);
        return;
    }

    int n;
    char c;
    int word_index = 0;
    int word_cap = 16;
    char *word = malloc(word_cap);
    if(word == NULL){
        perror("malloc");
        close(fd);
        return;
    }

    while((n = read(fd, &c, 1)) > 0){
        if(isspace(c)){
            word[word_index] = '\0';
            if(word_index > 0){
                insert_wordNode(dist, word);
            }
            word_index = 0;
        }
        else if(isalnum(c)){
            if(word_index >= word_cap - 1){
                char *temp = realloc(word, word_cap * 2);
                if(temp == NULL){
                    perror("realloc");
                    free(word);
                    close(fd);
                    return;
                }
                word = temp;
                word_cap *= 2;
            }
            word[word_index] = tolower(c);
            word_index++;
        }
        else{
            continue;
        }
    }

    // handle last word if file doesn't end with space/newline
    if(word_index > 0){
        word[word_index] = '\0';
        insert_wordNode(dist, word);
    }

    free(word);
    close(fd);
}

double computeJSD(WFD *file_a, WFD *file_b){
    double kld_a = 0;
    double kld_b = 0;
    WordNode *current_a = file_a->head;
    WordNode *current_b = file_b->head;
    while(current_a || current_b){
        if(current_a && current_b){
            int cmp = strcmp(current_a->word, current_b->word);
            if(cmp == 0){
                double mean = (current_a->frequency + current_b->frequency ) / 2;
                kld_a += current_a->frequency * log2(current_a->frequency / mean);
                kld_b += current_b->frequency * log2(current_b->frequency / mean);
                current_a = current_a->next;
                current_b = current_b->next;
            }
            else if(cmp < 0){
                double mean = current_a->frequency / 2;
                kld_a += current_a->frequency * log2(current_a->frequency / mean);
                current_a = current_a->next;
            }
            else{
                double mean = current_b->frequency / 2;
                kld_b += current_b->frequency * log2(current_b->frequency / mean);
                current_b = current_b->next;
            }
        }
        else if(current_a && !current_b){
            double mean = current_a->frequency / 2;
            kld_a += current_a->frequency * log2(current_a->frequency / mean);
            current_a = current_a->next;
        }
        else{
            double mean = current_b->frequency / 2;
            kld_b += current_b->frequency * log2(current_b->frequency / mean);
            current_b = current_b->next;
        }
    }

    return sqrt(0.5 * kld_a + 0.5 * kld_b);
}

//needed for qsort so it knows how to compare the elements
int compare_fn(const void *a, const void *b){
    Comparison *ca = (Comparison *)a;
    Comparison *cb = (Comparison *)b;
    if(cb->combined_word_count > ca->combined_word_count){
        return 1;
    } 
    if(cb->combined_word_count < ca->combined_word_count){
        return -1;
    } 
    return 0;
}

void print_all_wfds(WFD *wfds, char **filenames, int file_count) {
    for (int i = 0; i < file_count; i++) {
        printf("\nFILE: %s\n", filenames[i]);
        printf("----------------------\n");

        WordNode *current = wfds[i].head;

        while (current != NULL) {
            printf("word: %-15s count: %-5d freq: %.5f\n",
                   current->word,
                   current->count,
                   current->frequency);

            current = current->next;
        }
    }
}

void analysis_phase(FileArr *files){
    int file_amount = files->count;
    if(file_amount < 2){
        //corrected from perror because not system file and was unreliable
        fprintf(stderr, "error: Not enough files found for analysis phase\n");
        exit(1);
    }

    WFD *wfds = malloc(file_amount * sizeof(WFD));
    if(wfds == NULL){
        perror("malloc");
        exit(1);
    }
    for(int i = 0; i < file_amount; i++){
        wfds[i].head = NULL;
        wfds[i].total_words = 0;
        tokenize_file(&wfds[i], files->paths[i]);
        if(wfds[i].total_words > 0){
            compute_frequency(&wfds[i]);
        }
    }

    // LOG: testing word frequency calcs
    print_all_wfds(wfds, files->paths, file_amount);

    int comparisons_length = file_amount * (file_amount - 1) / 2;
    Comparison *comparisons = malloc(comparisons_length * sizeof(Comparison));
    if(comparisons == NULL){
        perror("malloc");
        free(wfds);
        exit(1);
    }

    int comparison_index = 0;

    for(int i = 0; i < file_amount; i++){
        for(int j = i+1; j < file_amount; j++){
            comparisons[comparison_index].file1 = files->paths[i];
            comparisons[comparison_index].file2 = files->paths[j];
            comparisons[comparison_index].combined_word_count = wfds[i].total_words + wfds[j].total_words;
            comparisons[comparison_index].jsd = computeJSD(&wfds[i], &wfds[j]);
            comparison_index++;
        }
    }

    qsort(comparisons, comparisons_length, sizeof(Comparison), compare_fn);

    for(int i = 0; i < comparisons_length; i++){
        //formatted it to 5 decimal places because the pdf example does it but not really needed
        printf("%.5f %s %s\n", comparisons[i].jsd, comparisons[i].file1, comparisons[i].file2);
    }

    //free everything here
    for(int i = 0; i < files->count; i++){
        WordNode *current = wfds[i].head;
        while(current){
            WordNode *next = current->next;
            free(current->word);
            free(current);
            current = next;
        }
    }
    free(wfds);
    free(comparisons);
    for(int i = 0; i < files->count; i++){
        free(files->paths[i]);
    }
    free(files->paths);
}




int main(int argc, char *argv[]){
    // if(argc < 3){
    //     //also changed from perror
    //     fprintf(stderr, "error: not enough files\n");
    //     exit(1);
    // }

    FileArr files;
    files.paths = malloc(8 * sizeof(char *));
    if(files.paths == NULL){
        perror("malloc");
        exit(1);
    }
    files.count = 0;
    files.size = 8;
    //ask teacher about suffix because i am not sure how it is given
    char *suffix = ".txt";

    for(int i = 1; i < argc; i++){
        find_file(argv[i], &files, suffix);
    }

    analysis_phase(&files);
    return 0;
}