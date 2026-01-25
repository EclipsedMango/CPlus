#ifndef C__COMMON_H
#define C__COMMON_H

typedef struct SourceLocation {
    int line;
    int column;
    const char *filename;
} SourceLocation;

int max(int a, int b);

#endif //C__COMMON_H