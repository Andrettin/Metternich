cmake_minimum_required(VERSION 3.18.0)

project(metternich VERSION 1.0.0)

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "" FORCE)

if(MSVC)
	add_compile_options(/MP)
endif()

add_subdirectory(archimedes)

include_directories(
	archimedes/src
	src
	src/third_party
	src/third_party/maskedmousearea
	src/third_party/xbrz
)

set(character_SRCS
	src/character/advisor_category.cpp
	src/character/advisor_type.cpp
	src/character/advisor_type_container.cpp
	src/character/character.cpp
	src/character/character_container.cpp
	src/character/character_game_data.cpp
	src/character/character_role.cpp
	src/character/dynasty.cpp
	src/character/trait.cpp
	src/character/trait_type.cpp
)
source_group(character FILES ${character_SRCS})
set_source_files_properties(${character_SRCS} PROPERTIES UNITY_GROUP "character")

set(country_SRCS
	src/country/consulate.cpp
	src/country/consulate_container.cpp
	src/country/country.cpp
	src/country/country_container.cpp
	src/country/country_game_data.cpp
	src/country/country_history.cpp
	src/country/country_rank.cpp
	src/country/country_tier.cpp
	src/country/country_tier_data.cpp
	src/country/country_turn_data.cpp
	src/country/country_type.cpp
	src/country/cultural_group.cpp
	src/country/cultural_group_rank.cpp
	src/country/culture.cpp
	src/country/culture_base.cpp
	src/country/culture_container.cpp
	src/country/culture_history.cpp
	src/country/diplomacy_state.cpp
	src/country/government_group.cpp
	src/country/government_type.cpp
	src/country/government_type_container.cpp
	src/country/ideology.cpp
	src/country/ideology_container.cpp
	src/country/journal_entry.cpp
	src/country/law.cpp
	src/country/law_group.cpp
	src/country/law_group_container.cpp
	src/country/policy.cpp
	src/country/policy_container.cpp
	src/country/religion.cpp
	src/country/religion_base.cpp
	src/country/religion_container.cpp
	src/country/subject_type.cpp
	src/country/tradition.cpp
	src/country/tradition_category.cpp
	src/country/tradition_container.cpp
	src/country/tradition_group.cpp
	src/country/tradition_group_container.cpp
)
source_group(country FILES ${country_SRCS})
set_source_files_properties(${country_SRCS} PROPERTIES UNITY_GROUP "country")

set(database_SRCS
	src/database/defines.cpp
	src/database/preferences.cpp
)
source_group(database FILES ${database_SRCS})
set_source_files_properties(${database_SRCS} PROPERTIES UNITY_GROUP "database")

set(economy_SRCS
	src/economy/commodity.cpp
	src/economy/commodity_container.cpp
	src/economy/employment_location.cpp
	src/economy/expense_transaction.cpp
	src/economy/food_type.cpp
	src/economy/income_transaction.cpp
	src/economy/production_type.cpp
	src/economy/resource.cpp
	src/economy/resource_container.cpp
	src/economy/transaction.cpp
)
source_group(economy FILES ${economy_SRCS})
set_source_files_properties(${economy_SRCS} PROPERTIES UNITY_GROUP "economy")

set(game_SRCS
	src/game/event.cpp
	src/game/event_instance.cpp
	src/game/event_option.cpp
	src/game/event_random_group.cpp
	src/game/event_trigger.cpp
	src/game/game.cpp
	src/game/game_rules.cpp
	src/game/scenario.cpp
	src/game/scoped_event_base.cpp
)
source_group(game FILES ${game_SRCS})
set_source_files_properties(${game_SRCS} PROPERTIES UNITY_GROUP "game")

set(infrastructure_SRCS
	src/infrastructure/building_class.cpp
	src/infrastructure/building_class_container.cpp
	src/infrastructure/building_slot.cpp
	src/infrastructure/building_slot_type_container.cpp
	src/infrastructure/building_type.cpp
	src/infrastructure/building_type_container.cpp
	src/infrastructure/country_building_slot.cpp
	src/infrastructure/improvement.cpp
	src/infrastructure/improvement_container.cpp
	src/infrastructure/improvement_slot.cpp
	src/infrastructure/pathway.cpp
	src/infrastructure/pathway_container.cpp
	src/infrastructure/settlement_building_slot.cpp
	src/infrastructure/settlement_type.cpp
	src/infrastructure/wonder.cpp
)
source_group(infrastructure FILES ${infrastructure_SRCS})
set_source_files_properties(${infrastructure_SRCS} PROPERTIES UNITY_GROUP "infrastructure")

set(language_SRCS
	src/language/fallback_name_generator.cpp
)
source_group(language FILES ${language_SRCS})
set_source_files_properties(${language_SRCS} PROPERTIES UNITY_GROUP "language")

set(map_SRCS
	src/map/diplomatic_map_image_provider.cpp
	src/map/elevation_type.cpp
	src/map/forestation_type.cpp
	src/map/map.cpp
	src/map/map_country_model.cpp
	src/map/map_generator.cpp
	src/map/map_grid_model.cpp
	src/map/map_province_model.cpp
	src/map/map_site_model.cpp
	src/map/map_template.cpp
	src/map/moisture_type.cpp
	src/map/province.cpp
	src/map/province_container.cpp
	src/map/province_game_data.cpp
	src/map/province_history.cpp
	src/map/province_map_data.cpp
	src/map/region.cpp
	src/map/region_history.cpp
	src/map/route.cpp
	src/map/route_container.cpp
	src/map/site.cpp
	src/map/site_container.cpp
	src/map/site_game_data.cpp
	src/map/site_history.cpp
	src/map/site_map_data.cpp
	src/map/site_type.cpp
	src/map/temperature_type.cpp
	src/map/terrain_adjacency.cpp
	src/map/terrain_feature.cpp
	src/map/terrain_geodata_map.cpp
	src/map/terrain_type.cpp
	src/map/terrain_type_container.cpp
	src/map/tile.cpp
	src/map/tile_image_provider.cpp
	src/map/world.cpp
)
source_group(map FILES ${map_SRCS})
set_source_files_properties(${map_SRCS} PROPERTIES UNITY_GROUP "map")

set(population_SRCS
	src/population/phenotype.cpp
	src/population/phenotype_container.cpp
	src/population/population.cpp
	src/population/population_class.cpp
	src/population/population_class_container.cpp
	src/population/population_group_map.cpp
	src/population/population_type.cpp
	src/population/population_type_container.cpp
	src/population/population_unit.cpp
	src/population/profession.cpp
	src/population/profession_container.cpp
)
source_group(population FILES ${population_SRCS})
set_source_files_properties(${population_SRCS} PROPERTIES UNITY_GROUP "population")

set(script_SRCS
	src/script/context.cpp
	src/script/factor.cpp
	src/script/factor_modifier.cpp
	src/script/mean_time_to_happen.cpp
	src/script/modifier.cpp
	src/script/opinion_modifier.cpp
	src/script/opinion_modifier_container.cpp
	src/script/scripted_modifier_container.cpp
	src/script/scripted_scoped_modifier.cpp
	src/script/special_target_type.cpp
	src/script/text_processor.cpp
)
source_group(script FILES ${script_SRCS})
set_source_files_properties(${script_SRCS} PROPERTIES UNITY_GROUP "script")

set(script_condition_SRCS
	src/script/condition/and_condition.cpp
	src/script/condition/condition.cpp
	src/script/condition/not_condition.cpp
	src/script/condition/or_condition.cpp
	src/script/condition/scripted_condition.cpp
)
source_group(script\\condition FILES ${script_condition_SRCS})
set_source_files_properties(${script_condition_SRCS} PROPERTIES UNITY_GROUP "script_condition")

set(script_effect_SRCS
	src/script/effect/delayed_effect_instance.cpp
	src/script/effect/effect.cpp
	src/script/effect/effect_list.cpp
)
source_group(script\\effect FILES ${script_effect_SRCS})
set_source_files_properties(${script_effect_SRCS} PROPERTIES UNITY_GROUP "script_effect")

set(script_modifier_effect_SRCS
	src/script/modifier_effect/modifier_effect.cpp
)
source_group(script\\modifier_effect FILES ${script_modifier_effect_SRCS})
set_source_files_properties(${script_modifier_effect_SRCS} PROPERTIES UNITY_GROUP "script_modifier_effect")

set(spell_SRCS
	src/spell/spell.cpp
	src/spell/spell_container.cpp
	src/spell/spell_effect.cpp
	src/spell/spell_target.cpp
)
source_group(spell FILES ${spell_SRCS})
set_source_files_properties(${spell_SRCS} PROPERTIES UNITY_GROUP "spell")

set(technology_SRCS
	src/technology/technological_period.cpp
	src/technology/technology.cpp
	src/technology/technology_category.cpp
	src/technology/technology_container.cpp
)
source_group(technology FILES ${technology_SRCS})
set_source_files_properties(${technology_SRCS} PROPERTIES UNITY_GROUP "technology")

set(time_SRCS
	src/time/era.cpp
)
source_group(time FILES ${time_SRCS})
set_source_files_properties(${time_SRCS} PROPERTIES UNITY_GROUP "time")

set(ui_SRCS
	src/ui/icon.cpp
	src/ui/icon_base.cpp
	src/ui/icon_container.cpp
	src/ui/icon_image_provider.cpp
	src/ui/interface_image_provider.cpp
	src/ui/portrait.cpp
	src/ui/portrait_container.cpp
	src/ui/portrait_image_provider.cpp
)
source_group(ui FILES ${ui_SRCS})
set_source_files_properties(${ui_SRCS} PROPERTIES UNITY_GROUP "ui")

set(unit_SRCS
	src/unit/army.cpp
	src/unit/civilian_unit.cpp
	src/unit/civilian_unit_class.cpp
	src/unit/civilian_unit_class_container.cpp
	src/unit/civilian_unit_type.cpp
	src/unit/historical_civilian_unit.cpp
	src/unit/historical_military_unit.cpp
	src/unit/historical_transporter.cpp
	src/unit/military_unit.cpp
	src/unit/military_unit_class.cpp
	src/unit/military_unit_class_container.cpp
	src/unit/military_unit_domain.cpp
	src/unit/military_unit_category.cpp
	src/unit/military_unit_stat.cpp
	src/unit/military_unit_type.cpp
	src/unit/military_unit_type_container.cpp
	src/unit/promotion.cpp
	src/unit/promotion_container.cpp
	src/unit/transporter.cpp
	src/unit/transporter_category.cpp
	src/unit/transporter_class.cpp
	src/unit/transporter_class_container.cpp
	src/unit/transporter_stat.cpp
	src/unit/transporter_type.cpp
	src/unit/transporter_type_container.cpp
)
source_group(unit FILES ${unit_SRCS})
set_source_files_properties(${unit_SRCS} PROPERTIES UNITY_GROUP "unit")

set(maskedmousearea_SRCS
	src/third_party/maskedmousearea/maskedmousearea.cpp
)
source_group(maskedmousearea FILES ${maskedmousearea_SRCS})
set_source_files_properties(${maskedmousearea_SRCS} PROPERTIES UNITY_GROUP "maskedmousearea")

set(xbrz_SRCS
	src/third_party/xbrz/xbrz.cpp
)
source_group(xbrz FILES ${xbrz_SRCS})
set_source_files_properties(${xbrz_SRCS} PROPERTIES UNITY_GROUP "xbrz")

set(metternich_SRCS
	${character_SRCS}
	${country_SRCS}
	${database_SRCS}
	${economy_SRCS}
	${game_SRCS}
	${infrastructure_SRCS}
	${language_SRCS}
	${map_SRCS}
	${population_SRCS}
	${script_SRCS}
	${script_condition_SRCS}
	${script_effect_SRCS}
	${script_modifier_effect_SRCS}
	${spell_SRCS}
	${technology_SRCS}
	${time_SRCS}
	${ui_SRCS}
	${unit_SRCS}
	${maskedmousearea_SRCS}
	${xbrz_SRCS}
	src/engine_interface.cpp
)

set(character_HDRS
	src/character/advisor_category.h
	src/character/advisor_type.h
	src/character/advisor_type_container.h
	src/character/character.h
	src/character/character_container.h
	src/character/character_game_data.h
	src/character/character_history.h
	src/character/character_role.h
	src/character/dynasty.h
	src/character/trait.h
	src/character/trait_type.h
)
source_group(character FILES ${character_HDRS})

set(country_HDRS
	src/country/consulate.h
	src/country/consulate_container.h
	src/country/country.h
	src/country/country_container.h
	src/country/country_game_data.h
	src/country/country_history.h
	src/country/country_rank.h
	src/country/country_tier.h
	src/country/country_tier_data.h
	src/country/country_turn_data.h
	src/country/country_type.h
	src/country/cultural_group.h
	src/country/cultural_group_rank.h
	src/country/culture.h
	src/country/culture_base.h
	src/country/culture_container.h
	src/country/culture_history.h
	src/country/diplomacy_state.h
	src/country/government_group.h
	src/country/government_type.h
	src/country/government_type_container.h
	src/country/ideology.h
	src/country/ideology_container.h
	src/country/journal_entry.h
	src/country/law.h
	src/country/law_group.h
	src/country/law_group_container.h
	src/country/policy.h
	src/country/policy_container.h
	src/country/religion.h
	src/country/religion_base.h
	src/country/religion_container.h
	src/country/religious_group.h
	src/country/subject_type.h
	src/country/tradition.h
	src/country/tradition_category.h
	src/country/tradition_container.h
	src/country/tradition_group.h
	src/country/tradition_group_container.h
)
source_group(country FILES ${country_HDRS})

set(database_HDRS
	src/database/defines.h
	src/database/preferences.h
)
source_group(database FILES ${database_HDRS})

set(economy_HDRS
	src/economy/commodity.h
	src/economy/commodity_container.h
	src/economy/employment_location.h
	src/economy/expense_transaction.h
	src/economy/expense_transaction_type.h
	src/economy/food_type.h
	src/economy/income_transaction.h
	src/economy/income_transaction_type.h
	src/economy/production_type.h
	src/economy/resource.h
	src/economy/resource_container.h
	src/economy/transaction.h
)
source_group(economy FILES ${economy_HDRS})

set(game_HDRS
	src/game/character_event.h
	src/game/country_event.h
	src/game/event.h
	src/game/event_instance.h
	src/game/event_option.h
	src/game/event_random_group.h
	src/game/event_trigger.h
	src/game/game.h
	src/game/game_rules.h
	src/game/province_event.h
	src/game/scenario.h
	src/game/scoped_event_base.h
)
source_group(game FILES ${game_HDRS})

set(infrastructure_HDRS
	src/infrastructure/building_class.h
	src/infrastructure/building_class_container.h
	src/infrastructure/building_slot.h
	src/infrastructure/building_slot_type.h
	src/infrastructure/building_slot_type_container.h
	src/infrastructure/building_type.h
	src/infrastructure/building_type_container.h
	src/infrastructure/country_building_slot.h
	src/infrastructure/improvement.h
	src/infrastructure/improvement_container.h
	src/infrastructure/improvement_slot.h
	src/infrastructure/pathway.h
	src/infrastructure/pathway_container.h
	src/infrastructure/settlement_building_slot.h
	src/infrastructure/settlement_type.h
	src/infrastructure/wonder.h
)
source_group(infrastructure FILES ${infrastructure_HDRS})

set(language_HDRS
	src/language/fallback_name_generator.h
)
source_group(language FILES ${language_HDRS})

set(map_HDRS
	src/map/diplomatic_map_image_provider.h
	src/map/diplomatic_map_mode.h
	src/map/elevation_type.h
	src/map/forestation_type.h
	src/map/map.h
	src/map/map_country_model.h
	src/map/map_generator.h
	src/map/map_grid_model.h
	src/map/map_province_model.h
	src/map/map_site_model.h
	src/map/map_template.h
	src/map/moisture_type.h
	src/map/province.h
	src/map/province_container.h
	src/map/province_game_data.h
	src/map/province_history.h
	src/map/province_map_data.h
	src/map/region.h
	src/map/region_history.h
	src/map/route.h
	src/map/route_container.h
	src/map/route_game_data.h
	src/map/route_history.h
	src/map/site.h
	src/map/site_container.h
	src/map/site_game_data.h
	src/map/site_history.h
	src/map/site_map_data.h
	src/map/site_type.h
	src/map/temperature_type.h
	src/map/terrain_adjacency.h
	src/map/terrain_adjacency_type.h
	src/map/terrain_feature.h
	src/map/terrain_geodata_map.h
	src/map/terrain_type.h
	src/map/terrain_type_container.h
	src/map/tile.h
	src/map/tile_image_provider.h
	src/map/world.h
)
source_group(map FILES ${map_HDRS})

set(population_HDRS
	src/population/phenotype.h
	src/population/phenotype_container.h
	src/population/population.h
	src/population/population_class.h
	src/population/population_class_container.h
	src/population/population_group_map.h
	src/population/population_type.h
	src/population/population_type_container.h
	src/population/population_unit.h
	src/population/profession.h
	src/population/profession_container.h
)
source_group(population FILES ${population_HDRS})

set(script_HDRS
	src/script/context.h
	src/script/factor.h
	src/script/factor_modifier.h
	src/script/flag.h
	src/script/mean_time_to_happen.h
	src/script/modifier.h
	src/script/opinion_modifier.h
	src/script/opinion_modifier_container.h
	src/script/scripted_character_modifier.h
	src/script/scripted_modifier.h
	src/script/scripted_modifier_container.h
	src/script/scripted_province_modifier.h
	src/script/scripted_scoped_modifier.h
	src/script/scripted_site_modifier.h
	src/script/special_target_type.h
	src/script/target_variant.h
	src/script/text_processor.h
)
source_group(script FILES ${script_HDRS})

set(script_condition_HDRS
	src/script/condition/adjacent_terrain_condition.h
	src/script/condition/advisor_condition.h
	src/script/condition/advisor_category_condition.h
	src/script/condition/advisor_type_condition.h
	src/script/condition/age_condition.h
	src/script/condition/anarchy_condition.h
	src/script/condition/and_condition.h
	src/script/condition/any_global_population_unit_condition.h
	src/script/condition/any_known_country_condition.h
	src/script/condition/any_neighbor_country_condition.h
	src/script/condition/any_population_unit_condition.h
	src/script/condition/any_settlement_condition.h
	src/script/condition/artillery_condition.h
	src/script/condition/attacking_commander_condition.h
	src/script/condition/available_food_condition.h
	src/script/condition/available_health_condition.h
	src/script/condition/birth_year_condition.h
	src/script/condition/can_gain_building_class_condition.h
	src/script/condition/can_have_trait_condition.h
	src/script/condition/capital_condition.h
	src/script/condition/cavalry_condition.h
	src/script/condition/coastal_condition.h
	src/script/condition/commodity_condition.h
	src/script/condition/condition.h
	src/script/condition/consciousness_condition.h
	src/script/condition/core_condition.h
	src/script/condition/country_condition.h
	src/script/condition/country_exists_condition.h
	src/script/condition/country_scope_condition.h
	src/script/condition/country_type_condition.h
	src/script/condition/cultural_group_condition.h
	src/script/condition/culture_condition.h
	src/script/condition/discovered_province_condition.h
	src/script/condition/discovered_region_condition.h
	src/script/condition/event_condition.h
	src/script/condition/game_rule_condition.h
	src/script/condition/gender_condition.h
	src/script/condition/government_type_condition.h
	src/script/condition/has_building_class_condition.h
	src/script/condition/has_building_condition.h
	src/script/condition/has_flag_condition.h
	src/script/condition/has_population_culture_condition.h
	src/script/condition/has_population_ideology_condition.h
	src/script/condition/has_population_religion_condition.h
	src/script/condition/has_population_type_condition.h
	src/script/condition/has_resource_condition.h
	src/script/condition/has_route_condition.h
	src/script/condition/has_terrain_condition.h
	src/script/condition/health_condition.h
	src/script/condition/ideology_condition.h
	src/script/condition/improvement_condition.h
	src/script/condition/independent_condition.h
	src/script/condition/infantry_condition.h
	src/script/condition/inflation_condition.h
	src/script/condition/is_advisor_condition.h
	src/script/condition/is_military_unit_category_available_condition.h
	src/script/condition/is_ruler_condition.h
	src/script/condition/law_condition.h
	src/script/condition/militancy_condition.h
	src/script/condition/military_unit_category_condition.h
	src/script/condition/military_unit_domain_condition.h
	src/script/condition/military_unit_type_condition.h
	src/script/condition/near_water_condition.h
	src/script/condition/not_condition.h
	src/script/condition/numerical_condition.h
	src/script/condition/or_condition.h
	src/script/condition/owns_province_condition.h
	src/script/condition/owns_site_condition.h
	src/script/condition/population_scaled_commodity_condition.h
	src/script/condition/population_type_condition.h
	src/script/condition/population_unit_count_condition.h
	src/script/condition/produces_commodity_condition.h
	src/script/condition/promotion_condition.h
	src/script/condition/province_condition.h
	src/script/condition/province_count_condition.h
	src/script/condition/provincial_capital_condition.h
	src/script/condition/religion_condition.h
	src/script/condition/religious_group_condition.h
	src/script/condition/resource_condition.h
	src/script/condition/river_condition.h
	src/script/condition/root_character_condition.h
	src/script/condition/ruler_condition.h
	src/script/condition/ruler_scope_condition.h
	src/script/condition/saved_scope_condition.h
	src/script/condition/scripted_modifier_condition.h
	src/script/condition/scope_condition.h
	src/script/condition/scope_condition_base.h
	src/script/condition/scripted_condition.h
	src/script/condition/scripted_condition_condition.h
	src/script/condition/settlement_condition.h
	src/script/condition/settlement_type_condition.h
	src/script/condition/source_character_condition.h
	src/script/condition/source_site_condition.h
	src/script/condition/source_site_scope_condition.h
	src/script/condition/subject_type_condition.h
	src/script/condition/technology_condition.h
	src/script/condition/terrain_condition.h
	src/script/condition/tooltip_condition.h
	src/script/condition/tradition_condition.h
	src/script/condition/trait_condition.h
	src/script/condition/war_condition.h
	src/script/condition/wealth_condition.h
	src/script/condition/wealth_inflated_condition.h
	src/script/condition/year_condition.h
)
source_group(script\\condition FILES ${script_condition_HDRS})

set(script_effect_HDRS
	src/script/effect/add_building_class_effect.h
	src/script/effect/add_improvement_effect.h
	src/script/effect/any_known_country_effect.h
	src/script/effect/any_neighbor_country_effect.h
	src/script/effect/any_population_unit_effect.h
	src/script/effect/battle_effect.h
	src/script/effect/capital_effect.h
	src/script/effect/change_opinion_effect.h
	src/script/effect/clear_flag_effect.h
	src/script/effect/commodity_effect.h
	src/script/effect/commodity_percent_effect.h
	src/script/effect/consciousness_effect.h
	src/script/effect/country_effect.h
	src/script/effect/create_military_unit_effect.h
	src/script/effect/create_transporter_effect.h
	src/script/effect/delayed_effect.h
	src/script/effect/delayed_effect_instance.h
	src/script/effect/effect.h
	src/script/effect/effect_list.h
	src/script/effect/event_effect.h
	src/script/effect/free_technologies_effect.h
	src/script/effect/gain_spell_scroll_effect.h
	src/script/effect/hidden_effect.h
	src/script/effect/if_effect.h
	src/script/effect/inflation_effect.h
	src/script/effect/location_effect.h
	src/script/effect/migrate_to_effect.h
	src/script/effect/militancy_effect.h
	src/script/effect/opinion_modifiers_effect.h
	src/script/effect/policy_effect.h
	src/script/effect/population_scaled_commodity_effect.h
	src/script/effect/random_effect.h
	src/script/effect/random_global_population_unit_effect.h
	src/script/effect/random_known_country_effect.h
	src/script/effect/random_list_effect.h
	src/script/effect/random_neighbor_country_effect.h
	src/script/effect/random_settlement_effect.h
	src/script/effect/save_scope_as_effect.h
	src/script/effect/saved_scope_effect.h
	src/script/effect/scope_effect.h
	src/script/effect/scope_effect_base.h
	src/script/effect/scripted_effect.h
	src/script/effect/scripted_effect_effect.h
	src/script/effect/scripted_modifiers_effect.h
	src/script/effect/set_flag_effect.h
	src/script/effect/source_site_effect.h
	src/script/effect/tooltip_effect.h
	src/script/effect/traits_effect.h
	src/script/effect/wealth_effect.h
	src/script/effect/wealth_inflated_effect.h
)
source_group(script\\effect FILES ${script_effect_HDRS})

set(script_modifier_effect_HDRS
	src/script/modifier_effect/advisor_cost_modifier_effect.h
	src/script/modifier_effect/ai_building_desire_modifier_effect.h
	src/script/modifier_effect/air_discipline_modifier_effect.h
	src/script/modifier_effect/artillery_cost_modifier_effect.h
	src/script/modifier_effect/building_capacity_modifier_effect.h
	src/script/modifier_effect/building_cost_efficiency_modifier_effect.h
	src/script/modifier_effect/capital_commodity_bonus_modifier_effect.h
	src/script/modifier_effect/capital_commodity_bonus_per_population_modifier_effect.h
	src/script/modifier_effect/capital_commodity_output_modifier_effect.h
	src/script/modifier_effect/category_research_modifier_effect.h
	src/script/modifier_effect/cavalry_cost_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_for_tile_threshold_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_per_adjacent_terrain_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_per_building_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_per_improved_resource_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_per_improvement_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_per_population_modifier_effect.h
	src/script/modifier_effect/commodity_output_modifier_effect.h
	src/script/modifier_effect/commodity_throughput_modifier_effect.h
	src/script/modifier_effect/deployment_limit_modifier_effect.h
	src/script/modifier_effect/depot_level_modifier_effect.h
	src/script/modifier_effect/diplomatic_penalty_for_expansion_modifier_effect.h
	src/script/modifier_effect/entrenchment_bonus_modifier_effect.h
	src/script/modifier_effect/free_artillery_promotion_modifier_effect.h
	src/script/modifier_effect/free_building_class_modifier_effect.h
	src/script/modifier_effect/free_cavalry_promotion_modifier_effect.h
	src/script/modifier_effect/free_consulate_modifier_effect.h
	src/script/modifier_effect/free_infantry_promotion_modifier_effect.h
	src/script/modifier_effect/free_warship_promotion_modifier_effect.h
	src/script/modifier_effect/gain_technologies_known_by_others_modifier_effect.h
	src/script/modifier_effect/industrial_output_modifier_effect.h
	src/script/modifier_effect/infantry_cost_modifier_effect.h
	src/script/modifier_effect/inflation_change_modifier_effect.h
	src/script/modifier_effect/land_damage_modifier_effect.h
	src/script/modifier_effect/land_discipline_modifier_effect.h
	src/script/modifier_effect/land_morale_recovery_modifier_effect.h
	src/script/modifier_effect/land_recovery_modifier_effect.h
	src/script/modifier_effect/law_cost_modifier_effect.h
	src/script/modifier_effect/leader_cost_modifier_effect.h
	src/script/modifier_effect/merchant_ship_stat_modifier_effect.h
	src/script/modifier_effect/military_unit_category_stat_modifier_effect.h
	src/script/modifier_effect/military_unit_stat_modifier_effect.h
	src/script/modifier_effect/military_unit_type_stat_modifier_effect.h
	src/script/modifier_effect/modifier_effect.h
	src/script/modifier_effect/naval_discipline_modifier_effect.h
	src/script/modifier_effect/output_modifier_effect.h
	src/script/modifier_effect/population_type_bonus_modifier_effect.h
	src/script/modifier_effect/population_type_militancy_modifier_effect.h
	src/script/modifier_effect/port_level_modifier_effect.h
	src/script/modifier_effect/profession_commodity_bonus_modifier_effect.h
	src/script/modifier_effect/resource_output_modifier_effect.h
	src/script/modifier_effect/ship_stat_modifier_effect.h
	src/script/modifier_effect/storage_capacity_modifier_effect.h
	src/script/modifier_effect/throughput_modifier_effect.h
	src/script/modifier_effect/unit_upgrade_cost_modifier_effect.h
	src/script/modifier_effect/warship_cost_modifier_effect.h
	src/script/modifier_effect/wonder_cost_efficiency_modifier_effect.h
)
source_group(script\\modifier_effect FILES ${script_modifier_effect_HDRS})

set(spell_HDRS
	src/spell/damage_spell_effect.h
	src/spell/healing_spell_effect.h
	src/spell/spell.h
	src/spell/spell_container.h
	src/spell/spell_effect.h
	src/spell/spell_target.h
)
source_group(spell FILES ${spell_HDRS})

set(technology_HDRS
	src/technology/technological_period.h
	src/technology/technology.h
	src/technology/technology_category.h
	src/technology/technology_container.h
)
source_group(technology FILES ${technology_HDRS})

set(time_HDRS
	src/time/era.h
)
source_group(time FILES ${time_HDRS})

set(ui_HDRS
	src/ui/icon.h
	src/ui/icon_base.h
	src/ui/icon_container.h
	src/ui/icon_image_provider.h
	src/ui/interface_image_provider.h
	src/ui/portrait.h
	src/ui/portrait_container.h
	src/ui/portrait_image_provider.h
)
source_group(ui FILES ${ui_HDRS})

set(unit_HDRS
	src/unit/army.h
	src/unit/civilian_unit.h
	src/unit/civilian_unit_class.h
	src/unit/civilian_unit_class_container.h
	src/unit/civilian_unit_type.h
	src/unit/historical_civilian_unit.h
	src/unit/historical_civilian_unit_history.h
	src/unit/historical_military_unit.h
	src/unit/historical_military_unit_history.h
	src/unit/historical_transporter.h
	src/unit/historical_transporter_history.h
	src/unit/military_unit.h
	src/unit/military_unit_class.h
	src/unit/military_unit_class_container.h
	src/unit/military_unit_domain.h
	src/unit/military_unit_category.h
	src/unit/military_unit_stat.h
	src/unit/military_unit_type.h
	src/unit/military_unit_type_container.h
	src/unit/promotion.h
	src/unit/promotion_container.h
	src/unit/transporter.h
	src/unit/transporter_category.h
	src/unit/transporter_class.h
	src/unit/transporter_class_container.h
	src/unit/transporter_stat.h
	src/unit/transporter_type.h
	src/unit/transporter_type_container.h
)
source_group(unit FILES ${unit_HDRS})

set(maskedmousearea_HDRS
	src/third_party/maskedmousearea/maskedmousearea.h
)
source_group(maskedmousearea FILES ${maskedmousearea_HDRS})

set(xbrz_HDRS
	src/third_party/xbrz/xbrz.h
	src/third_party/xbrz/xbrz_config.h
	src/third_party/xbrz/xbrz_tools.h
)
source_group(xbrz FILES ${xbrz_HDRS})

set(metternich_HDRS
	${character_HDRS}
	${country_HDRS}
	${database_HDRS}
	${economy_HDRS}
	${game_HDRS}
	${infrastructure_HDRS}
	${language_HDRS}
	${map_HDRS}
	${population_HDRS}
	${script_HDRS}
	${script_condition_HDRS}
	${script_effect_HDRS}
	${script_modifier_effect_HDRS}
	${spell_HDRS}
	${technology_HDRS}
	${time_HDRS}
	${ui_HDRS}
	${unit_HDRS}
	${maskedmousearea_HDRS}
	${xbrz_HDRS}
	src/engine_interface.h
	src/metternich.h
)

set(metternich_main_SRCS
	src/main.cpp
)

set(game_test_SRCS
#	test/game/game_test.cpp
)
source_group(game FILES ${game_test_SRCS})

set(metternich_test_SRCS
	${game_test_SRCS}
	test/main.cpp
)

option(WITH_GEOJSON "Compile with support for generating map data from GeoJSON files" OFF)

find_package(Boost 1.69.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
add_definitions(-DBOOST_DATE_TIME_NO_LIB)

#Qt6 modules
#different modules have different licenses, make sure all modules used here are compatible with the GPL 3.0 license
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.5 COMPONENTS Core REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt6 6.5 COMPONENTS Gui REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt6 6.5 COMPONENTS Widgets REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt6 6.5 COMPONENTS Multimedia REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt6 6.5 COMPONENTS Location REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt6 6.5 COMPONENTS Qml REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt6 6.5 COMPONENTS Quick REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt6 6.5 COMPONENTS Charts REQUIRED) #licensed under the GPL 3.0
find_package(Qt6 6.5 COMPONENTS ShaderTools REQUIRED)

if(WITH_GEOJSON)
	add_definitions(-DUSE_GEOJSON)
endif()

if(MSVC)
	add_definitions(/FI"metternich.h")
else()
	#GCC/Clang
	add_definitions(-include metternich.h)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_UNITY_BUILD ON)

if(MSVC)
	add_compile_options(/W4 /w44800 /wd4244 /wd4458 /bigobj)
else()
	add_compile_options(-Wall -Wno-unknown-pragmas)
endif()

add_library(metternich STATIC ${metternich_SRCS} ${metternich_HDRS})
add_executable(metternich_main WIN32 ${metternich_main_SRCS})

add_executable(metternich_test ${metternich_test_SRCS})
add_test(metternich_test metternich_test)
enable_testing()

qt_add_shaders(metternich_main "shaders"
	PREFIX "/"
	FILES
		"shaders/opacitymask.frag"
	OUTPUTS
		"opacitymask.frag.qsb"
)

target_precompile_headers(metternich REUSE_FROM archimedes)

set_target_properties(metternich PROPERTIES UNITY_BUILD_MODE GROUP)

set_target_properties(metternich_test PROPERTIES UNITY_BUILD_MODE GROUP)
set_source_files_properties(${game_test_SRCS} PROPERTIES UNITY_GROUP "game_test")

target_link_libraries(metternich PUBLIC archimedes)

target_precompile_headers(metternich_main REUSE_FROM archimedes)
target_precompile_headers(metternich_test REUSE_FROM archimedes)

set_target_properties(metternich_main PROPERTIES OUTPUT_NAME "metternich")

target_link_libraries(metternich_main PUBLIC metternich Qt6::Charts)
target_link_libraries(metternich_test PUBLIC metternich)
