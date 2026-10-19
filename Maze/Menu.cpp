#include "menu.h"
#include <iostream>
#include <limits>

int showMenu()
{
    int choice = 0;

    while (true)
    {
        std::cout << "\n========== MAZE ==========\n";
        std::cout << "        Team NXRP\n\n";
        std::cout << "1. Continue\n";
        std::cout << "2. New Game\n";
        std::cout << "3. How to Play\n";
        std::cout << "4. Settings\n";
        std::cout << "5. Exit\n";
        std::cout << "==========================\n";
        std::cout << "Choose an option: ";

        if (std::cin >> choice && choice >= 1 && choice <= 5)
        {
            return choice;
        }

        if (std::cin.eof())
        {
            return 5;
        }

        std::cin.clear();
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(), '\n'
        );

        std::cout << "Invalid choice. Enter a number from 1 to 5.\n";
    }
}