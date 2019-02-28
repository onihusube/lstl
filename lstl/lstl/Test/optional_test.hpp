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

		TEST_METHOD(optional_construct_test) {
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

				//trivially_copy_constructibleでない型
				lstl::optional<std::string> str{ "string." };
				lstl::optional<std::string> str_copy{ str };

				Assert::IsTrue(str.has_value());
				Assert::IsTrue(str_copy.has_value());

				Assert::IsTrue("string." == *str);
				Assert::IsTrue("string." == *str_copy);

			}

			//4. ムーブコンストラクタ
			{

				lstl::optional<int> n{ 10 };
				lstl::optional<int> m{ std::move(n) };

				Assert::IsTrue(m.has_value());
				Assert::AreEqual(10, *m);

				//trivially_move_constructibleでない型
				lstl::optional<std::string> str{ "string." };
				lstl::optional<std::string> str_copy{ std::move(str) };

				Assert::IsTrue(str_copy.has_value());

				Assert::IsTrue("string." == *str_copy);
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

		TEST_METHOD(optional_assign_test) {
			//1. 無効値の代入
			{
				lstl::optional<int> n{ 10 };

				Assert::IsTrue(bool(n));

				n = lstl::nullopt;

				Assert::IsFalse(bool(n));
			}

			//2. コピー代入
			{
				lstl::optional<int> n{ 10 };
				lstl::optional<int> m{ 0 };

				Assert::IsTrue(bool(n));
				Assert::IsTrue(bool(m));

				Assert::AreEqual(10, *n);
				Assert::AreEqual(0, *m);

				n = m;

				Assert::IsTrue(bool(n));
				Assert::IsTrue(bool(m));

				Assert::AreEqual(0, *n);
				Assert::AreEqual(0, *m);
			}

			//3. ムーブ代入
			{
				lstl::optional<int> n{ 10 };
				lstl::optional<int> m{ 0 };

				Assert::IsTrue(bool(n));
				Assert::IsTrue(bool(m));

				Assert::AreEqual(10, *n);
				Assert::AreEqual(0, *m);

				n = std::move(m);

				Assert::IsTrue(bool(n));

				Assert::AreEqual(0, *n);
			}

			//4. 有効値を代入
			{
				lstl::optional<int> n{};

				Assert::IsFalse(bool(n));

				n = 10;

				Assert::IsTrue(bool(n));

				Assert::AreEqual(10, *n);


				lstl::optional<std::string> str{"string."};
				Assert::IsTrue(bool(str));
				Assert::IsTrue("string." == *str);

				str = "string assigne.";

				Assert::IsTrue(bool(str));
				Assert::IsTrue("string assigne." == *str);
			}

			//5. 変換可能なoptionalを代入
			{
				lstl::optional<long long> n{1};
				lstl::optional<int> m{10};

				Assert::IsTrue(bool(n));
				Assert::IsTrue(bool(m));

				n = m;

				Assert::IsTrue(bool(n));
				Assert::IsTrue(bool(m));

				long long expected = 10;
				Assert::IsTrue(expected == *n);
			}

			//5. 変換可能なoptionalをムーブ代入
			{
				lstl::optional<long long> n{ 1 };
				lstl::optional<int> m{ 10 };

				Assert::IsTrue(bool(n));
				Assert::IsTrue(bool(m));

				n = std::move(m);

				Assert::IsTrue(bool(n));

				long long expected = 10;
				Assert::IsTrue(expected == *n);
			}
		}

		TEST_METHOD(optional_has_value_test) {
			constexpr lstl::optional<int> nullopt{};
			constexpr lstl::optional<int> hasvalue{ 10 };

			Assert::IsFalse(bool(nullopt));
			Assert::IsTrue(bool(hasvalue));

			Assert::IsFalse(nullopt.has_value());
			Assert::IsTrue(hasvalue.has_value());
		}

		TEST_METHOD(optional_emplace_test) {
			//通常のemplace
			{
				lstl::optional<std::string> p{};

				p.emplace(3, 'A');

				//同等のstring
				std::string str(3, 'A');

				Assert::IsTrue(p.has_value());
				Assert::IsTrue(str == *p);
			}

			//初期化リストを受け取るemplace
			{
				lstl::optional<std::vector<int>> p;

				std::allocator<int> alloc{};

				p.emplace({ 3, 1, 4 }, std::move(alloc));

				Assert::IsTrue(p.has_value());
				Assert::AreEqual(3, (*p)[0]);
				Assert::AreEqual(1, (*p)[1]);
				Assert::AreEqual(4, (*p)[2]);
			}
		}

		TEST_METHOD(optional_swap_test) {
			//1. 両方とも有効値を持つ場合
			{
				lstl::optional<int> a = 3;
				lstl::optional<int> b = 1;

				//aとbを入れ替える
				lstl::swap(a, b);

				Assert::AreEqual(1, *a);
				Assert::AreEqual(3, *b);

				//非trivially destructibleな型
				lstl::optional<std::string> c{ "string 1." };
				lstl::optional<std::string> d{ "string 2." };

				//aとbを入れ替える
				lstl::swap(c, d);

				Assert::IsTrue("string 2." == *c);
				Assert::IsTrue("string 1." == *d);
			}

			//2. 左側が有効値を持たない場合
			{
				lstl::optional<int> a{};
				lstl::optional<int> b = 1;

				//aとbを入れ替える
				lstl::swap(a, b);

				Assert::AreEqual(1, *a);
				Assert::IsFalse(b.has_value());

				//非trivially destructibleな型
				lstl::optional<std::string> c{};
				lstl::optional<std::string> d{ "string" };

				//aとbを入れ替える
				lstl::swap(c, d);

				Assert::IsTrue("string" == *c);
				Assert::IsFalse(d.has_value());
			}

			//3. 右側が有効値を持たない場合
			{
				lstl::optional<int> a = 1;
				lstl::optional<int> b{};

				//aとbを入れ替える
				lstl::swap(a, b);

				Assert::IsFalse(a.has_value());
				Assert::AreEqual(1, *b);

				//非trivially destructibleな型
				lstl::optional<std::string> c{ "string" };
				lstl::optional<std::string> d{};

				//aとbを入れ替える
				lstl::swap(c, d);

				Assert::IsFalse(c.has_value());
				Assert::IsTrue("string" == *d);
			}

			//3. 両方有効値を持たない場合
			{
				lstl::optional<int> a{};
				lstl::optional<int> b{};

				//aとbを入れ替える
				lstl::swap(a, b);

				Assert::IsFalse(a.has_value());
				Assert::IsFalse(b.has_value());

				//非trivially destructibleな型
				lstl::optional<std::string> c{};
				lstl::optional<std::string> d{};

				//aとbを入れ替える
				lstl::swap(c, d);

				Assert::IsFalse(c.has_value());
				Assert::IsFalse(d.has_value());
			}
		}

		TEST_METHOD(optional_reset_test) {
			lstl::optional<int> hasvalue{ 10 };

			Assert::IsTrue(bool(hasvalue));
			Assert::IsTrue(hasvalue.has_value());

			hasvalue.reset();

			Assert::IsFalse(bool(hasvalue));
			Assert::IsFalse(hasvalue.has_value());

		}

		TEST_METHOD(optional_value_access_test) {
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