int main() {
    print("Hello From C+!\n");
    while (1) {
        
    }
    return 0;
}

int print(string a) {
    asm("int 0x80");
    return 0;
}

int* get_display_buffer() {
    int* pointer = 0;
    asm("int 0x84" 
        : "r0"(pointer)
    );
    return pointer;
}
