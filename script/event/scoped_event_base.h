#pragma once

#include <memory>
#include <set>
#include <vector>

namespace metternich {

class character;
class gsml_data;
class gsml_property;
struct context;
struct read_only_context;

template <typename T>
class condition;

template <typename T>
class effect;

template <typename T>
class effect_list;

template <typename T>
class event_option;

template <typename T>
class event_trigger;

template <typename T>
class scoped_event_base
{
public:
	scoped_event_base();
	virtual ~scoped_event_base();

	void initialize();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	virtual const std::string &get_tag() const = 0;
	std::string get_title() const;
	std::string get_description() const;

	bool check_conditions(const T *scope, const read_only_context &ctx) const;
	void do_event(T *scope, const context &ctx) const;
	void pick_option(T *scope, const context &ctx) const;

private:
	std::set<event_trigger<T> *> triggers;
	bool random = false;
	bool hidden = false;
	std::unique_ptr<condition<T>> conditions;
	std::unique_ptr<effect_list<T>> immediate_effects;
	std::vector<std::unique_ptr<event_option<T>>> options;
};

extern template class scoped_event_base<character>;

}
