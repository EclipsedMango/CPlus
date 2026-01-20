int main() {
    int status = 0;

    int x = 42;
    int* ptr = &x;

    int y = *ptr;
    if (y != 42) {
        status = 1;
    }

    *ptr = 100;
    if (x != 100) {
        status = 2;
    }

    int** pptr = &ptr;
    int z = **pptr;
    if (z != 100) {
        status = 3;
    }

    **pptr = 200;
    if (x != 200) {
        status = 4;
    }

    int a = 10;
    int b = 20;
    int* p1 = &a;
    int* p2 = &b;

    int* temp = p1;
    p1 = p2;
    p2 = temp;

    if (*p1 != 20 || *p2 != 10) {
        status = 5;
    }

    *p1 = 30;
    *p2 = 40;

    if (a != 40 || b != 30) {
        status = 6;
    }

    int value = 7;
    int* ptr1 = &value;
    int** ptr2 = &ptr1;
    int*** ptr3 = &ptr2;

    if (***ptr3 != 7) {
        status = 7;
    }

    ***ptr3 = 77;
    if (value != 77) {
        status = 8;
    }

    int arr_val = 5;
    int* arr_ptr = &arr_val;
    *arr_ptr = *arr_ptr + 10;

    if (arr_val != 15) {
        status = 9;
    }

    int cond_val = 50;
    int* cond_ptr = &cond_val;

    if (*cond_ptr > 40) {
        *cond_ptr = 60;
    } else {
        status = 10;
    }

    if (cond_val != 60) {
        status = 11;
    }

    int n1 = 3;
    int n2 = 4;
    int* np1 = &n1;
    int* np2 = &n2;

    int sum = *np1 + *np2;
    if (sum != 7) {
        status = 12;
    }

    int original = 123;
    int* original_ptr = &original;
    int* same_ptr = &original;
    int different = 456;
    int* different_ptr = &different;

    if (*original_ptr != *same_ptr) {
        status = 13;
    }

    if (*original_ptr == *different_ptr) {
        status = 14;
    }

    int m = 2;
    int n = 3;
    int* pm = &m;
    int* pn = &n;

    int result = (*pm * *pn) + (*pm + *pn);
    if (result != 11) {
        status = 15;
    }

    return status;
}