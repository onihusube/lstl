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

			lstl::optional<int> c{ n };
			lstl::optional<int> m{ std::move(c) };
		}

		TEST_METHOD(construct_test) {
			//1. デフォルトコンストラクタ
			{
				lstl::optional<int> n{};

				Assert::IsFalse(n.has_value());
			}

			//2. nulloptを受けるコンストラクタ
			{
				lstl::optional<int> n = lstl::nullopt;

				Assert::IsFalse(n.has_value());
			}

			//3. コピーコンストラクタ
			{
				lstl::optional<int> n{ 10 };
				lstl::optional<int> m{ n };

				Assert::IsTrue(n.has_value());
				Assert::IsTrue(m.has_value());
				Assert::AreEqual(10, *n);
				Assert::AreEqual(10, *m);

				//lstl::optional<std::string> str{ "string." };
				//lstl::optional<std::string> str_copy{ str };
			}

			//4. ムーブコンストラクタ
			{

				lstl::optional<int> n{ 10 };
				lstl::optional<int> m{ std::move(n) };

				Assert::IsTrue(m.has_value());
				Assert::AreEqual(10, *m);
				
				//lstl::optional<std::string> str{ "string." };
				//lstl::optional<std::string> str_copy{ std::move(str) };
			}

			//5.直接構築
			{
				lstl::optional<std::string> n{ lstl::in_place, 3, 'A'};
				std::string str(3, 'A');

				Assert::IsTrue(str == *n);
			}

			//6.initilizer_listと追加の引数を受けての直接構築
			{
				std::allocator<int> alloc{};
				lstl::optional<std::vector<int>> p{ lstl::in_place, { 3, 1, 4 }, alloc };
				
				Assert::AreEqual(3, (*p)[0]);
				Assert::AreEqual(1, (*p)[1]);
				Assert::AreEqual(4, (*p)[2]);
			}

			//7. Tに変換して初期化
			{
				// const char*からstd::stringに暗黙的に型変換
				lstl::optional<std::string> p1 = "Hello";
				Assert::IsTrue(p1.value() == "Hello");

				// 整数値からstd::vectorに明示的に型変換
				lstl::optional<std::vector<int>> p2{ 3 };
				Assert::IsTrue(p2.value().size() == 3);
			}

			//8. 変換可能なoptionalからのコピー構築
			{
				lstl::optional<const char*> a = "Hello";
				lstl::optional<std::string> b = a;

				Assert::IsTrue(b.value() == "Hello");
			}

			//9. 変換可能なoptionalからのムーブ構築
			{
				struct Base {};
				struct Derived : Base {};

				lstl::optional<std::shared_ptr<Derived>> a = std::make_shared<Derived>();
				lstl::optional<std::shared_ptr<Base>> b = std::move(a);

				Assert::IsFalse(bool(*a));
				Assert::IsTrue (bool(*b));
			}
		}

		TEST_METHOD(has_value_test) {
			constexpr lstl::optional<int> nullopt{};
			constexpr lstl::optional<int> hasvalue{ 10 };

			Assert::IsFalse(bool(nullopt));
			Assert::IsTrue(bool(hasvalue));

			Assert::IsFalse(nullopt.has_value());
			Assert::IsTrue(hasvalue.has_value());
		}

		TEST_METHOD(reset_test) {
			lstl::optional<int> hasvalue{ 10 };

			Assert::IsTrue(bool(hasvalue));
			Assert::IsTrue(hasvalue.has_value());

			hasvalue.reset();

			Assert::IsFalse(bool(hasvalue));
			Assert::IsFalse(hasvalue.has_value());

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