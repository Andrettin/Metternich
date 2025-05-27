#pragma once

namespace metternich {

enum class taxonomic_rank {
	none,
	species,
	genus,
	subtribe,
	tribe,
	subfamily,
	family,
	superfamily,
	infraorder,
	suborder,
	order,
	infraclass,
	subclass,
	class_rank,
	superclass,
	infraphylum,
	subphylum,
	phylum,
	superphylum,
	infrakingdom,
	subkingdom,
	kingdom,
	domain,
	empire
};

}

Q_DECLARE_METATYPE(metternich::taxonomic_rank)
