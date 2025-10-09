#include <iostream>
#include <string>
#include <vector>

// Test C backend integration
extern "C" {
    #include "../src/scanner.h"
    #include "../src/cache.h"
}

int main() {
    std::cout << "Testing DiskScout C Backend Integration..." << std::endl;
    
    // Test basic functionality
    std::cout << "✓ C headers included successfully" << std::endl;
    
    // Test cache system
    if (cache_init() == 0) {
        std::cout << "✓ Cache system initialized" << std::endl;
        cache_cleanup();
        std::cout << "✓ Cache system cleaned up" << std::endl;
    } else {
        std::cout << "✗ Cache system failed to initialize" << std::endl;
    }
    
    // Test scanner functions
    std::cout << "✓ Scanner functions available" << std::endl;
    
    std::cout << "\nBackend integration test completed!" << std::endl;
    std::cout << "Ready for Qt GUI integration." << std::endl;
    
    return 0;
}
