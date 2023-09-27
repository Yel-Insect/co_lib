#include <iostream>
#include <yaml-cpp/yaml.h>
#include <assert.h>
#include <vector>
#include "co_lib.h"


#if 0
int cnt = 0;
co_lib::Mutex s_mutex;

void fun1() {
	LOG_INFO(g_logger) << "name: " << co_lib::Thread::GetName()
							 << " this.name: " << co_lib::Thread::GetThis()->getName()
							 << " id: " << co_lib::GetThreadId()
							 << " this.id" << co_lib::Thread::GetThis()->getId();
	for (int i = 0; i < 100000; i++) {
		co_lib::Mutex::Lock lock(s_mutex);
		//s_mutex.rdlock();
		cnt++;
		//s_mutex.unlock();
	}
}

void test_thread() {
	LOG_INFO(g_logger) << "thread test begin";
	YAML::Node root = YAML::LoadFile("/home/beautiful/桌面/server/co_lib/log.yml");
	co_lib::Config::LoadFromYaml(root);
	std::vector<co_lib::Thread::ptr> thrs;
	for (int i = 0; i < 5; i++) {
		co_lib::Thread::ptr thr(new co_lib::Thread(&fun1, "name_" + std::to_string(i)));
		thrs.push_back(thr);
	}
	for (int i = 0; i < 5; i++) {
		thrs[i]->join();
	}
	LOG_INFO(LOG_NAME("system")) << "fxxk system";
	std::cout << root << std::endl;
	LOG_INFO(g_logger) << "thread test end";
	LOG_INFO(g_logger) << cnt;

	co_lib::Config::Visit([](co_lib::ConfigVarBase::ptr var) {
		LOG_INFO(g_logger) << "name" << var->getName()
					<< " description = " << var->getDescription()
					<< " typename = " << var->getTypeName()
					<< " value = " << var->toString();
	});
}
co_lib::Logger::ptr g_logger = LOG_ROOT();
void test_assert() {
	LOG_INFO(g_logger) << co_lib::BacktraceToString(10);
	CO_LIB_ASSERT2(0 == 1, "fk yzt");
}
#endif

static co_lib::Logger::ptr g_logger = LOG_ROOT();
void run_in_fiber() {
	LOG_INFO(g_logger) << "run_in_fiber begin";
	co_lib::Fiber::YieldToHold();
	LOG_INFO(g_logger) << "run_in_fiber end";
	// co_lib::Fiber::YieldToHold();
}

void test_fiber() {
	co_lib::Fiber::GetThis();
	LOG_INFO(g_logger) << "main begin";
	co_lib::Fiber::ptr fiber(new co_lib::Fiber(run_in_fiber));
	fiber->swapIn();
	LOG_INFO(g_logger) << "main after swapIn";
	fiber->swapIn();
	LOG_INFO(g_logger) << "main after end";
}

int main(int argc, char** argv) {
	co_lib::Thread::SetName("main");
	
	std::vector<co_lib::Thread::ptr> thrs;
	for (int i = 0; i < 3; i++) {
		thrs.push_back(co_lib::Thread::ptr(new co_lib::Thread(&test_fiber, "name_" + std::to_string(i))));
	}

	for (auto& i : thrs) {
		i->join();
	}
	return 0;
 }
