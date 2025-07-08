#include <iostream>

long long Counting(int** &array, int* const &inds, int* &sizes, int inds_size, int array_size) {
  if (inds_size == array_size) {
    long long ans = 1;
    for (int i = 0; i < inds_size; i++) ans *= array[i][inds[i]];
    return ans;
  }
  long long ans = 0;
  for (int ind = 0; ind < sizes[inds_size]; ind++) {
    bool repeated = false;
    for (int j = 0; j < inds_size; j++) {
      if (ind == inds[j]) {
        repeated = true;
        break;
      }
    }
    if (repeated) {
      continue;
    }
    inds[inds_size] = ind;
    ans += Counting(array, inds, sizes, inds_size + 1, array_size);
  }
  return ans;
}

int main(int argc, char* argv[]) {
  int** array = new int*[argc-1];
  int* sizes = new int[argc-1];
  for (int i = 1; i < argc; i++) {
    // int* vector = new int[];
    int size;
    sscanf(argv[i], "%d", &size);
    sizes[i - 1] = size;
    int* vec = new int[size];
    for (int j = 0; j < size; j++) {
      std::cin >> vec[j];
    }
    array[i - 1] = vec;
  }
  int* inds = new int[argc-1];
  std::cout << Counting(array, inds, sizes, 0, argc-1);
  for (int i = 0; i < argc-1; i++) delete[] array[i];
  delete[] inds;
  delete[] array;
  delete[] sizes;
}