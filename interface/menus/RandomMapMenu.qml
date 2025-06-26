import QtQuick
import QtQuick.Controls
import ".."

MenuBase {
	id: random_map_menu
	title: qsTr("Random Map")
	
	property int generation_count: 0
	property var selected_era: null
	property var selected_map_size: Qt.size(256, 128)
	readonly property var selected_country: diplomatic_map.selected_country
	
	Rectangle {
		id: diplomatic_map_background
		anchors.horizontalCenter: diplomatic_map.horizontalCenter
		anchors.verticalCenter: diplomatic_map.verticalCenter
		width: diplomatic_map.width + 2 * scale_factor
		height: diplomatic_map.height + 2 * scale_factor
		color: Qt.rgba(0.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0, 1)
		border.color: "white"
		border.width: 1 * scale_factor
	}
	
	DiplomaticMap {
		id: diplomatic_map
		anchors.left: era_list.right
		anchors.leftMargin: 16 * scale_factor
		anchors.right: political_map_mode_button.left
		anchors.rightMargin: 16 * scale_factor
		anchors.top: title_item.bottom
		anchors.topMargin: 32 * scale_factor
		anchors.bottom: country_text.top
		anchors.bottomMargin: 16 * scale_factor
		width: 512 * scale_factor
	}
	
	IconButton {
		id: political_map_mode_button
		anchors.top: diplomatic_map_background.top
		anchors.right: parent.right
		anchors.rightMargin: 16 * scale_factor
		icon_identifier: "flag"
		tooltip: "Political Map Mode"
		highlighted: diplomatic_map.mode === DiplomaticMap.Mode.Political
		
		onClicked: {
			diplomatic_map.mode = DiplomaticMap.Mode.Political
		}
	}
	
	IconButton {
		id: terrain_map_mode_button
		anchors.top: political_map_mode_button.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.right: political_map_mode_button.right
		icon_identifier: "mountains"
		tooltip: "Terrain Map Mode"
		highlighted: diplomatic_map.mode === DiplomaticMap.Mode.Terrain
		
		onClicked: {
			diplomatic_map.mode = DiplomaticMap.Mode.Terrain
		}
	}
	
	IconButton {
		id: cultural_map_mode_button
		anchors.top: terrain_map_mode_button.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.right: political_map_mode_button.right
		icon_identifier: "music"
		tooltip: "Cultural Map Mode"
		highlighted: diplomatic_map.mode === DiplomaticMap.Mode.Cultural
		
		onClicked: {
			diplomatic_map.mode = DiplomaticMap.Mode.Cultural
		}
	}
	
	IconButton {
		id: religious_map_mode_button
		anchors.top: cultural_map_mode_button.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.right: political_map_mode_button.right
		icon_identifier: "wooden_cross"
		tooltip: "Religious Map Mode"
		highlighted: diplomatic_map.mode === DiplomaticMap.Mode.Religious
		
		onClicked: {
			diplomatic_map.mode = DiplomaticMap.Mode.Religious
		}
	}
	
	SmallText {
		id: country_text
		text: selected_country ? (
			selected_country.game_data.name
			+ "\n"
			+ "\n" + selected_country.game_data.type_name
			+ (selected_country.game_data.overlord ? (
				"\n" + selected_country.game_data.subject_type.name + " of " + selected_country.game_data.overlord.name
			) : "")
			+ "\n" + selected_country.game_data.title_name
			+ (selected_country.game_data.anarchy ? "\nAnarchy" : "")
			+ (selected_country.great_power && !selected_country.game_data.anarchy ? ("\nScore: " + number_string(selected_country.game_data.score) + " (#" + (selected_country.game_data.score_rank + 1) + ")") : "")
			+ "\nPopulation: " + number_string(selected_country.game_data.population.size)
			+ get_subject_type_counts_string(selected_country.game_data.subject_type_counts)
			+ "\n" + selected_country.game_data.provinces.length + " " + (selected_country.game_data.provinces.length > 1 ? "Provinces" : "Province")
			+ get_resource_counts_string(selected_country.game_data.resource_counts)
		) : ""
		anchors.left: diplomatic_map.left
		anchors.leftMargin: 4 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 4 * scale_factor
		height: 128 * scale_factor
		
		function get_subject_type_counts_string(subject_type_counts) {
			var str = "";
			
			for (const kv_pair of subject_type_counts) {
				var subject_type = kv_pair.key
				var count = kv_pair.value
				str += "\n" + count + " " + get_plural_form(subject_type.name)
			}
			
			return str
		}
		
		function get_resource_counts_string(resource_counts) {
			var str = "";
			
			for (const kv_pair of resource_counts) {
				var resource = kv_pair.key
				var count = kv_pair.value
				str += "\n" + count + " " + (count > 1 && resource.plural_name.length > 0 ? resource.plural_name : resource.name)
			}
			
			return str
		}
	}
	
	SmallText {
		id: population_type_chart_label
		anchors.top: country_text.top
		anchors.horizontalCenter: population_type_chart.horizontalCenter
		text: "Population Type"
		visible: population_type_chart.visible
	}
	
	PopulationTypeChart {
		id: population_type_chart
		anchors.top: culture_chart.top
		anchors.right: culture_chart.left
		anchors.rightMargin: 16 * scale_factor
		visible: selected_country !== null
		data_source: selected_country ? selected_country.game_data.population : null
	}
	
	SmallText {
		id: culture_chart_label
		anchors.top: country_text.top
		anchors.horizontalCenter: culture_chart.horizontalCenter
		text: "Culture"
		visible: culture_chart.visible
	}
	
	CultureChart {
		id: culture_chart
		anchors.top: culture_chart_label.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.right: religion_chart.left
		anchors.rightMargin: 16 * scale_factor
		visible: selected_country !== null
		data_source: selected_country ? selected_country.game_data.population : null
	}
	
	SmallText {
		id: religion_chart_label
		anchors.top: country_text.top
		anchors.horizontalCenter: religion_chart.horizontalCenter
		text: "Religion"
		visible: religion_chart.visible
	}
	
	ReligionChart {
		id: religion_chart
		anchors.top: population_type_chart.top
		anchors.right: phenotype_chart.left
		anchors.rightMargin: 16 * scale_factor
		visible: selected_country !== null
		data_source: selected_country ? selected_country.game_data.population : null
	}
	
	SmallText {
		id: phenotype_chart_label
		anchors.top: country_text.top
		anchors.horizontalCenter: phenotype_chart.horizontalCenter
		text: "Phenotype"
		visible: phenotype_chart.visible
	}
	
	PhenotypeChart {
		id: phenotype_chart
		anchors.top: population_type_chart.top
		anchors.right: diplomatic_map_background.right
		anchors.rightMargin: 4 * scale_factor
		visible: selected_country !== null
		data_source: selected_country ? selected_country.game_data.population : null
	}
	
	Rectangle {
		id: era_list_border
		anchors.horizontalCenter: era_list.horizontalCenter
		anchors.verticalCenter: era_list.verticalCenter
		width: era_list.width + 2
		height: era_list.height + 2
		color: "transparent"
		border.color: "white"
		border.width: 1
	}
	
	ListView {
		id: era_list
		anchors.left: parent.left
		anchors.leftMargin: 16 * scale_factor
		anchors.top: title_item.bottom
		anchors.topMargin: 32 * scale_factor
		width: contentItem.childrenRect.width
		height: contentItem.childrenRect.height
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		model: metternich.get_eras()
		delegate: Rectangle {
			width: 256 * scale_factor
			height: visible ? 16 * scale_factor : 0
			visible: !model.modelData.hidden
			color: (selected_era == model.modelData) ? "olive" : "black"
			border.color: "white"
			border.width: 1
			
			SmallText {
				text: model.modelData.name
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.verticalCenter: parent.verticalCenter
			}
			
			MouseArea {
				anchors.fill: parent
				
				onClicked: {
					if (selected_era === model.modelData) {
						return
					}
					
					selected_era = model.modelData
					metternich.game.create_random_map(selected_map_size, selected_era)
				}
			}
		}
	}
	
	Rectangle {
		id: map_size_list_border
		anchors.horizontalCenter: map_size_list.horizontalCenter
		anchors.verticalCenter: map_size_list.verticalCenter
		width: map_size_list.width + 2
		height: map_size_list.height + 2
		color: "transparent"
		border.color: "white"
		border.width: 1
		visible: map_size_list.visible
	}
	
	ListView {
		id: map_size_list
		anchors.left: parent.left
		anchors.leftMargin: 16 * scale_factor
		anchors.top: era_list.bottom
		anchors.topMargin: 32 * scale_factor
		width: contentItem.childrenRect.width
		height: contentItem.childrenRect.height
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		visible: false
		model: [ Qt.size(256, 128), Qt.size(512, 256), Qt.size(1024, 512) ]
		delegate: Rectangle {
			width: 256 * scale_factor
			height: visible ? 16 * scale_factor : 0
			color: (selected_map_size == model.modelData) ? "olive" : "black"
			border.color: "white"
			border.width: 1
			
			SmallText {
				text: model.modelData.width + "x" + model.modelData.height
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.verticalCenter: parent.verticalCenter
			}
			
			MouseArea {
				anchors.fill: parent
				
				onClicked: {
					if (selected_map_size === model.modelData) {
						return
					}
					
					selected_map_size = model.modelData
					metternich.game.create_random_map(selected_map_size, selected_era)
				}
			}
		}
	}
	
	TextButton {
		id: start_game_button
		anchors.horizontalCenter: era_list.horizontalCenter
		anchors.bottom: previous_menu_button.top
		anchors.bottomMargin: 8 * scale_factor
		text: qsTr("Start Game")
		width: 96 * scale_factor
		height: 24 * scale_factor
		enabled: selected_country !== null && selected_country.great_power && !selected_country.game_data.anarchy
		tooltip: enabled ? "" : small_text(
			selected_country === null ? "You must select a country to play" : (
				!selected_country.great_power ? "You cannot play as a Minor Nation" : "You cannot play as a country under anarchy"
			)
		)
		
		onClicked: {
			metternich.game.player_country = selected_country
			metternich.game.start()
		}
	}
	
	TextButton {
		id: previous_menu_button
		anchors.horizontalCenter: start_game_button.horizontalCenter
		anchors.bottom: diplomatic_map.bottom
		anchors.bottomMargin: 8 * scale_factor
		text: qsTr("Previous Menu")
		width: 96 * scale_factor
		height: 24 * scale_factor
		
		onClicked: {
			menu_stack.pop()
		}
	}
	
	Connections {
		target: metternich.game
		function onSetup_finished() {
			diplomatic_map.ocean_suffix = random_map_menu.generation_count
			
			if (selected_country !== null && !metternich.game.countries.includes(selected_country)) {
				diplomatic_map.selected_country = null
			}
			
			diplomatic_map.country_suffix = random_map_menu.generation_count
			
			random_map_menu.generation_count += 1
		}
	}
	
	Component.onCompleted: {
		//get the first non-hidden era
		for (var era of metternich.get_eras()) {
			if (!era.hidden) {
				selected_era = era
				break
			}
		}
		
		metternich.game.create_random_map(selected_map_size, selected_era).then(() {
			diplomatic_map.selected_country = metternich.game.great_powers[random(metternich.game.great_powers.length)]
			diplomatic_map.center_on_selected_country_capital()
		})
	}
}
