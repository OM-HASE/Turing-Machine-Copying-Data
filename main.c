#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BOX_WIDTH 0.08f

// ------------------ Data Structures ------------------
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
char marked_symbol = '\0'; // New: stores the last marked symbol ('1' or '0')

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
}

// ------------------ Drawing ------------------
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
    drawText(-0.9f, 0.8f, current_state);
}

// ------------------ Turing Machine Logic ------------------
void logic_step() {
    if (!running || !read_head) return;

    if (strcmp(current_state, "q0") == 0) {
        if (read_head->symbol == 'B') {
            read_head = read_head->next;
            strcpy(current_state, "q0b");  // Moved past the first 'B'
        }
    }
    else if (strcmp(current_state, "q0b") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') {
            read_head = read_head->next;
        } else if (read_head->symbol == 'B') {
            // This is the second 'B'
            read_head->symbol = 'C';
            read_head = head;  // Reset head for copy phase
            strcpy(current_state, "q1");
        }
    }
    else if (strcmp(current_state, "q1") == 0) {
        if (read_head->symbol == 'B') {
            read_head = read_head->next;
        } else if (read_head->symbol == '1') {
            read_head->symbol = 'X';
            marked_symbol = '1'; // Remember marked symbol
            read_head = read_head->next;
            strcpy(current_state, "q2");
        } else if (read_head->symbol == '0') {
            read_head->symbol = 'Y';
            marked_symbol = '0'; // Remember marked symbol
            read_head = read_head->next;
            strcpy(current_state, "q2");
        } else if (read_head->symbol == 'C') {
            strcpy(current_state, "q8");
        }
    }
    else if (strcmp(current_state, "q2") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') {
            read_head = read_head->next;
        } else if (read_head->symbol == 'C') {
            read_head = read_head->next;
            strcpy(current_state, "q3");
        }
    }
    else if (strcmp(current_state, "q3") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') {
            read_head = read_head->next;
        } else if (read_head->symbol == 'B') {
            read_head->symbol = marked_symbol;  // Use stored mark
            read_head = read_head->prev;
            strcpy(current_state, "q4");
        }
    }
    else if (strcmp(current_state, "q4") == 0) {
        if (read_head->symbol != 'C') {
            read_head = read_head->prev;
        } else {
            read_head = read_head->prev;
            strcpy(current_state, "q5");
        }
    }
    else if (strcmp(current_state, "q5") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') {
            read_head = read_head->prev;
        } else if (read_head->symbol == 'X' || read_head->symbol == 'Y') {
            read_head = read_head->next;
            strcpy(current_state, "q1");
        }
    }
    else if (strcmp(current_state, "q8") == 0) {
        read_head = read_head->prev;
        strcpy(current_state, "q9");
    }
    else if (strcmp(current_state, "q9") == 0) {
        if (read_head->symbol == 'X') {
            read_head->symbol = '1';
            read_head = read_head->prev;
        } else if (read_head->symbol == 'Y') {
            read_head->symbol = '0';
            read_head = read_head->prev;
        } else if (read_head->symbol == 'C') {
            read_head = read_head->prev;
        } else if (read_head->symbol == 'B') {
            strcpy(current_state, "qhalt");
            running = 0;
        }
    }
}

// ------------------ GLUT Setup ------------------
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
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
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
    glutCreateWindow("Turing Machine Copy Binary");

    initGL();
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}
