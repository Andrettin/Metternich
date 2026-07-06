#pragma once

#include "culture/culture_base.h"
#include "culture/culture_container.h"
#include "database/data_type.h"
#include "species/phenotype_container.h"

namespace archimedes {
	class language;
}

namespace metternich {

class character;
class cultural_group;
class population_unit;
class species;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class mean_time_to_happen;

template <typename scope_type>
class modifier;

class culture final : public culture_base, public data_type<culture>
{
	Q_OBJECT

	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)
	Q_PROPERTY(const archimedes::language* language MEMBER language READ get_language NOTIFY changed)
	Q_PROPERTY(bool surname_first MEMBER surname_first READ is_surname_first NOTIFY changed)

public:
	struct cultural_derivation final
	{
		explicit cultural_derivation(const metternich::culture *culture);
		~cultural_derivation();

		void process_gsml_scope(const gsml_data &scope);

		const metternich::culture *culture = nullptr;
		std::unique_ptr<const and_condition<population_unit>> conditions;
		std::unique_ptr<metternich::mean_time_to_happen<population_unit>> mean_time_to_happen;
	};

	static constexpr const char class_identifier[] = "culture";
	static constexpr const char property_class_identifier[] = "metternich::culture*";
	static constexpr const char database_folder[] = "cultures";
	static constexpr bool history_enabled = true;

	using culture_base::database_dependencies;

	explicit culture(const std::string &identifier);
	~culture();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	using culture_base::get_group;

	const QColor &get_color() const
	{
		return this->color;
	}

	virtual const language *get_language() const override
	{
		return this->language;
	}

	bool is_surname_first() const
	{
		return this->surname_first;
	}

	const std::vector<const metternich::species *> &get_species() const
	{
		return this->species;
	}

	const phenotype_map<int64_t> get_phenotype_weights() const;

	const std::vector<std::unique_ptr<const cultural_derivation>> &get_cultural_derivations() const
	{
		return this->cultural_derivations;
	}

	const metternich::modifier<const character> *get_character_modifier() const
	{
		return this->character_modifier.get();
	}

signals:
	void changed();

private:
	QColor color;
	const archimedes::language *language = nullptr;
	bool surname_first = false;
	std::vector<const metternich::species *> species; //species which can have this culture
	std::vector<std::unique_ptr<const cultural_derivation>> cultural_derivations;
	std::unique_ptr<const metternich::modifier<const character>> character_modifier;
};

}
