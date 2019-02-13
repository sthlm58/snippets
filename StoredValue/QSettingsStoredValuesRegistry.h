#pragma once

#include "QSettingsStoredValue.h"

#include <QSettings>


/**
 * @brief The QSettingsStoredValuesRegistry class provides instances of StoredValue for given setting \a key
 *
 * Instances of this class must outlive the StoredValue's returned by it, e.g. by using static storage type, e.g.
 *
 * @code
 *
 *	QSettingsStoredValuesRegistry& qsettingsValuesRegistry()
 *	{
 *		static QSettingsStoredValuesRegistry registry_instance("file.ini");
 *		return registry_instance;
 *	}
 *
 * This class bases stored values on local ini file storage.
 *
 * @see StoredValue
 *
 */
class QSettingsStoredValuesRegistry
{
public:
	explicit QSettingsStoredValuesRegistry(QString file)
		: mSettings(std::move(file), QSettings::IniFormat)
	{
	}

	/**
	 * @brief create() is a factory method providing QSettingsStoredValue<T, U> proxy object
	 *
	 * @param args... @see QSettingsStoredValue::ctor
	 *
	 * @note Objects returned by this method must outlive `this` object
	 * @note If you decide for type `U` to be different than `T`, you must provide serializer/deserializer functors
	 */
	template <typename T, typename U = T, typename... Args>
	auto create(Args... args)
	{
		return new QSettingsStoredValue<T, U>(mSettings, std::forward<Args>(args)...);
	}

private:
	QSettings mSettings;
};
