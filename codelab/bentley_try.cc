
#include "bentley_cel.h"
#include <iostream>


using namespace bentley::pw::cel::codelab;

int main()
{
    std::cout << "$ ok, here we are now. keep typing. Enter exit when tired.\n";

    while(true) {
        std::string ln;
        std::cin >> ln;
        if (ln == "exit")
            break;

        auto res = ParseAndEvaluate(ln.c_str());
        if (!res.ok()){
            std::cout << "$ Do not get you... The way I see it, you are coming with: " << res.status() << "\n";
            continue;
        }

        std::cout << "$ " << *res << "\n";
    }
    std::cout << "$ later.\n";

    return 0;
}

