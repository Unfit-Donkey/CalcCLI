#include "src/general.h"
#include "src/arb.h"
#include "src/compute.h"
#include "src/help.h"
#include "src/functions.h"
#include "src/parser.h"
#include "src/misc.h"
#include <stdarg.h>
#include <time.h>
#if defined __linux__ || defined unix || defined __unix__ || defined __APPLE__
#include <signal.h>
#define USE_TERMIOS_H
#endif
#if defined __WIN32 || defined _WIN64 || defined _WIN32
#define USE_CONIO_H
#endif
char* currentInput = NULL;
void runLine(char* input);
#pragma region Command Line Arguments
double useColors = 1;
double rawMode = 0;
double showHelp = 0;
struct CommandArg {
    const char* name;
    double* location;
    int argvPos;
} commands[] = {
    {"--color",&useColors,0},
    {"--raw",&rawMode,0},
    {"--help",&showHelp,0},
    {"-h",&showHelp,0},
};
void showUsage(char** argv) {
    char* type;
    if(commands[2].argvPos != 0) type = argv[commands[2].argvPos + 1];
    if(commands[3].argvPos != 0) type = argv[commands[3].argvPos + 1];
    if(type == NULL || type[0] == 0) {
        printf("Calc is a command line calculator written in C\nTry running:\n\tcalc -h commands\n\tcalc -h functions\n\tcalc -h features\n\tcalc -h args\n\tor calc -h info\nFor a more detailed help page, visit https://unfit-donkey.github.io/CalcForJS.\n");
    }
    else if(memcmp(type, "args", 4) == 0) {
        printf("Command line arguments:\n\t--color=x\tWhether to enable colors (0 is disable)\n\t--raw\t\tEnable raw mode\n\t--help\t\tShow help messages\n");
    }
    else if(memcmp(type, "command", 7) == 0) {
        printf("Commands:\n\t-def name(a,b)=[exp]\tDefine a function with arguments a and b\n\t-del name\tDelete the function named name\n\t-g [exp]\tGraph the expression when it comes to x\n\t-dx [exp]\tFind the derivative of the function along x\n\t-f [FILE]\tOpen the file and run every line\n\t-ls\tList all user-defined functions\n\t-quit\tExit the program (Ctrl-C also works)\n");
    }
    else if(memcmp(type, "functions", 8) == 0) {
        printf("Built-in functions:\ni            \tThe imaginary number\nadd(a,b)    \tAddition (also +)\nmult(a,b)   \tMultiplication (also *)\ndiv(a,b)    \tDivision (also /)\nmod(a,b)    \tModulo (also %%)\npow(a,b)    \tPower (also ^)\nneg(a)      \tNegate (also -)\nsubtract(a,b)\tSubtraction (also -)\nmultneg(a,b)\tNegative multiplication (also *-)\ndivneg(a,b) \tNegative divide (also /-)\nmodneg(a,b) \tNegative modulo (also %%-)\npowneg(a,b) \tNegative power (also ^-)\n\nsin(x)      \tSine\ncos(x)      \tCosine\ntan(x)      \tTangent\ncsc(x)      \tCosecant (reciprocal sine)\nsec(x)      \tSecant (reciprocal cosine)\ncot(x)      \tCotangent (reciprocal tangent)\nasin(x)     \tInverse sine\nacos(x)     \tInverse cosine(x)\natan(x)     \tInverse tangent\nacsc(x)\tInverse cosecant\nasec(x)      \tInverse secant\nacot(x)     \tInverse cotangent\n\nln(x)        \tNatural log\nlogten(x)   \tLog base 10\nlog(x,b)    \tShorthand for ln(x)/ln(b)\nround(x)    \tNearest integer\nfloor(x)    \tRound down\nceil(x)     \tRound up\nsgn(x)      \tSign\nabs(x)      \tAbsolute value\narg(x)      \tComplex argument\ngetr(x)     \tReal component\ngeti(x)     \tImaginary component\n\nnot(x)      \tBinary not\nand(a,b)    \tBinary and\nor(a,b)     \tBinary or\nxor(a,b)    \tBinary exclusive-or\nls(a,b)     \tBinary leftshift\nrs(a,b)     \tBinary rightshift\n\npi          \tThe circle constant\nphi         \tThe golden ratio\ne           \tEuler's constant\nans         \tPrevious answer\nhist(x)     \tGet xth result from history\nhistnum     \tNumber of items in history\n");
    }
    else if(memcmp(type, "info", 4) == 0) {
        printf("CalcCLI is a command line program witten by Benjamin Cates in C. More info can be found at https://github.com/unfit-donkey/CalcCLI. A web version is hosted at https://unfit-donkey.github.io/CalcForJS.\n");
    }
    else if(memcmp(type, "features", 7) == 0) {
        printf("Calc currently has these features:\n\tFunctions\n\tCustom functions\n\tHistory\n\tCLI graphing\n\tComplex number support\n\tBinary operation support\n\tDerivatives\n\tBase convertion\n");
    }
    else {
        printf("Could not find help type '%s'", type);
    }
    exit(0);
}
#pragma endregion
#pragma region Preferences
char preferencePath[1000];
int hasPreferencePath = false;
#if defined USE_CONIO_H
void generatePreferencePath() {
    hasPreferencePath = true;
    snprintf(preferencePath, 1000, "%s\\AppData\\Roaming\\CalcCLI\\preferences.conf", getenv("USERPROFILE"));
}
#else
void generatePreferencePath() {
    hasPreferencePath = true;
    snprintf(preferencePath, 1000, "%s/.config/calccli.conf", getenv("HOME"));
}
#endif
const bool allowedPreferences[preferenceCount] = { 1,0,1,1 };
void savePreferences() {
    //Generate preference path if it doesn't exists
    if(!hasPreferencePath) generatePreferencePath();
    int preferenceLen = sizeof(preferences) / sizeof(struct Preference);
    //Check if file exists
    FILE* testRead = fopen(preferencePath, "r");
    if(testRead == NULL) printf("Notice: creating preferences file at %s\n", preferencePath);
    else fclose(testRead);
    FILE* f = fopen(preferencePath, "w");
    if(f == NULL) {
        error("could not create preferences file");
        globalError = false;
        return;
    }
    for(int i = 0;i < preferenceLen;i++) if(allowedPreferences[i]) {
        char* val = valueToString(preferences[i].current, 10);
        fprintf(f, "%s=%s\n", preferences[i].name, val);
        free(val);
    }
    fclose(f);
}
void loadPreferences() {
    //Generate preference path if it doesn't exists
    if(!hasPreferencePath) generatePreferencePath();
    char line[10000];
    FILE* f = fopen(preferencePath, "r");
    //Create default preferences file if it doesn't exist
    if(f == NULL) {
        printf("Preference file missing, creating new one\n");
        savePreferences();
        return;
    }
    int lineCount = 0;
    //Iterate over each line
    while(fgets(line, 10000, f) != NULL) {
        lineCount++;
        int eqPos = 0;
        //If line is empty or comment
        if(line[0] == '#' || line[0] == 0 || line[0] == '\n') continue;
        //Find equal sign position
        while(line[eqPos] != '=' && line[eqPos] != 0) eqPos++;
        if(line[eqPos] == 0) {
            error("missing assignment in line %d of preferences", lineCount);
            globalError = false;
            continue;
        }
        //Remove final newline
        for(int i = 0;line[i];i++) if(line[i] == '\n') line[i] = 0;
        //Calculate value
        Value val = calculate(line + eqPos + 1, 10);
        if(globalError) {
            printf("in line %d of preferences\n", lineCount);
            globalError = false;
            continue;
        }
        //Set preference
        line[eqPos] = 0;
        if(setPreference(line, val, false) == 0) {
            freeValue(val);
            error("No preference named %s (line %d)", line, lineCount);
            globalError = false;
            continue;
        }
    }
}
void updatePreference(int id) {
    Value val = preferences[id].current;
    if(id == 0) useColors = getR(val);
    if(id == 2) rawMode = getR(val);
}
void readAutostart() {
    if(preferences[3].current.type != value_string) return;
    char* string = preferences[3].current.string;
    FILE* f = fopen(string, "r");
    if(f == NULL) {
        error("could not find autostart file");
        globalError = false;
        return;
    }
    char line[1000];
    int lineCount = 0;
    while(fgets(line, 1000, f) != NULL) {
        lineCount++;
        for(int i = 0;line[i];i++) if(line[i] == '\n') line[i] = 0;
        runLine(line);
        if(globalError) {
            printf("in line %d of autostart file\n", lineCount);
            globalError = false;
        }
    }
}
#pragma endregion
void CLI_cleanup() {
    free(currentInput);
    cleanup();
    if(useColors) printf("\b\b\33[34;1m-quit\33[0m\n");
    else printf("\b\b-quit\n");
    exit(0);
}
#pragma region Printing
bool prevWasNewline = true;
void printWithHighlighting(char* str) {
    const char* colorCodes[] = {
        "0;0","37;1","33","32","31","0","30;1","33;1","34;1","0","30;1","0","31","31","33;1","35;1","36;1","36","36","35;1","36",
    };
    int strLen = strlen(str);
    char* colors = highlightLine(str);
    int prevColor = 0;
    printf("\033[0m");
    for(int i = 0;i < strLen;i++) {
        if(prevColor != colors[i]) {
            prevColor = colors[i];
            printf("\033[0m\033[%sm", colorCodes[prevColor]);
        }
        putchar(str[i]);
    }
    printf("\033[0m");
    free(colors);
}
void printHTML(char* content) {
    int contentLen = strlen(content);
    for(int i = 0;i < contentLen;i++) {
        if(content[i] == '<') {
            //Ignore ending tags
            if(content[i + 1] == '/') {
                while(content[i] != '>' && content[i] != 0) i++;
                continue;
            }
            int start = i;
            char tagName[20];
            while(content[i] != ' ' && content[i] != '>' && content[i] != 0) i++;
            memcpy(tagName, content + start + 1, i - start - 1);
            tagName[i - start - 1] = 0;
            if(strcmp(tagName, "br") == 0 || strcmp(tagName, "br/") == 0) {
                putchar('\n');
                continue;
            }
            while(content[i] != '>' && content[i] != 0) i++;
            //Continue if self closing bracket
            if(tagName[start - i - 1] == '/') continue;
            // | start     | i      | endTag
            // < tag       >        </tag>
            int endTag = i;
            int bracket = 0;
            while(true) {
                if(content[endTag] == 0) break;
                if(bracket == 0 && content[endTag] == '<' && content[endTag + 1] == '/') break;
                if(content[endTag] == '<') {
                    if(content[endTag + 1] == '/') bracket--;
                    else bracket++;
                }
                endTag++;
            }
            int len = i - endTag - 1;
            char* inside = content + i + 1;
            char oldContent = content[endTag];
            if(content[endTag] != 0) content[endTag] = 0;
            if(strcmp(tagName, "syntax") == 0) {
                removeHTML(inside);
                //Ignore tags within, then print the syntax highlighted version<tab><tab>
                printWithHighlighting(inside);
            }
            else if(strcmp(tagName, "table") == 0) {
                putchar('\n');
                printHTML(inside);
            }
            else if(strcmp(tagName, "td") == 0) {
                printHTML(inside);
                putchar('\t');
            }
            else if(strcmp(tagName, "tr") == 0) {
                printHTML(inside);
                putchar('\n');
            }
            else if(strcmp(tagName, "h3") == 0 || strcmp(tagName, "h2") == 0) {
                putchar('\n');
                if(useColors) printf("\033[36;1m");
                printHTML(inside);
                if(useColors) printf("\033[0m");
                putchar('\n');
            }
            else if(strcmp(tagName, "ul") == 0) {
                putchar('\n');
                printHTML(inside);

            }
            else if(strcmp(tagName, "li") == 0) {
                if(useColors) printf("\033[34;1m");
                printf("- ");
                if(useColors) printf("\033[0m");
                printHTML(inside);
                putchar('\n');
            }
            else {
                printHTML(inside);
                // ignore a, help, em, and strong
            }
            if(oldContent != 0) content[endTag] = oldContent;
            i = endTag;
            while(content[i] != '>' && content[i] != 0) i++;
        }
        else if(content[i] == '&') {
            if(memcmp(content + i + 1, "nbsp;", 5) == 0) {
                putchar(' ');
                i += 5;
            }
            else if(memcmp(content + i + 1, "gt;", 3) == 0) {
                putchar('>');
                i += 3;
            }
            else if(memcmp(content + i + 1, "lt;", 3) == 0) {
                putchar('<');
                i += 3;
            }
            else putchar(content[i]);
        }
        else putchar(content[i]);
    }
}
void printHelpPage(struct HelpPage page) {
    if(useColors) printf("\033[32;1m");
    printf("%s", page.name);
    if(useColors) printf("\033[0m");
    if(page.symbol) printf(" - %s", page.symbol);
    printf("\n");
    //Generated help pages
    if(page.type == 8) {
        char* content = getGeneratedPage(page);
        printHTML(content);
        putchar('\n');
        free(content);
    }
    else {
        char contentCopy[strlen(page.content) + 1];
        strcpy(contentCopy, page.content);
        printHTML(contentCopy);
        printf("\n");
    }
}
void printString(Value string) {
    printf("%s", string.string);
    if(string.string[strlen(string.string) - 1] != '\n') prevWasNewline = false;
}
void confirmNewLine() {
    if(!prevWasNewline) {
        prevWasNewline = true;
        printf("\n");
    }
}
#pragma endregion
#pragma region Stdin Reading
char* readLineRaw() {
    char* out = calloc(10, 1);
    if(out == NULL) { error(mallocError);return NULL; }
    int outLen = 10;
    int charPos = 0;
    while(true) {
        int character = getchar();
        if(character == -1) {
            free(out);
            return NULL;
        }
        if(character == '\n') return out;
        if(charPos == outLen) {
            out = realloc(out, (outLen += 10));
            if(out == NULL) { error(mallocError);return NULL; }
        }
        out[charPos++] = character;
    }
    return out;
}
int readCharacter();
#ifdef USE_TERMIOS_H
#define USE_FANCY_INPUT
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
int getTermWidth() {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return ws.ws_col;
}
struct termios orig_termios;
void disableUnixRawRead() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
void enableUnixRawRead() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableUnixRawRead);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
int readCharacter() {
    return getchar();
}
#elif defined USE_CONIO_H
#define USE_FANCY_INPUT
#include <conio.h>
#include <windows.h>
int getTermWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}
void enableRawMode() {

}
void disableRawMode() {
}
int readCharacter() {
    int out = getch();
    if(out == 3) CLI_cleanup();
    return out;
}
#else
int readCharacter() {
    return getchar();
}
void enableRawMode() {

}
void disableRawMode() {

}
#endif
#ifdef USE_FANCY_INPUT
void printInput(char* string, int cursorPos) {
    //Clear old input
    printf("\0338\033[J\r");
    //Print input
    if(useColors) printWithHighlighting(string);
    else printf("%s ", string);
    //Set cursor position
    if(cursorPos == -1) return;
    printf("\0338");
    int width = getTermWidth();
    if(cursorPos % width != 0) printf("\033[%dC", cursorPos % width);
    if(cursorPos > width - 1) printf("\033[%dB", cursorPos / width);
}
char* readLine(bool erasePrevious) {
    if(rawMode) return readLineRaw();
    char* input = calloc(11, 1);
    currentInput = input;
    if(input == NULL) { error(mallocError);return NULL; }
    int strLenAllocated = 10;
    int strLen = 0;
    int cursorPos = 0;
    int character = 0;
    int i;
    if(!erasePrevious) printf("\0337");
    while(1) {
        if(!erasePrevious) printInput(input, cursorPos);
        else {
            printf("\338");
            erasePrevious = false;
        }
        character = readCharacter();
        //Unix escape sequences
        if(character == 27) {
            int next = readCharacter();
            if(next == '[') {
                int next2 = readCharacter();
                //Left Arrow
                if(next2 == 'D') {
                    cursorPos = cursorPos - 1;
                    if(cursorPos == -1) cursorPos = 0;
                    continue;
                }
                //Right Arrow
                if(next2 == 'C') {
                    cursorPos++;
                    if(cursorPos > strLen) cursorPos--;
                    continue;
                }
                //END
                if(next2 == 'F') {
                    cursorPos = strLen;
                    continue;
                }
                //HOME
                if(next2 == 'H') {
                    cursorPos = 0;
                    continue;
                }
                //DELETE
                if(next2 == '3') {
                    readCharacter();
                    if(cursorPos == strLen) continue;
                    int i;
                    for(i = cursorPos;i < strLen;i++) {
                        input[i] = input[i + 1];
                    }
                    strLen--;
                    continue;
                }
            }
            continue;
        }
        //Windows escape sequences
        if(character == 0) {
            int next = readCharacter();
            //Left arrow
            if(next == 'K') {
                if(cursorPos != 0)
                    cursorPos--;
                continue;
            }
            //Right arrow
            if(next == 'M') {
                cursorPos++;
                if(cursorPos > strLen)
                    cursorPos--;
                continue;
            }
            //Up arrow is 'H'
            //Down arrow is 'P'
            //HOME
            if(next == 'G') {
                cursorPos = 0;
                continue;
            }
            //END
            if(next == 'O') {
                cursorPos = strLen;
                continue;
            }
            //DELETE
            if(next == 'S') {
                if(cursorPos == strLen) continue;
                int i;
                for(i = cursorPos;i < strLen;i++) {
                    input[i] = input[i + 1];
                }
                strLen--;
                continue;
            }
            continue;
        }
        //Ignore non-ascii utf-8 encodings
        if(character >= 128) {
            /*
                Basically 11110000 indicates a four-byte sequence, so ignore the next three
                11100000 indicates a three-byte sequence
                11000000 indicates a two-byte sequence
                10000000 is not a valid starting byte, ignore the character
            */
            if(character >= 0b11110000) readCharacter();
            if(character >= 0b11100000) readCharacter();
            if(character >= 0b11000000) readCharacter();
            continue;
        }
        if(character == 127 || character == 8) {
            if(cursorPos == 0)
                continue;
            cursorPos--;
            input[cursorPos] = '\0';
            for(i = cursorPos;i < strLen;i++) {
                input[i] = input[i + 1];
            }
            strLen--;
            continue;
        }
        else if(character == 127) continue;
        if(character == 10 || character == 13) break;
        if(character == 3) CLI_cleanup();
        if(character == 12) {
            printf("\33[2J\033[f\0337");
            continue;
        }
        if(character < 27) {
            //Control character info
            if(character == 'i' - 96) {
                printf("\nCtrl+I - Show info about control characters\nCtrl+D - Debugging codes\nCtrl+A - Delete all of the input\n\0337");
                continue;
            }
            //Delete All contents
            else if(character == 'a' - 96) {
                memset(input, 0, strLenAllocated + 1);
                cursorPos = 0;
                continue;
            }
            //Debugging
            else if(character == 'd' - 96) {
                printf("E - Echo current buffer contents\nP - Print cursor position\nL - Print buffer length\nDebugging code is: ");
                int ch = readCharacter();
                printf("%c\n\0337", ch);
                //Echo buffer contents
                if(ch == 'E' || ch == 'e') {
                    printf("Current buffer contents: ");
                    for(int j = 0;j < strLen;j++) printf("%d, ", (input[j] + 256) % 256);
                    printf("\n\0337");
                }
                //Position of the cursor
                else if(ch == 'P' || ch == 'p') {
                    printf("Current cursor position: %d\n\0337", cursorPos);
                }
                //Length of the buffer
                else if(ch == 'L' || ch == 'l') {
                    printf("Current length: %d, strlen lenght: %d\n\0337", strLen, strlen(input));
                }
                continue;
            }
            else {
                for(i = strLen;i > cursorPos - 1;i--) {
                    input[i + 1] = input[i];
                }
                input[cursorPos] = '^';
                strLen++;
                cursorPos++;
                character += 64;
            }
        }
        for(i = strLen;i > cursorPos - 1;i--) {
            input[i + 1] = input[i];
        }
        input[cursorPos] = character;
        strLen++;
        if(strLen == strLenAllocated) {
            strLenAllocated += 10;
            input = realloc(input, strLenAllocated + 1);
            if(input == NULL) { error(mallocError);return NULL; }
        }
        cursorPos++;
    }
    printInput(input, -1);
    printf("\n");
    return input;
}
#else
char* readLine(bool erasePrevious) {
    return readLineRaw();
};
#endif
#pragma endregion
void error(const char* format, ...) {
    if(ignoreError) return;
    //Print error
    confirmNewLine();
    if(useColors) printf("\033[1;31m");
    printf("Error: ");
    if(useColors) printf("\033[0m");
    char* dest[256];
    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);
    printf("\n");
    //Set error to true
    globalError = true;
};
void printPerformance(const char* type, clock_t t, int runCount) {
    //Print name
    printf("%s took ", type);
    //Print time
    float msTime = ((float)t) / CLOCKS_PER_SEC * 1000;
    if(msTime > 1000) printf("%f s", msTime / 1000.0);
    else printf("%f ms", msTime);
    //Print clock time and runCount
    printf(" (%d clock) to complete %d runs", t, runCount, msTime * 1000 / (float)runCount);
    //Print average
    float avg = msTime / (float)runCount;
    if(avg < 1.0) printf("(%f µs avg)", avg * 1000);
    else printf("(%f ms avg)", avg);
    //Print endline
    putchar('\n');
}
void graphEquation(const char* equation, double left, double right, double top, double bottom, int rows, int columns) {
    double columnWidth = (right - left) / columns;
    double rowHeight = (top - bottom) / rows;
    char** xArgName = calloc(2, sizeof(char*));
    if(xArgName == NULL) { error(mallocError);return; }
    xArgName[0] = calloc(2, 1);
    if(xArgName[0] == NULL) { error(mallocError);return; }
    xArgName[0][0] = 'x';
    Tree tree = generateTree(equation, xArgName, globalLocalVariables, 0);
    int i;
    Value x = NULLVAL;
    //Compute columns number of values
    double yvalues[columns + 1];
    for(i = 0; i <= columns; i++) {
        x.r = left + columnWidth * i;
        double out = computeTree(tree, &x, 1, globalLocalVariableValues).r;
        yvalues[i] = (out - bottom) / rowHeight;
    }
    //Fill text columns
    char text[columns][rows + 1];
    for(i = 0; i < columns; i++) {
        //set column to spaces
        memset(text[i], ' ', rows);
        double start = yvalues[i];
        double end = yvalues[i + 1];
        //how high start and end are within the box
        double startMod = fmod(yvalues[i], 1);
        double endMod = fmod(yvalues[i + 1], 1);
        // -, ", or _
        if(fabs(start - end) < 0.6) {
            if(start < rows && start > 0 && end < rows && end > 0) {
                double avg = (start + end) / 2;
                double avgMod = fmod(avg, 1);
                if(avgMod < 0.7 && avgMod > 0.3) {
                    text[i][(int)avg] = '-';
                    continue;
                }
                if(avgMod > 0.7)
                    text[i][(int)(avg)] = '"';
                else
                    text[i][(int)(avg)] = '_';
            }
            continue;
        }
        char slash = '\\';
        //if(end>start) swap end and start
        if(end > start) {
            double temp = end;
            end = start;
            start = temp;
            slash = '/';
            temp = startMod;
            startMod = endMod;
            endMod = temp;
        }
        //continue if out of range
        if(end > rows - 0.3 || start < 0.3) continue;
        //if dist is close, set character to \ or /
        double dist = start - end;
        if(dist < 1.2) {
            text[i][(int)(start - 0.3)] = slash;
            continue;
        }
        //else have two slashes filled with pipes in between
        int startBox = (int)(start - 0.3);
        if(startBox > rows - 1) startBox = rows - 1;
        int endBox = (int)(end + 0.3);
        if(endBox < 0) endBox = 0;
        int j;
        for(j = endBox; j <= startBox; j++) text[i][j] = '|';
        if(start < rows - 0.6)
            text[i][startBox] = startMod < 0.5 ? ',' : slash;
        if(end > -0.4)
            text[i][endBox] = endMod > 0.5 ? '\'' : slash;
    }

    printf("Graphing: %s %dx%d x: %g to %g y: %g to %g\n", equation, columns, rows, left, right, bottom, top);
    //Print text columns row by row
    for(i = rows - 1; i >= 0; i--) {
        int j;
        for(j = 0; j < columns; j++)
            putchar(text[j][i]);
        putchar('\n');
    }
    freeTree(tree);
}
void runLine(char* input) {
    int i;
    //If command
    if(input[0] == '-' && input[1] >= 'a' && input[1] <= 'z') {
        char* output = runCommand(input);
        if(globalError) return;
        if(output != NULL) {
            confirmNewLine();
            if(output[0] != '\0') {
                //Highligh history
                if(output[0] == '$') {
                    int i = 1;
                    while(output[++i] != '=');
                    output[i + 1] = '\0';
                    printf("%s", output);
                    output[i + 1] = ' ';
                    printWithHighlighting(output + i + 1);
                    putchar('\n');
                }
                //Highlight results from parse
                else if(startsWith(input, "-parse")) {
                    printWithHighlighting(output);
                    putchar('\n');
                }
                //Else print string
                else printf("%s\n", output);
            }
            free(output);
        }
        else if(startsWith(input, "-help")) {
            if(input[5] != ' ') {
                printf("Search the help pages with the -help command. Try \"-help getting started for a basic guide.\n");
                return;
            }
            //Get search results
            char* search = input + 6;
            int* searchResults = searchHelpPages(search);
            int decidedSearch = -1;
            if(searchResults[0] == -1) {
                error("No search results for '%s'", search);
                return;
            }
            else if(searchResults[1] == -1) {
                decidedSearch = searchResults[0];
            }
            else {
                //print results and give an option
                //print in reverse order and number them (maximum of 10 options)
                int resultCount = 0;
                while(searchResults[resultCount] != -1) resultCount++;
                if(resultCount > 10) resultCount = 10;
                for(int i = resultCount - 1;i >= 0;i--) {
                    struct HelpPage page = pages[searchResults[i]];
                    printf("%d: %s", i, page.name);
                    if(page.symbol != NULL) printf(" - %s", page.symbol);
                    printf("\n");
                }
                //Prompt for selected page
                printf("Select a page => ");
                int selectionId = readCharacter();
                printf("\n");
                if(selectionId < '0' || selectionId >= '0' + resultCount) {
                    error("Input out of range");
                    return;
                }
                decidedSearch = searchResults[selectionId - '0'];
            }
            //Print selected help page
            printHelpPage(pages[decidedSearch]);
        }
        else if(startsWith(input, "-g ")) {
            //Graph
            char clean[strlen(input)];
            strcpy(clean, input + 3);
            inputClean(clean);
            if(globalError) return;
            graphEquation(clean, -10, 10, 10, -10, 20, 50);
        }
        else if(startsWith(input, "-f ")) {
            int strLen = strlen(input);
            //Read lines from a file
            FILE* file = fopen(input + 3, "r");
            if(file == NULL) {
                error("file '%s' not found", input + 3);
                return;
            }
            //Initialize lines
            unsigned long lineSize = 1000;
            char* line = malloc(1000);
            if(line == NULL) { error("malloc error");return; }
            int lineCount = 0;
            //Loop through lines
            while(fgets(line, lineSize, file)) {
                lineCount++;
                int i = 0;
                //Remove endline character
                while(line[i++] != '\0') if(line[i] == '\n')
                    line[i] = '\0';
                //Print and run line
                printf("%s", line);
                runLine(line);
                globalError = false;
            }
            if(lineCount == 0) {
                error("'%s' is empty or a directory", input + 3);
            }
            //Close and free line
            free(line);
            fclose(file);
        }
        //Debug commands
        else if(startsWith(input, "-d")) {
            if(startsWith(input, "-dsyntax")) {
                input += 9;
                char* syntax = highlightLine(input);
                int i;
                int len = strlen(input);
                putchar('"');
                int prevType = 0;
                for(int j = 0;j < len + 1;j++) {
                    if(syntax[j] != prevType) {
                        if(j != 0) printf("\" %s, \"", syntaxTypes[prevType]);
                        prevType = syntax[j];
                    }
                    putchar(input[j]);
                }
                putchar('"');
                if(syntax[len - 1] == 0) printf(" NULL");
                putchar('\n');
                free(syntax);
                return;
            }
            if(startsWith(input, "-darb")) {
                input += 5;
                Value out = calculate(input, 0);
                confirmNewLine();
                if(out.type != value_arb || out.numArb == NULL) {
                    error("Returned value is not arbitrary precision");
                    return;
                }
                struct ArbNumber n = *out.numArb;
                if(n.r.mantissa != NULL) {
                    printf("r (len:%d, accu:%d): %c{", n.r.len, n.r.accu, n.r.sign ? '-' : '+');
                    for(int i = 0;i < n.r.len;i++) printf("%d,", n.r.mantissa[i]);
                    printf("}exp%d\n", n.r.exp);
                }
                else printf("r: NULL");
                if(n.i.mantissa != NULL) {
                    printf("i (len:%d, accu:%d): %c{", n.i.len, n.i.accu, n.i.sign ? '-' : '+');
                    for(int i = 0;i < n.i.len;i++) printf("%d,", n.i.mantissa[i]);
                    printf("}exp%d\n", n.i.exp);
                }
                else printf("i: NULL");
                appendToHistory(out, 10, true);
                freeValue(out);
            }
            if(startsWith(input, "-dperf")) {
                //Syntax: -dperf{count} {type} {exp}
                //Find space
                int i = 5;
                while(input[++i] != ' ');
                //Parse runCount
                input[i] = '\0';
                int runCount = parseNumber(input + 6, 10);
                if(runCount == 0) runCount = 1;
                printf("Run Count: %d, Clocks per second: %d\n", runCount, CLOCKS_PER_SEC);
                //Get runtype
                char* runType = input + i + 1;
                if(startsWith(runType, "syntax")) {
                    char* in = runType + 6;
                    clock_t t = clock();
                    //Time basic syntax
                    for(int i = 0;i < runCount;i++) {
                        free(highlightLine(in));
                    }
                    t = clock() - t;
                    printPerformance("Syntax", t, runCount);
                    return;
                }
                else if(startsWith(runType, "runline")) {
                    char* in = runType + 8;
                    clock_t t = clock();
                    for(int i = 0;i < runCount;i++) runLine(in);
                    t = clock() - t;
                    printPerformance("Runline", t, runCount);
                    return;
                }
                else if(startsWith(runType, "parse")) {
                    char in[strlen(runType)];
                    strcpy(in, runType + 6);
                    inputClean(in);
                    clock_t t = clock();
                    for(int i = 0;i < runCount;i++) {
                        Tree out = generateTree(in, NULL, globalLocalVariables, 0);
                        freeTree(out);
                    }
                    t = clock() - t;
                    printPerformance("Parse", t, runCount);
                    return;
                }
                else if(startsWith(runType, "calc")) {
                    char in[strlen(runType)];
                    strcpy(in, runType + 5);
                    inputClean(in);
                    Tree tr = generateTree(in, NULL, globalLocalVariables, 0);
                    clock_t t = clock();
                    for(int i = 0;i < runCount;i++) {
                        Value val = computeTree(tr, NULL, 0, globalLocalVariableValues);
                        freeValue(val);
                    }
                    t = clock() - t;
                    printPerformance("Calculation", t, runCount);
                    freeTree(tr);
                    return;
                }
                else if(startsWith(runType, "startup")) {
                    cleanup();
                    clock_t t = clock();
                    for(int i = 0;i < runCount;i++) {
                        startup();
                        cleanup();
                    }
                    t = clock() - t;
                    printPerformance("startup", t, runCount);
                    startup();
                    return;
                }
                else error("Performance test type unrecognized.");
                return;
            }
            if(startsWith(input, "-dchars")) {
                while(true) {
                    int ch = readCharacter();
                    printf("%d, ", ch);
                    //Should exit the program normally on ctrl+c, this is a safeguard
                    if(ch == 3) break;
                }
                return;
            }
            else {
                error("command '%s' is not a valid debugging command", input + 2);
            }
        }
        else {
            error("command '%s' not recognized.", input + 1);
            return;
        }
    }
    //If comment
    else if(input[0] == '#' || (input[0] == '/' && input[0] == '/')) {
        //Replace $() instances
        if(!rawMode) {
            printf("\0338\33[J\r");
            if(useColors) printf("\33[32m");
            int i = -1;
            //Print each character
            while(input[++i] != '\0') {
                //If is $()
                if(input[i] == '$' && input[i + 1] == '(') {
                    int j = i, endBracket = 0, bracketCount = 0;
                    //Find end bracket
                    while(input[++j]) {
                        if(input[j] == '(') bracketCount++;
                        else if(input[j] == ')') if(--bracketCount == 0) {
                            endBracket = j;
                            break;
                        }
                    }
                    //If no end bracket
                    if(endBracket == 0) {
                        error("no ending bracket");
                        return;
                    }
                    //Copy expression to stringToParse
                    char stringToParse[endBracket - i - 1];
                    memcpy(stringToParse, input + i + 2, endBracket - i - 2);
                    stringToParse[endBracket - i - 2] = '\0';
                    //Calculate value
                    Value out = calculate(stringToParse, 0);
                    char* outString = valueToString(out, 10);
                    freeValue(out);
                    //Print value
                    printf("%s", outString);
                    free(outString);
                    i = endBracket + 1;
                }
                //Print character
                putchar(input[i]);
            }
            //Print endline and color reset
            putchar('\n');
            printf("\33[0m");
        }
        //If it is a comment, ignore
        return;
    }
    //If dot command
    else if(input[0] == '.') {
        if(rawMode) {
            error("dot command is not supported in raw mode");
            return;
        }
        //Delete input
        printf("\0338\033[J\r");
        //Calculate value
        Value out = calculate(input + 1, 0);
        char* outString = valueToString(out, 10);
        freeValue(out);
        //Print if no error
        if(!globalError) {
            if(useColors) printf("\33[1;34m= \033[0m%s", outString);
            else printf("= %s", outString);
        }
        free(outString);
        //Read character before being erased by next readLine() call
        readCharacter();
        printf("\0338");
        return;
    }
    //Else compute it as a value
    else {
        int eqPos = isLocalVariableStatement(input);
        if(eqPos != 0) {
            Value out = calculate(input + eqPos + 1, 0);
            if(globalError) return;
            char* name = calloc(eqPos + 1, 1);
            int varIndex = -1;
            //The name variable may be freed when appendGlobalLocalVariable is called
            int nameEnd = eqPos;
            //Accessors
            if(input[eqPos - 1] == ']') {
                int bracket = 0;
                while(input[bracket] != '[') bracket++;
                strncpy(name, input, bracket);
                lowerCase(name);
                inputClean(name);
                nameEnd = bracket;
                //Create variable if it doesn't exist
                varIndex = appendGlobalLocalVariable(name, NULLVAL, false);
                //Calculate key
                input[eqPos - 1] = 0;
                Value key = calculate(input + bracket + 1, 0);
                input[eqPos - 1] = ']';
                //Set key
                setKey(globalLocalVariableValues + varIndex, key, out);
                if(globalError) { freeValue(out);return; }
            }
            else {
                strncpy(name, input, eqPos);
                lowerCase(name);
                inputClean(name);
                varIndex = appendGlobalLocalVariable(name, out, true);
            }
            //Print output
            char* output = valueToString(globalLocalVariableValues[varIndex], 10);
            char outStr[strlen(output) + nameEnd + 3];
            memcpy(outStr, input, nameEnd);
            outStr[nameEnd] = 0;
            strcat(outStr, "=");
            strcat(outStr, output);
            printWithHighlighting(outStr);
            putchar('\n');
            free(output);
            return;
        }
        if(input[0] == '\0') {
            error("no input", NULL);
            return;
        }
        Value out = calculate(input, 0);
        if(globalError) return;
        //Print output highlighted
        char* output = appendToHistory(out, 10, false);
        confirmNewLine();
        int i = 1;
        while(output[++i] != '=');
        output[i + 1] = '\0';
        printf("%s", output);
        output[i + 1] = ' ';
        printWithHighlighting(output + i + 1);
        putchar('\n');
        free(output);
    }
}
int main(int argc, char** argv) {
    //Startup
    startup();
    //Set cleanup on interupt
    signal(SIGINT, CLI_cleanup);
    //Parse command line arguments
    int commandCount = sizeof(commands) / sizeof(struct CommandArg);
    for(int i = 1;i < argc;i++) {
        for(int j = 0;j < commandCount;j++) {
            if(memcmp(argv[i], commands[j].name, strlen(commands[j].name)) == 0) {
                int eqpos = strlen(commands[j].name);
                //Set to one if no number provided
                if(argv[i][eqpos] == 0) {
                    *(commands[j].location) = 1;
                    commands[j].argvPos = i;
                }
                if(argv[i][eqpos] != '=') continue;
                *(commands[j].location) = parseNumber(argv[i] + eqpos + 1, 10);
                commands[j].argvPos = i;
            }
        }
    }
    readAutostart();
    //Show help
    if(showHelp) showUsage(argv);
#ifdef USE_FANCY_INPUT
    if(!rawMode) enableUnixRawRead();
#else
    rawMode = 1;
#endif
    if(!rawMode) {
        //Test VT-100 compatability
        printf("\033[c");
        int firstChar = readCharacter();
        if(firstChar != 27) {
            rawMode = 1;
            printf("\r    \r");
            printf("Reverting to raw mode\n");
#ifdef USE_FANCY_INPUT
            disableUnixRawRead();
#endif
            printf("%c", firstChar);
#ifdef USE_CONIO_H
            ungetch(firstChar);
#else
            ungetc(firstChar, stdin);
#endif
        }
        else {
            while(readCharacter() != 'c');
            printf("\r           \r");
        }
    }
    //Main loop
    while(true) {
        char* input = readLine(globalError);
        if(input == NULL) break;
        globalError = false;
        runLine(input);
        currentInput = NULL;
        free(input);
    }
    cleanup();
    return 0;
}