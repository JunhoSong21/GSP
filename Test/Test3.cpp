#include <iostream>
#include <chrono>

using namespace std;
using namespace chrono;

const int N = N;

int main()
{
	unsigned int* h_A = new unsigned int[N * N];
	unsigned int* h_B = new unsigned int[N * N];
	unsigned int* h_C = new unsigned int[N * N];
	for (int i = 0; i < N; ++i) {
		h_A[i] = i;
		h_B[i] = i;
		h_C[i] = 0;
	}

	auto start = high_resolution_clock::now();

	for (unsigned int y = 0; y < N; ++y) {
		for (unsigned int x = 0; x < N; x++) {
			for (unsigned int i = 0; i < N; ++i)
				h_C[y * N + x] += h_A[i + y * N] * h_B[i * N + x];
		}
	}
	/*
	for (unsigned int y = 0; y < N; ++y) {
		for (unsigned int x = 0; x < N; x++)
			for (unsigned int i = 0; i < N; ++i)
				h_C[y * N + i] += h_A[x + y * N] * h_B[x * N + i];
	}
	*/

	auto end = high_resolution_clock::now();

	cout << "Time " << duration_cast<milliseconds>(end - start).count() << endl;
}