#pragma once

#include "StoredValue.h"

#include <QSettings>
#include <QString>

#include <functional>
#include <type_traits>

namespace
{
	const auto identity = [](auto x) { return x; };
}

/**
 * @brief QSettingsStoredValue is a proxy object that provides access to a storable type T,
 * with a given \a key, under provided \a settings_ref
 *
 * Type T: general setting type, with which one interacts in code
 * Type U: actually serialized type; assume T is-convertible-to U (e.g. via \a set_modifier callback)
 *
 * Optionally, this type can store a \a default_value (when no such \a key exists)
 *
 * @note This class provides custom conversion and assignment operators
 */
template <typename T, typename U = T>
class QSettingsStoredValue : public StoredValue<T>
{
public:
	/**
	 * Constructor
	 *
	 * @param settings_ref - reference to QSettings store
	 * @param key - QSettings' key
	 * @param default_value - value returned if none found in QSettings
	 * @param serializer - functor operating on type `T` to convert it to serializable type `U` while possibly doing additional processing on given `StoredValue::value`
	 * @param deserializer - functor operating on type `U` to convert it back to type `T` while possibly doing additional processing on given `StoredValue::value`
	 */
	QSettingsStoredValue(QSettings& settings_ref,
						 QString key,
						 QVariant default_value = {},
						 std::function<U(T)> serializer = identity,
						 std::function<T(U)> deserializer = identity)
		: mSettingsRef(settings_ref)
		, mKey(std::move(key))
		, mDefaultValue(std::move(default_value))
		, mSerializer(std::move(serializer))
		, mDeserializer(std::move(deserializer))
	{
	}

	T get() const override
	{
		return mDeserializer(mSettingsRef.value(mKey, mDefaultValue).template value<U>());
	}

	void set(const T& value) override
	{
		mSettingsRef.setValue(mKey, mSerializer(value));
	}


private:
	QString mKey;
	QVariant mDefaultValue;
	QSettings& mSettingsRef;
	std::function<U(T)> mSerializer;
	std::function<T(U)> mDeserializer;
};
