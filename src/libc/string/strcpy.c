char* strcpy(char* dest, const char* src) {
    char* ptr = dest; // Save the start of dest for return
    while (*src != '\0') {
        *dest = *src; // Copy each character
        dest++;
        src++;
    }
    *dest = '\0'; // Add null terminator
    return ptr;   // Return pointer to start of dest
}