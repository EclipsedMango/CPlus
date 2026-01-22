
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
