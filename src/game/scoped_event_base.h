#pragma once

namespace archimedes {
	class gsml_data;
}

namespace metternich {

class character;
class country;
class province;
enum class event_trigger;
struct context;
struct read_only_context;

template <typename scope_type>
class condition;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class event_option;

template <typename scope_type>
class factor;

template <typename scope_type>
class mean_time_to_happen;

template <typename scope_type>
class scoped_event_base
{
public:
	static void clear()
	{
		scoped_event_base::trigger_events.clear();
		scoped_event_base::trigger_random_events.clear();
	}

	static const std::vector<const scoped_event_base *> &get_trigger_events(const event_trigger trigger)
	{
		static std::vector<const scoped_event_base *> empty_list;

		const auto find_iterator = scoped_event_base::trigger_events.find(trigger);
		if (find_iterator != scoped_event_base::trigger_events.end()) {
			return find_iterator->second;
		}

		return empty_list;
	}

	static const std::vector<const scoped_event_base *> &get_trigger_random_events(const event_trigger trigger)
	{
		static std::vector<const scoped_event_base *> empty_list;

		const auto find_iterator = scoped_event_base::trigger_random_events.find(trigger);
		if (find_iterator != scoped_event_base::trigger_random_events.end()) {
			return find_iterator->second;
		}

		return empty_list;
	}

	static void add_trigger_none_random_weight(const event_trigger trigger, const int weight)
	{
		std::vector<const scoped_event_base *> &events = scoped_event_base::trigger_random_events[trigger];

		for (int i = 0; i < weight; ++i) {
			events.push_back(nullptr);
		}
	}

	static const scope_type *get_scope_from_context(const read_only_context &ctx);
	static bool is_player_scope(const scope_type *scope);

	static void check_events_for_scope(const scope_type *scope, const event_trigger trigger, const context &ctx);
	static void check_events_for_scope(const scope_type *scope, const event_trigger trigger);
	static void check_random_events_for_scope(const scope_type *scope, const context &ctx, const std::vector<const scoped_event_base *> &potential_events, const int delay);
	static void check_random_event_groups_for_scope(const scope_type *scope, const event_trigger trigger, const context &ctx);
	static void check_mtth_events_for_scope(const scope_type *scope);

private:
	static inline std::map<event_trigger, std::vector<const scoped_event_base *>> trigger_events;
	static inline std::map<event_trigger, std::vector<const scoped_event_base *>> trigger_random_events;
	static inline std::vector<const scoped_event_base *> mtth_events;
	static inline std::set<const scoped_event_base *> fired_events;

public:
	scoped_event_base();
	virtual ~scoped_event_base();

	bool process_gsml_scope(const gsml_data &scope);
	void initialize();
	void check() const;

	virtual const std::string &get_identifier() const = 0;
	virtual const std::string &get_name() const = 0;

	virtual event_trigger get_trigger() const = 0;
	virtual event_random_group *get_random_group() const = 0;

	bool is_random() const
	{
		return this->get_random_weight_factor() != nullptr;
	}

	void set_random_weight(const int weight);

	const factor<std::remove_const_t<scope_type>> *get_random_weight_factor() const
	{
		return this->random_weight_factor.get();
	}

	const metternich::mean_time_to_happen<std::remove_const_t<scope_type>> *get_mean_time_to_happen() const
	{
		return this->mean_time_to_happen.get();
	}

	virtual bool fires_only_once() const = 0;

	const condition<std::remove_const_t<scope_type>> *get_conditions() const
	{
		return this->conditions.get();
	}

	bool can_fire(const scope_type *scope, const read_only_context &ctx) const;

	void do_immediate_effects(scope_type *scope, context &ctx) const;

	const std::vector<std::unique_ptr<event_option<scope_type>>> &get_options() const
	{
		return this->options;
	}

	int get_option_count() const
	{
		return static_cast<int>(this->get_options().size());
	}

	bool is_option_available(const int option_index, const read_only_context &ctx) const;
	const std::string &get_option_name(const int option_index) const;
	std::string get_option_tooltip(const int option_index, const read_only_context &ctx) const;
	void do_option_effects(const int option_index, context &ctx) const;

	bool has_fired() const
	{
		return scoped_event_base::fired_events.contains(this);
	}

	void fire(const scope_type *scope, const context &ctx) const;
	virtual void create_instance(const context &ctx) const = 0;

private:
	std::unique_ptr<factor<std::remove_const_t<scope_type>>> random_weight_factor;
	std::unique_ptr<metternich::mean_time_to_happen<std::remove_const_t<scope_type>>> mean_time_to_happen;
	std::unique_ptr<const condition<std::remove_const_t<scope_type>>> conditions;
	std::unique_ptr<effect_list<scope_type>> immediate_effects;
	std::vector<std::unique_ptr<event_option<scope_type>>> options;
};

extern template class scoped_event_base<const character>;
extern template class scoped_event_base<const country>;
extern template class scoped_event_base<const province>;

}
