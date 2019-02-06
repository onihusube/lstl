#pragma once

#include "common.h"

#include "Include/scope.hpp"

namespace lstl::test::scope
{
	TEST_CLASS(scope_test)
	{
	public:
		TEST_METHOD(common_scope_exit_test)
		{
			auto f = []() {};

			//代入/コピー構築不可
			Assert::IsFalse(std::is_copy_constructible<lstl::scope_exit<decltype(f)>>::value);
			Assert::IsFalse(std::is_copy_assignable<lstl::scope_exit<decltype(f)>>::value);
			Assert::IsFalse(std::is_move_assignable<lstl::scope_exit<decltype(f)>>::value);

			//ムーブ構築可
			Assert::IsTrue(std::is_move_constructible<lstl::scope_exit<decltype(f)>>::value);

			//デストラクタは例外を投げない
			Assert::IsTrue(std::is_nothrow_destructible<lstl::scope_exit<decltype(f)>>::value);
		}

		TEST_METHOD(scope_exit_test)
		{
			int n = 10;

			//nを20にする
			auto&& f = [&n]() {
				n += 10;
			};

			//このスコープの終わりで実行
			{
				lstl::scope_exit<std::remove_reference_t<decltype(f)>> test_scope_exit{ std::move(f) };

				//未実行
				Assert::AreEqual(10, n);
			}
			
			//実行済
			Assert::AreEqual(20, n);
		}

		TEST_METHOD(scope_succes_test)
		{
			int n = 10;

			//nを+10する
			auto&& f = [&n]() {
				n += 10;
			};

			//このスコープの終わりで実行
			{
				lstl::scope_success<std::remove_reference_t<decltype(f)>> test_scope_exit{ f };

				//未実行
				Assert::AreEqual(10, n);
			}

			//実行済
			Assert::AreEqual(20, n);

			//ここは実行されない
			try {
				lstl::scope_success<std::remove_reference_t<decltype(f)>> test_scope_exit{ f };

				throw std::exception{};
			}
			catch (...) {
			}

			//未実行
			Assert::AreEqual(20, n);

			//ここも実行されない
			try {
				try {
					lstl::scope_success<std::remove_reference_t<decltype(f)>> test_scope_exit{ std::move(f) };

					throw std::exception{};
				}
				catch (...) {
				}
			}
			catch (...) {
			}

			//未実行
			Assert::AreEqual(20, n);
		}

		TEST_METHOD(scope_fail_test)
		{
			int n = 10;

			//nを+10する
			auto&& f = [&n]() {
				n += 10;
			};

			//ここは実行されない
			{
				lstl::scope_fail<std::remove_reference_t<decltype(f)>> test_scope_exit{ f };
			}

			//未実行
			Assert::AreEqual(10, n);

			//このスコープの終わりで実行
			try {
				lstl::scope_fail<std::remove_reference_t<decltype(f)>> test_scope_exit{ f };

				throw std::exception{};
			}
			catch (...) {
			}

			//実行済
			Assert::AreEqual(20, n);

			//ここも実行される
			try {
				try {
					lstl::scope_fail<std::remove_reference_t<decltype(f)>> test_scope_exit{ std::move(f) };

					throw std::exception{};
				}
				catch (...) {
				}
			}
			catch (...) {
			}

			//実行済
			Assert::AreEqual(30, n);
		}

		TEST_METHOD(release_test)
		{
			//呼び出されると失敗
			auto&& f = []() {
				Assert::Fail();
			};

			{
				lstl::scope_exit<std::remove_reference_t<decltype(f)>> test_scope_exit{ f };

				//実行責任を手放す
				test_scope_exit.release();
			}

			{
				lstl::scope_success<std::remove_reference_t<decltype(f)>> test_scope_exit{ f };

				//実行責任を手放す
				test_scope_exit.release();
			}

			try {
				lstl::scope_fail<std::remove_reference_t<decltype(f)>> test_scope_exit{ f };

				//実行責任を手放す
				test_scope_exit.release();

				throw std::exception{};
			}
			catch (...) {
			}
		}
	};
}