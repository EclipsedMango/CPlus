
int string_length(char* str) {
    int len = 0;
    while (*str != 0) {
        str = str + 1;
        len = len + 1;
    }

    return len;
}

void print(string msg) {
    int len = string_length(msg);
    asm("mov rax, 1; mov rdi, 1; mov rsi, $0; mov rdx, $1; syscall", msg, len);
}

void set_value(int* ptr, int val) {
    *ptr = val;
    return;
    *ptr = 0;
}

int main() {
    // Test!!!
    print("Fuck you zane!\n");

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

    int v_test = 0;
    set_value(&v_test, 999);
    if (v_test != 999) {
        status = 16;
    }

    char c = 65;
    if (c != 65) {
        status = 17;
    }

    char* str = "hello";
    if (*str != 104) {
        status = 18;
    }

    char* str2 = str;
    if (*str2 != *str) {
        status = 19;
    }

    int w_counter = 0;
    while (w_counter < 10) {
        w_counter = w_counter + 1;
    }
    if (w_counter != 10) {
        status = 20;
    }

    int f_sum = 0;
    for (int i = 0; i < 5; i = i + 1) {
        f_sum = f_sum + i;
    }

    if (f_sum != 10) {
        status = 21;
    }

    int b_counter = 0;
    while (1) {
        if (b_counter == 5) {
            break;
        }
        b_counter = b_counter + 1;
    }
    if (b_counter != 5) {
        status = 22;
    }

    int c_sum = 0;
    for (int k = 0; k < 10; k = k + 1) {
        if (k == 5) {
            continue;
        }
        c_sum = c_sum + 1;
    }

    if (c_sum != 9) {
        status = 23;
    }

    int outer_count = 0;
    int inner_count = 0;
    for (int i = 0; i < 3; i = i + 1) {
        outer_count = outer_count + 1;
        for (int j = 0; j < 2; j = j + 1) {
            inner_count = inner_count + 1;
        }
    }
    if (outer_count != 3) { status = 24; }
    if (inner_count != 6) { status = 25; }

    return status;
}