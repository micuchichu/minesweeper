#include "app.h"

#ifdef NDEBUG
#ifdef WIN32
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup") 
#endif
#endif

int main() {
    Minesweeper app;
    app.Run();
    return 0;
}