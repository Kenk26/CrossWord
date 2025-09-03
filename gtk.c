#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>

#define GRID_SIZE 33
#define numWords 5
#define MAX_WORD_LENGTH 20
#define MAX_HINT_LENGTH 100
#define MAX_NAME_LENGTH 50
#define MAX_USERS 101
#define ALPHABET_SIZE 26
#define MAX_WORDS 200
#define WORDS_FILE "crossword_words.txt"
#define ADMIN_USERNAME "admin"
#define ADMIN_PASSWORD "admin"



// Forward declarations
struct TrieNode;
struct User;

// Trie node structure
typedef struct TrieNode {
    struct TrieNode* children[ALPHABET_SIZE];
    bool isEndOfWord;
    char hint[MAX_HINT_LENGTH];
    int index;
    bool used;
} TrieNode;

// User structure
typedef struct User {
    char name[MAX_USERS][MAX_NAME_LENGTH];
    float time_u[MAX_USERS];
    int points[MAX_USERS];
    int size_u;
} User;

// Word database structure
typedef struct WordDatabase {
    char words[MAX_WORDS][MAX_WORD_LENGTH];
    char hints[MAX_WORDS][MAX_HINT_LENGTH];
    int count;
} WordDatabase;

// Global instances
WordDatabase db;
User user_scores;

// Function declarations for callbacks
void on_start_clicked(GtkWidget *widget, gpointer data);
void on_student_clicked(GtkWidget *widget, gpointer data);
void on_teacher_clicked(GtkWidget *widget, gpointer data);
void show_teacher_menu(void);
void on_add_word_clicked(GtkWidget *widget, gpointer data);
void on_play_game_clicked(GtkWidget *widget, gpointer data);
void on_view_scores_clicked(GtkWidget *widget, gpointer data);
void show_main_menu(void);

// Global variables
GtkWidget *main_window;
GtkWidget *current_container;
int HintCounter = 0;

void EmptyHints(char hints[][MAX_HINT_LENGTH], int size, int index[]);
void EmptyGrid(char grid[][GRID_SIZE], int size);
void CopyGrid(char source[][GRID_SIZE], char dest[][GRID_SIZE]);
void generateCrossword(char grid[][GRID_SIZE], WordDatabase* db, TrieNode* mainTrie, TrieNode* gameTrie, char hints2[][MAX_HINT_LENGTH]);
void printGrid(char grid[][GRID_SIZE]);
void HintsDisplayer(char hints[][MAX_HINT_LENGTH], int index[]);
bool CompareGrids(char grid1[][GRID_SIZE], char grid2[][GRID_SIZE]);
void updateDisplayGrid(char source[][GRID_SIZE], char display[][GRID_SIZE], const char* word);
void heapify_min(User* u, int i, int n);
void user_min_sort(User* u, int n);

static TrieNode* gameTrie = NULL;
static char game_grid[GRID_SIZE][GRID_SIZE];
static char display_grid[GRID_SIZE][GRID_SIZE];
static char game_hints[numWords][MAX_HINT_LENGTH];
static int hint_indices[numWords];
static clock_t game_start_time;
static char current_player_name[MAX_NAME_LENGTH];
static GtkWidget *game_window = NULL;
static GtkWidget *grid_display = NULL;
static GtkWidget *hints_display = NULL;
static GtkWidget *word_entry = NULL;

// Trie functions (keeping original implementations)
TrieNode* createNode() {
    TrieNode* node = (TrieNode*)malloc(sizeof(TrieNode));
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        node->children[i] = NULL;
    }
    node->isEndOfWord = false;
    node->hint[0] = '\0';
    node->index = -1;
    node->used = false;
    return node;
}

void insertTrie(TrieNode* root, const char* word, const char* hint) {
    TrieNode* current = root;
    for (int i = 0; i < strlen(word); i++) {
        int index = word[i] - 'a';
        if (!current->children[index]) {
            current->children[index] = createNode();
        }
        current = current->children[index];
    }
    current->isEndOfWord = true;
    strcpy(current->hint, hint);
}

void insertTrieWithIndex(TrieNode* root, const char* word, const char* hint, int index) {
    TrieNode* current = root;
    for (int i = 0; i < strlen(word); i++) {
        int idx = word[i] - 'a';
        if (!current->children[idx]) {
            current->children[idx] = createNode();
        }
        current = current->children[idx];
    }
    current->isEndOfWord = true;
    strcpy(current->hint, hint);
    current->index = index;
}

bool searchTrie(TrieNode* root, const char* word) {
    TrieNode* current = root;
    for (int i = 0; i < strlen(word); i++) {
        int index = word[i] - 'a';
        if (!current->children[index]) {
            return false;
        }
        current = current->children[index];
    }
    return (current != NULL && current->isEndOfWord);
}

int searchTrieWithIndex(TrieNode* root, const char* word) {
    TrieNode* current = root;
    for (int i = 0; i < strlen(word); i++) {
        int index = word[i] - 'a';
        if (!current->children[index]) {
            return -1;
        }
        current = current->children[index];
    }
    if (current != NULL && current->isEndOfWord) {
        return current->index;
    }
    return -1;
}

int countWords(TrieNode* node) {
    int count = 0;
    if (node->isEndOfWord) {
        count++;
    }
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i]) {
            count += countWords(node->children[i]);
        }
    }
    return count;
}

bool getRandomWordHelper(TrieNode* node, char* word, int depth, char* hint, int* index, int target, bool markUsed) {
    static int count = 0;
    if (target == 0) {
        count = 0;
    }
    if (node->isEndOfWord && !node->used) {
        if (count == target) {
            word[depth] = '\0';
            strcpy(hint, node->hint);
            *index = node->index;
            if (markUsed) {
                node->used = true;
            }
            count = 0;
            return true;
        }
        count++;
    }
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i]) {
            word[depth] = 'a' + i;
            if (getRandomWordHelper(node->children[i], word, depth + 1, hint, index, target, markUsed)) {
                return true;
            }
        }
    }
    return false;
}

bool getRandomWord(TrieNode* root, char prefix, char* word, char* hint, int* index, bool markUsed) {
    TrieNode* current = root;
    int prefixIndex = prefix - 'a';
    if (!current->children[prefixIndex]) {
        return false;
    }
    current = current->children[prefixIndex];
    int wordCount = countWords(current);
    if (wordCount == 0) {
        return false;
    }
    int randomIndex = rand() % wordCount;
    char tempWord[MAX_WORD_LENGTH] = {0};
    tempWord[0] = prefix;
    if (getRandomWordHelper(current, tempWord, 1, hint, index, randomIndex, markUsed)) {
        strcpy(word, tempWord);
        return true;
    }
    return false;
}

void freeTrie(TrieNode* root) {
    if (root == NULL) {
        return;
    }
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (root->children[i]) {
            freeTrie(root->children[i]);
        }
    }
    free(root);
}

void resetUsedFlags(TrieNode* root) {
    if (root == NULL) {
        return;
    }
    root->used = false;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (root->children[i]) {
            resetUsedFlags(root->children[i]);
        }
    }
}

// User management functions
void initUser(User* u) {
    u->size_u = 0;
    u->time_u[0] = -1;
}

void insert_min_user(User* u, float value, const char* str) {
    u->size_u = u->size_u + 1;
    int i = u->size_u;
    strcpy(u->name[i], str);
    u->time_u[i] = value;

    while (i > 1) {
        int temp = i / 2;
        if (u->time_u[temp] > u->time_u[i]) {
            float tempFloat = u->time_u[temp];
            u->time_u[temp] = u->time_u[i];
            u->time_u[i] = tempFloat;

            char tempName[MAX_NAME_LENGTH];
            strcpy(tempName, u->name[temp]);
            strcpy(u->name[temp], u->name[i]);
            strcpy(u->name[i], tempName);

            i = temp;
        } else {
            return;
        }
    }
}

// Database functions
void initializeDefaultWordDatabase(WordDatabase* db) {
    db->count = 20;
    
    char initialWords[][MAX_WORD_LENGTH] = {
        "apple", "banana", "cherry", "dog", "elephant",
        "flower", "guitar", "happy", "icecream", "jungle",
        "kangaroo", "lemon", "mountain", "notebook", "orange",
        "penguin", "quasar", "rainbow", "sunflower", "turtle"
    };

    char initialHints[][MAX_HINT_LENGTH] = {
        "A common fruit with a red or green skin.",
        "A yellow fruit that is peeled before eating.",
        "A small, round fruit with a pit inside.",
        "A domesticated, loyal animal often kept as a pet.",
        "A large mammal known for its long trunk and tusks.",
        "A colorful and fragrant bloom.",
        "A musical instrument with strings.",
        "An emotion characterized by joy and contentment.",
        "A frozen dessert often enjoyed on a hot day.",
        "A dense and overgrown forest.",
        "A marsupial native to Australia.",
        "A citrus fruit with a sour taste.",
        "A towering landform with rocky peaks.",
        "A notebook for writing and recording information.",
        "A citrus fruit with a sweet and tangy flavor.",
        "A flightless bird with distinctive black and white feathers.",
        "A celestial object emitting powerful energy.",
        "A spectrum of colors in the sky after rain.",
        "A bright and cheerful flower.",
        "A reptile with a hard shell."
    };

    for (int i = 0; i < db->count; i++) {
        strcpy(db->words[i], initialWords[i]);
        strcpy(db->hints[i], initialHints[i]);
    }
}

bool loadWordDatabase(WordDatabase* db) {
    FILE* file = fopen(WORDS_FILE, "r");
    if (file == NULL) {
        initializeDefaultWordDatabase(db);
        return false;
    }

    if (fscanf(file, "%d\n", &db->count) != 1) {
        fclose(file);
        return false;
    }

    for (int i = 0; i < db->count; i++) {
        if (fgets(db->words[i], MAX_WORD_LENGTH, file) == NULL) {
            fclose(file);
            return false;
        }
        db->words[i][strcspn(db->words[i], "\n")] = 0;
        
        if (fgets(db->hints[i], MAX_HINT_LENGTH, file) == NULL) {
            fclose(file);
            return false;
        }
        db->hints[i][strcspn(db->hints[i], "\n")] = 0;
    }
    
    fclose(file);
    return true;
}

void saveWordDatabase(WordDatabase* db) {
    FILE* file = fopen(WORDS_FILE, "w");
    if (file == NULL) {
        return;
    }

    fprintf(file, "%d\n", db->count);
    
    for (int i = 0; i < db->count; i++) {
        fprintf(file, "%s\n%s\n", db->words[i], db->hints[i]);
    }
    
    fclose(file);
}

void addWordToDatabase(WordDatabase* db, const char* word, const char* hint) {
    if (db->count < MAX_WORDS) {
        strcpy(db->words[db->count], word);
        strcpy(db->hints[db->count], hint);
        db->count++;
        saveWordDatabase(db);
    }
}

// UI Helper functions
void clear_container() {
    if (current_container) {
        GtkWidget *child = gtk_widget_get_first_child(current_container);
        while (child) {
            GtkWidget *next = gtk_widget_get_next_sibling(child);
            gtk_box_remove(GTK_BOX(current_container), child);
            child = next;
        }
    }
}

void set_window_content(GtkWidget *widget) {
    clear_container();
    gtk_box_append(GTK_BOX(current_container), widget);
    gtk_widget_set_visible(main_window, TRUE);
}

// Dialog response callback for login
void on_login_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    GtkWidget **entries = (GtkWidget **)user_data;
    GtkWidget *username_entry = entries[0];
    GtkWidget *password_entry = entries[1];
    
    if (response_id == GTK_RESPONSE_OK) {
        GtkEntryBuffer *username_buffer = gtk_entry_get_buffer(GTK_ENTRY(username_entry));
        GtkEntryBuffer *password_buffer = gtk_entry_get_buffer(GTK_ENTRY(password_entry));
        
        const char *username = gtk_entry_buffer_get_text(username_buffer);
        const char *password = gtk_entry_buffer_get_text(password_buffer);
        
        if (strcmp(username, ADMIN_USERNAME) == 0 && strcmp(password, ADMIN_PASSWORD) == 0) {
            gtk_window_destroy(GTK_WINDOW(dialog));
            show_teacher_menu();
        } else {
            GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                           GTK_DIALOG_MODAL,
                                                           GTK_MESSAGE_ERROR,
                                                           GTK_BUTTONS_OK,
                                                           "Invalid credentials!");
            gtk_widget_set_visible(error_dialog, TRUE);
            g_signal_connect(error_dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        }
    } else {
        gtk_window_destroy(GTK_WINDOW(dialog));
    }
    
    g_free(entries);
}

// Empty the hints array and reset indices
void EmptyHints(char hints[][MAX_HINT_LENGTH], int size, int index[]) {
    for (int i = 0; i < size; i++) {
        hints[i][0] = '\0';
        index[i] = -1;
    }
}

// Initialize grid with spaces
void EmptyGrid(char grid[][GRID_SIZE], int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            grid[i][j] = ' ';
        }
    }
}

// Copy one grid to another
void CopyGrid(char source[][GRID_SIZE], char dest[][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (source[i][j] != ' ') {
                dest[i][j] = '_';
            } else {
                dest[i][j] = source[i][j];
            }
        }
    }
}

// Compare two grids for equality
bool CompareGrids(char grid1[][GRID_SIZE], char grid2[][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid1[i][j] != grid2[i][j]) {
                return false;
            }
        }
    }
    return true;
}

// Update display grid when a word is found
void updateDisplayGrid(char source[][GRID_SIZE], char display[][GRID_SIZE], const char* word) {
    int word_len = strlen(word);
    
    // Search horizontally
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j <= GRID_SIZE - word_len; j++) {
            bool match = true;
            for (int k = 0; k < word_len; k++) {
                if (source[i][j + k] != word[k]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                for (int k = 0; k < word_len; k++) {
                    display[i][j + k] = word[k];
                }
                return;
            }
        }
    }
    
    // Search vertically
    for (int i = 0; i <= GRID_SIZE - word_len; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            bool match = true;
            for (int k = 0; k < word_len; k++) {
                if (source[i + k][j] != word[k]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                for (int k = 0; k < word_len; k++) {
                    display[i + k][j] = word[k];
                }
                return;
            }
        }
    }
}

// Min heapify function for user scores
void heapify_min(User* u, int i, int n) {
    int smallest = i;
    int left = 2 * i;
    int right = 2 * i + 1;
    
    if (left <= n && u->time_u[left] < u->time_u[smallest]) {
        smallest = left;
    }
    
    if (right <= n && u->time_u[right] < u->time_u[smallest]) {
        smallest = right;
    }
    
    if (smallest != i) {
        // Swap times
        float temp_time = u->time_u[i];
        u->time_u[i] = u->time_u[smallest];
        u->time_u[smallest] = temp_time;
        
        // Swap names
        char temp_name[MAX_NAME_LENGTH];
        strcpy(temp_name, u->name[i]);
        strcpy(u->name[i], u->name[smallest]);
        strcpy(u->name[smallest], temp_name);
        
        heapify_min(u, smallest, n);
    }
}

// Sort users by minimum time (heap sort)
void user_min_sort(User* u, int n) {
    // Build min heap
    for (int i = n / 2; i >= 1; i--) {
        heapify_min(u, i, n);
    }
    
    // Extract elements from heap one by one
    for (int i = n; i >= 2; i--) {
        // Move current root to end
        float temp_time = u->time_u[1];
        u->time_u[1] = u->time_u[i];
        u->time_u[i] = temp_time;
        
        char temp_name[MAX_NAME_LENGTH];
        strcpy(temp_name, u->name[1]);
        strcpy(u->name[1], u->name[i]);
        strcpy(u->name[i], temp_name);
        
        // Call heapify on the reduced heap
        heapify_min(u, 1, i - 1);
    }
}

// Generate a simple crossword puzzle
void generateCrossword(char grid[][GRID_SIZE], WordDatabase* db, TrieNode* mainTrie, TrieNode* gameTrie, char hints2[][MAX_HINT_LENGTH]) {
    // Clear the grid first
    EmptyGrid(grid, GRID_SIZE);
    
    int center = GRID_SIZE / 2;
    int words_placed = 0;
    
    // Array to store placed words info for intersection checking
    struct {
        char word[MAX_WORD_LENGTH];
        int row, col;
        bool horizontal;
        int length;
    } placed_words[numWords];
    
    if (db->count == 0) return;
    
    // Place first word horizontally in the center
    if (words_placed < numWords) {
        int word_idx = rand() % db->count;
        char* word = db->words[word_idx];
        int word_len = strlen(word);
        int start_col = center - word_len / 2;
        
        if (start_col >= 0 && start_col + word_len < GRID_SIZE) {
            // Place the word
            for (int i = 0; i < word_len; i++) {
                grid[center][start_col + i] = word[i];
            }
            
            // Store word info
            strcpy(placed_words[words_placed].word, word);
            placed_words[words_placed].row = center;
            placed_words[words_placed].col = start_col;
            placed_words[words_placed].horizontal = true;
            placed_words[words_placed].length = word_len;
            
            strcpy(hints2[words_placed], db->hints[word_idx]);
            insertTrieWithIndex(gameTrie, word, db->hints[word_idx], words_placed);
            words_placed++;
        }
    }
    
    // Place remaining words by finding intersections with already placed words
    for (int attempt = 0; attempt < 100 && words_placed < numWords && words_placed < db->count; attempt++) {
        int word_idx = rand() % db->count;
        char* word = db->words[word_idx];
        int word_len = strlen(word);
        
        // Check if word is already placed
        bool already_placed = false;
        for (int i = 0; i < words_placed; i++) {
            if (strcmp(placed_words[i].word, word) == 0) {
                already_placed = true;
                break;
            }
        }
        if (already_placed) continue;
        
        // Try to find intersection with existing words
        bool word_placed = false;
        for (int existing = 0; existing < words_placed && !word_placed; existing++) {
            char* existing_word = placed_words[existing].word;
            int existing_len = placed_words[existing].length;
            
            // Look for common letters
            for (int i = 0; i < word_len && !word_placed; i++) {
                for (int j = 0; j < existing_len && !word_placed; j++) {
                    if (word[i] == existing_word[j]) {
                        int new_row, new_col;
                        bool new_horizontal = !placed_words[existing].horizontal;
                        bool can_place = true;
                        
                        if (placed_words[existing].horizontal) {
                            // Existing word is horizontal, new word will be vertical
                            new_row = placed_words[existing].row - i;
                            new_col = placed_words[existing].col + j;
                            
                            // Check bounds
                            if (new_row < 0 || new_row + word_len >= GRID_SIZE || new_col < 0 || new_col >= GRID_SIZE) {
                                can_place = false;
                            }
                            
                            // Check for conflicts
                            if (can_place) {
                                for (int k = 0; k < word_len; k++) {
                                    if (grid[new_row + k][new_col] != ' ' && grid[new_row + k][new_col] != word[k]) {
                                        can_place = false;
                                        break;
                                    }
                                    // Check adjacent cells for proper spacing (optional)
                                    if (k == 0 && new_row > 0 && grid[new_row - 1][new_col] != ' ') {
                                        can_place = false;
                                        break;
                                    }
                                    if (k == word_len - 1 && new_row + word_len < GRID_SIZE - 1 && grid[new_row + word_len][new_col] != ' ') {
                                        can_place = false;
                                        break;
                                    }
                                }
                            }
                            
                            // Place the word if possible
                            if (can_place) {
                                for (int k = 0; k < word_len; k++) {
                                    grid[new_row + k][new_col] = word[k];
                                }
                                
                                // Store word info
                                strcpy(placed_words[words_placed].word, word);
                                placed_words[words_placed].row = new_row;
                                placed_words[words_placed].col = new_col;
                                placed_words[words_placed].horizontal = false;
                                placed_words[words_placed].length = word_len;
                                
                                strcpy(hints2[words_placed], db->hints[word_idx]);
                                insertTrieWithIndex(gameTrie, word, db->hints[word_idx], words_placed);
                                words_placed++;
                                word_placed = true;
                            }
                        } else {
                            // Existing word is vertical, new word will be horizontal
                            new_row = placed_words[existing].row + j;
                            new_col = placed_words[existing].col - i;
                            
                            // Check bounds
                            if (new_row < 0 || new_row >= GRID_SIZE || new_col < 0 || new_col + word_len >= GRID_SIZE) {
                                can_place = false;
                            }
                            
                            // Check for conflicts
                            if (can_place) {
                                for (int k = 0; k < word_len; k++) {
                                    if (grid[new_row][new_col + k] != ' ' && grid[new_row][new_col + k] != word[k]) {
                                        can_place = false;
                                        break;
                                    }
                                    // Check adjacent cells for proper spacing (optional)
                                    if (k == 0 && new_col > 0 && grid[new_row][new_col - 1] != ' ') {
                                        can_place = false;
                                        break;
                                    }
                                    if (k == word_len - 1 && new_col + word_len < GRID_SIZE - 1 && grid[new_row][new_col + word_len] != ' ') {
                                        can_place = false;
                                        break;
                                    }
                                }
                            }
                            
                            // Place the word if possible
                            if (can_place) {
                                for (int k = 0; k < word_len; k++) {
                                    grid[new_row][new_col + k] = word[k];
                                }
                                
                                // Store word info
                                strcpy(placed_words[words_placed].word, word);
                                placed_words[words_placed].row = new_row;
                                placed_words[words_placed].col = new_col;
                                placed_words[words_placed].horizontal = true;
                                placed_words[words_placed].length = word_len;
                                
                                strcpy(hints2[words_placed], db->hints[word_idx]);
                                insertTrieWithIndex(gameTrie, word, db->hints[word_idx], words_placed);
                                words_placed++;
                                word_placed = true;
                            }
                        }
                    }
                }
            }
        }
    }
    
    for (int attempt = 0; attempt < 50 && words_placed < numWords && words_placed < db->count; attempt++) {
        int word_idx = rand() % db->count;
        char* word = db->words[word_idx];
        int word_len = strlen(word);
        
        // Check if word is already placed
        bool already_placed = false;
        for (int i = 0; i < words_placed; i++) {
            if (strcmp(placed_words[i].word, word) == 0) {
                already_placed = true;
                break;
            }
        }
        if (already_placed) continue;
        
        // Try to place horizontally
        if (rand() % 2 == 0) {
            for (int row = 2; row < GRID_SIZE - 2; row += 2) { // Skip every other row for spacing
                for (int col = 1; col <= GRID_SIZE - word_len - 1; col++) {
                    bool can_place = true;
                    
                    // Check if area is clear
                    for (int k = 0; k < word_len; k++) {
                        if (grid[row][col + k] != ' ') {
                            can_place = false;
                            break;
                        }
                    }
                    
                    if (can_place) {
                        // Place the word
                        for (int k = 0; k < word_len; k++) {
                            grid[row][col + k] = word[k];
                        }
                        
                        // Store word info
                        strcpy(placed_words[words_placed].word, word);
                        placed_words[words_placed].row = row;
                        placed_words[words_placed].col = col;
                        placed_words[words_placed].horizontal = true;
                        placed_words[words_placed].length = word_len;
                        
                        strcpy(hints2[words_placed], db->hints[word_idx]);
                        insertTrieWithIndex(gameTrie, word, db->hints[word_idx], words_placed);
                        words_placed++;
                        goto next_word;
                    }
                }
            }
        } else {
            // Try to place vertically
            for (int col = 2; col < GRID_SIZE - 2; col += 2) { // Skip every other column for spacing
                for (int row = 1; row <= GRID_SIZE - word_len - 1; row++) {
                    bool can_place = true;
                    
                    // Check if area is clear
                    for (int k = 0; k < word_len; k++) {
                        if (grid[row + k][col] != ' ') {
                            can_place = false;
                            break;
                        }
                    }
                    
                    if (can_place) {
                        // Place the word
                        for (int k = 0; k < word_len; k++) {
                            grid[row + k][col] = word[k];
                        }
                        
                        // Store word info
                        strcpy(placed_words[words_placed].word, word);
                        placed_words[words_placed].row = row;
                        placed_words[words_placed].col = col;
                        placed_words[words_placed].horizontal = false;
                        placed_words[words_placed].length = word_len;
                        
                        strcpy(hints2[words_placed], db->hints[word_idx]);
                        insertTrieWithIndex(gameTrie, word, db->hints[word_idx], words_placed);
                        words_placed++;
                        goto next_word;
                    }
                }
            }
        }
        next_word:;
    }
}

// Dialog response callback for add word
void on_add_word_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    GtkWidget **entries = (GtkWidget **)user_data;
    GtkWidget *word_entry = entries[0];
    GtkWidget *hint_entry = entries[1];
    
    if (response_id == GTK_RESPONSE_OK) {
        GtkEntryBuffer *word_buffer = gtk_entry_get_buffer(GTK_ENTRY(word_entry));
        GtkEntryBuffer *hint_buffer = gtk_entry_get_buffer(GTK_ENTRY(hint_entry));
        
        const char *word = gtk_entry_buffer_get_text(word_buffer);
        const char *hint = gtk_entry_buffer_get_text(hint_buffer);
        
        if (strlen(word) > 0 && strlen(hint) > 0) {
            addWordToDatabase(&db, word, hint);
            
            GtkWidget *success_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                             GTK_DIALOG_MODAL,
                                                             GTK_MESSAGE_INFO,
                                                             GTK_BUTTONS_OK,
                                                             "Word and hint added successfully!");
            gtk_widget_set_visible(success_dialog, TRUE);
            g_signal_connect(success_dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        } else {
            GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                           GTK_DIALOG_MODAL,
                                                           GTK_MESSAGE_ERROR,
                                                           GTK_BUTTONS_OK,
                                                           "Please enter both word and hint!");
            gtk_widget_set_visible(error_dialog, TRUE);
            g_signal_connect(error_dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        }
    }
    
    gtk_window_destroy(GTK_WINDOW(dialog));
    g_free(entries);
}

// Callback functions
void on_start_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='20000' weight='bold'>Crossword Puzzle Game</span>");
    gtk_widget_set_margin_bottom(title, 20);
    gtk_box_append(GTK_BOX(vbox), title);
    
    GtkWidget *student_btn = gtk_button_new_with_label("Student");
    gtk_widget_set_size_request(student_btn, 200, 50);
    gtk_widget_set_margin_bottom(student_btn, 10);
    g_signal_connect(student_btn, "clicked", G_CALLBACK(on_student_clicked), NULL);
    gtk_box_append(GTK_BOX(vbox), student_btn);
    
    GtkWidget *teacher_btn = gtk_button_new_with_label("Teacher");
    gtk_widget_set_size_request(teacher_btn, 200, 50);
    gtk_widget_set_margin_bottom(teacher_btn, 10);
    g_signal_connect(teacher_btn, "clicked", G_CALLBACK(on_teacher_clicked), NULL);
    gtk_box_append(GTK_BOX(vbox), teacher_btn);
    
    GtkWidget *back_btn = gtk_button_new_with_label("Back");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(show_main_menu), NULL);
    gtk_box_append(GTK_BOX(vbox), back_btn);
    
    set_window_content(vbox);
}

void on_student_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='18000' weight='bold'>Student Menu</span>");
    gtk_widget_set_margin_bottom(title, 20);
    gtk_box_append(GTK_BOX(vbox), title);
    
    GtkWidget *play_btn = gtk_button_new_with_label("Play Game");
    gtk_widget_set_size_request(play_btn, 200, 50);
    gtk_widget_set_margin_bottom(play_btn, 10);
    g_signal_connect(play_btn, "clicked", G_CALLBACK(on_play_game_clicked), NULL);
    gtk_box_append(GTK_BOX(vbox), play_btn);
    
    GtkWidget *score_btn = gtk_button_new_with_label("View Scores");
    gtk_widget_set_size_request(score_btn, 200, 50);
    gtk_widget_set_margin_bottom(score_btn, 10);
    g_signal_connect(score_btn, "clicked", G_CALLBACK(on_view_scores_clicked), NULL);
    gtk_box_append(GTK_BOX(vbox), score_btn);
    
    GtkWidget *back_btn = gtk_button_new_with_label("Back");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_box_append(GTK_BOX(vbox), back_btn);
    
    set_window_content(vbox);
}

void on_teacher_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Admin Authentication",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "Login", GTK_RESPONSE_OK,
                                                   "Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 200);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 20);
    gtk_widget_set_margin_bottom(grid, 20);
    
    GtkWidget *username_label = gtk_label_new("Username:");
    GtkWidget *username_entry = gtk_entry_new();
    GtkWidget *password_label = gtk_label_new("Password:");
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 1, 1, 1);
    
    gtk_box_append(GTK_BOX(content_area), grid);
    
    // Store entry widgets for callback
    GtkWidget **entries = g_malloc(2 * sizeof(GtkWidget*));
    entries[0] = username_entry;
    entries[1] = password_entry;
    
    g_signal_connect(dialog, "response", G_CALLBACK(on_login_response), entries);
    gtk_widget_set_visible(dialog, TRUE);
}

void show_teacher_menu() {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='18000' weight='bold'>Teacher Menu</span>");
    gtk_widget_set_margin_bottom(title, 20);
    gtk_box_append(GTK_BOX(vbox), title);
    
    GtkWidget *add_word_btn = gtk_button_new_with_label("Add New Word & Hint");
    gtk_widget_set_size_request(add_word_btn, 200, 50);
    gtk_widget_set_margin_bottom(add_word_btn, 10);
    g_signal_connect(add_word_btn, "clicked", G_CALLBACK(on_add_word_clicked), NULL);
    gtk_box_append(GTK_BOX(vbox), add_word_btn);
    
    GtkWidget *back_btn = gtk_button_new_with_label("Back");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_box_append(GTK_BOX(vbox), back_btn);
    
    set_window_content(vbox);
}

void on_add_word_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add New Word",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "Add", GTK_RESPONSE_OK,
                                                   "Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 200);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 20);
    gtk_widget_set_margin_bottom(grid, 20);
    
    GtkWidget *word_label = gtk_label_new("Word:");
    GtkWidget *word_entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(word_entry), MAX_WORD_LENGTH - 1);
    
    GtkWidget *hint_label = gtk_label_new("Hint:");
    GtkWidget *hint_entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(hint_entry), MAX_HINT_LENGTH - 1);
    
    gtk_grid_attach(GTK_GRID(grid), word_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), word_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), hint_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), hint_entry, 1, 1, 1, 1);
    
    gtk_box_append(GTK_BOX(content_area), grid);
    
    // Store entry widgets for callback
    GtkWidget **entries = g_malloc(2 * sizeof(GtkWidget*));
    entries[0] = word_entry;
    entries[1] = hint_entry;
    
    g_signal_connect(dialog, "response", G_CALLBACK(on_add_word_response), entries);
    gtk_widget_set_visible(dialog, TRUE);
}

// Helper function to create game grid display
GtkWidget* create_grid_display(char grid[][GRID_SIZE]) {
    GtkWidget *grid_widget = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_widget), 1);
    gtk_grid_set_column_spacing(GTK_GRID(grid_widget), 1);
    
    // Create CSS provider for better styling
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, 
        ".grid-empty { "
        "  background-color: #f8f8f8; "
        "  border: 1px solid #ddd; "
        "  min-width: 20px; "
        "  min-height: 20px; "
        "} "
        ".grid-blank { "
        "  background-color: #fff; "
        "  border: 2px solid #333; "
        "  font-weight: bold; "
        "  font-size: 14px; "
        "  text-align: center; "
        "  min-width: 20px; "
        "  min-height: 20px; "
        "} "
        ".grid-filled { "
        "  background-color: #e6ffe6; "
        "  border: 2px solid #333; "
        "  font-weight: bold; "
        "  font-size: 14px; "
        "  text-align: center; "
        "  color: #006600; "
        "  min-width: 20px; "
        "  min-height: 20px; "
        "}");
    
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            GtkWidget *cell = gtk_label_new(NULL);
            gtk_widget_set_size_request(cell, 25, 25);
            
            char cell_text[2] = {0};
            if (grid[i][j] == ' ') {
                gtk_label_set_text(GTK_LABEL(cell), "");
                gtk_widget_add_css_class(cell, "grid-empty");
            } else if (grid[i][j] == '_') {
                gtk_label_set_text(GTK_LABEL(cell), "");
                gtk_widget_add_css_class(cell, "grid-blank");
            } else {
                cell_text[0] = toupper(grid[i][j]); // Display uppercase letters
                gtk_label_set_text(GTK_LABEL(cell), cell_text);
                gtk_widget_add_css_class(cell, "grid-filled");
            }
            
            GtkStyleContext *context = gtk_widget_get_style_context(cell);
            gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
            
            gtk_grid_attach(GTK_GRID(grid_widget), cell, j, i, 1, 1);
        }
    }
    
    g_object_unref(provider);
    return grid_widget;
}

// Helper function to create hints display
GtkWidget* create_hints_display(char hints[][MAX_HINT_LENGTH], int indices[]) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_hexpand(vbox, TRUE);
    gtk_widget_set_vexpand(vbox, TRUE);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Hints:</b>");
    gtk_box_append(GTK_BOX(vbox), title);
    
    for (int i = 0; i < numWords; i++) {
        if (strlen(hints[i]) > 0) {
            char hint_text[200];
            
            // Check if word has been found by comparing if indices[i] equals i
            if (indices[i] == i) {
                // Word has been found - show with checkmark
                snprintf(hint_text, sizeof(hint_text), "✓ %d. %s", i + 1, hints[i]);
            } else {
                // Word not found yet - show normally
                snprintf(hint_text, sizeof(hint_text), "%d. %s", i + 1, hints[i]);
            }
            
            GtkWidget *hint_label = gtk_label_new(hint_text);
            gtk_label_set_wrap(GTK_LABEL(hint_label), TRUE);
            gtk_widget_set_halign(hint_label, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(vbox), hint_label);
        }
    }
    
    return vbox;
}

// Function to update the game display
void update_game_display() {
    if (grid_display) {
        // Remove old grid display
        GtkWidget *parent = gtk_widget_get_parent(grid_display);
        if (parent && GTK_IS_BOX(parent)) {
            gtk_box_remove(GTK_BOX(parent), grid_display);
            
            // Create new grid display
            grid_display = create_grid_display(display_grid);
            gtk_box_prepend(GTK_BOX(parent), grid_display);
        }
    }
    
    if (hints_display) {
        // Remove old hints display
        GtkWidget *parent = gtk_widget_get_parent(hints_display);
        if (parent && GTK_IS_BOX(parent)) {
            gtk_box_remove(GTK_BOX(parent), hints_display);
            
            // Create new hints display
            hints_display = create_hints_display(game_hints, hint_indices);
            gtk_box_append(GTK_BOX(parent), hints_display);
        }
    }
}

void initialize_user_scores() {
    user_scores.size_u = 0;
    // Initialize all arrays to default values
    for (int i = 0; i < MAX_USERS; i++) {
        strcpy(user_scores.name[i], "");
        user_scores.time_u[i] = 0.0;
        user_scores.points[i] = 1000;
    }
}

// To insert user with points
void insert_min_user_with_points(User *user_scores, float time, const char *name, int points) {
    if (user_scores->size_u < MAX_USERS) {
        strcpy(user_scores->name[user_scores->size_u], name);
        user_scores->time_u[user_scores->size_u] = time;
        user_scores->points[user_scores->size_u] = points;
        user_scores->size_u++;
    } else {
        // If array is full, replace the worst score (highest time)
        int worst_index = 0;
        for (int i = 1; i < MAX_USERS; i++) {
            if (user_scores->time_u[i] > user_scores->time_u[worst_index]) {
                worst_index = i;
            }
        }
        
        // Only replace if new time is better
        if (time < user_scores->time_u[worst_index]) {
            strcpy(user_scores->name[worst_index], name);
            user_scores->time_u[worst_index] = time;
            user_scores->points[worst_index] = points;
        }
    }
}

// Callback for word entry submission
void on_word_submitted(GtkWidget *widget, gpointer data) {
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(word_entry));
    const char *word = gtk_entry_buffer_get_text(buffer);
    
    if (strlen(word) == 0) {
        return;
    }
    
    // Convert to lowercase for comparison
    char lowercase_word[MAX_WORD_LENGTH];
    strcpy(lowercase_word, word);
    for (int i = 0; lowercase_word[i]; i++) {
        lowercase_word[i] = tolower(lowercase_word[i]);
    }
    
    if (searchTrie(gameTrie, lowercase_word)) {
        int ind = searchTrieWithIndex(gameTrie, lowercase_word);
        if (ind != -1) {
            hint_indices[ind] = ind;
        }
        updateDisplayGrid(game_grid, display_grid, lowercase_word);
        
        // Clear the entry
        gtk_entry_buffer_set_text(buffer, "", 0);
        
        // Update display
        update_game_display();
        
        // Check if game is complete
        if (CompareGrids(game_grid, display_grid)) {
            clock_t end_time = clock();
            float duration = (float)(end_time - game_start_time) / CLOCKS_PER_SEC;
            
            // Calculate points based on time (faster = more points)
            int points = 1000 - (int)(duration * 2); // Lose 10 points per second
            if (points < 0) points = 0; // Minimum 0 points // +1 to avoid division by very small numbers
            
            // Add score to leaderboard
            insert_min_user_with_points(&user_scores, duration, current_player_name, points);
            int temp = user_scores.size_u;
            for (int i = temp / 2; i > 0; i--) {
                heapify_min(&user_scores, i, temp);
            }
            user_min_sort(&user_scores, user_scores.size_u);
            
            // Show completion dialog
            char completion_msg[300];
            snprintf(completion_msg, sizeof(completion_msg),
                    "Congratulations %s!\nYou completed the puzzle in %.2f seconds!\nPoints earned: %d",
                    current_player_name, duration, points);
            
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(game_window),
                                                     GTK_DIALOG_MODAL,
                                                     GTK_MESSAGE_INFO,
                                                     GTK_BUTTONS_OK,
                                                     "%s", completion_msg);
            gtk_widget_set_visible(dialog, TRUE);
            g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
            
            // Close game window
            gtk_window_destroy(GTK_WINDOW(game_window));
            game_window = NULL;
        }
    } else {
        // Show incorrect word message
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(game_window),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_WARNING,
                                                 GTK_BUTTONS_OK,
                                                 "Incorrect word: %s", word);
        gtk_widget_set_visible(dialog, TRUE);
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        
        // Clear the entry
        gtk_entry_buffer_set_text(buffer, "", 0);
    }
}

// Dialog response callback for player name
void on_name_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    GtkWidget *name_entry = (GtkWidget *)user_data;
    
    if (response_id == GTK_RESPONSE_OK) {
        GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(name_entry));
        const char *name = gtk_entry_buffer_get_text(buffer);
        
        if (strlen(name) > 0) {
            strcpy(current_player_name, name);
            
            // Initialize game
            TrieNode* mainTrie = createNode();
            gameTrie = createNode();
            
            // Insert all words from database into the trie
            for (int i = 0; i < db.count; i++) {
                insertTrie(mainTrie, db.words[i], db.hints[i]);
            }
            
            EmptyHints(game_hints, numWords, hint_indices);
            EmptyGrid(game_grid, GRID_SIZE);
            EmptyGrid(display_grid, GRID_SIZE);
            
            // Reset used flags
            resetUsedFlags(mainTrie);
            
            // Generate crossword
            generateCrossword(game_grid, &db, mainTrie, gameTrie, game_hints);
            CopyGrid(game_grid, display_grid);
            
            // Start timer
            game_start_time = clock();
            
            // Create game window
            game_window = gtk_window_new();
            gtk_window_set_title(GTK_WINDOW(game_window), "Crossword Puzzle Game");
            gtk_window_set_default_size(GTK_WINDOW(game_window), 400, 300);
            gtk_window_set_transient_for(GTK_WINDOW(game_window), GTK_WINDOW(main_window));
            
            // Create game layout
            GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
            gtk_widget_set_margin_start(hbox, 20);
            gtk_widget_set_margin_end(hbox, 20);
            gtk_widget_set_margin_top(hbox, 20);
            gtk_widget_set_margin_bottom(hbox, 20);
            
            // Left side - Grid
            GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
            grid_display = create_grid_display(display_grid);
            gtk_box_append(GTK_BOX(left_vbox), grid_display);
            
            // Word entry
            GtkWidget *entry_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
            GtkWidget *entry_label = gtk_label_new("Enter word:");
            word_entry = gtk_entry_new();
            gtk_entry_set_max_length(GTK_ENTRY(word_entry), MAX_WORD_LENGTH - 1);
            g_signal_connect(word_entry, "activate", G_CALLBACK(on_word_submitted), NULL);
            
            GtkWidget *submit_btn = gtk_button_new_with_label("Submit");
            g_signal_connect(submit_btn, "clicked", G_CALLBACK(on_word_submitted), NULL);
            
            gtk_box_append(GTK_BOX(entry_hbox), entry_label);
            gtk_box_append(GTK_BOX(entry_hbox), word_entry);
            gtk_box_append(GTK_BOX(entry_hbox), submit_btn);
            gtk_box_append(GTK_BOX(left_vbox), entry_hbox);
            
            // Right side - Hints
            GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
            hints_display = create_hints_display(game_hints, hint_indices);
            
            GtkWidget *scrolled = gtk_scrolled_window_new();
            gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
                                         GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
            gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), hints_display);
            gtk_widget_set_size_request(scrolled, 300, -1);
            
            gtk_box_append(GTK_BOX(right_vbox), scrolled);
            
            gtk_box_append(GTK_BOX(hbox), left_vbox);
            gtk_box_append(GTK_BOX(hbox), right_vbox);
            
            gtk_window_set_child(GTK_WINDOW(game_window), hbox);
            gtk_widget_set_visible(game_window, TRUE);
            
            // Focus on word entry
            gtk_widget_grab_focus(word_entry);
            
            // Clean up main trie (gameTrie will be cleaned up when game ends)
            freeTrie(mainTrie);
        } else {
            GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                           GTK_DIALOG_MODAL,
                                                           GTK_MESSAGE_ERROR,
                                                           GTK_BUTTONS_OK,
                                                           "Please enter your name!");
            gtk_widget_set_visible(error_dialog, TRUE);
            g_signal_connect(error_dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        }
    }
    
    gtk_window_destroy(GTK_WINDOW(dialog));
}

void on_play_game_clicked(GtkWidget *widget, gpointer data) {
    // Create name input dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Enter Your Name",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "Start Game", GTK_RESPONSE_OK,
                                                   "Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);
    
    GtkWidget *label = gtk_label_new("Enter your name to start the game:");
    GtkWidget *name_entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(name_entry), MAX_NAME_LENGTH - 1);
    
    gtk_box_append(GTK_BOX(vbox), label);
    gtk_box_append(GTK_BOX(vbox), name_entry);
    gtk_box_append(GTK_BOX(content_area), vbox);
    
    g_signal_connect(dialog, "response", G_CALLBACK(on_name_response), name_entry);
    gtk_widget_set_visible(dialog, TRUE);
}

void on_view_scores_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='18000' weight='bold'>*SCORE CARD</span>");
    gtk_widget_set_margin_bottom(title, 20);
    gtk_box_append(GTK_BOX(vbox), title);
    
    if (user_scores.size_u == 0) {
        GtkWidget *no_scores = gtk_label_new("No scores available yet!");
        gtk_widget_set_margin_bottom(no_scores, 10);
        gtk_box_append(GTK_BOX(vbox), no_scores);
    } else {
        char score_text[1500] = "Name                Time        Points\n";
        strcat(score_text, "--------------------------------\n");
        for (int i = 0; i < user_scores.size_u; i++) {
            char temp[150];
            sprintf(temp, "%-15s     %.2f sec    %d\n", 
                    user_scores.name[i], 
                    user_scores.time_u[i], 
                    user_scores.points[i]);
            strcat(score_text, temp);
        }
        
        GtkWidget *scores_label = gtk_label_new(score_text);
        gtk_widget_set_margin_bottom(scores_label, 10);
        gtk_widget_add_css_class(scores_label, "monospace");
        
        gtk_box_append(GTK_BOX(vbox), scores_label);
    }
    
    GtkWidget *back_btn = gtk_button_new_with_label("Back");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_student_clicked), NULL);
    gtk_box_append(GTK_BOX(vbox), back_btn);
    
    set_window_content(vbox);
}

void show_main_menu() {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(vbox, 50);
    gtk_widget_set_margin_end(vbox, 50);
    gtk_widget_set_margin_top(vbox, 50);
    gtk_widget_set_margin_bottom(vbox, 50);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='24000' weight='bold' color='blue'>CROSSWORD PUZZLE GAME</span>");
    gtk_widget_set_margin_bottom(title, 30);
    gtk_box_append(GTK_BOX(vbox), title);
    
    GtkWidget *subtitle = gtk_label_new("Welcome! Click START to begin");
    gtk_widget_set_margin_bottom(subtitle, 10);
    gtk_box_append(GTK_BOX(vbox), subtitle);
    
    GtkWidget *start_btn = gtk_button_new_with_label("START");
    gtk_widget_set_size_request(start_btn, 150, 60);
    gtk_widget_set_margin_bottom(start_btn, 20);
    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_box_append(GTK_BOX(vbox), start_btn);
    
    set_window_content(vbox);
}

static void on_app_activate(GtkApplication *app, gpointer user_data) {
    // Initialize data
    loadWordDatabase(&db);
    initUser(&user_scores);
    srand(time(NULL));
    
    // Create main window
    main_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(main_window), "Crossword");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 1000, 800);
    
    // Create main container
    current_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(main_window), current_container);
    
    // Show main menu
    show_main_menu();
    
    gtk_window_present(GTK_WINDOW(main_window));
}

int main(int argc, char *argv[]) {
    GtkApplication *app;
    int status;
    
    app = gtk_application_new("com.example.crossword", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}