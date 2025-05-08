#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef struct HashEntry {
    char *key;
    void *value;
    struct HashEntry *next;
} HashEntry;

typedef struct {
    int size;
    HashEntry **entries;
} Hashtable;

typedef struct StackNode {
    void *data;
    struct StackNode *next;
} StackNode;

typedef struct {
    StackNode *top;
    int count;
} Stack;

typedef struct QueueNode {
    void *data;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front, *rear;
    int count;
} Queue;

Hashtable *create_hashtable(int size) {
    Hashtable *ht = malloc(sizeof(Hashtable));
    ht->size = size;
    ht->entries = calloc(size, sizeof(HashEntry*));
    return ht;
}

unsigned int hash(Hashtable *ht, char *key) {
    unsigned int hashval = 0;
    for (; *key != '\0'; key++) {
        hashval = *key + (hashval << 5) - hashval;
    }
    return hashval % ht->size;
}

void hashtable_put(Hashtable *ht, char *key, void *value) {
    unsigned int bucket = hash(ht, key);
    HashEntry *entry = malloc(sizeof(HashEntry));
    entry->key = strdup(key);
    entry->value = value;
    entry->next = ht->entries[bucket];
    ht->entries[bucket] = entry;
}

void *hashtable_get(Hashtable *ht, char *key) {
    unsigned int bucket = hash(ht, key);
    HashEntry *entry = ht->entries[bucket];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

Stack *create_stack() {
    Stack *s = malloc(sizeof(Stack));
    s->top = NULL;
    s->count = 0;
    return s;
}

void stack_push(Stack *s, void *data) {
    StackNode *node = malloc(sizeof(StackNode));
    node->data = data;
    node->next = s->top;
    s->top = node;
    s->count++;
}

void *stack_pop(Stack *s) {
    if (s->top == NULL) return NULL;
    StackNode *node = s->top;
    void *data = node->data;
    s->top = node->next;
    free(node);
    s->count--;
    return data;
}

Queue *create_queue() {
    Queue *q = malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    q->count = 0;
    return q;
}

void enqueue(Queue *q, void *data) {
    QueueNode *node = malloc(sizeof(QueueNode));
    node->data = data;
    node->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = node;
    } else {
        q->rear->next = node;
        q->rear = node;
    }
    q->count++;
}

void *dequeue(Queue *q) {
    if (q->front == NULL) return NULL;
    QueueNode *node = q->front;
    void *data = node->data;
    q->front = node->next;
    if (q->front == NULL) q->rear = NULL;
    free(node);
    q->count--;
    return data;
}

GLuint imageTexID;
int imageWindow;

#define BOX_WIDTH 0.08f

typedef struct TapeNode {
    char symbol;
    struct TapeNode *prev, *next;
} TapeNode;

TapeNode *head = NULL, *tail = NULL;
TapeNode *read_head = NULL;

char binary[32];
int binary_len = 0;
char current_state[10] = "q0";
int running = 0;
int showDiagram = 0;
char marked_symbol = '\0';

Hashtable *state_ht = NULL;
Stack *state_stack = NULL;
Queue *event_queue = NULL;

void dec_to_binary(int n) {
    int i = 0;
    while (n > 0) {
        binary[i++] = (n % 2) + '0';
        n /= 2;
    }
    binary[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char tmp = binary[j];
        binary[j] = binary[i - j - 1];
        binary[i - j - 1] = tmp;
    }
    binary_len = i;
}

void loadImageTexture(const char *filename) {
    int width, height, channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 0);
    if (!data) {
        printf("Failed to load image: %s\n", filename);
        return;
    }

    glGenTextures(1, &imageTexID);
    glBindTexture(GL_TEXTURE_2D, imageTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
}

void displayImageWindow() {
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, imageTexID);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(-1, -1);
    glTexCoord2f(1, 1); glVertex2f(1, -1);
    glTexCoord2f(1, 0); glVertex2f(1, 1);
    glTexCoord2f(0, 0); glVertex2f(-1, 1);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glutSwapBuffers();
}

void addTapeSymbol(char symbol) {
    TapeNode *node = (TapeNode *)malloc(sizeof(TapeNode));
    node->symbol = symbol;
    node->next = NULL;
    node->prev = tail;
    if (tail) tail->next = node;
    else head = node;
    tail = node;
}

void initTape() {
    addTapeSymbol('B');
    for (int i = 0; i < binary_len; i++) addTapeSymbol(binary[i]);
    addTapeSymbol('B');
    for (int i = 0; i < binary_len + 4; i++) addTapeSymbol('B');
    read_head = head;

    state_ht = create_hashtable(10);
    state_stack = create_stack();
    event_queue = create_queue();

    hashtable_put(state_ht, "q0", "Initial state");
    hashtable_put(state_ht, "qhalt", "Final state");
    stack_push(state_stack, "State pushed to stack");
    enqueue(event_queue, "Event queued");
}

void drawText(float x, float y, const char *text) {
    glRasterPos2f(x, y);
    for (int i = 0; text[i]; i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
}

void drawCenteredText(float y, const char *text) {
    int width = 0;
    for (int i = 0; text[i]; i++)
        width += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, text[i]);
    float normX = -((float)width / 1000.0f);
    glRasterPos2f(normX, y);
    for (int i = 0; text[i]; i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
}

void drawTape() {
    TapeNode *temp = head;
    float x = -0.95f;
    while (temp) {
        glColor3f(1, 1, 1);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, -0.2f); glVertex2f(x + BOX_WIDTH, -0.2f);
        glVertex2f(x + BOX_WIDTH, 0.2f); glVertex2f(x, 0.2f);
        glEnd();

        if (temp->symbol == 'B') glColor3f(1, 0, 0);
        else if (temp->symbol == 'C') glColor3f(1, 0, 1);
        else if (temp->symbol == 'X' || temp->symbol == 'Y') glColor3f(1, 1, 0);
        else glColor3f(0, 1, 0);

        drawText(x + 0.03f, 0.0f, (char[]){temp->symbol, '\0'});
        if (temp == read_head)
            drawText(x + 0.03f, 0.3f, "^");
        temp = temp->next;
        x += 0.09f;
    }

    glColor3f(1, 1, 0);
    char stateText[50];
    sprintf(stateText, "State Transformation: %s", current_state);
    drawText(-0.9f, 0.8f, stateText);
}

void logic_step() {
    if (!running || !read_head) return;

    stack_push(state_stack, current_state);

    if (strcmp(current_state, "q0") == 0) {
        if (read_head->symbol == 'B') {
            read_head = read_head->next;
            strcpy(current_state, "q1");
        }
    }
    else if (strcmp(current_state, "q1") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') {
            read_head = read_head->next;
        }
        else if (read_head->symbol == 'B') {
            read_head->symbol = 'C';
            strcpy(current_state, "q2");
        }
    }
    else if (strcmp(current_state, "q2") == 0) {
        if (read_head->symbol != 'B') {
            read_head = read_head->prev;
        } else {
            read_head = read_head->next;
            strcpy(current_state, "q3");
        }
    }
    else if (strcmp(current_state, "q3") == 0) {
        if (read_head->symbol == '1') {
            read_head->symbol = 'X';
            marked_symbol = '1';
            read_head = read_head->next;
            strcpy(current_state, "q4");
        }
        else if (read_head->symbol == '0') {
            read_head->symbol = 'Y';
            marked_symbol = '0';
            read_head = read_head->next;
            strcpy(current_state, "q4");
        }
        else if (read_head->symbol == 'C') {
            strcpy(current_state, "q9");
        }
    }
    else if (strcmp(current_state, "q4") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') {
            read_head = read_head->next;
        }
        else if (read_head->symbol == 'C') {
            read_head = read_head->next;
            strcpy(current_state, "q5");
        }
    }
    else if (strcmp(current_state, "q5") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') {
            read_head = read_head->next;
        }
        else if (read_head->symbol == 'B') {
            read_head->symbol = marked_symbol;
            read_head = read_head->prev;
            strcpy(current_state, "q6");
        }
    }
    else if (strcmp(current_state, "q6") == 0) {
        if (read_head->symbol != 'C') {
            read_head = read_head->prev;
        }
        else {
            read_head = read_head->prev;
            strcpy(current_state, "q7");
        }
    }
    else if (strcmp(current_state, "q7") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') {
            read_head = read_head->prev;
        }
        else if (read_head->symbol == 'X' || read_head->symbol == 'Y') {
            read_head = read_head->next;
            strcpy(current_state, "q3");
        }
    }
    else if (strcmp(current_state, "q9") == 0) {
        read_head = read_head->prev;
        strcpy(current_state, "q10");
    }
    else if (strcmp(current_state, "q10") == 0) {
        if (read_head->symbol == 'X') {
            read_head->symbol = '1';
            read_head = read_head->prev;
        }
        else if (read_head->symbol == 'Y') {
            read_head->symbol = '0';
            read_head = read_head->prev;
        }
        else if (read_head->symbol == 'C') {
            read_head = read_head->prev;
        }
        else if (read_head->symbol == 'B') {
            strcpy(current_state, "qhalt");
            running = 0;
            showDiagram = 1;
            enqueue(event_queue, "Machine halted");
        }
    }
}

void timer(int v) {
    if (running) logic_step();
    glutPostRedisplay();
    if (running) glutTimerFunc(500, timer, 0);
}

void mouse(int btn, int state, int x, int y) {
    if (btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN && !running) {
        running = 1;
        glutTimerFunc(500, timer, 0);
        enqueue(event_queue, "Machine started");
    }
}

void initGL() {
    glClearColor(0, 0, 0.2, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1, 1, 1);
    drawCenteredText(0.9f, "Turing Machine: Binary Copy Simulation");
    drawTape();
    glutSwapBuffers();
}

int main(int argc, char **argv) {
    int num;
    printf("Enter decimal number: ");
    scanf("%d", &num);

    dec_to_binary(num);
    initTape();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    glutInitWindowSize(1000, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Turing Machine Copy Binary");
    initGL();
    glutDisplayFunc(display);
    glutMouseFunc(mouse);

    glutInitWindowSize(500, 500);
    glutInitWindowPosition(1120, 100);
    imageWindow = glutCreateWindow("Reference Diagram");
    initGL();
    loadImageTexture("p1.png");
    glutDisplayFunc(displayImageWindow);

    glutMainLoop();

    if (state_stack) free(state_stack);
    if (event_queue) free(event_queue);

    return 0;
}
