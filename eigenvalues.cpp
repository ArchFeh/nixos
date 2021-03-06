#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

short sign(double num) {
  if (num > 0)
    return 1;
  if (num < 0)
    return -1;
}

double cosinus_Fk(double x, double y) {
  if (y == 0) {
    return sqrt(2) / 2;
  } else {
    return sqrt(0.5 + abs(y) / (2 * sqrt(pow(x, 2) + pow(y, 2))));
  }
}

double sinus_Fk(double x, double y) {
  if (y == 0) {
    return sqrt(2) / 2;
  } else {
    return sign(x * y) * abs(x) /
           (2 * cosinus_Fk(x, y) * sqrt(pow(x, 2) + pow(y, 2)));
  }
}

double out_of_diag_sum(double **A, int matrix_size) {
  double sum = 0;
  for (int i = 0; i < matrix_size; i++) {
    for (int j = 0; j < matrix_size; j++) {
      if (i != j) {
        sum += pow(abs(A[i][j]), 2);
      }
    }
  }
}

void chosen_element(double **A, short strategy, int matrix_size, int *i, int *j,
                    int iteration, double *string_sums) {
  switch (strategy) {
  case 0: { // Выбираем опорный элемент среди всех элементов матрицы
    double max_elem = 0;
    for (int x = 0; x < matrix_size; x++) {
      for (int y = 0; y < matrix_size; y++) {
        if ((x != y) && (abs(A[x][y]) > abs(max_elem))) {
          max_elem = A[x][y];
          *i = x;
          *j = y;
        }
      }
    }
    break;
  }
  case 1: { // Пронумеровали все элементы и соотнесли с итерацией цикла
    int number = pow(matrix_size, 2) - matrix_size;
    iteration = iteration % number;
    int current_string = (iteration) / (matrix_size - 1);
    *i = current_string;

    int mod;
    mod = iteration % (matrix_size - 1);
    if (mod >= current_string) {
      *j = mod + 1;
    } else {
      *j = mod;
    }
    break;
  }

  case 2: { // Выбор максимального элемента из максимальной по модулю строчки
    int max_line = 0;
    double max_sum = string_sums[0];

    for (int string = 1; string < matrix_size; string++) {
      double line_sum = string_sums[string];
      if (line_sum >= max_sum) {
        max_line = string;
        max_sum = line_sum;
      }
    }

    double max_elem = 0;

    for (int column = 0; column < matrix_size; column++) {
      if ((column != max_line) && (abs(A[max_line][column]) > abs(max_elem))) {
        max_elem = abs(A[max_line][column]);
        *j = column;
        *i = max_line;
      }
    }
    break;
  }

  default: {
  }
  }
}

int yakobi_rotation(int matrix_size, double **A, double **Eig, short strategy,
                    double epsilon, double *eigenvalues) {
  int iteration = 0;
  while (out_of_diag_sum(A, matrix_size) > epsilon) {
    int i = 0;
    int j = 0;
    double *square_sums = (double *)calloc(sizeof(double), matrix_size);
    int max_string1 = 0, max_string2 = 0, max_string3 = 0;
    double max_value1 = 0, max_value2 = 0, max_value3 = 0;

    for (int x = 0; x < matrix_size; x++) {
      for (int y = 0; y < matrix_size; y++) {
        if (x != y) {
          square_sums[x] = square_sums[x] + A[x][y] * A[x][y];
        }
      }
    }

    chosen_element(A, strategy, matrix_size, &i, &j, iteration, square_sums);

    square_sums[i] = 0;
    square_sums[j] = 0;

    double cos = cosinus_Fk(A[i][j] * (-2), A[i][i] - A[j][j]);
    double sin = sinus_Fk(A[i][j] * (-2), A[i][i] - A[j][j]);
    double Aii =
        pow(cos, 2) * A[i][i] + pow(sin, 2) * A[j][j] - 2 * cos * sin * A[i][j];
    double Ajj =
        pow(cos, 2) * A[j][j] + pow(sin, 2) * A[i][i] + 2 * cos * sin * A[i][j];

    for (int k = 0; k < matrix_size; k++) {
      double temp3 = Eig[i][k] * cos - sin * Eig[j][k];
      double temp4 = Eig[i][k] * sin + cos * Eig[j][k];
      if ((k != i) && (k != j)) {
        double temp1 = A[i][k] * cos - sin * A[j][k];
        double temp2 = A[k][i] * sin + cos * A[k][j];
        square_sums[k] = square_sums[k] - A[k][i] * A[k][i] -
                         A[k][j] * A[k][j] + temp1 * temp1 + temp2 * temp2;
        A[i][k] = temp1;
        A[k][i] = temp1;
        A[k][j] = temp2;
        A[j][k] = temp2;
        square_sums[i] += temp1 * temp1;
        square_sums[j] += temp2 * temp2;
      }
      Eig[i][k] = temp3;
      Eig[j][k] = temp4;
    }
    A[i][i] = Aii;
    A[j][j] = Ajj;
    A[i][j] = 0;
    A[j][i] = 0;
    iteration++;
  }
  for (int i = 0; i < matrix_size; i++) {
    eigenvalues[i] = A[i][i];
  }
  return iteration;
}

double formula(int x, int y, int matrix_size) {
  if (x == y) {
    if (x != matrix_size - 1) {
      return 1;
    }
  }
  if (x == matrix_size - 1) {
    return y + 1;
  }
  if (y == matrix_size - 1) {
    return x + 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  FILE *file = fopen("matrix.txt", "r+");
  int matrix_size = atoi(argv[1]);
  // int matrix_size = 3;
  double end;
  double begin;
  double **A = (double **)calloc(sizeof(double *), matrix_size);
  for (int i = 0; i < matrix_size; i++) {
    A[i] = (double *)calloc(sizeof(double), matrix_size);
  }

  double **Eig = (double **)calloc(sizeof(double *), matrix_size);
  for (int i = 0; i < matrix_size; i++) {
    Eig[i] = (double *)calloc(sizeof(double), matrix_size);
    Eig[i][i] = 1;
  }

  double *eigenvalues = (double *)calloc(sizeof(double), matrix_size);
  char *input = argv[2];
  // char input = 'k';
  if (strcmp(input, "formula") == 0) {
    for (int i = 0; i < matrix_size; i++) {
      for (int j = 0; j < matrix_size; j++) {
        A[i][j] = formula(i, j, matrix_size);
      }
    }
  } else {
    for (int i = 0; i < matrix_size; i++) {
      for (int j = 0; j < matrix_size; j++) {
        fscanf(file, "%lf", &A[i][j]);
      }
    }
  }
  if (matrix_size < 99) {
    for (int i = 0; i < matrix_size; i++) {
      for (int j = 0; j < matrix_size; j++) {
        printf("%lf  ", A[i][j]);
      }
      printf("\n");
    }
  }

  short strategy = atoi(argv[3]);
  begin = clock();
  int iters =
      yakobi_rotation(matrix_size, A, Eig, strategy, 0.00001, eigenvalues);
  end = clock();

  printf("\nСобственные значения:\n");
  for (int j = 0; j < matrix_size; j++) {
    printf("%lf  ", eigenvalues[j]);
  }

  FILE *filestream;
  if (matrix_size < 11) {
    filestream = stdout;
  } else {
    filestream = file;
  }
  // if (matrix_size < 99) {
  fprintf(filestream, "\n\nСобственный вектор:\n");
  for (int j = 0; j < matrix_size; j++) {
    fprintf(filestream, "[ ");
    for (int k = 0; k < matrix_size; k++) {
      fprintf(filestream, "%lf  ", Eig[k][j]);
    }
    fprintf(filestream, "] - λ%d\n", j + 1);
  }
  //}

  printf("\n");

  printf("Итерация:%d\n", iters);
  printf("Время:%f\n", ((end - begin) / CLOCKS_PER_SEC));

  // fclose(file);
  for (int k = 0; k < matrix_size; k++) {
    free(A[k]);
    free(Eig[k]);
  }
  free(Eig);
  free(eigenvalues);
  free(A);
}
