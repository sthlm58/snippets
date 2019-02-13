#pragma once

#include <FooGlobal.h>

#include <QLoggingCategory>
#include <QFile>
#include <QMutex>

namespace foo::qt
{
	/**
	 * @brief The LogWriterBase class allows to write the Qt-specific logging info
	 */
	class FOOSHARED_EXPORT LogWriterBase
	{
	public:
		virtual void write(QtMsgType type, const QMessageLogContext& context, const QString& message) = 0;
		virtual void flush() = 0;
		virtual ~LogWriterBase() = default;
	};

	/**
	 * @brief The FileWriter class uses a file to store the log information
	 */
	class FOOSHARED_EXPORT FileWriter : public LogWriterBase
	{
	public:
		FileWriter(const QString& file_prefix = {}, const QString& directory_prefix = "foo_");

		void write(QtMsgType type, const QMessageLogContext& context, const QString& message) override;
		void flush() override;

	private:
		QFile		mLogFile;
		QTextStream	mLogStream;
	};


	/**
	 * @brief The ConsoleWriter class sends the logs to std::wclog
	 */
	class FOOSHARED_EXPORT ConsoleWriter : public LogWriterBase
	{
	public:
		void write(QtMsgType type, const QMessageLogContext& context, const QString& message) override;
		void flush() override;

	private:
		QMutex mStreamLock;
	};


	/**
	 * @brief The Logger class acts like a scoped composer of log writers
	 *
	 * It provides a pointer (\a messageHandler()) to a Qt-compliant message log callback
	 * that forwards the arguments to the inner (statically stored) log writers.
	 *
	 * This class doesn't take ownership of the LogWriters and only uses them for it's lifetime.
	 * When leaving the scope, this instance's dtor clears the static container of log writer.
	 *
	 * The \a messageHandler() remains valid even if the instance is destroyed.
	 *
	 * @example
	 *
	 *		FileWriter file{};
	 *		ConsoleWriter console{};
	 *
	 *		Logger logger(&file, &console);
	 *		qInstallMessageHandler(logger.messageHandler());
	 *
	 */
	class FOOSHARED_EXPORT Logger
	{
	public:
		template <typename... LogWriters>
		Logger(LogWriters*... writers)
		{
			(sLogWriters.append(writers), ...);
		}

		~Logger()
		{
			sLogWriters.clear();
		}

		QtMessageHandler messageHandler() const;

	private:
		static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message);
		static QList<LogWriterBase*> sLogWriters;

	};

}
