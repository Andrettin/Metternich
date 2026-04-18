#pragma once

namespace metternich {

enum class population_strata {
	lower,
	middle,
	upper
};

inline int64_t get_population_strata_income_weight(const population_strata strata)
{
	switch (strata) {
		case population_strata::lower:
			return 1;
		case population_strata::middle:
			return 5;
		case population_strata::upper:
			return 25;
	}

	throw std::runtime_error(std::format("Invalid population strata: \"{}\".", static_cast<int>(strata)));
}

}
