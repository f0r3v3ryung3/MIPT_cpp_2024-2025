#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <string.h>

void ResizeStr(char*& str, size_t size, size_t cap) {
  char* new_str = new char[cap];
  for (size_t i = 0; i <= size; i++) {
    new_str[i] = str[i];
  }
  delete[] str;
  str = new_str;
}

char* GetStr() {
  getchar();
  size_t size = 0;
  size_t cap = 64;
  char* str = new char[cap];
  while (true) {
    if (size >= cap - 1) {
      cap *= 2;
      ResizeStr(str, size, cap);
    }
    str[size] = getchar();
    if(str[size] == '\n') {
      str[size]= '\0';
      break;
    }
    ++size;
  }
  return str;
}

void ResizeStack(char**& stack, size_t size, size_t new_cap) {
  char** new_stack = new char*[new_cap];
  for (size_t i = 0; i <= size; i++) {
    new_stack[i] = stack[i];
  }
  delete[] stack;
  stack = new_stack;
}

char* back(char**& stack, const size_t& size) {
  if (size <= 0) {
    std::cout << "error\n";
    return nullptr;
  }
  std::cout << stack[size] << '\n';
  return stack[size];
}

void pop(char**& stack, size_t& size, size_t& cap) {
  back(stack, size);
  if (size <= 0) return;
  delete[] stack[size];
  --size;
  if (size * 4 <= cap) {
    cap /= 2;
    ResizeStack(stack, size, cap);
  }
}

// void pop(char**& stack, size_t& size, size_t& cap) {
//   if (size <= 0) return;
//   delete[] stack[size];
//   --size;
//   if (size * 4 <= cap) {
//     cap /= 2;
//     ResizeStack(stack, size, cap);
//   }
// }

void clear(char**& stack, size_t& size) {
  while (size > 0) {
    delete[] stack[size];
    --size;
  }
}

void push(char**& stack, size_t& size, size_t& cap) {
  ++size;
  if (size >= cap) {
    cap *= 2;
    ResizeStack(stack, size, cap);
  }
  stack[size] = GetStr();
}

int main() {
  size_t cap = 64;
  size_t size = 0;
  char** stack = new char*[cap];
  char comm[8];
  while (true) {
    std::cin >> comm;
    if (std::strcmp(comm, "push") == 0) {
      push(stack, size, cap);
      std::cout << "ok\n";

    } else if (std::strcmp(comm, "pop") == 0) {
      pop(stack, size, cap);

    } else if (std::strcmp(comm, "back") == 0) {
      char* b = back(stack, size);

    } else if (std::strcmp(comm, "size") == 0) {
      std::cout << size << '\n';

    } else if (std::strcmp(comm, "clear") == 0) {
      clear(stack, size);
      std::cout << "ok\n";

    } else if (std::strcmp(comm, "exit") == 0) {
      clear(stack, size);
      delete[] stack;
      std::cout << "bye\n";
      break;
    }
  }
}