#include "Logger.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

#include <iostream>

namespace
{
	const char* toString(QtMsgType type)
	{
		switch (type)
		{
			case QtDebugMsg:
				return "[D]";
			case QtInfoMsg:
				return "[I]";
			case QtWarningMsg:
				return "[W]";
			case QtCriticalMsg:
				return "[C]";
			case QtFatalMsg:
				return "[F]";
		}

		return "";
	}

	std::wostream& operator<<(std::wostream& stream, const QString& text)
	{
	#ifdef Q_OS_WIN
		stream << reinterpret_cast<const wchar_t*>(text.utf16());
	#else
		stream << text.toStdWString();
	#endif
		return stream;
	}

	const QStringView TIME_FORMAT = u"hh:mm:ss.zzz";
	const QStringView DATE_TIME_FORMAT = u"yyyy-MM-dd hh:mm:ss.zzz";

	template <typename OutputStream>
	void outputMessageLine(OutputStream& out, QtMsgType type, const QMessageLogContext& context, const QString& message, bool shortened)
	{
		// "%1 %2 %3 (%4:%5)\n"
		out << QDateTime::currentDateTime().toString(shortened ? TIME_FORMAT : DATE_TIME_FORMAT) << ' ';
		out << toString(type) << ' ';
		out << message << ' ';
		out << '(' << QFileInfo(context.file).fileName() << ":" << context.line << ')' << '\n';
	}
}

using namespace foo::qt;

FileWriter::FileWriter(const QString& file_prefix, const QString& directory_prefix)
{
	const auto log_filename = QString("%1log_%2.txt")
								.arg(file_prefix).arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"));
	const auto logs_path = QString("%1/%2logs")
								.arg(QDir::tempPath()).arg(directory_prefix);
	QDir().mkdir(logs_path);

	mLogFile.setFileName(logs_path + "/" + log_filename);
	mLogFile.open(QFile::WriteOnly);
	mLogStream.setDevice(&mLogFile);
}

void FileWriter::write(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
	outputMessageLine(mLogStream, type, context, message, false);
}

void FileWriter::flush()
{
	if (mLogFile.isOpen())
	{
		mLogStream.flush();
	}
}

void ConsoleWriter::write(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
	QMutexLocker locker(&mStreamLock);
	outputMessageLine(std::wclog, type, context, message, true);
}

void ConsoleWriter::flush()
{
	std::wclog << std::flush;
}

QtMessageHandler Logger::messageHandler() const
{
	return &Logger::messageHandler;
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
	for (auto logger : sLogWriters)
	{
		logger->write(type, context, message);
	}
}

QList<LogWriterBase*> Logger::sLogWriters;
