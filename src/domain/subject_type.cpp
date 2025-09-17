#include "metternich.h"

#include "domain/subject_type.h"

namespace metternich {

subject_type::subject_type(const std::string &identifier) : named_data_entry(identifier)
{
}

subject_type::~subject_type()
{
}

}
