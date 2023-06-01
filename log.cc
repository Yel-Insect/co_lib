#include "log.h"
#include <map>
#include <iostream>
#include <functional>
#include <string>
#include <time.h>
#include "config.h"





namespace sylar{

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
			, uint32_t thread_id, uint32_t fiber_id, uint32_t time) 
			: m_file(file), m_line(line), m_elapse(elapse)
			, m_threadId(thread_id), m_fiberId(fiber_id), m_time(time)
			, m_logger(logger), m_level(level) {
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

Logger::Logger (const std::string& name) 
	: m_name(name), m_level(LogLevel::Level::DEBUG) {
	m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
	if (level >= m_level) {
		auto self = shared_from_this();
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
	for (auto it = m_appenders.begin(); it != m_appenders.end(); it++) {
		if (*it == appender) {
			m_appenders.erase(it);
			break;
		}
	}
}

void Logger::clearaAppender() {
	m_appenders.clear();
}

void Logger::addAppender(LogAppender::ptr appender) {
	if (!appender->getFormatter()) {
		appender->setFormatter(m_formatter);
	}
	m_appenders.push_back(appender);
}

void Logger::setFormatter(LogFormatter::ptr val) {
	m_formatter = val;
}

void Logger::setFormatter(std::string& val) {
	//m_formatter.reset(new sylar::LogFormatter(val));
	sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
	// 判断new_val是否解析错误
	if (new_val->isError()) {
		std::cout << "Logger setFormatter name = " << m_name
				<< " value = " << val << " invalid formatter"
				<< std::endl;
	}
	m_formatter = new_val;
}

LogFormatter::ptr Logger::getFormatter() {
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
	if (m_filestream) {
		m_filestream.close();
	}
	m_filestream.open(m_filename);
	return !m_filestream;
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	if (level >= m_level) {
		//reopen();
		m_filestream << m_formatter->format(logger, level, event);
	}
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	if (level >= m_level) {
		std::cout << m_formatter->format(logger, level, event);
	}
}

LogFormatter::LogFormatter(const std::string& pattern) 
	: m_pattern(pattern) {
		init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	std::stringstream ss;
	for (auto& i : m_items) {
		i->format(ss, logger, level, event);
	}
	return ss.str();
}

//%xxx %xxx{xxx} %%
void LogFormatter::init() {
	std::vector<std::tuple<std::string, std::string, int>> vec;
	std::string nstr;	// 主要存与格式无关的字符串(或者说是%前的内容吧)
	for (size_t i = 0; i < m_pattern.size(); i++) {	
		if (m_pattern[i] != '%') {
			nstr.append(1, m_pattern[i]);
			continue;
		}
		
		// 提取%% 使它变为%
		if (i + 1 < m_pattern.size()) {
			if (m_pattern[i + 1] == '%') {
				nstr.append(1, '%');
				continue;
			}
		}
		
		size_t n = i + 1;
		int fmt_status = 0;
		size_t fmt_begin = 0;
		
		std::string str;	//%后面的内容
		std::string fmt;  //提取大括号里内容
		
		while (n < m_pattern.size()) { 
			if (!fmt_status && !isalpha(m_pattern[n]) && m_pattern[n] != '{'
				&& m_pattern[n] != '}') {
				str = m_pattern.substr(i + 1, n - i - 1);
				break ;
			}
			if (fmt_status == 0) {
				if (m_pattern[n] == '{') {
					str = m_pattern.substr(i + 1, n - i - 1);
					fmt_status = 1; //解析格式
					fmt_begin = n;
					++n;
					continue;
					
				}
			} else if (fmt_status == 1) {
				if (m_pattern[n] == '}') {
					fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);	//提取大括号里的内容
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
	XX(m, MessageFormatItem),
	XX(p, LevelFormatItem),
	XX(r, ElapseFormatItem),
	XX(c, NameFormatItem),
	XX(t, ThreadIdFormatItem),
	XX(n, NewLineFormatItem),
	XX(d, DateTimeFormatItem),
	XX(f, FilenameFormatItem),
	XX(l, LineFormatItem),
	XX(T, TabFormatItem),
	XX(F, FiberIdFormatItem)
	
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

	init();
}
Logger::ptr LoggerManager::getLogger(const std::string& name) {
	auto it = m_loggers.find(name);
	if (it != m_loggers.end()) {
		return it->second;
	}
	// 找不到你要的就给你可爱的root吧
	Logger::ptr logger(new Logger(name));
	logger->m_root = m_root;
	m_loggers[name] = logger;
	return logger;
}

// appender的配置信息？
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

sylar::ConfigVar<std::set<LogDefine>>::ptr g_log_define = 
	sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");


//在main函数前初始化
struct LogIniter {
	LogIniter() {
		g_log_define->addListener(0xF1E231, [](const std::set<LogDefine>& old_value,
								const std::set<LogDefine>& new_value) {
			SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changer";
			for (auto& i : new_value) {
				// 先查询是否存在
				auto it = old_value.find(i);
				sylar::Logger::ptr logger;
				if (it == old_value.end()) {
					// 不存在则新增logger
					logger.reset(new sylar::Logger(i.name));
				} else {	// 存在
					if (!(i == *it)) {	// 不相同才修改， 相同就不动(为什么查找到了会不相同？？)
						// 修改logger
						logger = SYLAR_LOG_NAME(i.name);
					}
				}
				// 将logger的appenders清空(主要针对已存在的吧)
				logger->clearaAppender();
				for (auto& a : i.appenders) {	// 取出set中的LogDefine
					sylar::LogAppender::ptr ap;	// 根据结构体的属性为logger设新值
					if (a.type == 1) {			// 设置appender
						ap.reset(new FileLogAppender(a.file));
					} else if (a.type == 2) {
						ap.reset(new StdoutLogAppender);
					}
					ap->setLevel(a.level);		// 设置等级
					logger->addAppender(ap);
				}
			}

			// 删除旧值
			for (auto& i : old_value) {
				// 查找该旧值是否出现在新值中
				auto it = new_value.find(i);
				// 如果不存在就删除该旧值， 存在则不处理(因为上面已经修改过了)
				if (it == new_value.end()) {
					// 删除logger
					auto logger = SYLAR_LOG_NAME(i.name);	
					logger->setLevel((LogLevel::Level)100);	// 给这个logger设置不可能达到的等级(删除即忽略这个logger)
					logger->clearaAppender();				// 清空appender
				}
			}
		});
	}
};

void LoggerManager::init() {
}

}








