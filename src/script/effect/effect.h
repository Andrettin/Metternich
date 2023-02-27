#pragma once

#include "script/target_variant.h"

namespace archimedes {
	class gsml_data;
	class gsml_property;
	enum class gsml_operator;
}

namespace metternich {

class country;
class event;
class province;
struct context;
struct read_only_context;

//a scripted effect
template <typename scope_type>
class effect
{
public:
	static std::unique_ptr<effect> from_gsml_property(const gsml_property &property);
	static std::unique_ptr<effect> from_gsml_scope(const gsml_data &scope);

	static const country *get_scope_country(const scope_type *scope);
	static const province *get_scope_province(const scope_type *scope);

	static scope_type *get_target_scope(const target_variant<scope_type> &target, const context &ctx);
	static const scope_type *get_target_scope(const target_variant<scope_type> &target, const read_only_context &ctx);

	explicit effect(const gsml_operator effect_operator);

	virtual ~effect()
	{
	}

	virtual const std::string &get_class_identifier() const = 0;

	virtual void process_gsml_property(const gsml_property &property);
	virtual void process_gsml_scope(const gsml_data &scope);

	virtual void check() const
	{
	}

	void do_effect(scope_type *scope, context &ctx) const;

	virtual void do_assignment_effect(scope_type *scope) const
	{
		Q_UNUSED(scope);

		throw std::runtime_error("The assignment operator is not supported for \"" + this->get_class_identifier() + "\" effects.");
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const
	{
		Q_UNUSED(ctx);

		this->do_assignment_effect(scope);
	}

	virtual void do_addition_effect(scope_type *scope) const
	{
		Q_UNUSED(scope)

		throw std::runtime_error("The addition operator is not supported for \"" + this->get_class_identifier() + "\" effects.");
	}

	virtual void do_addition_effect(scope_type *scope, context &ctx) const
	{
		Q_UNUSED(ctx);

		this->do_addition_effect(scope);
	}

	virtual void do_subtraction_effect(scope_type *scope) const
	{
		Q_UNUSED(scope)

		throw std::runtime_error("The subtraction operator is not supported for \"" + this->get_class_identifier() + "\" effects.");
	}

	virtual void do_subtraction_effect(scope_type *scope, context &ctx) const
	{
		Q_UNUSED(ctx);

		this->do_subtraction_effect(scope);
	}

	std::string get_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const;

	virtual std::string get_assignment_string() const
	{
		throw std::runtime_error("The assignment operator is not supported for \"" + this->get_class_identifier() + "\" effects.");
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		return this->get_assignment_string();
	}

	virtual std::string get_addition_string() const
	{
		throw std::runtime_error("The addition operator is not supported for \"" + this->get_class_identifier() + "\" effects.");
	}

	virtual std::string get_addition_string(const scope_type *scope, const read_only_context &ctx) const
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);

		return this->get_addition_string();
	}

	virtual std::string get_subtraction_string() const
	{
		throw std::runtime_error("The subtraction operator is not supported for \"" + this->get_class_identifier() + "\" effects.");
	}

	virtual std::string get_subtraction_string(const scope_type *scope, const read_only_context &ctx) const
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);

		return this->get_subtraction_string();
	}

	virtual bool is_hidden() const
	{
		return false;
	}

	gsml_operator get_operator() const
	{
		return this->effect_operator;
	}

private:
	gsml_operator effect_operator;
};

extern template class effect<const character>;
extern template class effect<const country>;
extern template class effect<population_unit>;
extern template class effect<const province>;
extern template class effect<const site>;

}
