#include <Foo/External/catch.hpp>

#include <Foo/Core/OptionalAlgorithm.h>
#include <Foo/Models/TypedObjectListModel.h>

#include <boost/range.hpp>

namespace
{
	struct DerivedFromQObject : QObject
	{
		Q_OBJECT
	};
}

TEST_CASE("find")
{
	const QObjectList items;
	foo::optional<QObject*> maybe_object = foo::find(items, nullptr);

	CHECK(!maybe_object.has_value());
}

TEST_CASE("heterogenous find")
{
	SECTION("basic usage")
	{
		const QObjectList items;
		foo::optional<QObject*> maybe_object = foo::find(items, &QObject::objectName, QString{});

		CHECK(!maybe_object.has_value());
	}

	SECTION("with typed object list model")
	{

		const foo::models::TypedObjectListModel<DerivedFromQObject> object_model;
		foo::optional<DerivedFromQObject*> maybe_object = foo::find(object_model, &QObject::objectName, QString{});

		CHECK(!maybe_object.has_value());
	}
}

SCENARIO("foo::find with boost::range")
{
	GIVEN("A list of objects")
	{
		auto makeObject = [](const QString& name)
		{
			auto obj = new QObject;
			obj->setObjectName(name);
			return obj;
		};
		const QObjectList items {
			makeObject("1"),
			makeObject("2"),
			makeObject("3")
		};

		WHEN("searching in the whole container")
		{
			auto obj1 = foo::find(items, &QObject::objectName, "1");

			THEN("the object is found")
			{
				REQUIRE(obj1.has_value());
				CHECK(obj1.value() == items.at(0));
				CHECK(obj1.value()->objectName() == "1");
			}
		}
		WHEN("searching in a subrange (passed as lvalue)")
		{
			auto begin = items.begin();
			std::advance(begin, 1);
			auto range = boost::make_iterator_range(begin, items.end());

			auto obj1 = foo::find(range, &QObject::objectName, "1");
			auto obj2 = foo::find(range, &QObject::objectName, "2");

			THEN("the first object is not found, and the second is")
			{
				REQUIRE(!obj1.has_value());
				REQUIRE(obj2.has_value());
				CHECK(obj2.value() == items.at(1));
				CHECK(obj2.value()->objectName() == "2");
			}
		}
		WHEN("searching in a subrange (passed as rvalue)")
		{
			auto begin = items.begin();
			std::advance(begin, 1);

			auto obj1 = foo::find(boost::make_iterator_range(begin, items.end()), &QObject::objectName, "1");
			auto obj2 = foo::find(boost::make_iterator_range(begin, items.end()), &QObject::objectName, "2");

			THEN("the first object is not found, and the second is")
			{
				REQUIRE(!obj1.has_value());
				REQUIRE(obj2.has_value());
				CHECK(obj2.value() == items.at(1));
				CHECK(obj2.value()->objectName() == "2");
			}
		}

		qDeleteAll(items);
	}
}

SCENARIO("foo::chained() combines selectors")
{
	GIVEN("an object with nested properties")
	{
		struct Innermost { int value = 42; bool operator==(Innermost const& other) { return value == other.value; } };
		struct Middleware { Innermost inner() { return Innermost{}; } };
		struct Outermost { Middleware middle; };

		Outermost object;
		std::vector<Outermost> objects(1);

		WHEN("the object property & member selectors are chained")
		{
			auto value_selector = foo::chained(&Outermost::middle, &Middleware::inner, &Innermost::value);

			THEN("the chaining result can be invoked")
			{
				CHECK(value_selector(object) == 42);
				CHECK(std::invoke(value_selector, object) == 42);
			}
			THEN("the chaining result can be used in foo::find")
			{
				CHECK(foo::find(objects, value_selector, 42).has_value());
				CHECK(foo::find(objects, foo::chained(&Outermost::middle, &Middleware::inner), Innermost{}).has_value());
			}
		}
	}
}

#include "OptionalAlgorithm_test.moc"
