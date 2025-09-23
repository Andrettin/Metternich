cmake_minimum_required(VERSION 3.18.0)

project(metternich VERSION 1.0.0)

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "" FORCE)

if(MSVC)
	add_compile_options(/MP)
endif()

add_subdirectory(archimedes)

include_directories(
	archimedes/src
	archimedes/src/third_party/magic_enum/include
	archimedes/src/third_party/maskedmousearea
	src
	src/third_party
	src/third_party/xbrz
)

set(character_SRCS
	src/character/character.cpp
	src/character/character_class.cpp
	src/character/character_class_container.cpp
	src/character/character_container.cpp
	src/character/character_data_model.cpp
	src/character/character_game_data.cpp
	src/character/character_history.cpp
	src/character/character_trait.cpp
	src/character/dynasty.cpp
	src/character/family_tree_model.cpp
	src/character/monster_type.cpp
	src/character/party.cpp
)
source_group(character FILES ${character_SRCS})
set_source_files_properties(${character_SRCS} PROPERTIES UNITY_GROUP "character")

set(database_SRCS
	src/database/data_entry_container.cpp
	src/database/defines.cpp
	src/database/preferences.cpp
)
source_group(database FILES ${database_SRCS})
set_source_files_properties(${database_SRCS} PROPERTIES UNITY_GROUP "database")

set(domain_SRCS
	src/domain/consulate.cpp
	src/domain/consulate_container.cpp
	src/domain/country_ai.cpp
	src/domain/country_economy.cpp
	src/domain/country_government.cpp
	src/domain/country_military.cpp
	src/domain/country_rank.cpp
	src/domain/country_technology.cpp
	src/domain/country_tier_data.cpp
	src/domain/country_turn_data.cpp
	src/domain/cultural_group.cpp
	src/domain/culture.cpp
	src/domain/culture_base.cpp
	src/domain/culture_container.cpp
	src/domain/culture_history.cpp
	src/domain/domain.cpp
	src/domain/domain_container.cpp
	src/domain/domain_game_data.cpp
	src/domain/domain_history.cpp
	src/domain/government_group.cpp
	src/domain/government_type.cpp
	src/domain/government_type_container.cpp
	src/domain/idea.cpp
	src/domain/idea_slot.cpp
	src/domain/idea_trait.cpp
	src/domain/journal_entry.cpp
	src/domain/law.cpp
	src/domain/law_group.cpp
	src/domain/law_group_container.cpp
	src/domain/office.cpp
	src/domain/subject_type.cpp
)
source_group(domain FILES ${domain_SRCS})
set_source_files_properties(${domain_SRCS} PROPERTIES UNITY_GROUP "domain")

set(economy_SRCS
	src/economy/commodity.cpp
	src/economy/commodity_container.cpp
	src/economy/commodity_unit.cpp
	src/economy/expense_transaction.cpp
	src/economy/income_transaction.cpp
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
	src/game/game.cpp
	src/game/game_rules.cpp
	src/game/scenario.cpp
	src/game/scenario_model.cpp
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
	src/infrastructure/dungeon.cpp
	src/infrastructure/dungeon_area.cpp
	src/infrastructure/holding_type.cpp
	src/infrastructure/improvement.cpp
	src/infrastructure/improvement_container.cpp
	src/infrastructure/pathway.cpp
	src/infrastructure/pathway_container.cpp
	src/infrastructure/settlement_building_slot.cpp
	src/infrastructure/wonder.cpp
)
source_group(infrastructure FILES ${infrastructure_SRCS})
set_source_files_properties(${infrastructure_SRCS} PROPERTIES UNITY_GROUP "infrastructure")

set(item_SRCS
	src/item/enchantment.cpp
	src/item/item.cpp
	src/item/item_class.cpp
	src/item/item_material.cpp
	src/item/item_type.cpp
)
source_group(item FILES ${item_SRCS})
set_source_files_properties(${item_SRCS} PROPERTIES UNITY_GROUP "item")

set(language_SRCS
	src/language/fallback_name_generator.cpp
)
source_group(language FILES ${language_SRCS})
set_source_files_properties(${language_SRCS} PROPERTIES UNITY_GROUP "language")

set(map_SRCS
	src/map/celestial_body_type.cpp
	src/map/diplomatic_map_image_provider.cpp
	src/map/map.cpp
	src/map/map_country_model.cpp
	src/map/map_generator.cpp
	src/map/map_grid_model.cpp
	src/map/map_province_model.cpp
	src/map/map_site_model.cpp
	src/map/map_template.cpp
	src/map/province.cpp
	src/map/province_container.cpp
	src/map/province_game_data.cpp
	src/map/province_history.cpp
	src/map/province_map_data.cpp
	src/map/province_map_image_provider.cpp
	src/map/region.cpp
	src/map/region_history.cpp
	src/map/route.cpp
	src/map/route_container.cpp
	src/map/site.cpp
	src/map/site_container.cpp
	src/map/site_game_data.cpp
	src/map/site_history.cpp
	src/map/site_map_data.cpp
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
	src/population/population.cpp
	src/population/population_class.cpp
	src/population/population_class_container.cpp
	src/population/population_group_map.cpp
	src/population/population_type.cpp
	src/population/population_type_container.cpp
	src/population/population_unit.cpp
)
source_group(population FILES ${population_SRCS})
set_source_files_properties(${population_SRCS} PROPERTIES UNITY_GROUP "population")

set(religion_SRCS
	src/religion/deity.cpp
	src/religion/deity_slot.cpp
	src/religion/deity_trait.cpp
	src/religion/religion.cpp
	src/religion/religion_base.cpp
	src/religion/religion_container.cpp
)
source_group(religion FILES ${religion_SRCS})
set_source_files_properties(${religion_SRCS} PROPERTIES UNITY_GROUP "religion")

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
	src/script/text_processor.cpp
)
source_group(script FILES ${script_SRCS})
set_source_files_properties(${script_SRCS} PROPERTIES UNITY_GROUP "script")

set(script_condition_SRCS
	src/script/condition/and_condition.cpp
	src/script/condition/condition.cpp
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

set(species_SRCS
	src/species/phenotype.cpp
	src/species/phenotype_container.cpp
	src/species/species.cpp
	src/species/taxon.cpp
	src/species/taxon_base.cpp
)
source_group(species FILES ${species_SRCS})
set_source_files_properties(${species_SRCS} PROPERTIES UNITY_GROUP "species")

set(spell_SRCS
	src/spell/spell.cpp
	src/spell/spell_container.cpp
	src/spell/spell_effect.cpp
)
source_group(spell FILES ${spell_SRCS})
set_source_files_properties(${spell_SRCS} PROPERTIES UNITY_GROUP "spell")

set(technology_SRCS
	src/technology/technological_period.cpp
	src/technology/technology.cpp
	src/technology/technology_category.cpp
	src/technology/technology_container.cpp
	src/technology/technology_model.cpp
	src/technology/technology_subcategory.cpp
)
source_group(technology FILES ${technology_SRCS})
set_source_files_properties(${technology_SRCS} PROPERTIES UNITY_GROUP "technology")

set(ui_SRCS
	src/ui/cursor.cpp
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
	src/unit/military_unit_type.cpp
	src/unit/military_unit_type_container.cpp
	src/unit/promotion.cpp
	src/unit/promotion_container.cpp
	src/unit/transporter.cpp
	src/unit/transporter_class.cpp
	src/unit/transporter_class_container.cpp
	src/unit/transporter_type.cpp
	src/unit/transporter_type_container.cpp
)
source_group(unit FILES ${unit_SRCS})
set_source_files_properties(${unit_SRCS} PROPERTIES UNITY_GROUP "unit")

set(xbrz_SRCS
	src/third_party/xbrz/xbrz.cpp
)
source_group(xbrz FILES ${xbrz_SRCS})
set_source_files_properties(${xbrz_SRCS} PROPERTIES UNITY_GROUP "xbrz")

set(metternich_SRCS
	${character_SRCS}
	${database_SRCS}
	${domain_SRCS}
	${economy_SRCS}
	${game_SRCS}
	${infrastructure_SRCS}
	${item_SRCS}
	${language_SRCS}
	${map_SRCS}
	${population_SRCS}
	${religion_SRCS}
	${script_SRCS}
	${script_condition_SRCS}
	${script_effect_SRCS}
	${script_modifier_effect_SRCS}
	${species_SRCS}
	${spell_SRCS}
	${technology_SRCS}
	${ui_SRCS}
	${unit_SRCS}
	${xbrz_SRCS}
	src/engine_interface.cpp
	src/trait_base.cpp
)

set(character_HDRS
	src/character/character.h
	src/character/character_attribute.h
	src/character/character_class.h
	src/character/character_class_container.h
	src/character/character_container.h
	src/character/character_data_model.h
	src/character/character_game_data.h
	src/character/character_history.h
	src/character/character_trait.h
	src/character/character_trait_type.h
	src/character/dynasty.h
	src/character/family_tree_model.h
	src/character/monster_type.h
	src/character/party.h
	src/character/starting_age_category.h
)
source_group(character FILES ${character_HDRS})

set(database_HDRS
	src/database/data_entry_container.h
	src/database/defines.h
	src/database/preferences.h
)
source_group(database FILES ${database_HDRS})

set(domain_HDRS
	src/domain/consulate.h
	src/domain/consulate_container.h
	src/domain/country_ai.h
	src/domain/country_economy.h
	src/domain/country_government.h
	src/domain/country_military.h
	src/domain/country_rank.h
	src/domain/country_technology.h
	src/domain/country_tier.h
	src/domain/country_tier_data.h
	src/domain/country_turn_data.h
	src/domain/country_type.h
	src/domain/cultural_group.h
	src/domain/cultural_group_rank.h
	src/domain/culture.h
	src/domain/culture_base.h
	src/domain/culture_container.h
	src/domain/culture_history.h
	src/domain/diplomacy_state.h
	src/domain/domain.h
	src/domain/domain_container.h
	src/domain/domain_game_data.h
	src/domain/domain_history.h
	src/domain/government_group.h
	src/domain/government_type.h
	src/domain/government_type_container.h
	src/domain/idea.h
	src/domain/idea_slot.h
	src/domain/idea_trait.h
	src/domain/idea_type.h
	src/domain/journal_entry.h
	src/domain/law.h
	src/domain/law_group.h
	src/domain/law_group_container.h
	src/domain/office.h
	src/domain/subject_type.h
)
source_group(domain FILES ${domain_HDRS})

set(economy_HDRS
	src/economy/commodity.h
	src/economy/commodity_container.h
	src/economy/commodity_type.h
	src/economy/commodity_unit.h
	src/economy/expense_transaction.h
	src/economy/expense_transaction_type.h
	src/economy/food_type.h
	src/economy/income_transaction.h
	src/economy/income_transaction_type.h
	src/economy/resource.h
	src/economy/resource_container.h
	src/economy/transaction.h
)
source_group(economy FILES ${economy_HDRS})

set(game_HDRS
	src/game/character_event.h
	src/game/domain_event.h
	src/game/event.h
	src/game/event_instance.h
	src/game/event_option.h
	src/game/event_random_group.h
	src/game/event_trigger.h
	src/game/game.h
	src/game/game_rules.h
	src/game/province_event.h
	src/game/scenario.h
	src/game/scenario_model.h
	src/game/scoped_event_base.h
	src/game/site_event.h
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
	src/infrastructure/dungeon.h
	src/infrastructure/dungeon_area.h
	src/infrastructure/holding_type.h
	src/infrastructure/improvement.h
	src/infrastructure/improvement_container.h
	src/infrastructure/improvement_slot.h
	src/infrastructure/pathway.h
	src/infrastructure/pathway_container.h
	src/infrastructure/settlement_building_slot.h
	src/infrastructure/wonder.h
)
source_group(infrastructure FILES ${infrastructure_HDRS})

set(item_HDRS
	src/item/affix_type.h
	src/item/enchantment.h
	src/item/item.h
	src/item/item_class.h
	src/item/item_material.h
	src/item/item_slot.h
	src/item/item_type.h
)
source_group(item FILES ${item_HDRS})

set(language_HDRS
	src/language/fallback_name_generator.h
)
source_group(language FILES ${language_HDRS})

set(map_HDRS
	src/map/celestial_body_type.h
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
	src/map/province_map_image_provider.h
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
	src/map/site_tier.h
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
	src/population/population.h
	src/population/population_class.h
	src/population/population_class_container.h
	src/population/population_group_map.h
	src/population/population_type.h
	src/population/population_type_container.h
	src/population/population_unit.h
)
source_group(population FILES ${population_HDRS})

set(religion_HDRS
	src/religion/deity.h
	src/religion/deity_slot.h
	src/religion/deity_trait.h
	src/religion/pantheon.h
	src/religion/religion.h
	src/religion/religion_base.h
	src/religion/religion_container.h
	src/religion/religious_group.h
)
source_group(religion FILES ${religion_HDRS})

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
	src/script/scripted_country_modifier.h
	src/script/scripted_modifier.h
	src/script/scripted_modifier_container.h
	src/script/scripted_province_modifier.h
	src/script/scripted_scoped_modifier.h
	src/script/scripted_site_modifier.h
	src/script/text_processor.h
)
source_group(script FILES ${script_HDRS})

set(script_condition_HDRS
	src/script/condition/adjacent_terrain_condition.h
	src/script/condition/advisor_condition.h
	src/script/condition/age_condition.h
	src/script/condition/anarchy_condition.h
	src/script/condition/and_condition.h
	src/script/condition/any_adjacent_site_condition.h
	src/script/condition/any_known_country_condition.h
	src/script/condition/any_neighbor_country_condition.h
	src/script/condition/any_settlement_condition.h
	src/script/condition/any_site_condition.h
	src/script/condition/any_subject_country_condition.h
	src/script/condition/artillery_condition.h
	src/script/condition/attacking_commander_condition.h
	src/script/condition/available_food_condition.h
	src/script/condition/available_housing_condition.h
	src/script/condition/can_gain_building_class_condition.h
	src/script/condition/can_gain_trait_condition.h
	src/script/condition/capital_condition.h
	src/script/condition/cavalry_condition.h
	src/script/condition/character_condition.h
	src/script/condition/character_attribute_condition.h
	src/script/condition/character_class_condition.h
	src/script/condition/coastal_condition.h
	src/script/condition/commodity_condition.h
	src/script/condition/condition.h
	src/script/condition/core_condition.h
	src/script/condition/country_condition.h
	src/script/condition/country_exists_condition.h
	src/script/condition/country_scope_condition.h
	src/script/condition/country_type_condition.h
	src/script/condition/cultural_group_condition.h
	src/script/condition/culture_condition.h
	src/script/condition/discovered_province_condition.h
	src/script/condition/discovered_region_condition.h
	src/script/condition/dungeon_condition.h
	src/script/condition/event_condition.h
	src/script/condition/game_rule_condition.h
	src/script/condition/government_type_condition.h
	src/script/condition/has_building_class_condition.h
	src/script/condition/has_building_condition.h
	src/script/condition/has_character_condition.h
	src/script/condition/has_flag_condition.h
	src/script/condition/has_pathway_condition.h
	src/script/condition/has_population_culture_condition.h
	src/script/condition/has_population_religion_condition.h
	src/script/condition/has_population_type_condition.h
	src/script/condition/has_resource_condition.h
	src/script/condition/has_route_condition.h
	src/script/condition/has_terrain_condition.h
	src/script/condition/holding_type_condition.h
	src/script/condition/housing_condition.h
	src/script/condition/improvement_condition.h
	src/script/condition/independent_condition.h
	src/script/condition/infantry_condition.h
	src/script/condition/is_military_unit_category_available_condition.h
	src/script/condition/law_condition.h
	src/script/condition/military_unit_category_condition.h
	src/script/condition/military_unit_domain_condition.h
	src/script/condition/military_unit_type_condition.h
	src/script/condition/month_condition.h
	src/script/condition/near_water_condition.h
	src/script/condition/owns_province_condition.h
	src/script/condition/owns_site_condition.h
	src/script/condition/population_scaled_commodity_condition.h
	src/script/condition/population_type_condition.h
	src/script/condition/primary_attribute_condition.h
	src/script/condition/produces_commodity_condition.h
	src/script/condition/promotion_condition.h
	src/script/condition/province_condition.h
	src/script/condition/province_count_condition.h
	src/script/condition/provincial_capital_condition.h
	src/script/condition/random_chance_condition.h
	src/script/condition/religion_condition.h
	src/script/condition/religious_group_condition.h
	src/script/condition/resource_condition.h
	src/script/condition/river_condition.h
	src/script/condition/root_character_condition.h
	src/script/condition/ruler_condition.h
	src/script/condition/ruler_scope_condition.h
	src/script/condition/scripted_condition.h
	src/script/condition/scripted_condition_condition.h
	src/script/condition/scripted_modifier_condition.h
	src/script/condition/site_condition.h
	src/script/condition/site_scope_condition.h
	src/script/condition/source_character_condition.h
	src/script/condition/source_site_condition.h
	src/script/condition/source_site_scope_condition.h
	src/script/condition/species_condition.h
	src/script/condition/subject_type_condition.h
	src/script/condition/taxon_condition.h
	src/script/condition/technology_condition.h
	src/script/condition/terrain_condition.h
	src/script/condition/trait_condition.h
	src/script/condition/war_condition.h
	src/script/condition/year_condition.h
)
source_group(script\\condition FILES ${script_condition_HDRS})

set(script_effect_HDRS
	src/script/effect/add_building_class_effect.h
	src/script/effect/add_improvement_effect.h
	src/script/effect/any_known_country_effect.h
	src/script/effect/any_neighbor_country_effect.h
	src/script/effect/battle_effect.h
	src/script/effect/capital_effect.h
	src/script/effect/change_opinion_effect.h
	src/script/effect/clear_flag_effect.h
	src/script/effect/commodity_effect.h
	src/script/effect/commodity_percent_effect.h
	src/script/effect/country_effect.h
	src/script/effect/create_military_unit_effect.h
	src/script/effect/create_transporter_effect.h
	src/script/effect/delayed_effect.h
	src/script/effect/delayed_effect_instance.h
	src/script/effect/effect.h
	src/script/effect/effect_list.h
	src/script/effect/else_effect.h
	src/script/effect/event_effect.h
	src/script/effect/experience_effect.h
	src/script/effect/free_technologies_effect.h
	src/script/effect/gain_spell_scroll_effect.h
	src/script/effect/hidden_effect.h
	src/script/effect/if_effect.h
	src/script/effect/location_effect.h
	src/script/effect/office_holder_effect.h
	src/script/effect/opinion_modifiers_effect.h
	src/script/effect/population_scaled_commodity_effect.h
	src/script/effect/random_effect.h
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
)
source_group(script\\effect FILES ${script_effect_HDRS})

set(script_modifier_effect_HDRS
	src/script/modifier_effect/ai_building_desire_modifier_effect.h
	src/script/modifier_effect/armor_class_modifier_effect.h
	src/script/modifier_effect/artillery_cost_modifier_effect.h
	src/script/modifier_effect/building_cost_efficiency_modifier_effect.h
	src/script/modifier_effect/capital_commodity_bonus_modifier_effect.h
	src/script/modifier_effect/capital_commodity_bonus_per_population_modifier_effect.h
	src/script/modifier_effect/capital_commodity_output_modifier_effect.h
	src/script/modifier_effect/cavalry_cost_modifier_effect.h
	src/script/modifier_effect/character_attribute_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_for_tile_threshold_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_per_adjacent_terrain_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_per_building_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_per_improved_resource_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_per_improvement_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_per_settlement_modifier_effect.h
	src/script/modifier_effect/commodity_bonus_per_population_modifier_effect.h
	src/script/modifier_effect/commodity_demand_modifier_effect.h
	src/script/modifier_effect/commodity_output_modifier_effect.h
	src/script/modifier_effect/commodity_throughput_modifier_effect.h
	src/script/modifier_effect/deployment_limit_modifier_effect.h
	src/script/modifier_effect/diplomatic_penalty_for_expansion_modifier_effect.h
	src/script/modifier_effect/free_artillery_promotion_modifier_effect.h
	src/script/modifier_effect/free_building_class_modifier_effect.h
	src/script/modifier_effect/free_cavalry_promotion_modifier_effect.h
	src/script/modifier_effect/free_consulate_modifier_effect.h
	src/script/modifier_effect/free_infantry_promotion_modifier_effect.h
	src/script/modifier_effect/free_warship_promotion_modifier_effect.h
	src/script/modifier_effect/gain_technologies_known_by_others_modifier_effect.h
	src/script/modifier_effect/industrial_output_modifier_effect.h
	src/script/modifier_effect/infantry_cost_modifier_effect.h
	src/script/modifier_effect/law_cost_modifier_effect.h
	src/script/modifier_effect/leader_cost_modifier_effect.h
	src/script/modifier_effect/merchant_ship_stat_modifier_effect.h
	src/script/modifier_effect/military_unit_stat_modifier_effect.h
	src/script/modifier_effect/modifier_effect.h
	src/script/modifier_effect/output_modifier_effect.h
	src/script/modifier_effect/population_type_bonus_modifier_effect.h
	src/script/modifier_effect/resource_output_modifier_effect.h
	src/script/modifier_effect/ship_stat_modifier_effect.h
	src/script/modifier_effect/species_armor_class_bonus_modifier_effect.h
	src/script/modifier_effect/storage_capacity_modifier_effect.h
	src/script/modifier_effect/technology_cost_modifier_effect.h
	src/script/modifier_effect/throughput_modifier_effect.h
	src/script/modifier_effect/to_hit_bonus_modifier_effect.h
	src/script/modifier_effect/unit_upgrade_cost_modifier_effect.h
	src/script/modifier_effect/warship_cost_modifier_effect.h
	src/script/modifier_effect/wonder_cost_efficiency_modifier_effect.h
)
source_group(script\\modifier_effect FILES ${script_modifier_effect_HDRS})

set(species_HDRS
	src/species/geological_era.h
	src/species/phenotype.h
	src/species/phenotype_container.h
	src/species/species.h
	src/species/taxon.h
	src/species/taxon_base.h
	src/species/taxonomic_rank.h
)
source_group(species FILES ${species_HDRS})

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
	src/technology/technology_model.h
	src/technology/technology_subcategory.h
)
source_group(technology FILES ${technology_HDRS})

set(ui_HDRS
	src/ui/cursor.h
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
	src/unit/military_unit_category.h
	src/unit/military_unit_domain.h
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

set(xbrz_HDRS
	src/third_party/xbrz/xbrz.h
	src/third_party/xbrz/xbrz_config.h
	src/third_party/xbrz/xbrz_tools.h
)
source_group(xbrz FILES ${xbrz_HDRS})

set(metternich_HDRS
	${character_HDRS}
	${database_HDRS}
	${domain_HDRS}
	${economy_HDRS}
	${game_HDRS}
	${infrastructure_HDRS}
	${item_HDRS}
	${language_HDRS}
	${map_HDRS}
	${population_HDRS}
	${religion_HDRS}
	${script_HDRS}
	${script_condition_HDRS}
	${script_effect_HDRS}
	${script_modifier_effect_HDRS}
	${species_HDRS}
	${spell_HDRS}
	${technology_HDRS}
	${ui_HDRS}
	${unit_HDRS}
	${xbrz_HDRS}
	src/engine_interface.h
	src/metternich.h
	src/trait_base.h
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

set(CMAKE_CXX_STANDARD 23)
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

if(WIN32)
	set_target_properties(metternich_main 
		PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${Qt6_DIR}/../../../bin;"
	)
	
	set_target_properties(metternich_main 
		PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(ProjectDir).."
	)
endif()

set_target_properties(metternich_main PROPERTIES OUTPUT_NAME "metternich")

target_link_libraries(metternich_main PUBLIC metternich Qt6::Charts)
target_link_libraries(metternich_test PUBLIC metternich)
