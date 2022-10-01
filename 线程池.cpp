#include<iostream>
#include<random>
#include"pool.h"
random_device rd;
mt19937 mt(rd());

uniform_int_distribution<int> dist(-1000, 1000);

auto rnd = bind(dist, mt);

void simulate_hard_computation()
{
	this_thread::sleep_for(chrono::milliseconds(2000+rnd()));
}
void multiply(const int a, const int b)
{
	simulate_hard_computation();
	const int res = a * b;
	std::cout << a << " * " << b << " = " << res << std::endl;
}
void multiply_output(long&out, const long a, const long b)
{
	simulate_hard_computation();
	out = a * b;
	cout << a << " * " << b << " = " << out << endl;
}
int multiply_return(const int a, const int b)
{
	simulate_hard_computation();
	const int res = a * b;
	std::cout << a << " * " << b << " = " << res << std::endl;
	return res;
}
void example()
{
	ThreadPool pool(1,100,1000);
	pool.Init();

	for (int i = 1; i <= 10000; i+=1)
	{
		for (int j = 1; j <=10000; j+=1)
		{
			auto x = pool.submit(multiply, i, j);
		
		}
	}

	long output_ref;
	auto fut1 = pool.submit(multiply_output, ref(output_ref),5,6);

	fut1.get();
	cout << "Last operation result is equals to " << output_ref <<endl;

	auto fut2 = pool.submit(multiply_return, 5, 3);
	int res = fut2.get();
	cout<< "Last operation result is equals to " << res <<endl;

	pool.ShutDown();
}

int main()
{
	example();
}
