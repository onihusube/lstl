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

	};
}