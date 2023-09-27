#include "log.h"
#include <map>
#include <iostream>
#include <functional>
#include <string>
#include <time.h>
#include "config.h"





namespace co_lib{

class MessageFormatItem : public LogFormatter::FormatItem {
public:
	MessageFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getContent();
	}
};


class LevelFormatItem : public LogFormatter::FormatItem {
public:
	LevelFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << LogLevel::ToString(level);
	}
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
	ElapseFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getElapse();
	}
};
class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLogger()->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
	ThreadIdFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getThreadId();
	}
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
	FiberIdFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getFiberId();
	}
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
	ThreadNameFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getThreadName();
	}
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
	TabFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << "\t";
	}
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
	DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S") 
		: m_format(format) {
			if (m_format.empty()) {
				m_format = "%Y-%m-%d %H:%M:%S";
			}
	}
	void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
		struct tm tm;
		time_t time = event->getTime();
		localtime_r(&time, &tm);
		char buf[64];
		strftime(buf, sizeof(buf), m_format.c_str(), &tm);
		os << buf;
	}
private:
	std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
	FilenameFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getFile();
	}
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
	LineFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getLine();
	}
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
	NewLineFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << std::endl;
	}
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
	StringFormatItem(const std::string& str)
		: m_string(str) {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << m_string;
	}
private:
	std::string m_string;
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
			, const char* file, int32_t line, uint32_t elapse
			, uint32_t thread_id, uint32_t fiber_id
			, uint32_t time, const std::string& threadName) 
			: m_file(file), m_line(line), m_elapse(elapse)
			, m_threadId(thread_id), m_fiberId(fiber_id), m_time(time)
			, m_logger(logger), m_level(level), m_threadName(threadName) {
			}

const char* LogLevel::ToString(LogLevel::Level level) {
	switch(level) {
#define XX(name) \
	case LogLevel::name: \
		return #name; \
		break;
	XX(DEBUG);
	XX(INFO);
	XX(WARN);
	XX(ERROR);
	XX(FATAL);
#undef XX
	default:
		return "UNKNOW";
	}
	return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(name) \
	if (str == #name) { \
		return LogLevel::name;	\
	}
	XX(DEBUG);
	XX(INFO);
	XX(WARN);
	XX(ERROR);
	XX(FATAL);
#undef XX
	return LogLevel::UNKNOW;
}

void LogEvent::format(const char* fmt, ...) {
	va_list al;
	va_start(al, fmt);
	format(fmt, al);
	va_end(al);

}

void LogEvent::format(const char* fmt, va_list al) {
	char* buf = nullptr;
	int len = vasprintf(&buf, fmt, al);
	if (len != -1) {
		m_ss << std::string(buf, len);
		free(buf);
	}
}

LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e) {
}

LogEventWrap::~LogEventWrap() {
	m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream& LogEventWrap::getSS() {
	return m_event->getSS();
}

void LogAppender::setFormatter(LogFormatter::ptr val) {
	MutexType::Lock lock(m_mutex);
	m_formatter = val;
	if (m_formatter) {
		m_hasFormatter = true;
	} else {
		m_hasFormatter = false;
	}
}

LogFormatter::ptr LogAppender::getFormatter() {
	MutexType::Lock lock(m_mutex);
	return m_formatter;
}

Logger::Logger (const std::string& name) 
	: m_name(name), m_level(LogLevel::Level::DEBUG) {
	m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
	if (level >= m_level) {
		auto self = shared_from_this();
		MutexType::Lock lock(m_mutex);
		if (!m_appenders.empty()) {
			for (auto& i : m_appenders) {
				i->log(self, level, event);
			}
		} else if (m_root) {
			m_root->log(level, event);
		}
		
	}
}
void Logger::delAppender(LogAppender::ptr appender) {
	MutexType::Lock lock(m_mutex);
	for (auto it = m_appenders.begin(); it != m_appenders.end(); it++) {
		if (*it == appender) {
			m_appenders.erase(it);
			break;
		}
	}
}

void Logger::clearAppender() {
	MutexType::Lock lock(m_mutex);
	m_appenders.clear();
}

void Logger::addAppender(LogAppender::ptr appender) {
	MutexType::Lock lock(m_mutex);
	// appender有没有formatter， 没有则把logger给它
	if (!appender->getFormatter()) {
		MutexType::Lock lock1(appender->m_mutex);
		appender->m_formatter = m_formatter;
	}
	m_appenders.push_back(appender);
}

void Logger::setFormatter(LogFormatter::ptr val) {
	MutexType::Lock lock(m_mutex);
	m_formatter = val;

	for (auto& i : m_appenders) {
		MutexType::Lock lock1(i->m_mutex);
		if (!i->m_hasFormatter) {
			i->m_formatter = m_formatter;
		}
	}
}

void Logger::setFormatter(const std::string& val) {
	//m_formatter.reset(new co_lib::LogFormatter(val));
	co_lib::LogFormatter::ptr new_val(new co_lib::LogFormatter(val));
	// 判断new_val是否解析错误
	if (new_val->isError()) {
		std::cout << "Logger setFormatter name = " << m_name
				<< " value = " << val << " invalid formatter"
				<< std::endl;
	}
	setFormatter(new_val);
}

std::string Logger::toYamlString() {
	MutexType::Lock lock(m_mutex);
	YAML::Node node;
	node["name"] = m_name;
	if (m_level != LogLevel::UNKNOW) {
		node["level"] = LogLevel::ToString(m_level);
	}
	if (m_formatter) {
		node["formatter"] = m_formatter->getPattern();
	}

	for (auto& i : m_appenders) {
		node["appenders"].push_back(YAML::Load(i->toYamlString()));
	}
	std::stringstream ss;
	ss << node;
	return ss.str();
}

LogFormatter::ptr Logger::getFormatter() {
	MutexType::Lock lock(m_mutex);
	return m_formatter;
}

void Logger::debug(LogEvent::ptr event) {
	log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event) {
	log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event) {
	log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event) {
	log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event) {
	log(LogLevel::FATAL, event);
}

FileLogAppender::FileLogAppender(const std::string filename) 
	: m_filename(filename) {
	reopen();
}

bool FileLogAppender::reopen() {
	MutexType::Lock lock(m_mutex);
	if (m_filestream) {
		m_filestream.close();
	}
	m_filestream.open(m_filename, std::ios::app);
	return !!m_filestream;
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	if(level >= m_level) {
        uint64_t now = event->getTime();
        if(now >= (m_lastTime + 3)) {
            reopen();
            m_lastTime = now;
        }
        MutexType::Lock lock(m_mutex);
        if(!(m_filestream << m_formatter->format(logger, level, event))) {
            std::cout << "error" << std::endl;
        }
    }
}

std::string FileLogAppender::toYamlString() {
	MutexType::Lock lock(m_mutex);
	YAML::Node node;
	node["type"] = "FileLogAppender";
	node["file"] = m_filename;
	if (m_level != LogLevel::Level::UNKNOW) {
		node["level"] = LogLevel::ToString(m_level);
	}
	if (m_hasFormatter && m_formatter) {
		node["formatter"] = m_formatter->getPattern();
	}
	std::stringstream ss;
	ss << node;
	return ss.str();
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	if (level >= m_level) {
		MutexType::Lock lock(m_mutex);
		std::cout << m_formatter->format(logger, level, event);
	}
}

std::string StdoutLogAppender::toYamlString() {
	MutexType::Lock lock(m_mutex);
	YAML::Node node;
	node["type"] = "StdoutLogAppender";
	if (m_level != LogLevel::Level::UNKNOW) {
		node["level"] = LogLevel::ToString(m_level);
	}
	if (m_hasFormatter && m_formatter) {
		node["formatter"] = m_formatter->getPattern();
	}
	std::stringstream ss;
	ss << node;
	return ss.str();
}

LogFormatter::LogFormatter(const std::string& pattern) 
	: m_pattern(pattern) {
		init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	std::stringstream ss;
	//std::cout << m_items.size() << std::endl;;
	for (auto& i : m_items) {
		i->format(ss, logger, level, event);
	}
	return ss.str();
}

// %xxx %xxx{xxx} %%
// {}一般括取能够被系统自行处理的格式，如 %d{%Y-%m-%d %H:%M:%S}
void LogFormatter::init() {
	std::vector<std::tuple<std::string, std::string, int>> vec;
	std::string nstr;	// 主要存与格式无关的字符串(如空格)
	for (size_t i = 0; i < m_pattern.size(); i++) {	
		// 提取与格式无关的字符
		if (m_pattern[i] != '%') {
			nstr.append(1, m_pattern[i]);
			continue;
		}
		
		// 识别%%
		if (i + 1 < m_pattern.size()) {
			if (m_pattern[i + 1] == '%') {
				nstr.append(1, '%');
				continue;
			}
		}
		

		// 如果为格式相关字符串
		size_t n = i + 1;
		int fmt_status = 0;
		size_t fmt_begin = 0;
		
		std::string str;	// %后面的格式
		std::string fmt;  // 提取大括号里的格式
		
		while (n < m_pattern.size()) { 
			// 如果没有附带额外格式
			if (!fmt_status && !isalpha(m_pattern[n]) && m_pattern[n] != '{'
				&& m_pattern[n] != '}') {
				str = m_pattern.substr(i + 1, n - i - 1);
				break ;
			}
			if (fmt_status == 0) {
				if (m_pattern[n] == '{') {
					str = m_pattern.substr(i + 1, n - i - 1);
					fmt_status = 1; // 解析格式
					fmt_begin = n;
					++n;
					continue;
					
				}
			} else if (fmt_status == 1) {
				if (m_pattern[n] == '}') {
					fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);	// 提取大括号里的内容
					fmt_status = 0;
					++n;
					break;
				}
			}
			++n;
			if (n == m_pattern.size()) {
				if (str.empty()) {
					str = m_pattern.substr(i + 1);
				}
			}
		}
		
		if (fmt_status == 0) {
			if (!nstr.empty()) {
				vec.push_back(std::make_tuple(nstr, std::string(), 0));
				nstr.clear();
			}
			vec.push_back(std::make_tuple(str, fmt, 1));
			i = n - 1;
		} else if (fmt_status == 1) {	//格式不正确
			//std::cout << "pattern parse error " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
			m_error = true;
			vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
		} 
	}
	
	if (!nstr.empty()) {
		vec.push_back(std::make_tuple(nstr, "", 0));
	}
	//%m -- 消息体
	//%p -- level
	//%r -- 启动后的时间
	//%c -- 日志名称
	//%t -- 线程id
	//%n -- 回车换行
	//%d -- 时间
	//%f -- 文件名
	//%l -- 行号
	
	//在map中对每种格式写入函数
	static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
#define XX(str, C) \
	{#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}
	XX(m, MessageFormatItem),		// m: 消息
	XX(p, LevelFormatItem),			// p: 日志级别
	XX(r, ElapseFormatItem),		// r: 累积毫秒数
	XX(c, NameFormatItem),			// c: 日志名称
	XX(t, ThreadIdFormatItem),		// t: 线程id
	XX(n, NewLineFormatItem),		// n: 换行
	XX(d, DateTimeFormatItem),		// d: 时间
	XX(f, FilenameFormatItem),		// f: 文件名
	XX(l, LineFormatItem),			// l: 行号
	XX(T, TabFormatItem),			// T: Tab
	XX(F, FiberIdFormatItem), 		// F: 协程id
	XX(N, ThreadNameFormatItem)		// N: 线程名称
	
#undef XX
	};
	
	for (auto& i : vec) {
		if (std::get<2>(i) == 0) {
			m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
		} else {
			auto it = s_format_items.find(std::get<0>(i));
			if (it == s_format_items.end()) {
				m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
				m_error = true;
			} else {
				m_items.push_back(it->second(std::get<1>(i)));
			}
		}
		//std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
	}

}

LoggerManager::LoggerManager() {
	m_root.reset(new Logger);
	m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
	m_loggers[m_root->m_name] = m_root; 
	init();
}
Logger::ptr LoggerManager::getLogger(const std::string& name) {
	MutexType::Lock lock(m_mutex);
	auto it = m_loggers.find(name);
	if (it != m_loggers.end()) {
		return it->second;
	}
	// new一个logger返回
	Logger::ptr logger(new Logger(name));
	logger->m_root = m_root;
	m_loggers[name] = logger;
	return logger;
}

std::string LoggerManager::toYamlString() {
	MutexType::Lock lock(m_mutex);
	YAML::Node node;
	for (auto& i : m_loggers) {
		node.push_back(YAML::Load(i.second->toYamlString()));
	}
	std::stringstream ss;
	ss << node;
	return ss.str();
}

// appender的配置信息
struct LogAppenderDefine {
	int type = 0; // 1 File, 2 Stdout
	LogLevel::Level level = LogLevel::UNKNOW;
	std::string formatter;
	std::string file;

	bool operator==(const LogAppenderDefine& oth) const {
		return type == oth.type 
			&& level == oth.level 
			&& formatter == oth.formatter
			&& file == oth.file;
	}
};

// logger的配置信息？
struct LogDefine {
	std::string name;
	LogLevel::Level level = LogLevel::UNKNOW;
	std::string formatter;
	std::vector<LogAppenderDefine> appenders;

	bool operator==(const LogDefine& oth) const {
		return name == oth.name
			&& level == oth.level
			&& formatter == oth.formatter
			&& appenders == oth.appenders;
	}

	bool operator<(const LogDefine& oth) const {
		 return name < oth.name;
	}
};

template<>
class LexicalCast<std::string, std::set<LogDefine>> {
public:
	std::set<LogDefine> operator()(const std::string& v) {
		YAML::Node node = YAML::Load(v);
		std::set<LogDefine> vec;
		for (size_t i = 0; i < node.size(); i++) {	// 遍历node节点
			auto n = node[i];
			if (!n["name"].IsDefined()) {			// 判断name是否定义, 没定义则跳过
				std::cout << "log config error: name is null, " << n
						<< std::endl;
				continue ;
			}
			LogDefine ld;
			ld.name = n["name"].as<std::string>();	// 将名字载入结构体中
			ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() 
				: "");								// 等级
			if (n["formatter"].IsDefined()) {		// 格式刷
				ld.formatter = n["formatter"].as<std::string>();
			}

			if (n["appenders"].IsDefined()) {		// appenders
				for (size_t x = 0; x < n["appenders"].size(); x++) {	// appenders是个数组， 遍历
					auto a = n["appenders"][x];
					if (!a["type"].IsDefined()) {	// 没定义则跳过
						std::cout << "log config error: appender type is null, " << a
							<< std::endl;
							continue;
					}
					std::string type = a["type"].as<std::string>();		// 将appender类型提取出来
					LogAppenderDefine lad;			// 生成一个LogAppenderDefine结构体
					if (type == "FileLogAppender") {	
						lad.type = 1;				// 置为file类型
						if (!a["file"].IsDefined()) {	//文件路径文件名是否被定义
							std::cout << "log config error: fileappender file is null, "
								<< a << std::endl;
							continue;
						}
						lad.file = a["file"].as<std::string>();
						if (a["formatter"].IsDefined()) {	
							lad.formatter = a["formatter"].as<std::string>();
						}
					} else if (type == "StdoutLogAppender") {
						lad.type = 2;			// 置为stdout类型
					} else {
						std::cout << "log config error: error appender type is invalid, " <<
							a << std::endl;
						continue;
					}	// 将appender结构体加入log的appender数组中
					ld.appenders.push_back(lad);
				}
			}
			// set中插入该log结构体
			vec.insert(ld);
		}
		return vec;
	}
};

template<>
class LexicalCast<std::set<LogDefine>, std::string> {
public:
	std::string operator()(const std::set<LogDefine>& v) {
		YAML::Node node;
		for (auto& i : v) {									// 遍历set取出LogDefine
			YAML::Node n;									// 新建一个node
			n["name"] = i.name;								// 赋上属性
			if (i.level != LogLevel::UNKNOW) {
				n["level"] = LogLevel::ToString(i.level);
			}
			if (i.formatter.empty()) {
				n["formatter"] = i.formatter;
			}
															// 遍历appenders
			for (auto& a : i.appenders) {
				YAML::Node na;								// 新建一个节点
				if (a.type == 1) {							// 将appender属性赋给节点
					na["type"] = "FileLogAppender";
					na["file"] = a.file;
				} else if (a.type == 2) {
					na["type"] = "StdoutLogAppender";
				}
				if (a.level != LogLevel::UNKNOW) {
					na["level"] = LogLevel::ToString(a.level);
				}
				if (!a.formatter.empty()) {
					na["formatter"] = a.formatter;
				}
				n["appenders"].push_back(na);				// 将appender节点加入到log的appenders中
			}
			node.push_back(n);								// 将log节点加入到总节点中
		}
		std::stringstream ss;
		ss << node;			// 输出node字符串序列
		return ss.str();
	}
};

co_lib::ConfigVar<std::set<LogDefine>>::ptr g_log_define = 
	co_lib::Config::Lookup("logs", std::set<LogDefine>(), "logs config");


//在main函数前初始化
struct LogIniter {
	LogIniter() {
		g_log_define->addListener([](const std::set<LogDefine>& old_value,
								const std::set<LogDefine>& new_value) {
			LOG_INFO(LOG_ROOT()) << "on_logger_conf_changed";
			for (auto& i : new_value) {
				auto it = old_value.find(i);
				co_lib::Logger::ptr logger;
				if (it == old_value.end()) {
					//logger.reset(new co_lib::Logger(i.name));
					logger = LOG_NAME(i.name);
				} else {
					if (!(i == *it)) {	
						// 修改logger
						logger = LOG_NAME(i.name);
					}
				}
				logger->setLevel(i.level);
				if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }   
				// 将logger的appenders清空
				logger->clearAppender();
				for (auto& a : i.appenders) {	// 取出set中的LogDefine
					co_lib::LogAppender::ptr ap;	// 根据结构体的属性为logger设新值
					if (a.type == 1) {			// 设置appender
						ap.reset(new FileLogAppender(a.file));
					} else if (a.type == 2) {
						ap.reset(new StdoutLogAppender);
					}
					ap->setLevel(a.level);		// 设置等级
					logger->addAppender(ap);
					if (!a.formatter.empty()) {
						LogFormatter::ptr fmt(new LogFormatter(a.formatter));
						if (!fmt->isError()) {
							ap->setFormatter(fmt);
						} else {
							std::cout << "log.name = " << i.name << " appender type = " << a.type
								<< " formatter = " << a.formatter << " is invalid" << std::endl;
						}
					}
				}
			}

			// 删除旧值
			for (auto& i : old_value) {
				auto it = new_value.find(i);
				// 如果不存在就删除该旧值， 存在则不处理
				if (it == new_value.end()) {
					// 删除logger
					auto logger = LOG_NAME(i.name);	
					logger->setLevel((LogLevel::Level)100);	// 给这个logger设置不可能达到的等级(删除即忽略这个logger)
					logger->clearAppender();				// 清空appender
				}
			}
		});
	}
};

static LogIniter __log_init;

void LoggerManager::init() {
}

}








