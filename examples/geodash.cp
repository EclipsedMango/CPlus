
// ----- LIBRARY CODE -----

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

// ----- GEODASH CODE -----

// square_bounds[0] == X, square_bounds[1] == Y, square_bounds[2] == W, square_bounds[3] == H
void draw_rect(int colour, int* square_bounds) {
    int* buffer;
    get_display_buffer(&buffer);

    for (int i = square_bounds[0]; i < square_bounds[0] + square_bounds[2]; i = i + 1) {
        for (int j = square_bounds[1]; j < square_bounds[1] + square_bounds[3]; j = j + 1) {

        }
    }
}

void fill_ground(int colour) {
    int* buffer;
    get_display_buffer(&buffer);

    for (int i = 0; i < 512; i = i + 1) {
        int* c = buffer + (i * 4);
        *c = colour;
    }

    for (int i = 512 - 128; i < 512; i = i + 1) {
        memcpy(buffer, buffer + (i * 2048), 2048);
    }
}

int main() {
    int[5] colours;
    colours[0] = 16711680; // Red
    colours[1] = 65280;    // Green
    colours[2] = 255;      // Blue
    colours[3] = 16777215; // White
    colours[4] = 0;        // Black

    int* display_buffer;
    get_display_buffer(&display_buffer);

    while (1) {
        fill_screen(colours[4]);
        fill_ground(colours[2]);

        int[4] square_bounds;
        square_bounds[0] = 50;
        square_bounds[1] = 50;
        square_bounds[2] = 206;
        square_bounds[3] = 206;
        draw_rect(colours[0], square_bounds);

        update_display();
    }

    halt();
    return 0;
}