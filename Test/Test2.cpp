#include <iostream>
#include <chrono>

using namespace std;
using namespace chrono;
constexpr int CACHE_LINE_SIZE = 64;

int main()
{
	for (int i = 0; i < 20; ++i) {
		const int size = 1024 << i;
		char* a = (char*)malloc(size);
		unsigned int index = 0;
		int tmp = 0;

		auto start = high_resolution_clock::now();
		for (int j = 0; j < 1'0000'0000; ++j) {
			tmp += a[index % size];
			index += CACHE_LINE_SIZE * 11;
		}
		auto dur = high_resolution_clock::now() - start;

		cout << "Size : " << size / 1024 << "K, ";
		cout << "Time " << duration_cast<milliseconds>(dur).count();
		cout << " msec " << tmp << endl;
	}

	// 캐시 미스로 인해 2MB > 8MB에서 속도가 현저히 느려짐
	// CPU의 L1, L2, L3의 용량을 넘는 시점에 느려짐
}