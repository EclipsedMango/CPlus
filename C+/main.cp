int main() {
    int status = 0;

    int pos = 10;
    int neg = -10;

    if (pos + neg != 0) {
        status = 1;
    }

    bool is_big = pos > 5;
    bool is_small = pos < 100;

    if (is_big && is_small) {
    } else {
        status = 2;
    }

    if (pos == 99 || pos == 10) {
    } else {
        status = 3;
    }

    bool false_val = 1 == 2;

    if (!false_val) {
    } else {
        status = 4;
    }

    if (1 == 0 || 1 == 1 && 1 == 1) {
    } else {
        status = 5;
    }

    return status;
}