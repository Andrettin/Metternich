#pragma once

namespace metternich {

class culture;
class phenotype;
class population_type;

struct population_group_key final
{
	population_group_key() = default;

	explicit population_group_key(const population_type *type, const metternich::culture *culture, const metternich::phenotype *phenotype)
		: type(type), culture(culture), phenotype(phenotype)
	{
	}

	explicit population_group_key(const std::string &key_str);

private:
	int get_defined_property_count() const
	{
		int count = 0;

		if (this->type != nullptr) {
			++count;
		}

		if (this->culture != nullptr) {
			++count;
		}

		if (this->phenotype != nullptr) {
			++count;
		}

		return count;
	}

public:
	bool contains(const population_group_key &other_group_key) const
	{
		if ((*this) == other_group_key) {
			return true;
		}

		if (this->type != other_group_key.type) {
			if (other_group_key.type != nullptr) {
				return false;
			}
		}

		if (this->culture != other_group_key.culture) {
			if (other_group_key.culture != nullptr) {
				return false;
			}
		}

		if (this->phenotype != other_group_key.phenotype) {
			if (other_group_key.phenotype != nullptr) {
				return false;
			}
		}

		return true;
	}

	bool operator==(const population_group_key &rhs) const = default;
	bool operator<(const population_group_key &rhs) const;

	const population_type *type = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::phenotype *phenotype = nullptr;
};

template <typename T>
using population_group_map = std::map<population_group_key, T>;

}
