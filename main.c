#include <GL/glut.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TAPE 100

char tape[MAX_TAPE];
char binary[32];
int input_len = 0;
int copied = 0;
int start_index;

int head_read = 1;
int head_write = 0;

char state[10] = "q0";
int running = 1;
char transition_info[50] = "";

// Convert decimal to binary string
void dec_to_binary(int n) {
    int i = 0;
    while (n > 0) {
        binary[i++] = (n % 2) + '0';
        n /= 2;
    }
    binary[i] = '\0';

    // Reverse binary to get correct order
    for (int j = 0; j < i / 2; j++) {
        char tmp = binary[j];
        binary[j] = binary[i - j - 1];
        binary[i - j - 1] = tmp;
    }

    input_len = i;
}

// Initialize tape with binary and C marker
void init_tape() {
    for (int i = 0; i < MAX_TAPE; i++) tape[i] = 'B';

    start_index = 2;
    for (int i = 0; i < input_len; i++)
        tape[start_index + i] = binary[i];

    tape[start_index + input_len] = 'C';

    head_read = start_index;
    head_write = start_index + input_len + 1;
}

// Draw text using GLUT
void drawText(float x, float y, const char *text) {
    glRasterPos2f(x, y);
    for (int i = 0; text[i]; i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
}

// Draw the tape, boxes, and heads
void drawTape() {
    for (int i = 0; i < 21; i++) {
        int tapeIndex = start_index - 2 + i;
        float x = -0.95f + i * 0.09f;

        // Draw tape box
        glColor3f(1, 1, 1);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, -0.2f);
        glVertex2f(x + 0.08f, -0.2f);
        glVertex2f(x + 0.08f, 0.2f);
        glVertex2f(x, 0.2f);
        glEnd();

        // Draw tape symbol
        char c = tape[tapeIndex];
        if (c == 'B') {
            glColor3f(1, 0, 0); // Red for blank
        } else if (c == 'C') {
            glColor3f(1, 0, 1); // Magenta for marker
        } else {
            glColor3f(0, 1, 0); // Green for binary digits
        }
        drawText(x + 0.03f, 0.0f, (char[]){c, '\0'});

        // Draw read head
        if (tapeIndex == head_read && strcmp(state, "q1") == 0) {
            glColor3f(1, 0, 0);
            drawText(x + 0.025f, 0.3f, "R");
        }

        // Draw write head
        if (tapeIndex == head_write && strcmp(state, "q1") == 0) {
            glColor3f(0, 1, 0);
            drawText(x + 0.025f, 0.4f, "W");
        }

        // During scan to C
        if (tapeIndex == head_read && strcmp(state, "q0") == 0) {
            glColor3f(1, 1, 0);
            drawText(x + 0.03f, 0.3f, "^");
        }
    }

    // Show state and transition info
    glColor3f(1, 1, 0);
    char stateLabel[30];
    sprintf(stateLabel, "State: %s", state);
    drawText(-0.9f, 0.8f, stateLabel);
    drawText(-0.9f, 0.7f, transition_info);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawTape();
    glutSwapBuffers();
}

// Turing machine logic step
void logic_step() {
    if (!running) return;

    if (strcmp(state, "q0") == 0) {
        if (tape[head_read] == '0' || tape[head_read] == '1') {
            head_read++;
            strcpy(transition_info, "q0: scanning to marker 'C'");
        } else if (tape[head_read] == 'C') {
            strcpy(state, "q1");
            head_read = start_index;
            head_write = start_index + input_len + 1;
            copied = 0;
            strcpy(transition_info, "q0->q1: found 'C', start copying");
        }
    } else if (strcmp(state, "q1") == 0) {
        if (copied < input_len) {
            char symbol = tape[head_read];
            tape[head_write] = symbol;
            head_read++;
            head_write++;
            copied++;
            sprintf(transition_info, "q1: copying '%c' -> tape", symbol);
        } else {
            strcpy(state, "q_accept");
            strcpy(transition_info, "q1->q_accept: done copying");
            running = 0;
        }
    }
}

// Timer to auto-step every 500ms
void timer(int value) {
    logic_step();
    glutPostRedisplay();
    if (running) glutTimerFunc(500, timer, 0);
}

void initGL() {
    glClearColor(0, 0, 0.2, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);
}

int main(int argc, char **argv) {
    int decimal;
    printf("Enter a decimal number: ");
    scanf("%d", &decimal);

    dec_to_binary(decimal);
    init_tape();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1000, 400);
    glutCreateWindow("Turing Machine Copy Visualization");
    initGL();

    glutDisplayFunc(display);
    glutTimerFunc(500, timer, 0);
    glutMainLoop();
    return 0;
}
