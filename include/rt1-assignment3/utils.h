#include <iostream>
#include <string>
#include <future>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <termios.h>

/// Some utility functions

/**
 * Clears the input buffer
 */
void clearInputBuffer() {
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
}

/**
 * Detects user key press
 * @return ASCII code of pressed key
 */
int detectKeyPress () {
  int input;
  struct termios orig_term_attr;
  struct termios new_term_attr;

  // Set the terminal to raw mode rather than canonical mode
  tcgetattr(fileno(stdin), &orig_term_attr);
  memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
  new_term_attr.c_lflag &= ~(ECHO|ICANON);
  new_term_attr.c_cc[VTIME] = 0;
  new_term_attr.c_cc[VMIN] = 1;
  tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

  // Read a character from the stdin stream without blocking
  // Returns EOF (-1) if no character is available
  input = getchar();

  // Restore the original terminal attributes
  tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);

  return input;
}

/**
 * Changes color of terminal text
 * @param colorCode  ANSI color code
 * @param isBold     Bold text or not
 */
void terminalColor(int colorCode, bool isBold) {
  std::string specialCode = "";
  if (isBold) {
    specialCode = "1;";
  }

  std::cout << "\033[" << specialCode << colorCode << "m";
  fflush(stdout);
}

/**
 * Clears the terminal and resets it to initial state
 */
void clearTerminal() {
  printf("\033c");
  system("clear");
  terminalColor(31, true);
  printf("==========================\n");
  printf("= LUNAR ROVER CONTROLLER =\n");
  printf("==========================\n\n");
  terminalColor(37, true);
  fflush(stdout);
}

/**
 * Prints text with typing effect
 * @param str    Text to be printed
 * @param delay  Delay between each character, in microseconds
 */
void displayText (std::string str, int delay) {
  for (int i = 0; i < str.length(); i++) {
    std::cout << str[i];
    fflush(stdout);
    usleep(delay);
  }
}

/**
 * Check if string is numeric or not
 */
bool isStringNumeric(std::string str) {
  for (int i = 0; i < str.length(); i++) {
    if (std::isdigit(str[i])) {
      return true;
    }
  }

  return false;
}
