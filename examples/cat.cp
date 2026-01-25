

// Print a string to stdout.
int print(string a) {
    asm("int 0x80");
    return 0;
}

// Print a number to stdout
// ONLY WHEN --test-ints IS ENABLED IN VM.
void debug_num(int a) {
    asm("int 0x90" : : "r1"(a));
}

// Get a pointer to the display buffer (512x512).
void get_display_buffer(int** pointer) {
    int* addr = 0;
    asm("int 0x84" 
        : "r0"(addr)
    );
    
    *pointer = addr;
}

// Refresh the display and draw all new things
// if you don't do this nothing on the screen
// will change.
void update_display() {
    asm("int 0x86");
}

// Pause the VM.
void halt() {
    asm("int 0x81");
}

// Shutdown and close the VM.
void shutdown() {
    asm("int 0x82");
}

// Restart the VM.
void reset() {
    asm("int 0x83");
}

// Gets time in ms since the VM started.
int get_uptime() {
    int val;
    asm("int 0x85" : "r0"(val));
    return val;
}

// Check if there is an available input
// ready to be handled.
// If not it returns -1.
// If there is it returns the key code that
// was pressed (will be the unicode value of the character)
// and type will be set to either 0: key down, 1: key up
int poll_input(int* type) {
    int val;
    asm("in r1, 0" : "r1"(val));
    
    if (val == -1) {
        return -1;
    }
    
    asm("in r1, 0" : "r1"(val));
    *type = val;
    
    asm("in r1, 0" : "r1"(val));
    return val;
}

void memcpy(int* src, int* dest, int length) {
    asm("cpy r1, r2" : : "r0"(dest), "r1"(src), "r2"(length));
}

int min(int a, int b) {
    if (a > b) {
        return b;
    }
    return a;
}

void fillmem(int* dest, int length, int value) {
    *dest = value;
    length = length - 4;  // we did one
    
    int size = 4;
    while (length > 0) {
        int amount = size;
        if (amount > length) {
            amount = length;
        }
        memcpy(dest, dest + size, amount);
        length = length - amount;
        size = size + amount;
    }
}


void fill_screen(int colour) {
    int* buffer;
    get_display_buffer(&buffer);
    fillmem(buffer, 1048576, colour);
}

void fill_screen_fast(int colour) {
    asm("int 0x84\nmov r4, r0\nmov r2, 0x100000\nmov @r0, r1\nmov r3, 4\nadd r0, 4\nsub r2, 4\n.loop:\ncpy r4, r3\nadd r0, r3\nsub r2, r3\nadd r3, r3\nint 0x86\ncmp r2, 0\njne .loop" : : "r1"(colour) : "r1", "r4", "r3", "r2", "r0");
}

void draw_rect(int* args) {
    int x = args[0];
    int y = args[1];
    int w = args[2];
    int h = args[3];
    int colour = args[4];
    
    debug_num(x);
    debug_num(y);
    debug_num(w);
    debug_num(h);
    debug_num(colour);
    
    int* disp;
    get_display_buffer(&disp);
    
    // fill one row, put disp at start
    disp = disp + (x * 4) + (y * 2048);
    int* firstRow = disp;
    
    for (int i = 0; i < w; i = i + 1) {
        *disp = colour;
        disp = disp + 4;
    }
    
    disp = firstRow + 2048;
    for (int i = 0; i < h - 1; i = i + 1) {
        memcpy(firstRow, disp, w * 4);
        disp = disp + 2048;
        update_display();
    }
}

void test(int a, int b, int c, int d, int e, int f, int g) {
    debug_num(a);
    debug_num(b);
    debug_num(c);
    debug_num(d);
    debug_num(e);
    debug_num(f);
    debug_num(g);
}


int main() {
    // test(1,2,3,4,5,6,7);
    
    fill_screen_fast(14509602);
    
    int[5] args;
    args[0] = 256;  // x
    args[1] = 100;  // y
    args[2] = 50;   // width
    args[3] = 50;  // height
    args[4] = 255;  // colour
    draw_rect(args);
    
    print("DONE!");
    
    update_display();

    halt();
    return 0;
}