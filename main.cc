#include <iostream>
#include <yaml-cpp/yaml.h>
#include <assert.h>
#include "sylar.h"


#if 0
int cnt = 0;
sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
sylar::Mutex s_mutex;

void fun1() {
	SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
							 << " this.name: " << sylar::Thread::GetThis()->getName()
							 << " id: " << sylar::GetThreadId()
							 << " this.id" << sylar::Thread::GetThis()->getId();
	for (int i = 0; i < 100000; i++) {
		sylar::Mutex::Lock lock(s_mutex);
		//s_mutex.rdlock();
		cnt++;
		//s_mutex.unlock();
	}
}

void test_thread() {
	SYLAR_LOG_INFO(g_logger) << "thread test begin";
	YAML::Node root = YAML::LoadFile("/home/beautiful/桌面/server/sylar/log.yml");
	sylar::Config::LoadFromYaml(root);
	std::vector<sylar::Thread::ptr> thrs;
	for (int i = 0; i < 5; i++) {
		sylar::Thread::ptr thr(new sylar::Thread(&fun1, "name_" + std::to_string(i)));
		thrs.push_back(thr);
	}
	for (int i = 0; i < 5; i++) {
		thrs[i]->join();
	}
	SYLAR_LOG_INFO(SYLAR_LOG_NAME("system")) << "fxxk system";
	std::cout << root << std::endl;
	SYLAR_LOG_INFO(g_logger) << "thread test end";
	SYLAR_LOG_INFO(g_logger) << cnt;

	sylar::Config::Visit([](sylar::ConfigVarBase::ptr var) {
		SYLAR_LOG_INFO(g_logger) << "name" << var->getName()
					<< " description = " << var->getDescription()
					<< " typename = " << var->getTypeName()
					<< " value = " << var->toString();
	});
}
sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void test_assert() {
	SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
	SYLAR_ASSERT2(0 == 1, "fk yzt");
}
#endif

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void run_in_fiber() {
	SYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";
	sylar::Fiber::GetThis()->swapOut();
	SYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
}

int main(int argc, char** argv) {
	sylar::Fiber::GetThis();
	SYLAR_LOG_INFO(g_logger) << "main begin";
	sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
	fiber->swapIn();
	SYLAR_LOG_INFO(g_logger) << "main after swapIn";
	fiber->swapIn();
	SYLAR_LOG_INFO(g_logger) << "main after end";
	return 0;
 }
