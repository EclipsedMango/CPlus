int main() {
    print("Hello From C+!");
}

int print(string a) {
    asm("int 0x80");
    return 0;
}
