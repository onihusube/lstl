#pragma once

#include "common.h"

#include "Include/optional.hpp"

namespace lstl::test::optional
{
	TEST_CLASS(optional_test)
	{
	public:

		TEST_METHOD(impl_sandobox)
		{
			constexpr lstl::optional<int> o{};
			lstl::optional<int> n{ 10 };

			int v = *n;

			constexpr bool valid = bool(o);
		}

		TEST_METHOD(value_access_test) {
			constexpr lstl::optional<int> nullopt{};
			constexpr lstl::optional<int> hasvalue{ 10 };

			Assert::IsFalse(bool(nullopt));
			Assert::IsTrue(bool(hasvalue));

			Assert::AreEqual(10, *hasvalue);
			Assert::AreEqual(10, hasvalue.value());

			try {
				int v = nullopt.value();
			}
			catch (const lstl::bad_optional_access& exception) {
				Assert::AreEqual("Bad optional access", exception.what());
			}

		}


	};
}