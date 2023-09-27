#include <iostream>
#include <yaml-cpp/yaml.h>
#include <assert.h>
#include <vector>
#include "co_lib.h"

static co_lib::Logger::ptr g_logger = LOG_ROOT();

void test_fiber() {
	static int s_count = 5;
	LOG_ERROR(g_logger) << "test in fiber s_count : " << s_count;
	sleep(1);
	if (--s_count >= 0){
		co_lib::Scheduler::GetThis()->schedule(&test_fiber);//, co_lib::GetThreadId());
	}
}

int main(int argc, char** argv) {
	co_lib::Scheduler sc(1, true, "test");
	
	sc.start();
	// sleep(2);
	sc.schedule(&test_fiber);
	// LOG_DEBUG(g_logger) << " before stop()";
	sc.stop();
	// LOG_DEBUG(g_logger) << " after stop()";
	return 0;
 }
