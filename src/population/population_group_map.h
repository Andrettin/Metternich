#pragma once

namespace metternich {

class population_type;
class culture;

struct population_group_key final
{
	population_group_key() = default;

	explicit population_group_key(const population_type *type, const metternich::culture *culture)
		: type(type), culture(culture)
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

		return true;
	}

	bool operator==(const population_group_key &rhs) const = default;
	bool operator<(const population_group_key &rhs) const;

	const population_type *type = nullptr;
	const metternich::culture *culture = nullptr;
};

template <typename T>
using population_group_map = std::map<population_group_key, T>;

}
