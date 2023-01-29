#pragma once

namespace archimedes {
	class gsml_data;
}

namespace metternich {

class country;
class country_event;
enum class event_trigger;
struct context;
struct read_only_context;

template <typename scope_type>
class condition;

template <typename scope_type>
class event_option;

template <typename scope_type>
class factor;

template <typename scope_type>
class scoped_event_base
{
public:
	using event_type = std::conditional_t<std::is_same_v<scope_type, country>, country_event, void>;

	static void clear()
	{
		scoped_event_base::trigger_events.clear();
		scoped_event_base::trigger_random_events.clear();
	}

	static const std::vector<const event_type *> &get_trigger_events(const event_trigger trigger)
	{
		static std::vector<const event_type *> empty_list;

		const auto find_iterator = scoped_event_base::trigger_events.find(trigger);
		if (find_iterator != scoped_event_base::trigger_events.end()) {
			return find_iterator->second;
		}

		return empty_list;
	}

	static const std::vector<const event_type *> &get_trigger_random_events(const event_trigger trigger)
	{
		static std::vector<const event_type *> empty_list;

		const auto find_iterator = scoped_event_base::trigger_random_events.find(trigger);
		if (find_iterator != scoped_event_base::trigger_random_events.end()) {
			return find_iterator->second;
		}

		return empty_list;
	}

	static void add_trigger_none_random_weight(const event_trigger trigger, const int weight)
	{
		std::vector<const event_type *> &events = scoped_event_base::trigger_random_events[trigger];

		for (int i = 0; i < weight; ++i) {
			events.push_back(nullptr);
		}
	}

	static bool is_player_scope(const scope_type *scope);

private:
	static inline std::map<event_trigger, std::vector<const event_type *>> trigger_events;
	static inline std::map<event_trigger, std::vector<const event_type *>> trigger_random_events;

public:
	scoped_event_base();
	virtual ~scoped_event_base();

	bool process_gsml_scope(const gsml_data &scope);
	void initialize();
	void check() const;

	virtual event_trigger get_trigger() const = 0;

	bool is_random() const
	{
		return this->get_random_weight_factor() != nullptr;
	}

	void set_random_weight(const int weight);

	const factor<scope_type> *get_random_weight_factor() const
	{
		return this->random_weight_factor.get();
	}

	const condition<scope_type> *get_conditions() const
	{
		return this->conditions.get();
	}

	const std::vector<std::unique_ptr<event_option<scope_type>>> &get_options() const
	{
		return this->options;
	}

	int get_option_count() const
	{
		return static_cast<int>(this->get_options().size());
	}

	const std::string &get_option_name(const int option_index) const;
	std::string get_option_tooltip(const int option_index, const read_only_context &ctx) const;
	void do_option_effects(const int option_index, context &ctx) const;

private:
	std::unique_ptr<factor<scope_type>> random_weight_factor;
	std::unique_ptr<const condition<scope_type>> conditions;
	std::vector<std::unique_ptr<event_option<scope_type>>> options;
};

extern template class scoped_event_base<country>;

}
