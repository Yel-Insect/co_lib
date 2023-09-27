#include <iostream>
#include "log.h"
// #include "util.h"

static co_lib::Logger::ptr g_logger = LOG_ROOT();
int main(int argc, char** argv) {

	// co_lib::Logger::ptr logger(new co_lib::Logger);
	// logger->addAppender(co_lib::LogAppender::ptr(new co_lib::StdoutLogAppender));

	// co_lib::FileLogAppender::ptr file_appender(new co_lib::FileLogAppender("./log.txt"));
	// logger->addAppender(file_appender);
	// //co_lib::LogEvent::ptr event(new co_lib::LogEvent(__FILE__, __LINE__, 0, co_lib::GetThreadId(), co_lib::GetFiberId(), time(0)));
	// //logger->log(co_lib::LogLevel::DEBUG, event);
	// SYLAR_LOG_INFO(logger) << "test macro";
	// SYLAR_LOG_ERROR(logger) << "test macro error";

	// SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");
	LOG_DEBUG(g_logger) << "hh";
	
	return 0;
}

