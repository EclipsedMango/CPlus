
#include "stdlib.hp"
#include "test.hp"

#define MAX_SIZE 100
#define PI 3.14159
#define SQUARE(x) ((x) * (x))

// Function returning int pointer
int* set_value(int* ptr, int val) {
    *ptr = val;
    return ptr;
}

// Function returning char pointer (test new feature)
char* get_greeting() {
    return "Hello from function";
}

// Function returning pointer to pointer (test new feature)
int** get_ptr_ptr(int** p) {
    return p;
}

int counter = 0;
void increment() { counter = counter + 1; }

int main() {
    print("Hello World!\n");
    __cplus_print_("This is a builtin!\n");

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

    // Test function returning pointer
    int v_test = 0;
    int set_val = *set_value(&v_test, 999);
    if (v_test != 999) {
        status = 16;
    }

    // Test function returning string (char*)
    char* greeting = get_greeting();
    if (*greeting != 72) { // 'H'
        status = 26;
    }

    // Test function returning pointer-pointer
    int val_ptr_ptr = 55;
    int* ptr_to_val = &val_ptr_ptr;
    int** ptr_ptr_val = &ptr_to_val;

    int** returned_pp = get_ptr_ptr(ptr_ptr_val);
    if (**returned_pp != 55) {
        status = 27;
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
    for (int i = 0; i < 5; i++) {
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
    for (int i = 0; i < 3; i++) {
        outer_count = outer_count + 1;
        for (int j = 0; j < 2; j++) {
            inner_count = inner_count + 1;
        }
    }

    if (outer_count != 3) { status = 24; }
    if (inner_count != 6) { status = 25; }

    int[5] arr1;
    arr1[0] = 10;
    arr1[1] = 20;
    arr1[2] = 30;
    arr1[3] = 40;
    arr1[4] = 50;

    if (arr1[0] != 10) { status = 28; }
    if (arr1[2] != 30) { status = 28; }
    if (arr1[4] != 50) { status = 28; }

    int idx = 2;
    int val_at_idx = arr1[idx];
    if (val_at_idx != 30) { status = 29; }

    arr1[idx] = 99;
    if (arr1[2] != 99) { status = 30; }

    int[10] arr2;
    for (int i = 0; i < 10; i++) {
        arr2[i] = i * 2;
    }

    if (arr2[0] != 0) { status = 31; }
    if (arr2[5] != 10) { status = 31; }
    if (arr2[9] != 18) { status = 31; }

    int arr_sum = 0;
    for (int i = 0; i < 10; i++) {
        arr_sum = arr_sum + arr2[i];
    }
    if (arr_sum != 90) { status = 32; }

    int base = 3;
    int offset = 2;
    int[10] arr3;
    arr3[base + offset] = 777;
    if (arr3[5] != 777) { status = 33; }

    int[5] arr4;
    arr4[2] = 123;
    int* ptr_to_elem = &arr4[2];
    if (*ptr_to_elem != 123) { status = 34; }

    *ptr_to_elem = 456;
    if (arr4[2] != 456) { status = 35; }

    int[3] arr5;
    arr5[0] = 5;
    arr5[1] = 10;
    arr5[2] = 15;
    int total = arr5[0] + arr5[1] + arr5[2];
    if (total != 30) { status = 36; }

    char[6] char_arr;
    char_arr[0] = 72;  // 'H'
    char_arr[1] = 101; // 'e'
    char_arr[2] = 108; // 'l'
    char_arr[3] = 108; // 'l'
    char_arr[4] = 111; // 'o'
    char_arr[5] = 0;   // null terminator

    if (char_arr[0] != 72) { status = 37; }
    if (char_arr[4] != 111) { status = 37; }

    int[3] arr6;
    arr6[0] = 1;
    arr6[1] = arr6[0] + 1;
    arr6[2] = arr6[1] + 1;
    if (arr6[2] != 3) { status = 38; }

    int[2] swap_arr;
    swap_arr[0] = 100;
    swap_arr[1] = 200;
    int tmp = swap_arr[0];
    swap_arr[0] = swap_arr[1];
    swap_arr[1] = tmp;
    if (swap_arr[0] != 200) { status = 39; }
    if (swap_arr[1] != 100) { status = 39; }

    int[5] max_arr;
    max_arr[0] = 15;
    max_arr[1] = 42;
    max_arr[2] = 8;
    max_arr[3] = 99;
    max_arr[4] = 23;

    int max_val = max_arr[0];
    for (int i = 1; i < 5; i++) {
        if (max_arr[i] > max_val) {
            max_val = max_arr[i];
        }
    }
    if (max_val != 99) { status = 40; }

    int[5] coooool;
    coooool[0] = 100;
    int* aaaa = coooool;
    if (*aaaa != 100) { status = 41; }

    int* abcd = &coooool[0];
    abcd[2] = 69;
    if (abcd[2] != 69) { status = 42; }

	increment();
	if (counter != 1) { status = 43; }

    int[MAX_SIZE] arr;
    float area = PI * 5 * 5;
    int sq = SQUARE(5);

    if (sq != 25) { status = 44; }
    if (INCLUDE_TEST_VALUE != 50) { status = 45; }

    float ff = 3.14;
    int xxx = (int)ff;

    return xxx;
}