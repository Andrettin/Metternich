#pragma once

#include "economy/commodity_container.h"
#include "util/decimillesimal_int.h"

Q_MOC_INCLUDE("culture/culture.h")
Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("economy/employment_type.h")
Q_MOC_INCLUDE("map/province.h")
Q_MOC_INCLUDE("map/site.h")
Q_MOC_INCLUDE("population/population_type.h")
Q_MOC_INCLUDE("religion/religion.h")
Q_MOC_INCLUDE("species/phenotype.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class culture;
class domain;
class employment_type;
class icon;
class phenotype;
class population_type;
class province;
class religion;
class site;
class species;
enum class food_type;
struct population_unit_percentages;

class population_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::population_type* type READ get_type NOTIFY type_changed)
	Q_PROPERTY(const metternich::culture* culture READ get_culture NOTIFY culture_changed)
	Q_PROPERTY(const metternich::religion* religion READ get_religion NOTIFY religion_changed)
	Q_PROPERTY(const metternich::phenotype* phenotype READ get_phenotype NOTIFY phenotype_changed)
	Q_PROPERTY(const metternich::employment_type* employment_type READ get_employment_type NOTIFY employment_type_changed)
	Q_PROPERTY(const metternich::icon* icon READ get_icon NOTIFY icon_changed)
	Q_PROPERTY(const metternich::domain* domain READ get_domain NOTIFY domain_changed)
	Q_PROPERTY(const metternich::province* province READ get_province NOTIFY province_changed)
	Q_PROPERTY(const metternich::site* site READ get_site NOTIFY site_changed)
	Q_PROPERTY(qint64 size READ get_size NOTIFY size_changed)
	Q_PROPERTY(archimedes::decimillesimal_int wealth READ get_wealth NOTIFY wealth_changed)
	Q_PROPERTY(int sick_percent READ get_sick_percent_int NOTIFY sick_percent_changed)
	Q_PROPERTY(int starving_percent READ get_starving_percent_int NOTIFY starving_percent_changed)
	Q_PROPERTY(int fulfilled_everyday_needs_percent READ get_fulfilled_everyday_needs_percent NOTIFY fulfilled_everyday_needs_percent_changed)
	Q_PROPERTY(int fulfilled_luxury_needs_percent READ get_fulfilled_luxury_needs_percent NOTIFY fulfilled_luxury_needs_percent_changed)

public:
	population_unit();
	explicit population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const metternich::employment_type *employment_type, const int64_t size, const population_unit_percentages &percentages, const site *site);
	explicit population_unit(const gsml_data &scope);

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	[[nodiscard]] QCoro::Task<void> do_cultural_change();
	[[nodiscard]] QCoro::Task<void> do_promotion();
	[[nodiscard]] QCoro::Task<void> do_promotion(const bool is_demotion);

	std::string get_scope_name() const;

	const population_type *get_type() const
	{
		return this->type;
	}

	const culture *get_culture() const
	{
		return this->culture;
	}

	const religion *get_religion() const
	{
		return this->religion;
	}

	const phenotype *get_phenotype() const
	{
		return this->phenotype;
	}

	const species *get_species() const;

	const employment_type *get_employment_type() const
	{
		return this->employment_type;
	}

	int64_t get_effective_employment_size(const employment_type *employment_type, const int64_t employee_size, const decimillesimal_int &sick_and_starving_percent) const;
	int64_t get_effective_employment_size() const;
	int64_t get_employment_size_from_effective_size(const employment_type *employment_type, const int64_t effective_employment_size, const decimillesimal_int &sick_and_starving_percent, const bool ceil) const;
	int64_t get_employment_size_from_effective_size(const int64_t effective_employment_size) const;

	const icon *get_icon() const;
	const icon *get_small_icon() const;

	const metternich::domain *get_domain() const
	{
		return this->domain;
	}

	void set_domain(const metternich::domain *domain);

	const province *get_province() const;

	const site *get_site() const
	{
		return this->site;
	}

	void set_site(const site *site);

	int64_t get_size() const
	{
		return this->size;
	}

	[[nodiscard]] QCoro::Task<void> change_size(const int64_t change, const population_unit_percentages &percentages, const bool change_input_storage);

	int64_t get_weighted_size() const;
	int64_t get_literate_size() const;

	const decimillesimal_int &get_literacy_rate() const
	{
		return this->literacy_rate;
	}

	void set_literacy_rate(const decimillesimal_int &literacy_rate);

	void change_literacy_rate(const decimillesimal_int &change)
	{
		this->set_literacy_rate(this->get_literacy_rate() + change);
	}

	const decimillesimal_int &get_wealth() const
	{
		return this->wealth;
	}

	Q_INVOKABLE QString get_wealth_qstring() const;

	void set_wealth(const decimillesimal_int &wealth);

	void change_wealth(const decimillesimal_int &change)
	{
		this->set_wealth(this->get_wealth() + change);
	}

	decimillesimal_int get_population_growth_rate() const;

	const decimillesimal_int &get_sick_percent() const
	{
		return this->sick_percent;
	}

	int get_sick_percent_int() const
	{
		return this->get_sick_percent().to_int();
	}

	const decimillesimal_int &get_starving_percent() const
	{
		return this->starving_percent;
	}

	int get_starving_percent_int() const
	{
		return this->get_starving_percent().to_int();
	}

	decimillesimal_int get_sick_and_starving_percent() const
	{
		return this->get_sick_percent() + this->get_starving_percent();
	}

	//this function returns true if the population unit was removed as a consequence of it
	[[nodiscard]] QCoro::Task<void> set_sick_and_starving_percent(const decimillesimal_int &sick_percent, const decimillesimal_int &starving_percent, const bool update_effective_employment);

	int get_fulfilled_everyday_needs_percent() const
	{
		return this->fulfilled_everyday_needs_percent;
	}

	void set_fulfilled_everyday_needs_percent(const int percent)
	{
		if (percent == this->get_fulfilled_everyday_needs_percent()) {
			return;
		}

		this->fulfilled_everyday_needs_percent = percent;

		emit fulfilled_everyday_needs_percent_changed();
	}

	int get_fulfilled_luxury_needs_percent() const
	{
		return this->fulfilled_luxury_needs_percent;
	}

	void set_fulfilled_luxury_needs_percent(const int percent)
	{
		if (percent == this->get_fulfilled_luxury_needs_percent()) {
			return;
		}

		this->fulfilled_luxury_needs_percent = percent;

		emit fulfilled_luxury_needs_percent_changed();
	}

	bool is_food_producer() const;

	std::map<food_type, decimillesimal_int> get_food_consumption(decimillesimal_int &total_food_consumption) const;
	std::map<food_type, decimillesimal_int> get_food_consumption_after_free_consumption(decimillesimal_int &total_food_consumption, decimillesimal_int *remaining_free_food_consumption) const;

	void consume_food_commodity(const commodity *food_commodity, decimillesimal_int &consumption);
	void consume_imported_food_commodity(const commodity *food_commodity, decimillesimal_int &consumption, const std::vector<const metternich::domain *> &trade_domains);
	[[nodiscard]] QCoro::Task<void> consume_food(const std::vector<const metternich::domain *> &trade_domains, const std::map<food_type, std::vector<const commodity *>> &food_type_commodities, const std::vector<const commodity *> &food_commodities, std::unordered_map<const site *, decimillesimal_int> &remaining_free_food_consumption_per_holding);
	void purchase_needs(const decimillesimal_int &consumption_wealth, const std::vector<const metternich::domain *> &trade_domains, commodity_map<decimillesimal_int> &remaining_demands);
	int purchase_needs(const decimillesimal_int &consumption_wealth, const commodity_map<int> &need_weights, const std::vector<const metternich::domain *> &trade_domains, commodity_map<decimillesimal_int> &remaining_demands);

	population_unit_percentages get_percentages() const;

signals:
	void type_changed();
	void culture_changed();
	void religion_changed();
	void phenotype_changed();
	void employment_type_changed();
	void icon_changed();
	void domain_changed();
	void province_changed();
	void site_changed();
	void size_changed();
	void literacy_rate_changed();
	void wealth_changed();
	void sick_percent_changed();
	void starving_percent_changed();
	void fulfilled_everyday_needs_percent_changed();
	void fulfilled_luxury_needs_percent_changed();

private:
	const population_type *type = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::phenotype *phenotype = nullptr;
	const metternich::employment_type *employment_type = nullptr;
	const metternich::domain *domain = nullptr;
	const metternich::site *site = nullptr;
	int64_t size = 0; //number of individuals in this population unit
	decimillesimal_int literacy_rate; //in percent
	decimillesimal_int wealth;
	decimillesimal_int sick_percent;
	decimillesimal_int starving_percent;
	int fulfilled_everyday_needs_percent = 100;
	int fulfilled_luxury_needs_percent = 100;
};

}
