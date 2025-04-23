#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

// ------------------ State Diagram Data ------------------
typedef struct {
    char name[10];
    float x, y;
} StateNode;

typedef struct {
    int from, to;
    const char *label;
} Transition;

StateNode states[] = {
    {"q0", -0.9f, -0.6f}, {"q0b", -0.6f, -0.6f}, {"q1", -0.3f, -0.6f},
    {"q2", 0.0f, -0.6f}, {"q3", 0.3f, -0.6f}, {"q4", 0.6f, -0.6f},
    {"q5", 0.9f, -0.6f}, {"q8", -0.3f, -0.9f}, {"q9", 0.0f, -0.9f}, {"qhalt", 0.3f, -0.9f}
};

Transition transitions[] = {
    {0, 1, "B"}, {1, 2, "B/C"}, {2, 3, "1/X,0/Y"}, {3, 4, "C"}, {4, 5, "B/write"},
    {5, 2, "X,Y"}, {2, 6, "C"}, {6, 2, "back"}, {2, 7, "C"}, {7, 8, "left"},
    {8, 9, "X/1, Y/0"}, {9, 9, "←"}, {9, 10, "B/halt"}
};

// ------------------ Utility Drawing Functions ------------------
void drawText(float x, float y, const char *text) {
    glRasterPos2f(x, y);
    for (int i = 0; text[i]; i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
}

void drawCircle(float cx, float cy, float r) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 100; i++) {
        float theta = 2.0f * 3.1415926f * i / 100;
        glVertex2f(cx + r * cosf(theta), cy + r * sinf(theta));
    }
    glEnd();
}

void drawArrow(float x1, float y1, float x2, float y2, const char *label) {
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();

    float angle = atan2f(y2 - y1, x2 - x1);
    float arrowSize = 0.02f;
    float ax = x2 - arrowSize * cosf(angle - 0.3f);
    float ay = y2 - arrowSize * sinf(angle - 0.3f);
    float bx = x2 - arrowSize * cosf(angle + 0.3f);
    float by = y2 - arrowSize * sinf(angle + 0.3f);

    glBegin(GL_TRIANGLES);
    glVertex2f(x2, y2);
    glVertex2f(ax, ay);
    glVertex2f(bx, by);
    glEnd();

    drawText((x1 + x2) / 2, (y1 + y2) / 2 + 0.03f, label);
}

void drawSelfLoop(float cx, float cy, const char *label) {
    float radius = 0.07f;
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 20; i++) {
        float angle = M_PI / 4 + (M_PI / 2) * i / 20;
        glVertex2f(cx + radius * cosf(angle), cy + radius * sinf(angle));
    }
    glEnd();

    float angle = 3 * M_PI / 4;
    float x2 = cx + radius * cosf(angle);
    float y2 = cy + radius * sinf(angle);
    float arrowSize = 0.015f;

    float ax = x2 - arrowSize * cosf(angle - 0.4f);
    float ay = y2 - arrowSize * sinf(angle - 0.4f);
    float bx = x2 - arrowSize * cosf(angle + 0.4f);
    float by = y2 - arrowSize * sinf(angle + 0.4f);

    glBegin(GL_TRIANGLES);
    glVertex2f(x2, y2);
    glVertex2f(ax, ay);
    glVertex2f(bx, by);
    glEnd();

    drawText(cx - 0.03f, cy + 0.11f, label);
}

void drawStateDiagram() {
    glColor3f(1, 1, 1);
    for (int i = 0; i < sizeof(states) / sizeof(StateNode); i++) {
        drawCircle(states[i].x, states[i].y, 0.05f);
        drawText(states[i].x - 0.02f, states[i].y - 0.01f, states[i].name);
    }
    for (int i = 0; i < sizeof(transitions) / sizeof(Transition); i++) {
        StateNode from = states[transitions[i].from];
        StateNode to = states[transitions[i].to];
        if (transitions[i].from == transitions[i].to)
            drawSelfLoop(from.x, from.y, transitions[i].label);
        else
            drawArrow(from.x, from.y, to.x, to.y, transitions[i].label);
    }
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

// ------------------ Turing Machine Logic ------------------
void logic_step() {
    if (!running || !read_head) return;

    if (strcmp(current_state, "q0") == 0) {
        if (read_head->symbol == 'B') { read_head = read_head->next; strcpy(current_state, "q0b"); }
    } else if (strcmp(current_state, "q0b") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') read_head = read_head->next;
        else if (read_head->symbol == 'B') { read_head->symbol = 'C'; read_head = head; strcpy(current_state, "q1"); }
    } else if (strcmp(current_state, "q1") == 0) {
        if (read_head->symbol == 'B') read_head = read_head->next;
        else if (read_head->symbol == '1') { read_head->symbol = 'X'; marked_symbol = '1'; read_head = read_head->next; strcpy(current_state, "q2"); }
        else if (read_head->symbol == '0') { read_head->symbol = 'Y'; marked_symbol = '0'; read_head = read_head->next; strcpy(current_state, "q2"); }
        else if (read_head->symbol == 'C') strcpy(current_state, "q8");
    } else if (strcmp(current_state, "q2") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') read_head = read_head->next;
        else if (read_head->symbol == 'C') { read_head = read_head->next; strcpy(current_state, "q3"); }
    } else if (strcmp(current_state, "q3") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') read_head = read_head->next;
        else if (read_head->symbol == 'B') { read_head->symbol = marked_symbol; read_head = read_head->prev; strcpy(current_state, "q4"); }
    } else if (strcmp(current_state, "q4") == 0) {
        if (read_head->symbol != 'C') read_head = read_head->prev;
        else { read_head = read_head->prev; strcpy(current_state, "q5"); }
    } else if (strcmp(current_state, "q5") == 0) {
        if (read_head->symbol == '0' || read_head->symbol == '1') read_head = read_head->prev;
        else if (read_head->symbol == 'X' || read_head->symbol == 'Y') { read_head = read_head->next; strcpy(current_state, "q1"); }
    } else if (strcmp(current_state, "q8") == 0) {
        read_head = read_head->prev; strcpy(current_state, "q9");
    } else if (strcmp(current_state, "q9") == 0) {
        if (read_head->symbol == 'X') { read_head->symbol = '1'; read_head = read_head->prev; }
        else if (read_head->symbol == 'Y') { read_head->symbol = '0'; read_head = read_head->prev; }
        else if (read_head->symbol == 'C') read_head = read_head->prev;
        else if (read_head->symbol == 'B') { strcpy(current_state, "qhalt"); running = 0; showDiagram = 1; }
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
    glColor3f(1, 1, 1);
    drawCenteredText(0.9f, "Turing Machine: Binary Copy Simulation");
    drawTape();
    if (showDiagram) drawStateDiagram();
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
