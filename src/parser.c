#include "general.h"
#include "parser.h"
#include "functions.h"
#include "compute.h"
#include "arb.h"
int isLocalVariableStatement(const char* eq) {
    int i = -1;
    int isFirstChar = true;
    while(eq[++i] != '\0') {
        char ch = eq[i];
        if(ch == '=') {
            if(isFirstChar) return 0;
            int next = i + 1;
            while(eq[next] == ' ') next++;
            // => or == is not an asignment
            if(eq[next] == '>' || eq[next] == '=') return 0;
            return i;
        }
        if(ch == '[' && !isFirstChar) {
            int end = findNext(eq, i, ']');
            if(end == -1) return 0;
            if(eq[end + 1] == '=') return end + 1;
            return 0;
        }
        if(ch == ' ' || ch == '\n') continue;
        bool isDigit = ch >= '0' && ch <= '9';
        if(isDigit && isFirstChar) return 0;
        isFirstChar = false;
        if(ch == '_' || ch == '.' || isDigit || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) continue;
        else return 0;
    }
    return 0;
}
int nextSection(const char* eq, int start, int* end, int base) {
    if(eq[start] == 0) return sec_undef;
    while(eq[start] == ' ') start++;
    //Parenthesis
    if(eq[start] == '(') {
        int endPos = findNext(eq, start, ')');
        if(endPos == -1) {
            *end = strlen(eq);
            return sec_parenthesis;
        }
        int endNext = endPos + 1;
        while(eq[endNext] == ' ') endNext++;
        //Anonymous functions
        if(eq[endNext] == '=' && eq[endNext + 1] == '>') {
            endNext += 2;
            while(eq[endNext] == ' ') endNext++;
            if(eq[endNext] == '{') {
                endNext = findNext(eq, endNext, '}');
                if(endNext == -1) *end = strlen(eq);
                else *end = endNext;
                return sec_anonymousMultilineFunction;
            }
            else *end = strlen(eq);
            return sec_anonymousFunction;
        }
        *end = endPos;
        return sec_parenthesis;
    }
    //Square brackets
    if(eq[start] == '[') {
        int endPos = findNext(eq, start, ']');
        if(endPos == -1) {
            *end = strlen(eq);
            return sec_square;
        }
        *end = endPos;
        endPos++;
        while(eq[endPos] == ' ') endPos++;
        //Base notation
        if(eq[endPos] == '_') {
            *end = endPos;
            //Calling this will set end to the end of the next section
            nextSection(eq, endPos + 1, end, 10);
            return sec_squareWithBase;
        }
        return sec_square;
    }
    //Vectors
    if(eq[start] == '<') {
        *end = findNext(eq, start, '>');
        //Less than
        if(*end == -1) {
            *end = start;
            if(eq[(*end) + 1] == '=') (*end)++;
            if(eq[(*end) + 1] == '-') (*end)++;
            return sec_operator;
        }
        return sec_vector;
    }
    //Numbers
    if((eq[start] >= '0' && eq[start] <= '9') || eq[start] == '.') {
        //Parse the base
        int maxChar = 'A' + base - 10;
        if(eq[start] == '0') {
            int next = start + 1;
            while(eq[next] == ' ') next++;
            char baseChar = eq[next];
            if(baseChar >= 'A' && baseChar <= 'Z') baseChar -= 32;
            if(baseChar == 'x' || baseChar == 'b' || baseChar == 'd' || baseChar == 'o' || baseChar == 't') {
                if(baseChar == 'x') maxChar = 'F' + 1;
                else maxChar = 'A';
                next++;
                while(eq[next] == ' ') next++;
                start = next;
                if(eq[start] == 0) {
                    *end = start;
                    return sec_number;
                }
            }
        }
        *end = start;
        //Continue until a non-numeral is found
        while(true) {
            (*end)++;
            char ch = eq[*end];
            if(ch == '\0') break;
            if(ch == ' ' || ch == '.') continue;
            if(ch >= '0' && ch <= '9') continue;
            if(ch >= 'A' && ch < maxChar) continue;
            if(ch == 'e' && (eq[(*end) + 1] >= '0' && eq[(*end) - 1] <= '9')) continue;
            if(ch == 'e' && eq[(*end) + 1] == '-') { (*end)++;continue; }
            break;
        }
        (*end)--;
        return sec_number;
    }
    //History accessor $1...
    if(eq[start] == '$') {
        if(eq[start + 1] == '-') start++;
        if(eq[start + 1] >= '0' && eq[start + 1] <= '9') {
            *end = start + 1;
            while(eq[*end] >= '0' && eq[*end] <= '9') {
                if(eq[*end] == 0) break;
                (*end)++;
            }
            (*end)--;
            return sec_hist;
        }
        if(eq[start] != '$') start--;
    }
    //Variables and units
    if(eq[start] == '_' || eq[start] == '$' || (eq[start] >= 'a' && eq[start] <= 'z') || (eq[start] >= 'A' && eq[start] <= 'Z')) {
        *end = start;
        while(true) {
            (*end)++;
            char ch = eq[*end];
            if(ch >= 'a' && ch <= 'z') continue;
            if(ch >= 'A' && ch <= 'Z') continue;
            if(ch == ' ' || ch == '_' || ch == '.') continue;
            if(ch >= '0' && ch <= '9') continue;
            if(ch == '(') {
                *end = findNext(eq, *end, ')');
                if(*end == -1) *end = strlen(eq);
                return sec_function;
            }
            //Anonymous functions
            if(ch == '=' && eq[(*end) + 1] == '>') {
                int next = (*end) + 2;
                while(eq[next] == ' ') next++;
                if(eq[next] == '{') {
                    int endPos = findNext(eq, next, '}');
                    if(endPos == -1) *end = strlen(eq);
                    else *end = endPos;
                    return sec_anonymousMultilineFunction;
                }
                else *end = strlen(eq);
                return sec_anonymousFunction;
            }
            break;
        }
        (*end)--;
        return sec_variable;
    }
    //Operators
    if((eq[start] >= '*' && eq[start] <= '/') || eq[start] == '%' || eq[start] == '^' || eq[start] == '!' || eq[start] == '>' || eq[start] == '=') {
        int next = start;
        while(eq[++next]) {
            //Break on period
            if(eq[next] == '.') break;
            //Space
            if(eq[next] == ' ') continue;
            //Multiply, divide, add, subtract
            if(eq[next] >= '*' && eq[next] <= '/') continue;
            //Modulo and power
            if(eq[next] == '%' || eq[next] == '^') continue;
            //Comparisons
            if(eq[next] == '=') continue;
            if(eq[next] == '>') continue;
            break;
        }
        *end = next - 1;
        return sec_operator;
    }
    //Strings
    if(eq[start] == '"') {
        *end = start;
        bool isEscape = false;
        while(true) {
            start++;
            if(eq[start] == 0) break;
            if(isEscape) { isEscape = false;continue; }
            if(eq[start] == '\\') { isEscape = true;continue; }
            if(eq[start] == '"') break;
        }
        *end = start;
        return sec_string;
    }
    return sec_undef;
}
double parseNumber(const char* num, double base) {
    int i;
    int numLength = strlen(num);
    int periodPlace = numLength;
    int exponentPlace = numLength;
    //Find the position of 'e' and '.'
    for(i = 0; i < numLength; i++) {
        if(num[i] == '.') {
            if(periodPlace != numLength) { error("number cannot have more than one period");return 0; }
            periodPlace = i;
        }
        if(num[i] == 'e') {
            exponentPlace = i;
            break;
        }
    }
    if(periodPlace > exponentPlace) periodPlace = exponentPlace;
    //Parse integer digits
    double power = 1;
    double out = 0;
    for(i = periodPlace - 1; i > -1; i--) {
        double n = (double)(num[i] - 48);
        if(n > 10) n -= 7;
        out += power * n;
        power *= base;
    }
    //Parse fractional digits
    power = 1 / base;
    for(i = periodPlace + 1; i < exponentPlace; i++) {
        double n = (double)(num[i] - 48);
        if(n > 10) n -= 7;
        out += power * n;
        power /= base;
    }
    //Parse exponent
    if(exponentPlace != numLength) {
        double exponent = 0;
        bool isNegative = num[exponentPlace + 1] == '-';
        if(isNegative) exponentPlace++;
        for(i = exponentPlace + 1;i < numLength;i++) {
            exponent *= base;
            double n = num[i] - 48;
            if(n > 10) n -= 7;
            exponent += n;
        }
        if(isNegative) exponent = -exponent;
        out *= pow(base, exponent);
    }
    return out;
}
Tree generateTree(const char* eq, char** argNames, char** localVars, double base) {
    bool useUnits = base != 0;
    if(base == 0) base = 10;
    int i, eqLength = strlen(eq);
    //Find section edges
    int sections[eqLength + 1];
    signed char sectionTypes[eqLength + 1];
    int sectionCount;
    sections[0] = -1;
    for(int i = 0;true;i++) {
        int end = -1;
        int type = nextSection(eq, sections[i] + 1, &end, base);
        if(type == sec_undef) {
            error("could not parse '%.5s%s'", eq + sections[i] + 1, strlen(eq) - sections[i] > 6 ? "..." : "");
            return NULLOPERATION;
        }
        if(eq[end] == 0) {
            if(type == sec_function || type == sec_parenthesis || type == sec_square || type == sec_vector || type == sec_anonymousMultilineFunction) {
                error("bracket mismatch");
                return NULLOPERATION;
            }
            if(type == sec_string) {
                error("no closing quotes on string");
                return NULLOPERATION;
            }
        }
        sectionTypes[i] = type;
        sections[i + 1] = end;
        if(eq[end] == 0 || eq[end + 1] == 0) {
            sectionCount = i + 1;
            break;
        }
    }
    //Generate operations from strings
    Tree ops[sectionCount];
    memset(ops, 0, sizeof(ops));
    //set to true when it comes across *- or /- or ^- or %-
    bool nextNegative = false;
    bool firstNegative = false;
    for(i = 0;i < sectionCount;i++) {
        unsigned char type = sectionTypes[i];
        int sectionLen = sections[i + 1] - sections[i];
        char section[sectionLen + 1];
        memcpy(section, eq + sections[i] + 1, sectionLen);
        section[sectionLen] = 0;
        int j = 0;
        if(type == sec_number) {
            double baseToUse = base;
            char* numString = section;
            if(section[0] == '0' && section[1] >= 'a' && section[1] != 'e') {
                char base = section[1];
                if(base == 'b') baseToUse = 2;
                if(base == 't') baseToUse = 3;
                if(base == 'o') baseToUse = 8;
                if(base == 'd') baseToUse = 10;
                if(base == 'x') baseToUse = 16;
                numString = section + 2;
            }
            if(useArb) {
                Arb num = parseArb(numString, baseToUse, globalAccuracy);
                Value out;
                out.type = value_arb;
                out.numArb = calloc(1, sizeof(ArbNum));
                out.numArb->r = num;
                ops[i] = newOpValue(out);
            }
            else ops[i] = newOpVal(parseNumber(numString, baseToUse), 0, 0);
            if(globalError) goto error;
        }
        if(type == sec_variable) {
            if(!useUnits) lowerCase(section);
            Tree op = findFunction(section, useUnits, argNames, localVars);
            if(op.optype == 0 && op.op == 0) {
                error("variable '%s' not found", section);
                goto error;
            }
            if(op.optype == 0 && (stdfunctions[op.op].inputs[0] & 1) == 0 && stdfunctions[op.op].inputs[0] != 0) {
                error("no arguments for '%s'", section);
                goto error;
            }
            //Units
            if(op.optype == -1)
                ops[i] = newOpVal(op.value.r, 0, op.value.u);
            else ops[i] = newOp(NULL, 0, op.op, op.optype);
        }
        if(type == sec_function) {
            //Set name to lowercase and find function id
            int brac = 0;
            while(section[brac] != '(') brac++;
            section[brac] = '\0';
            lowerCase(section);
            Tree op = findFunction(section, false, NULL, NULL);
            //Verify that it is a valid function
            if(op.optype == 0 && op.op == 0) {
                error("variable '%s' not found", section);
                goto error;
            }
            if(op.optype != 0 && op.optype != optype_custom) {
                error("'%s' is not a function", section);
                goto error;
            }
            //Count and locate the commas
            int commaCount;
            int commas[sectionLen - brac];
            commas[0] = brac;
            for(commaCount = 0;true;commaCount += 1) {
                commas[commaCount + 1] = findNext(section, commas[commaCount] + 1, ',');
                if(commas[commaCount + 1] == -1) break;
            }
            commas[commaCount + 1] = sectionLen - 1;
            //Check for wrong number of arguments
            int argCount = commaCount + 1;
            if(op.optype == 0) {
                int builtinArgCount = strlen(stdfunctions[op.op].inputs);
                if(builtinArgCount < argCount && op.op != op_run) {
                    error("too many arguments for '%s'", section);
                    goto error;
                }
                if(builtinArgCount > argCount && (stdfunctions[op.op].inputs[argCount] & 1) != 1) {
                    error("too few arguments for '%s'", section);
                    goto error;
                }
            }
            else if(op.optype == 1 && customfunctions[op.op].argCount != argCount) {
                error("wrong number of arguments for '%s'", section);
                return NULLOPERATION;
            }
            Tree* args = calloc(argCount, sizeof(Tree));
            for(j = 0; j < argCount; j++) {
                int len = commas[j + 1] - commas[j] - 1;
                char argText[len + 1];
                memcpy(argText, section + commas[j] + 1, len);
                argText[len] = 0;
                args[j] = generateTree(argText, argNames, localVars, useUnits ? base : 0);
                if(globalError) {
                    for(int x = 0;x < j;x++) freeTree(args[x]);
                    free(args);
                    goto error;
                }
            }
            ops[i] = newOp(args, argCount, op.op, op.optype);
        }
        if(type == sec_parenthesis) {
            section[sectionLen - 1] = '\0';
            ops[i] = generateTree(section + 1, argNames, localVars, useUnits ? base : 0);
            if(globalError) goto error;
        }
        if(type == sec_square) {
            section[sectionLen - 1] = '\0';
            ops[i] = generateTree(section + 1, argNames, localVars, useUnits ? base : 10);
            if(globalError) { freeTree(ops[i]);goto error; }
        }
        if(type == sec_squareWithBase) {
            //Find underscore
            int underscore = findNext(section, 0, '_');
            if(underscore == -1) { error("could not find underscore");goto error; }
            //Parse base
            Tree baseTree = generateTree(section + underscore + 1, NULL, NULL, 10);
            Value newBase = computeTree(baseTree, NULL, 0, NULL);
            freeTree(baseTree);
            double baseR = getR(newBase);
            freeValue(newBase);
            //Parse inside the brackets
            section[underscore - 1] = '\0';
            ops[i] = generateTree(section + 1, argNames, localVars, baseR);
            if(globalError) goto error;
        }
        if(type == sec_operator) {
            if(section[0] == '-' && i == 0) {
                firstNegative = true;
                continue;
            }
            //Parse negative suffix, like *- or /-
            if(section[sectionLen - 1] == '-' && sectionLen != 1) {
                section[sectionLen - 1] = '\0';
                nextNegative = true;
                sectionLen -= 1;
            }
            int op = 0;
            int opOrder = 0;
            if(sectionLen == 2) {
                if(section[0] == '*' && section[1] == '*') op = op_pow, opOrder = 1;
                else if(section[0] == '=' && section[1] == '=') op = op_equal, opOrder = 4;
                else if(section[0] == '>' && section[1] == '=') op = op_gt_equal, opOrder = 4;
                else if(section[0] == '<' && section[1] == '=') op = op_lt_equal, opOrder = 4;
                else if(section[0] == '!' && section[1] == '=') op = op_not_equal, opOrder = 4;
            }
            if(sectionLen == 1) {
                if(section[0] == '+') op = op_add, opOrder = 3;
                else if(section[0] == '-') op = op_sub, opOrder = 3;
                else if(section[0] == '*') op = op_mult, opOrder = 2;
                else if(section[0] == '/') op = op_div, opOrder = 2;
                else if(section[0] == '%') op = op_mod, opOrder = 2;
                else if(section[0] == '^') op = op_pow, opOrder = 1;
                else if(section[0] == '>') op = op_gt, opOrder = 4;
                else if(section[0] == '<') op = op_lt, opOrder = 4;
                else if(section[0] == '=') op = op_equal, opOrder = 4;
            }
            if(op == 0) {
                error("operator '%s' not found", section);
                goto error;
            }
            ops[i] = newOp(NULL, -opOrder, op, optype_builtin);
            if(i != 0 && ops[i - 1].argCount == -1 && ops[i - 1].op != op_val) {
                error("cannot combine two different operators");
                goto error;
            }
        }
        if(type == sec_vector) {
            int commas[sectionLen];
            commas[0] = 0;
            int commaCount = 0;
            int nextComma = findNext(section, 1, ',');
            if(nextComma == -1) nextComma = 100000;
            int nextSemicolon = findNext(section, 1, ';');
            if(nextSemicolon == -1) nextSemicolon = 100000;
            int maxWidth = 1, width = 1;
            int height = 1;
            while(true) {
                if(nextSemicolon == 100000 && nextComma == 100000) {
                    break;
                }
                if(nextComma < nextSemicolon) {
                    width++;
                    commas[++commaCount] = nextComma;
                    nextComma = findNext(section, nextComma + 1, ',');
                    if(nextComma == -1) nextComma = 100000;
                }
                else if(nextComma > nextSemicolon) {
                    if(width > maxWidth) maxWidth = width;
                    width = 1;
                    height++;
                    commas[++commaCount] = nextSemicolon;
                    nextSemicolon = findNext(section, nextSemicolon + 1, ';');
                    if(nextSemicolon == -1) nextSemicolon = 100000;
                }
            }
            if(width < maxWidth) width = maxWidth;
            commas[commaCount + 1] = sectionLen - 1;
            Tree* cells = calloc(width * height, sizeof(Tree));
            int x = 0;
            int y = 0;
            for(int i = 0;i < commaCount + 1;i++) {
                if(section[commas[i]] == ',') x++;
                if(section[commas[i]] == ';') { x = 0;y++; }
                int len = commas[i + 1] - commas[i] - 1;
                char cell[len + 1];
                memcpy(cell, section + commas[i] + 1, len);
                cell[len] = 0;
                if(cell[0] != 0)
                    cells[x + y * width] = generateTree(cell, argNames, localVars, useUnits ? base : 0);
                if(globalError) {
                    for(int j = 0;j < x + y * width;j++) freeTree(cells[j]);
                    free(cells);
                    goto error;
                }
            }
            ops[i].optype = 0;
            ops[i].op = op_vector;
            ops[i].branch = cells;
            ops[i].argCount = width * height;
            ops[i].argWidth = width;
        }
        if(type == sec_anonymousFunction) {
            char** argList = parseArgumentList(section);
            if(globalError) goto error;
            char** argListMerged = mergeArgList(argNames, argList);
            int eqPos = findNext(section, 0, '=');
            if(eqPos == -1) { freeArgList(argList);free(argListMerged);error("could not find anonymous function operator");goto error; }
            ops[i] = NULLOPERATION;
            ops[i].optype = optype_anon;
            ops[i].argNames = argList;
            Tree func = generateTree(section + eqPos + 2, argListMerged, NULL, useUnits ? base : 0);
            free(argListMerged);
            if(globalError) {
                freeArgList(argList);
                goto error;
            }
            ops[i].code = malloc(sizeof(CodeBlock));
            *(ops[i].code) = codeBlockFromTree(func);
            ops[i].argCount = argListLen(argList);
            ops[i].argWidth = argListLen(argNames);
        }
        if(type == sec_anonymousMultilineFunction) {
            char** argList = parseArgumentList(section);
            if(globalError) goto error;
            char** argListMerged = mergeArgList(argNames, argList);
            int eqPos = findNext(section, 0, '=');
            if(eqPos == -1) { freeArgList(argList);free(argListMerged);error("could not find anonymous function operator");goto error; }
            if(section[sectionLen - 1] == 0) { error("curly bracket error");goto error; }
            section[sectionLen - 1] = 0;
            CodeBlock code = parseToCodeBlock(section + eqPos + 3, argListMerged, NULL, NULL, NULL);
            free(argListMerged);
            if(globalError) { freeArgList(argList);goto error; }
            ops[i].code = malloc(sizeof(CodeBlock));
            *(ops[i].code) = code;
            ops[i].optype = optype_anon;
            ops[i].argNames = argList;
            ops[i].argCount = argListLen(argList);
            ops[i].argWidth = argListLen(argNames);
        }
        if(type == sec_string) {
            char* string = calloc(sectionLen + 1, 1);
            int stringPos = 0;
            bool isEscape = false;
            for(int i = 1;i < sectionLen;i++) {
                if(isEscape) {
                    isEscape = false;
                    if(section[i] == 'n') string[stringPos++] = '\n';
                    else if(section[i] == 'r') string[stringPos++] = '\r';
                    else if(section[i] == 't') string[stringPos++] = '\t';
                    else string[stringPos++] = section[i];
                }
                else if(section[i] == '\\') {
                    isEscape = true;
                    continue;
                }
                else if(section[i] == '"') break;
                else string[stringPos++] = section[i];
            }
            ops[i].value.type = value_string;
            ops[i].value.string = string;
        }
        if(type == sec_hist) {
            int sign = 1;
            int start = 1;
            //Negative numbers
            if(section[1] == '-') {
                sign = -1;
                start++;
            }
            double id = parseNumber(section + start, 10) * sign;
            ops[i] = newOp(allocArg(newOpVal(id, 0, 0), 0), 1, op_hist, optype_builtin);
        }
        if(nextNegative && type != sec_operator) {
            nextNegative = false;
            ops[i] = newOp(allocArg(ops[i], false), 1, op_neg, 0);
        }
    }
error:
    if(globalError) {
        for(int x = 0;x < i;x++) freeTree(ops[x]);
        return NULLOPERATION;
    }
    if(firstNegative) {
        memmove(ops, ops + 1, (sectionCount - 1) * sizeof(Tree));
        ops[0] = newOp(allocArg(ops[0], false), 1, op_neg, 0);
        sectionCount -= 1;
    }
    //Return early if no compilation is necessary
    if(sectionCount == 1 && ops[0].argCount >= 0) return ops[0];
    //Compile operations into a single tree
    int offset = 0;
    for(i = 1; i < 5; i++) {
        offset = 0;
        for(int j = 0; j < sectionCount; j++) {
            ops[j] = ops[j + offset];
            //Configure order of operations
            if(ops[j].argCount != -i || ops[j].op == op_val) continue;
            //Check for missing argument
            if(j == 0 || j == sectionCount - 1) {
                error("missing argument in operation", NULL);
                for(int x = j + 1;x < sectionCount;x++) ops[x] = ops[x + offset];
                for(int x = 0;x < sectionCount;x++) freeTree(ops[x]);
                return NULLOPERATION;
            }
            //Combine previous and next, set offset
            ops[j - 1] = newOp(allocArgs(ops[j - 1], ops[j + 1 + offset], 0, 0), 2, ops[j].op, 0);
            j--;
            sectionCount -= 2;
            offset += 2;
        }
        //Do side by side multiplication after exponentiation
        if(i == 1) {
            offset = 0;
            int j;
            for(j = 1; j < sectionCount; j++) {
                ops[j] = ops[j + offset];
                //Check if ineligible
                if(ops[j].argCount < 0 && ops[j].op != op_val) continue;
                if(ops[j - 1].argCount < 0 && ops[j - 1].op != op_val) continue;
                //Combine them
                ops[j - 1] = newOp(allocArgs(ops[j - 1], ops[j], 0, 0), 2, op_mult, 0);
                offset++;
                sectionCount--;
                j--;
            }
            ops[j] = ops[j + offset];
        }
    }
    if(sectionCount != 1) {
        error("internal argument compile (%d)", sectionCount);
        for(int i = 0;i < sectionCount;i++) freeTree(ops[i]);
        return NULLOPERATION;
    }
    return ops[0];
}