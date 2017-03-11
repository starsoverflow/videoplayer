#pragma once

//class testRun {
//public:
//	int a;
//	int geta() { return a; }
//};
//
//template<typename T>
//class IRunnable {
//
//public:
//	IRunnable() {
//		_class = new T();
//	};
//	~IRunnable() {
//		delete _class;
//	}
//	IRunnable& operator= (const IRunnable &) = delete;
//	IRunnable(const IRunnable &) = delete;
//
//	T* operator->() {
//		return _class;
//	}
//
//private:
//	T* _class;
//};
//
//int main2()
//{
//	IRunnable<testRun> a;
//	a->a
//}