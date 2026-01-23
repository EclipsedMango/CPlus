

int main() {
    print("Hello From C+!\n");
    
    int[7] arr;
    arr[0] = 4;
    arr[1] = 3;
    arr[2] = 16711680;
    
    int c = arr[0];
    
    int* disp;
    get_display_buffer(&disp);
    
    *disp = 16711680;
    
    int a = 0;
    while (a < 262144) {
        print_num(a);
        *disp = arr[2];
        disp = disp + 4;
        a = a + 1;
        
        update_display();
    }
    
    while (1) {
        
    }
    return 0;
}

int print(string a) {
    asm("int 0x80");
    return 0;
}

void print_num(int a) {
    asm("int 0x90" : : "r1"(a));
}

void get_display_buffer(int** pointer) {
    int* addr = 0;
    asm("int 0x84" 
        : "r0"(addr)
    );
    
    *pointer = addr;
}

void update_display() {
    asm("int 0x86");
}
