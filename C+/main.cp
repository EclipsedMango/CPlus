int add(int a, int b) {
    return a + b;
}

int give5() {
    return add(3, 2);
}

string giveString() {
    return "Hello World!";
}

int main() {
    int x = 4;
    int y = add(x, 4);
    add(1, 2);
    return y;
}