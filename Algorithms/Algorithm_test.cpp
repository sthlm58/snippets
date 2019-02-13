#include <Foo/External/catch.hpp>

#include <Foo/Core/Common.h>
#include <Foo/Core/QtUtils.h>
#include <Foo/Models/TypedObjectListModel.h>


TEST_CASE("filtered")
{
	SECTION("basic usage returns the same collection type")
	{
		QList<int> numbers{1, 2, 3, 4, 5};
		QList<int> even_numbers = foo::filtered(numbers, [](int i){ return i % 2 == 0; });

		CHECK(even_numbers.size() == 2);

		std::vector<std::string> strings{"one", "two", "three"};
		std::vector<std::string> short_strings = foo::filtered(strings, [](auto s){ return s.size() < 4; });

		CHECK(short_strings.size() == 2);
	}

	SECTION("collection type can be overriden via filteredAs")
	{
		QList<int> numbers{1, 2, 3, 4, 5};
		std::vector<int> even_numbers = foo::filteredAs<std::vector<int>>(numbers, [](int i){ return i % 2 == 0; });

		CHECK(even_numbers.size() == 2);
	}

	SECTION("should also work with TypedObjectListModel when specifyling the resulting type")
	{
		const auto new_object = [](auto name) { auto object = new QObject; object->setObjectName(name); return object; };
		foo::models::Model<QObject> objects { { new_object("one"), new_object("two"), new_object("three") } };

		auto short_named = foo::filteredAs<QObjectList>(objects, [](auto object){ return object->objectName().size() < 4; });

		CHECK(short_named.size() == 2);
	}
}

TEST_CASE("transformed")
{
	SECTION("basic usage")
	{
		std::vector<int> numbers{1, 2, 3};

		auto doubled_doubles = foo::transformed(numbers, [](int i) { return i * 2.0; });
		CHECK(doubled_doubles == std::vector<double>{2., 4., 6.});
	}

	SECTION("works with QStringList")
	{
		QStringList texts{"one", "two"};

		auto uppercased = foo::transformed(texts, [](auto str) { return str.toUpper(); });
		CHECK(uppercased == QStringList{"ONE", "TWO"});
	}

	SECTION("can specify output type")
	{
		std::vector<QString> texts{"one", "two"};

		auto sizes = foo::transformedAs<QList<int>>(texts, [](auto str) { return str.size(); });
		CHECK(sizes == QList<int>{3, 3});
	}

	SECTION("can specify output type when working with TypedObjectListModel")
	{
		const auto new_object = [](auto name) { auto object = new QObject; object->setObjectName(name); return object; };
		foo::models::Model<QObject> objects { QObjectList{ } << new_object("one") };

		auto names = foo::transformedAs<QStringList>(objects, [](auto obj) { return obj->objectName(); });
		CHECK(names == QStringList{ "one" });
	}
}
