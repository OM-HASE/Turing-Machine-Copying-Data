#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOX_WIDTH 0.08f
#define MAX_TRANSITIONS 100

// ------------------ Data Structures ------------------
// Linked List Node for Tape
typedef struct TapeNode {
    char symbol;
    struct TapeNode *prev, *next;
} TapeNode;

// Stack for undo history
char stack[100];
int top = -1;

void push(char c) { stack[++top] = c; }
char pop() { return (top >= 0) ? stack[top--] : 'B'; }

// Queue for transition steps
char *queue[100];
int front = 0, rear = 0;

void enqueue(char *msg) { queue[rear++] = msg; }
char *dequeue() { return (front < rear) ? queue[front++] : ""; }

// Hash map replacement: basic rule mapping
typedef struct {
    char state[10];
    char read;
    char write;
    char next_state[10];
    char move; // 'L' or 'R'
} Rule;

Rule transition_table[MAX_TRANSITIONS];
int rule_count = 0;

// Set for visited states
char visited_states[10][10];
int visited_count = 0;

int isVisited(char *state) {
    for (int i = 0; i < visited_count; i++)
        if (strcmp(visited_states[i], state) == 0) return 1;
    return 0;
}

void markVisited(char *state) {
    if (!isVisited(state)) {
        strcpy(visited_states[visited_count++], state);
    }
}

// ------------------ Turing Machine ------------------
TapeNode *head = NULL, *tail = NULL;
TapeNode *read_head = NULL, *write_head = NULL;

char binary[32];
int binary_len = 0;
char current_state[10] = "q0";
int copying = 0;
int running = 0;

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

void addTapeSymbol(char symbol) {
    TapeNode *node = malloc(sizeof(TapeNode));
    node->symbol = symbol;
    node->next = NULL;
    node->prev = tail;

    if (tail) tail->next = node;
    else head = node;
    tail = node;
}

void initTape() {
    for (int i = 0; i < 2; i++) addTapeSymbol('B');
    for (int i = 0; i < binary_len; i++) addTapeSymbol(binary[i]);
    addTapeSymbol('C');
    for (int i = 0; i < binary_len + 4; i++) addTapeSymbol('B');

    read_head = head->next->next; // pointing to first binary
    write_head = read_head;
    for (int i = 0; i < binary_len + 1; i++) write_head = write_head->next;
}

// ------------------ OpenGL Drawing ------------------
void drawText(float x, float y, const char *text) {
    glRasterPos2f(x, y);
    for (int i = 0; text[i]; i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
}

void drawTape() {
    TapeNode *temp = head;
    float x = -0.95f;
    while (temp) {
        glColor3f(1, 1, 1);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, -0.2f);
        glVertex2f(x + BOX_WIDTH, -0.2f);
        glVertex2f(x + BOX_WIDTH, 0.2f);
        glVertex2f(x, 0.2f);
        glEnd();

        // Color based on symbol
        if (temp->symbol == 'B') glColor3f(1, 0, 0);
        else if (temp->symbol == 'C') glColor3f(1, 0, 1);
        else glColor3f(0, 1, 0);

        drawText(x + 0.03f, 0.0f, (char[]){temp->symbol, '\0'});

        if (temp == read_head && strcmp(current_state, "q1") == 0)
            drawText(x + 0.03f, 0.3f, "R");
        if (temp == write_head && strcmp(current_state, "q1") == 0)
            drawText(x + 0.03f, 0.4f, "W");
        if (temp == read_head && strcmp(current_state, "q0") == 0)
            drawText(x + 0.03f, 0.3f, "^");

        temp = temp->next;
        x += 0.09f;
    }

    glColor3f(1, 1, 0);
    drawText(-0.9f, 0.8f, current_state);
    drawText(-0.9f, 0.7f, dequeue());
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawTape();
    glutSwapBuffers();
}

void logic_step() {
    if (!running) return;
    markVisited(current_state);

    if (strcmp(current_state, "q0") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') {
            enqueue("q0: scanning to 'C'");
            read_head = read_head->next;
        } else if (read_head->symbol == 'C') {
            strcpy(current_state, "q1");
            read_head = head->next->next;
            write_head = read_head;
            for (int i = 0; i < binary_len + 1; i++) write_head = write_head->next;
            enqueue("q0->q1: found C, start copying");
            copying = 0;
        }
    } else if (strcmp(current_state, "q1") == 0) {
        if (copying < binary_len) {
            write_head->symbol = read_head->symbol;
            push(read_head->symbol);
            enqueue("q1: copying");
            read_head = read_head->next;
            write_head = write_head->next;
            copying++;
        } else {
            strcpy(current_state, "q_accept");
            enqueue("q1->q_accept: Done");
            running = 0;
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
    }
}

void initGL() {
    glClearColor(0, 0, 0.2, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);
}

int main(int argc, char **argv) {
    int num;
    printf("Enter decimal number: ");
    scanf("%d", &num);

    dec_to_binary(num);
    initTape();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1000, 400);
    glutCreateWindow("Turing Machine with DSA");

    initGL();
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}
