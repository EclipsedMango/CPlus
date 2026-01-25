
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

// copy length bytes from src to dest
void memcpy(int* src, int* dest, int length) {
    asm("cpy r1, r2" : : "r0"(dest), "r1"(src), "r2"(length));
}


// fill the screen buffer with the specified colour.
// Assumes the screen is 512x512.
// Doesn't use assembly, for reference, use fill_screen(int) for prod.
void fill_screen_cp(int colour) {
    int* buffer;
    get_display_buffer(&buffer);
    for (int i = 0; i < 512; i = i + 1) {
        int* c = buffer + (i * 4);
        *c = colour;
    }

    for (int i = 1; i < 512; i = i + 1) {
        memcpy(buffer, buffer + (i * 512), 2048);
    }
}

// Fill the screen buffer with the specified colour.
// Assumes the screen is 512x512.
// Uses purely assembly.
void fill_screen(int colour) {
    asm("int 0x84\nmov r4, r0\nmov r2, 0x100000\nmov @r0, r1\nmov r3, 4\nadd r0, 4\nsub r2, 4\n.loop:\ncpy r4, r3\nadd r0, r3\nsub r2, r3\nadd r3, r3\nint 0x86\ncmp r2, 0\njne .loop" : : "r1"(colour) : "r1", "r4", "r3", "r2", "r0");
}

// returns the smaller of the two numbers.
int min(int a, int b) {
    if (a > b) {
        return b;
    }
    return a;
}
