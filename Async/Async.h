#pragma once

#include "Expected.h"
#include "TypeTraits.h"
#include "FooGlobal.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>

#include <boost/iterator/zip_iterator.hpp>

#include <utility>
#include <initializer_list>
#include <functional>

namespace foo::async
{
	/**
	 * @brief Base class for all tasks
	 */
	class FOOSHARED_EXPORT AbstractTask: public QObject
	{
		Q_OBJECT

	public:
		virtual void start() = 0;

	signals:
		void finished(bool success) const;

	public slots:
		void finishWith(bool result)
		{
			emit finished(result);
			deleteLater();
		}
	};

	namespace detail
	{
		using namespace foo;

		/**
		 * AsyncTask - class template for generic data object living in the heap, implementation supporting onDone() only
		 */
		template <typename ResultType, typename TaskType, typename Enabled = void>
		class AsyncTask : public AbstractTask
		{
		public:
			AsyncTask(TaskType&& task)
				: mWatcher()
				, mTask(std::move(task))
			{}

			/** This is needed, so that MSVC does not complain about ambiguous onDone() (even though we have enable_if) */
			template <int> struct dummy { dummy(int) {} };

			/** Overload for VOID and ommited parameters */
			template <typename CallbackType, typename = std::enable_if_t<!traits::IsCallable_v<CallbackType, ResultType>>>
			void onDone(CallbackType&& callback, dummy<0> = 0)
			{
				QObject::connect(&mWatcher, &QFutureWatcher<ResultType>::finished, this, std::forward<CallbackType>(callback));
			}

			/** Overload for callback with parameters */
			template <typename CallbackType, typename = std::enable_if_t<traits::IsCallable_v<CallbackType, ResultType>>>
			void onDone(CallbackType&& callback, dummy<1> = 0)
			{
				QObject::connect(&mWatcher, &QFutureWatcher<ResultType>::finished, this, [=]{ callback(mWatcher.result()); });
			}

			template <typename CallbackType>
			void onError(CallbackType&&)
			{
				// generic implementation does nothing
			}

			void start() override
			{
				QObject::connect(&mWatcher, &QFutureWatcher<ResultType>::finished, this, &AsyncTask::deleteLater);
				mWatcher.setFuture(mTask());
			}

		private:
			QFutureWatcher<ResultType>	mWatcher;
			TaskType					mTask;
		};


		/**
		 * AsyncTask - class template specialization for Expected<T, E>, supporting both onDone() and onError()
		 */
		template <typename ResultType, typename TaskType>
		class AsyncTask<ResultType, TaskType, std::enable_if_t<traits::IsSpecializationOf_v<Expected, ResultType>>> : public AbstractTask
		{
		public:
			using SuccessType = typename ResultType::ValueType;
			using FailureType = typename ResultType::ErrorType;

			AsyncTask(TaskType&& task)
				: mWatcher()
				, mTask(std::move(task))
				, mSuccessCallback([](SuccessType){})
				, mFailureCallback([](FailureType){})
			{}

			template <typename CallbackType>
			void onDone(CallbackType&& callback)
			{
				mSuccessCallback = callback;
			}

			template <typename CallbackType>
			void onError(CallbackType&& callback)
			{
				mFailureCallback = callback;
			}

			void start() override
			{
				QObject::connect(&mWatcher, &QFutureWatcher<ResultType>::finished, this, &AsyncTask::onFinished);
				QObject::connect(&mWatcher, &QFutureWatcher<ResultType>::finished, this, &AsyncTask::deleteLater);
				mWatcher.setFuture(mTask());
			}

			void onFinished()
			{
				auto result = mWatcher.result();
				result.isValid()
						? mSuccessCallback(result.value())
						: mFailureCallback(result.error());

				emit finished(result.isValid());
			}

		private:
			QFutureWatcher<ResultType>			mWatcher;
			TaskType							mTask;
			std::function<void(SuccessType)>	mSuccessCallback;
			std::function<void(FailureType)>	mFailureCallback;
		};


		/**
		 * @brief Monitors state of given tasks. Notifies when
		 * all are done, or any fails.
		 */
		class CompositeTask: public AbstractTask
		{
		public:
			CompositeTask() = default;

			template <class... Tasks>
			CompositeTask(Tasks... tasks)
				: mTasks({ std::move(tasks)... })
			{
				for (auto task : mTasks)
				{
					connect(task, &AbstractTask::finished, this, [this](bool success)
					{
						mAllSucceeded &= success;

						if (++mFinishedCount >= mTasks.count())
						{
							finishWith(mAllSucceeded);
						}
					});
				}
			}

			void start() override
			{
				for (auto task : mTasks)
				{
					task->start();
				}
			}

		protected:
			QVector<AbstractTask*>	mTasks;
			int						mFinishedCount	= 0;
			bool					mAllSucceeded	= true;
		};


		class FifoTask: public CompositeTask
		{
		public:
			template <class... Tasks>
			FifoTask(Tasks... tasks)
				: CompositeTask(tasks...)
			{
				if (mTasks.isEmpty())
				{
					return;
				}

				connect(mTasks.last(), &AbstractTask::finished, this, &AbstractTask::finishWith);

				// connect pairs of consecutive tasks so that when
				// a preceding task finishes, the next one in the queue is started
				for (auto task = mTasks.begin(), end = mTasks.end() - 1; task != end; ++task)
				{
					auto next_task = *std::next(task);
					connect(*task, &AbstractTask::finished, [this, next_task](bool success)
					{
						if (!success)
						{
							finishWith(false);
							return;
						}

						next_task->start();
					});
				}
			}

			void start() override
			{
				if (mTasks.isEmpty())
				{
					finishWith(true);
				}

				mTasks.first()->start();
			}
		};


		/**
		 * TaskBuilder - simplifies AsyncTask creation with the help of
		 * method chaining.
		 */
		template <typename ResultType, typename TaskType>
		class TaskBuilder
		{
		public:
			// todo: it would be nice if it was able to create broader
			// range of Tasks, not only AsyncTask
			using Task = detail::AsyncTask<ResultType, TaskType>;

			TaskBuilder(TaskType&& task)
				: mTask(new Task{ std::forward<TaskType>(task) } )
			{
			}

			template <typename CallbackType>
			TaskBuilder& onDone(CallbackType&& callback)
			{
				mTask->onDone(std::forward<CallbackType>(callback));
				return *this;
			}

			template <typename CallbackType>
			TaskBuilder& onError(CallbackType&& callback)
			{
				mTask->onError(std::forward<CallbackType>(callback));
				return *this;
			}

			AbstractTask* get()
			{
				return mTask;
			}

		private:
			Task* mTask;
		};

		template <typename ResultType, typename TaskType>
		auto make_async_task(TaskType&& task)
		{
			return TaskBuilder<ResultType, TaskType>{ std::forward<TaskType>(task) };
		}
	}


	/**
	 * @brief Allows to create a AsyncTask, which will be executed asynchronously (thread pool),
	 * and allow a callback to be executed on the same thread context, that the Task was created on
	 *
	 * Has special treatment for Expected<T, E>, i.e. unpacking it to onDone() and onError()
	 *
	 * @example: (nested calls)
	 *
		Async::task([=]{ return common::make_expected(123); })
			.onDone([](int value)
			{
				Async::task([=] { return value * 2; })
						.onDone([=]( int val ){ qDebug() << "inner:" << val;})
						.start();
			})
			.onError([](std::error_code error){ qDebug() << "error:" << error.value(); })
			.start();

	* @example: (omitted parameter)
	*
		Async::task([=]{ return 123; })
			.onDone([]{ qDebug() << "done!"; })
			.start();

	 *
	 */
	template <typename... Params>
	auto task(Params&&... params)
	{
		using ResultType = foo::traits::FirstTemplateParameter_t<decltype(QtConcurrent::run(std::forward<Params>(params)...))>;
		return detail::make_async_task<ResultType>( [=]{ return QtConcurrent::run(params...); } );
	}



	template <typename... Tasks>
	auto weave(Tasks&&... tasks) -> AbstractTask*
	{
		return new detail::CompositeTask(std::forward<Tasks>(tasks)...);
	}

	template <typename... Tasks>
	auto queue(Tasks&&... tasks) -> AbstractTask*
	{
		return new detail::FifoTask(std::forward<Tasks>(tasks)...);
	}
}
