#ifndef __LOG_H__
#define __LOG_H__

#include <string>
#include <memory>
#include <stdint.h>
#include <vector>
#include <list>
#include <fstream>
#include <sstream>
#include <stdarg.h>
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"

#define LOG_LEVEL(logger, level) \
	if (logger->getLevel() <= level) 	\
		co_lib::LogEventWrap(co_lib::LogEvent::ptr(new \
		co_lib::LogEvent(logger, level, __FILE__, __LINE__, 0, co_lib::GetThreadId(),  \
		co_lib::GetFiberId(), time(0), co_lib::Thread::GetName()))).getSS()
	
#define LOG_DEBUG(logger) LOG_LEVEL(logger, co_lib::LogLevel::DEBUG)
#define LOG_INFO(logger) LOG_LEVEL(logger, co_lib::LogLevel::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger, co_lib::LogLevel::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger, co_lib::LogLevel::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger, co_lib::LogLevel::FATAL)

#define LOG_FMT_LEVEL(logger, level, fmt, ...) \
	if (logger->getLevel() <= level) \
		co_lib::LogEventWrap(co_lib::LogEvent::ptr(new co_lib::LogEvent(logger, level, \
							__FILE__, __LINE__, 0, co_lib::GetThreadId(), \
							co_lib::GetFiberId(), time(0), co_lib::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

#define LOG_FMT_DEBUG(logger, fmt, ...) LOG_FMT_LEVEL(logger, co_lib::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_FMT_INFO(logger, fmt, ...) LOG_FMT_LEVEL(logger, co_lib::LogLevel::INFO, fmt, __VA_ARGS__)
#define LOG_FMT_WARN(logger, fmt, ...) LOG_FMT_LEVEL(logger, co_lib::LogLevel::WARN, fmt, __VA_ARGS__)
#define LOG_FMT_ERROR(logger, fmt, ...) LOG_FMT_LEVEL(logger, co_lib::LogLevel::ERROR, fmt, __VA_ARGS__)
#define LOG_FMT_FATAL(logger, fmt, ...) LOG_FMT_LEVEL(logger, co_lib::LogLevel::FATAL, fmt, __VA_ARGS__)

#define LOG_ROOT() co_lib::LoggerMgr::GetInstance()->getRoot()
#define LOG_NAME(name) co_lib::LoggerMgr::GetInstance()->getLogger(name)

namespace co_lib{
class LoggerManager;
class Logger;

// 日志级别
class LogLevel {
public:
	enum Level{
		UNKNOW = 0,
		DEBUG = 1,
		INFO = 2,
		WARN = 3,
		ERROR = 4,
		FATAL = 5,
	};
	
	static const char* ToString(LogLevel::Level level);
	static LogLevel::Level FromString(const std::string& str);
};


//日志事件
class LogEvent{
public:
	typedef std::shared_ptr<LogEvent> ptr;
	LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
			, const char* file, int32_t line, uint32_t elapse
			, uint32_t thread_id, uint32_t fiber_id
			, uint32_t time, const std::string& threadName);
	~LogEvent() {}
	const char* getFile() const { return m_file; }
	int32_t getLine() const { return m_line; }
	uint32_t getElapse() const { return m_elapse; }
	uint32_t getThreadId() const { return m_threadId; }
	uint32_t getFiberId() const { return m_fiberId; }
	uint32_t getTime() const { return m_time; }
	const std::string& getThreadName() const { return m_threadName; }
	std::string getContent() const { return m_ss.str(); }
	std::shared_ptr<Logger> getLogger() const { return m_logger; }
	std::stringstream& getSS() { return m_ss; }
	LogLevel::Level getLevel() const { return m_level; }
	void format(const char* fmt, ...);
	void format(const char* fmt, va_list al);
private:
	const char* m_file = nullptr;	//文件名
	int32_t m_line = 0;				//行号
	uint32_t m_elapse = 0;			//程序启动开始到现在的毫秒数
	uint32_t m_threadId = 0;		//线程id
	uint32_t m_fiberId = 0;			//协程id
	uint64_t m_time = 0;			//时间戳
	std::stringstream m_ss;			//日志内容流
	std::string m_threadName;
	std::shared_ptr<Logger> m_logger;
	LogLevel::Level m_level;
};

class LogEventWrap {
public:
	LogEventWrap(LogEvent::ptr e);
	~LogEventWrap();
	std::stringstream& getSS();
	LogEvent::ptr getEvent() { return m_event; }
private:
	LogEvent::ptr m_event;
};



//日志格式器
class LogFormatter {
public:
	typedef std::shared_ptr<LogFormatter> ptr;
	std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
	LogFormatter(const std::string& pattern);
public:
	void init();
	class FormatItem {
	public:
		typedef std::shared_ptr<FormatItem> ptr;
		//FormatItem(const std::string fmt = "") {};
		virtual ~FormatItem() {}
		virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
	};

	bool isError() const { return m_error; }
	const std::string getPattern() const { return m_pattern; }
private:
	std::string m_pattern;
	std::vector<FormatItem::ptr> m_items;
	bool m_error = false;
};

//日志输出地
class LogAppender {
friend Logger;
public:
	typedef std::shared_ptr<LogAppender> ptr;
	typedef Mutex MutexType;
	virtual ~LogAppender() {};
	virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
	virtual std::string toYamlString() = 0;
	LogFormatter::ptr getFormatter();
	void setFormatter(LogFormatter::ptr val);
	LogLevel::Level getLevel() const { return m_level; }
	void setLevel(LogLevel::Level val) { m_level = val; }
protected:
	LogLevel::Level m_level = LogLevel::DEBUG;
	LogFormatter::ptr m_formatter;
	bool m_hasFormatter = false;
	MutexType m_mutex;
};

//日志器
class Logger : public std::enable_shared_from_this<Logger> {
friend class LoggerManager;
public:
	typedef std::shared_ptr<Logger> ptr;
	typedef Mutex MutexType;
	Logger(const std::string& name = "root");
	void log(LogLevel::Level level, LogEvent::ptr event);
	
	void debug(LogEvent::ptr event);
	void info(LogEvent::ptr event);
	void warn(LogEvent::ptr event);
	void error(LogEvent::ptr event);
	void fatal(LogEvent::ptr event);
	
	void addAppender(LogAppender::ptr appender);
	void delAppender(LogAppender::ptr appender);
	void clearAppender();
	LogLevel::Level getLevel() const { return m_level; }
	void setLevel(LogLevel::Level val) { m_level = val; }
	
	const std::string getName() const { return m_name; }

	void setFormatter(LogFormatter::ptr val);
	void setFormatter(const std::string& val);
	LogFormatter::ptr getFormatter();

	std::string toYamlString();
private:
	std::string m_name;			//日志名称
	LogLevel::Level m_level;		//日志等级
	std::list<LogAppender::ptr> m_appenders; 	//Appender集合
	LogFormatter::ptr m_formatter;
	Logger::ptr m_root;
	MutexType m_mutex;
};

//输出到控制台的Appender
class StdoutLogAppender : public LogAppender{
public:
	typedef std::shared_ptr<StdoutLogAppender> ptr;
	std::string toYamlString() override;
	void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
};

//定义输出到文件的Appender
class FileLogAppender : public LogAppender{
public:
	typedef std::shared_ptr<FileLogAppender> ptr;
	std::string toYamlString() override;
	FileLogAppender(const std::string filename);
	void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
	
	// 重新打开文件
	bool reopen();
private:
	std::string m_filename;			// 文件路径
	std::ofstream m_filestream;		// 文件流
	uint64_t m_lastTime = 0;		// 上次重新打开的时间
};

class LoggerManager {
public:
	typedef Mutex MutexType;
	LoggerManager();
	Logger::ptr getLogger(const std::string& name);
	Logger::ptr getRoot() { return m_root; }
	void init();
	std::string toYamlString();
private:
	MutexType m_mutex;
	std::map<std::string, Logger::ptr> m_loggers;
	Logger::ptr m_root;
};

typedef co_lib::Singleton<LoggerManager> LoggerMgr;

}


#endif







