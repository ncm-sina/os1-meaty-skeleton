char* strcat(char* dest, const char* src) {
    char* ptr = dest; // Save the start of dest for return
    // Move to the end of dest (find null terminator)
    while (*dest != '\0') {
        dest++;
    }
    // Copy src to dest, including null terminator
    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0'; // Add null terminator
    return ptr;   // Return pointer to start of dest
}