#pragma once

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class country;
struct context;
struct read_only_context;

template <typename scope_type>
class effect_list;

class event_option final
{
public:
	static constexpr const char default_name[] = "OK";

	explicit event_option();
	~event_option();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void check() const;

	const std::string &get_name() const
	{
		return this->name;
	}

	std::string get_tooltip(const read_only_context &ctx) const;

	int get_ai_weight() const
	{
		return this->ai_weight;
	}

	std::string get_effects_string(const read_only_context &ctx) const;
	void do_effects(const country *country, context &ctx) const;

private:
	std::string name;
	std::string tooltip;
	int ai_weight = 1;
	std::unique_ptr<effect_list<const country>> effects;
};

}
