#pragma once

#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <string>
#include <stdexcept>

namespace l2q_http::transparent{
	struct string_equal_to{
		using is_transparent = void;

		constexpr bool operator()(const std::string_view a, const std::string_view b) const noexcept {
			return a == b;
		}

		constexpr bool operator()(const std::string_view a, const std::string& b) const noexcept {
			return a == static_cast<std::string_view>(b);
		}

		constexpr bool operator()(const std::string& b, const std::string_view a) const noexcept {
			return static_cast<std::string_view>(b) == a;
		}

		constexpr bool operator()(const std::string& b, const std::string& a) const noexcept {
			return b == a;
		}
	};

	template <template <typename > typename Comp>
		requires std::regular_invocable<Comp<std::string_view>, std::string_view, std::string_view>
	struct string_comparator_of{
		static constexpr Comp<std::string_view> comp{};
		using is_transparent = void;
		auto operator()(const std::string_view a, const std::string_view b) const noexcept {
			return comp(a, b);
		}

		auto operator()(const std::string_view a, const std::string& b) const noexcept {
			return comp(a, b);
		}

		auto operator()(const std::string& a, const std::string_view b) const noexcept {
			return comp(a, b);
		}

		auto operator()(const std::string& a, const std::string& b) const noexcept {
			return comp(a, b);
		}
	};

	struct string_hasher{
		using is_transparent = void;
		static constexpr std::hash<std::string_view> hasher{};

		std::size_t operator()(const std::string_view val) const noexcept {
			return hasher(val);
		}

		std::size_t operator()(const std::string& val) const noexcept {
			return hasher(val);
		}
	};
}

namespace l2q_http{

	template <typename Alloc = std::allocator<std::string>>
	struct string_hash_set : std::unordered_set<std::string, transparent::string_hasher, transparent::string_equal_to, Alloc>{
	private:
		using self_type = std::unordered_set<std::string, transparent::string_hasher, transparent::string_equal_to, Alloc>;

	public:
		using self_type::unordered_set;
		using self_type::insert;

		decltype(auto) insert(const std::string_view string){
			return this->insert(std::string(string));
		}

		decltype(auto) insert(const char* string){
			return this->insert(std::string(string));
		}
	};


	template <typename V, typename Alloc = std::allocator<std::pair<const std::string, V>>>
	class string_hash_map : public std::unordered_map<std::string, V, transparent::string_hasher, transparent::string_equal_to, Alloc>{
	private:
		using self_type = std::unordered_map<std::string, V, transparent::string_hasher, transparent::string_equal_to, Alloc>;

	public:
		using std::unordered_map<std::string, V, transparent::string_hasher, transparent::string_equal_to, Alloc>::unordered_map;

		auto& at(const std::string_view key) const  {
			if(auto itr = this->find(key); itr != this->end()){
				return itr->second;
			}else{
				throw std::out_of_range("key not found");
			}
		}

		auto& at(const std::string_view key)  {
			if(auto itr = this->find(key); itr != this->end()){
				return itr->second;
			}else{
				throw std::out_of_range("key not found");
			}
		}

		V at(const std::string_view key, const V& def) const requires std::is_copy_assignable_v<V>{
			if(const auto itr = this->find(key); itr != this->end()){
				return itr->second;
			}
			return def;
		}

		V at(const std::string_view key, V&& def) const requires std::is_copy_assignable_v<V>{
			if(const auto itr = this->find(key); itr != this->end()){
				return itr->second;
			}
			return std::move(def);
		}

		V* try_find(const std::string_view key){
			if(const auto itr = this->find(key); itr != this->end()){
				return &itr->second;
			}
			return nullptr;
		}

		const V* try_find(const std::string_view key) const {
			if(const auto itr = this->find(key); itr != this->end()){
				return &itr->second;
			}
			return nullptr;
		}

		using self_type::insert_or_assign;
		using self_type::try_emplace;

		template <class ...Arg>
		std::pair<typename self_type::iterator, bool> try_emplace(const std::string_view key, Arg&& ...val){
			if(auto itr = this->find(key); itr != this->end()){
				return {itr, false};
			}else{
				return this->try_emplace(std::string(key), std::forward<Arg>(val) ...);
			}
		}

		template <class ...Arg>
		std::pair<typename self_type::iterator, bool> try_emplace(const char* key, Arg&& ...val){
			return this->try_emplace(std::string_view{key}, std::forward<Arg>(val) ...);
		}

		template <class ...Arg>
		std::pair<typename self_type::iterator, bool> insert_or_assign(const std::string_view key, Arg&& ...val) {
			return this->insert_or_assign(static_cast<std::string>(key), std::forward<Arg>(val) ...);
		}

		template <std::size_t sz, class ...Arg>
		std::pair<typename self_type::iterator, bool> insert_or_assign(const char (&key)[sz], Arg&& ...val) {
			return this->insert_or_assign(std::string_view(key, sz), std::forward<Arg>(val) ...);
		}

		using self_type::operator[];

		V& operator[](const std::string_view key) {
			if(auto itr = this->find(key); itr != this->end()){
				return itr->second;
			}

			return this->emplace(std::string(key), typename self_type::mapped_type{}).first->second;
		}

		V& operator[](const char* key) {
			return operator[](std::string_view(key));
		}
	};
}