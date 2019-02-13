#pragma once

/**
 * @brief StoredValue is a proxy object that provides access to a storable type T
 *
 * @note This class provides custom conversion and assignment operators
 */
template <typename T>
class StoredValue
{
public:
	/**
	 * @brief getter
	 */
	virtual T get() const = 0;

	/**
	 * @brief setter
	 */
	virtual void set(const T& value) = 0;

	/**
	 * @brief helper conversion operator
	 */
	operator T() const
	{
		return get();
	}

	/**
	 * @brief helper assignment operator
	 */
	StoredValue& operator=(const T& value)
	{
		set(value);
		return *this;
	}
};
